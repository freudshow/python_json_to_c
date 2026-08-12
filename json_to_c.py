#!/usr/bin/env python3
"""根据 JSON 示例生成可独立编译的 C 结构体及 JSON 编解码代码。"""

from __future__ import annotations

import argparse
import datetime as _datetime
import json
import keyword
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable, Optional


C_KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "int",
    "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile",
    "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex", "_Generic",
    "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local",
}


def snake_identifier(value: str, fallback: str = "value") -> str:
    """把 JSON 名称转换为稳定、合法的 C 标识符。"""
    result = re.sub(r"[^0-9A-Za-z_]", "_", value)
    result = re.sub(r"_+", "_", result).strip("_").lower()
    if not result:
        result = fallback
    if result[0].isdigit():
        result = "field_" + result
    if result in C_KEYWORDS or keyword.iskeyword(result):
        result += "_value"
    return result


def type_identifier(value: str, fallback: str = "root") -> str:
    """生成类型/文件使用的基础标识符。"""
    result = snake_identifier(value, fallback)
    return result


def c_string(value: str) -> str:
    """将 Python 字符串转为 UTF-8 C 字符串字面量。"""
    encoded = value.encode("utf-8")
    chunks: list[str] = []
    for byte in encoded:
        if byte == 0x22:
            chunks.append(r'\"')
        elif byte == 0x5C:
            chunks.append(r"\\")
        elif byte == 0x0A:
            chunks.append(r"\n")
        elif byte == 0x0D:
            chunks.append(r"\r")
        elif byte == 0x09:
            chunks.append(r"\t")
        elif 0x20 <= byte <= 0x7E:
            chunks.append(chr(byte))
        else:
            # 使用固定三位八进制转义，避免 C 的十六进制转义吞掉后续字符。
            chunks.append(f"\\{byte:03o}")
    return '"' + "".join(chunks) + '"'


@dataclass
class FieldModel:
    json_name: str
    c_name: str
    node: "NodeModel"


@dataclass
class NodeModel:
    kind: str
    path: str
    scalar: Optional[str] = None
    fields: list[FieldModel] = field(default_factory=list)
    item: Optional["NodeModel"] = None
    variants: list["NodeModel"] = field(default_factory=list)
    type_name: Optional[str] = None
    array_item_name: Optional[str] = None
    array_type: Optional[str] = None
    union_kind_name: Optional[str] = None
    union_item_name: Optional[str] = None


class NameAllocator:
    """避免不同 JSON 名称清洗后产生 C 标识符冲突。"""

    def __init__(self) -> None:
        self.used: set[str] = set()

    def allocate(self, base: str) -> str:
        candidate = snake_identifier(base)
        original = candidate
        suffix = 2
        while candidate in self.used:
            candidate = f"{original}_{suffix}"
            suffix += 1
        self.used.add(candidate)
        return candidate


def scalar_kind(value: Any) -> Optional[str]:
    if value is None:
        return None
    if isinstance(value, bool):
        return "bool"
    if isinstance(value, int):
        if -(2**63) <= value <= 2**63 - 1:
            return "int"
        return "double"
    if isinstance(value, float):
        return "double"
    if isinstance(value, str):
        return "string"
    return None


def node_signature(node: NodeModel) -> tuple[Any, ...]:
    if node.kind == "scalar":
        return ("scalar", node.scalar)
    if node.kind == "generic":
        return ("generic",)
    if node.kind == "object":
        return ("object", tuple((item.json_name, node_signature(item.node))
                                 for item in node.fields))
    if node.kind == "array":
        if node.item is not None:
            return ("array", node_signature(node.item))
        return ("array", tuple(node_signature(item) for item in node.variants))
    raise ValueError(f"未知节点类型: {node.kind}")


def infer_node(value: Any, path: str, allocator: NameAllocator) -> NodeModel:
    """从一个 JSON 值递归推导 schema。"""
    if (isinstance(value, int) and not isinstance(value, bool) and
            not (-(2**63) <= value <= 2**63 - 1)):
        # C 的固定数值类型无法无损承载超出 int64_t 的 JSON 整数，
        # 交给通用 DOM 保留其原始数字文本。
        return NodeModel("generic", path)
    kind = scalar_kind(value)
    if kind is not None:
        return NodeModel("scalar", path, scalar=kind)
    if value is None:
        return NodeModel("generic", path)
    if isinstance(value, dict):
        result = NodeModel("object", path)
        used_fields: set[str] = set()
        for key, child in value.items():
            base = snake_identifier(str(key), "field")
            c_name = base
            suffix = 2
            while c_name in used_fields:
                c_name = f"{base}_{suffix}"
                suffix += 1
            used_fields.add(c_name)
            result.fields.append(FieldModel(
                str(key), c_name,
                infer_node(child, f"{path}_{c_name}", allocator),
            ))
        return result
    if isinstance(value, list):
        if not value:
            return NodeModel("array", path)
        children = [infer_node(item, f"{path}_item_{index + 1}", allocator)
                    for index, item in enumerate(value)]
        if all(item.kind == "object" for item in children):
            grouped: list[NodeModel] = []
            signatures: list[tuple[Any, ...]] = []
            for child in children:
                signature = node_signature(child)
                if signature not in signatures:
                    signatures.append(signature)
                    grouped.append(child)
            if len(grouped) == 1:
                return NodeModel("array", path, item=grouped[0])
            return NodeModel("array", path, variants=grouped)
        child_signatures = {node_signature(item) for item in children}
        if len(child_signatures) == 1:
            return NodeModel("array", path, item=children[0])
        # int 和 double 混合时不能统一为 double，否则大整数会丢失精度。
        # 这类数组交给通用 DOM，并保留每个数字的原始文本。
        return NodeModel("array", path)
    raise TypeError(f"不支持的 JSON 值: {type(value)!r}")


def assign_names(node: NodeModel, base: str, is_root: bool = False) -> None:
    """为所有需要暴露到 C 头文件中的类型分配名称。"""
    base = type_identifier(base)
    if node.kind == "object":
        node.type_name = base + "_t"
        for item in node.fields:
            assign_names(item.node, base + "_" + item.c_name)
    elif node.kind == "array":
        node.array_type = base + "_t" if is_root else base + "_array_t"
        if node.item is not None:
            assign_names(node.item, base + "_item")
            node.array_item_name = c_type(node.item)
        elif node.variants:
            for index, variant in enumerate(node.variants, start=1):
                assign_names(variant, base + f"_variant_{index}")
            node.union_kind_name = base + "_kind_t"
            node.union_item_name = base + "_item_t"
            node.array_item_name = node.union_item_name
        else:
            node.array_item_name = "json_value_t"
    elif node.kind == "generic":
        return
    elif node.kind == "scalar":
        return


def c_type(node: NodeModel) -> str:
    if node.kind == "object":
        if node.type_name is None:
            raise ValueError("对象类型尚未命名")
        return node.type_name
    if node.kind == "array":
        if node.array_type is None:
            # 仅用于命名阶段的占位；真正生成前会再次计算。
            return "json_value_t"
        return node.array_type
    if node.kind == "generic":
        return "json_value_t"
    if node.scalar == "string":
        return "char *"
    if node.scalar == "bool":
        return "bool"
    if node.scalar == "int":
        return "int64_t"
    if node.scalar == "double":
        return "double"
    raise ValueError(f"未知 C 类型: {node.scalar}")


def all_nodes(node: NodeModel) -> Iterable[NodeModel]:
    yield node
    if node.kind == "object":
        for item in node.fields:
            yield from all_nodes(item.node)
    elif node.kind == "array":
        if node.item is not None:
            yield from all_nodes(node.item)
        for variant in node.variants:
            yield from all_nodes(variant)


def emit_node_definitions(root: NodeModel) -> list[str]:
    lines: list[str] = []
    emitted: set[int] = set()

    def emit(node: NodeModel) -> None:
        if id(node) in emitted:
            return
        if node.kind == "object":
            for item in node.fields:
                emit(item.node)
            lines.append(f"typedef struct {node.type_name} {{")
            if not node.fields:
                lines.append("    uint8_t _unused;  // 空对象占位字段")
            else:
                for item in node.fields:
                    lines.append(f"    {c_type(item.node)} {item.c_name};  // JSON 字段 {item.json_name!r}")
            lines.append(f"}} {node.type_name};")
            lines.append("")
        elif node.kind == "array":
            if node.item is not None:
                emit(node.item)
            else:
                for variant in node.variants:
                    emit(variant)
                if node.variants:
                    lines.append("typedef enum {")
                    for index, variant in enumerate(node.variants):
                        lines.append(f"    {node.union_kind_name.upper()}_VARIANT_{index + 1} = {index},")
                    lines.append(f"}} {node.union_kind_name};")
                    lines.append("")
                    lines.append("typedef struct {")
                    lines.append(f"    {node.union_kind_name} kind;  // 当前 union 成员")
                    lines.append("    union {")
                    for index, variant in enumerate(node.variants):
                        lines.append(f"        {variant.type_name} variant_{index + 1};")
                    lines.append("    } value;")
                    lines.append(f"}} {node.union_item_name};")
                    lines.append("")
            lines.append("typedef struct {")
            lines.append(f"    {node.array_item_name} *items;  // 数组元素")
            lines.append("    size_t count;  // 元素数量")
            lines.append(f"}} {node.array_type};")
            lines.append("")
        emitted.add(id(node))

    emit(root)
    return lines


def source_header_comment(file_name: str, overview: str) -> list[str]:
    today = _datetime.date.today().isoformat()
    return [
        "/**",
        f" * 文件名：{file_name}",
        " * 作者：json_to_c.py 自动生成",
        " * 版本：1.0.0",
        f" * 生成日期：{today}",
        f" * 概述：{overview}",
        " * 修改记录：首次生成。",
        " */",
    ]


def emit_header(root: NodeModel, guard: str, overview: str, root_base: str) -> str:
    lines = source_header_comment(root_base + ".h", overview)
    lines += [
        f"#ifndef {guard}", f"#define {guard}", "", "#include <stdbool.h>",
        "#include <stddef.h>", "#include <stdint.h>", "",
        "typedef enum {",
        "    JSON_VALUE_NULL = 0,",
        "    JSON_VALUE_BOOL,",
        "    JSON_VALUE_NUMBER,",
        "    JSON_VALUE_STRING,",
        "    JSON_VALUE_ARRAY,",
        "    JSON_VALUE_OBJECT",
        "} json_value_kind_t;",
        "",
        "typedef struct json_value json_value_t;",
        "typedef struct json_member json_member_t;",
        "",
        "struct json_value {",
        "    json_value_kind_t kind;",
        "    union {",
        "        bool boolean;",
        "        struct {",
        "            int64_t integer;",
        "            double real;",
        "            bool is_integer;",
        "            char *lexeme;  // 原始数字文本，用于通用 DOM 无损回写",
        "        } number;",
        "        char *string;",
        "        struct {",
        "            json_value_t *items;",
        "            size_t count;",
        "        } array;",
        "        struct {",
        "            json_member_t *items;",
        "            size_t count;",
        "        } object;",
        "    } as;",
        "};",
        "",
        "struct json_member {",
        "    char *key;",
        "    json_value_t value;",
        "};",
        "",
        "/** 释放通用 JSON 值及其递归分配的内存。 */",
        "void json_value_free(json_value_t *value);",
        "",
    ]
    lines += emit_node_definitions(root)
    root_type = c_type(root)
    root_lower = root_base
    lines += [
        f"/** 将 JSON 文本解码为 {root_type}。返回 0 表示成功。 */",
        f"int {root_lower}_from_json(const char *json_text, size_t json_length, {root_type} *out_value);",
        f"/** 将 {root_type} 编码为 JSON 文本；空间不足返回 -2。 */",
        f"int {root_lower}_to_json(const {root_type} *value, char *buffer, size_t buffer_size, size_t *written);",
        f"/** 释放 {root_type} 内部动态内存。 */",
        f"void {root_lower}_free({root_type} *value);",
        "",
        f"#endif /* {guard} */",
        "",
    ]
    return "\n".join(lines)


def field_decode_expr(node: NodeModel, source: str, target: str) -> list[str]:
    if node.kind == "scalar":
        helper = {
            "string": "json_decode_string",
            "bool": "json_decode_bool",
            "int": "json_decode_int",
            "double": "json_decode_double",
        }[node.scalar or ""]
        return [f"if ({helper}(&{source}, &{target}) != 0) {{", "    goto fail;", "}"]
    if node.kind == "generic":
        return [f"if (json_value_clone(&{target}, &{source}) != 0) {{", "    goto fail;", "}"]
    if node.kind == "object":
        return [f"if (decode_{node.type_name[:-2]}(&{source}, &{target}) != 0) {{", "    goto fail;", "}"]
    if node.kind == "array":
        return [f"if (decode_{node.array_type[:-2]}(&{source}, &{target}) != 0) {{", "    goto fail;", "}"]
    raise ValueError("不支持的字段类型")


def field_free_lines(node: NodeModel, target: str) -> list[str]:
    if node.kind == "scalar":
        return [f"free({target});"] if node.scalar == "string" else []
    if node.kind == "generic":
        return [f"json_value_free(&{target});"]
    if node.kind == "object":
        return [f"free_{node.type_name[:-2]}(&{target});"]
    if node.kind == "array":
        return [f"free_{node.array_type[:-2]}(&{target});"]
    raise ValueError("不支持的字段类型")


def emit_generic_helpers() -> list[str]:
    return r'''/* ---------- 通用 JSON DOM、解析器和写出器 ---------- */
typedef struct {
    const char *text;
    size_t length;
    size_t position;
} json_parser_t;

typedef struct {
    char *buffer;
    size_t capacity;
    size_t position;
    size_t required;
    bool failed;
} json_writer_t;

static void json_value_init(json_value_t *value)
{
    if (value != NULL) {
        memset(value, 0, sizeof(*value));
        value->kind = JSON_VALUE_NULL;
    }
}

static void json_value_free_members(json_member_t *members, size_t count)
{
    size_t index = 0U;
    if (members == NULL) {
        return;
    }
    for (index = 0U; index < count; ++index) {
        free(members[index].key);
        members[index].key = NULL;
        json_value_free(&members[index].value);
    }
    free(members);
}

void json_value_free(json_value_t *value)
{
    size_t index = 0U;
    if (value == NULL) {
        return;
    }
    if (value->kind == JSON_VALUE_STRING) {
        free(value->as.string);
    } else if (value->kind == JSON_VALUE_NUMBER) {
        free(value->as.number.lexeme);
    } else if (value->kind == JSON_VALUE_ARRAY) {
        for (index = 0U; index < value->as.array.count; ++index) {
            json_value_free(&value->as.array.items[index]);
        }
        free(value->as.array.items);
    } else if (value->kind == JSON_VALUE_OBJECT) {
        json_value_free_members(value->as.object.items, value->as.object.count);
    }
    json_value_init(value);
}

static bool json_size_multiply(size_t left, size_t right, size_t *result)
{
    if (result == NULL || (right != 0U && left > SIZE_MAX / right)) {
        return false;
    }
    *result = left * right;
    return true;
}

static bool json_append_byte(char **buffer, size_t *length, size_t *capacity,
                             unsigned char byte)
{
    char *new_buffer = NULL;
    size_t new_capacity = 0U;
    if (buffer == NULL || length == NULL || capacity == NULL) {
        return false;
    }
    if (*length == *capacity) {
        new_capacity = (*capacity == 0U) ? 32U : (*capacity * 2U);
        if (new_capacity < *capacity) {
            return false;
        }
        new_buffer = (char *)realloc(*buffer, new_capacity);
        if (new_buffer == NULL) {
            return false;
        }
        *buffer = new_buffer;
        *capacity = new_capacity;
    }
    (*buffer)[*length] = (char)byte;
    *length += 1U;
    return true;
}

static int json_hex_value(char value)
{
    if (value >= '0' && value <= '9') {
        return (int)(value - '0');
    }
    if (value >= 'a' && value <= 'f') {
        return (int)(value - 'a') + 10;
    }
    if (value >= 'A' && value <= 'F') {
        return (int)(value - 'A') + 10;
    }
    return -1;
}

static bool json_append_codepoint(char **buffer, size_t *length, size_t *capacity,
                                  uint32_t codepoint)
{
    if (codepoint <= 0x7FU) {
        return json_append_byte(buffer, length, capacity, (unsigned char)codepoint);
    }
    if (codepoint <= 0x7FFU) {
        return json_append_byte(buffer, length, capacity, (unsigned char)(0xC0U | (codepoint >> 6U))) &&
               json_append_byte(buffer, length, capacity, (unsigned char)(0x80U | (codepoint & 0x3FU)));
    }
    if (codepoint <= 0xFFFFU) {
        return json_append_byte(buffer, length, capacity, (unsigned char)(0xE0U | (codepoint >> 12U))) &&
               json_append_byte(buffer, length, capacity, (unsigned char)(0x80U | ((codepoint >> 6U) & 0x3FU))) &&
               json_append_byte(buffer, length, capacity, (unsigned char)(0x80U | (codepoint & 0x3FU)));
    }
    if (codepoint <= 0x10FFFFU) {
        return json_append_byte(buffer, length, capacity, (unsigned char)(0xF0U | (codepoint >> 18U))) &&
               json_append_byte(buffer, length, capacity, (unsigned char)(0x80U | ((codepoint >> 12U) & 0x3FU))) &&
               json_append_byte(buffer, length, capacity, (unsigned char)(0x80U | ((codepoint >> 6U) & 0x3FU))) &&
               json_append_byte(buffer, length, capacity, (unsigned char)(0x80U | (codepoint & 0x3FU)));
    }
    return false;
}

static bool json_parse_string(json_parser_t *parser, char **output)
{
    char *buffer = NULL;
    size_t length = 0U;
    size_t capacity = 0U;
    uint32_t first_codepoint = 0U;
    uint32_t codepoint = 0U;
    size_t index = 0U;
    if (parser == NULL || output == NULL || parser->position >= parser->length ||
        parser->text[parser->position] != '"') {
        return false;
    }
    parser->position += 1U;
    while (parser->position < parser->length) {
        unsigned char current = (unsigned char)parser->text[parser->position++];
        if (current == '"') {
            if (!json_append_byte(&buffer, &length, &capacity, 0U)) {
                free(buffer);
                return false;
            }
            *output = buffer;
            return true;
        }
        if (current == '\\') {
            if (parser->position >= parser->length) {
                free(buffer);
                return false;
            }
            current = (unsigned char)parser->text[parser->position++];
            if (current == '"' || current == '\\' || current == '/') {
                if (!json_append_byte(&buffer, &length, &capacity, current)) {
                    free(buffer);
                    return false;
                }
            } else if (current == 'b' || current == 'f' || current == 'n' ||
                       current == 'r' || current == 't') {
                unsigned char decoded = (current == 'b') ? '\b' :
                    (current == 'f') ? '\f' : (current == 'n') ? '\n' :
                    (current == 'r') ? '\r' : '\t';
                if (!json_append_byte(&buffer, &length, &capacity, decoded)) {
                    free(buffer);
                    return false;
                }
            } else if (current == 'u') {
                first_codepoint = 0U;
                for (index = 0U; index < 4U; ++index) {
                    int digit = 0;
                    if (parser->position >= parser->length) {
                        free(buffer);
                        return false;
                    }
                    digit = json_hex_value(parser->text[parser->position++]);
                    if (digit < 0) {
                        free(buffer);
                        return false;
                    }
                    first_codepoint = (first_codepoint << 4U) | (uint32_t)digit;
                }
                if (first_codepoint >= 0xD800U && first_codepoint <= 0xDBFFU) {
                    /* 非 BMP 字符必须由高、低代理项组成。 */
                    if (parser->length - parser->position < 6U ||
                        parser->text[parser->position] != '\\' ||
                        parser->text[parser->position + 1U] != 'u') {
                        free(buffer);
                        return false;
                    }
                    parser->position += 2U;
                    codepoint = 0U;
                    for (index = 0U; index < 4U; ++index) {
                        int digit = 0;
                        digit = json_hex_value(parser->text[parser->position++]);
                        if (digit < 0) {
                            free(buffer);
                            return false;
                        }
                        codepoint = (codepoint << 4U) | (uint32_t)digit;
                    }
                    if (codepoint < 0xDC00U || codepoint > 0xDFFFU) {
                        free(buffer);
                        return false;
                    }
                    codepoint = 0x10000U +
                        ((first_codepoint - 0xD800U) << 10U) +
                        (codepoint - 0xDC00U);
                } else if (first_codepoint >= 0xDC00U && first_codepoint <= 0xDFFFU) {
                    free(buffer);
                    return false;
                } else {
                    codepoint = first_codepoint;
                }
                if (!json_append_codepoint(&buffer, &length, &capacity, codepoint)) {
                    free(buffer);
                    return false;
                }
            } else {
                free(buffer);
                return false;
            }
        } else {
            if (current < 0x20U || !json_append_byte(&buffer, &length, &capacity, current)) {
                free(buffer);
                return false;
            }
        }
    }
    free(buffer);
    return false;
}

static void json_skip_space(json_parser_t *parser)
{
    if (parser == NULL) {
        return;
    }
    while (parser->position < parser->length &&
           (parser->text[parser->position] == ' ' || parser->text[parser->position] == '\n' ||
            parser->text[parser->position] == '\r' || parser->text[parser->position] == '\t')) {
        parser->position += 1U;
    }
}

static bool json_parse_value(json_parser_t *parser, json_value_t *output);

static bool json_parse_array(json_parser_t *parser, json_value_t *output)
{
    json_value_t *items = NULL;
    size_t count = 0U;
    size_t capacity = 0U;
    json_value_t item;
    if (parser == NULL || output == NULL || parser->position >= parser->length ||
        parser->text[parser->position] != '[') {
        return false;
    }
    parser->position += 1U;
    json_skip_space(parser);
    if (parser->position < parser->length && parser->text[parser->position] == ']') {
        parser->position += 1U;
        output->kind = JSON_VALUE_ARRAY;
        output->as.array.items = NULL;
        output->as.array.count = 0U;
        return true;
    }
    while (parser->position < parser->length) {
        if (count == capacity) {
            size_t new_capacity = (capacity == 0U) ? 4U : capacity * 2U;
            json_value_t *new_items = NULL;
            if (new_capacity < capacity || !json_size_multiply(new_capacity, sizeof(*items), &new_capacity)) {
                free(items);
                return false;
            }
            new_items = (json_value_t *)realloc(items, new_capacity);
            if (new_items == NULL) {
                free(items);
                return false;
            }
            items = new_items;
            capacity = new_capacity / sizeof(*items);
        }
        json_value_init(&item);
        if (!json_parse_value(parser, &item)) {
            json_value_free(&item);
            json_value_free(&(json_value_t){.kind = JSON_VALUE_ARRAY,
                .as.array = {items, count}});
            return false;
        }
        items[count] = item;
        count += 1U;
        json_skip_space(parser);
        if (parser->position < parser->length && parser->text[parser->position] == ']') {
            parser->position += 1U;
            output->kind = JSON_VALUE_ARRAY;
            output->as.array.items = items;
            output->as.array.count = count;
            return true;
        }
        if (parser->position >= parser->length || parser->text[parser->position] != ',') {
            json_value_t temporary = {JSON_VALUE_ARRAY, {.array = {items, count}}};
            json_value_free(&temporary);
            return false;
        }
        parser->position += 1U;
        json_skip_space(parser);
    }
    json_value_t temporary = {JSON_VALUE_ARRAY, {.array = {items, count}}};
    json_value_free(&temporary);
    return false;
}

static bool json_parse_object(json_parser_t *parser, json_value_t *output)
{
    json_member_t *members = NULL;
    size_t count = 0U;
    size_t capacity = 0U;
    if (parser == NULL || output == NULL || parser->position >= parser->length ||
        parser->text[parser->position] != '{') {
        return false;
    }
    parser->position += 1U;
    json_skip_space(parser);
    if (parser->position < parser->length && parser->text[parser->position] == '}') {
        parser->position += 1U;
        output->kind = JSON_VALUE_OBJECT;
        output->as.object.items = NULL;
        output->as.object.count = 0U;
        return true;
    }
    while (parser->position < parser->length) {
        char *key = NULL;
        json_value_t value;
        if (count == capacity) {
            size_t new_capacity = (capacity == 0U) ? 4U : capacity * 2U;
            json_member_t *new_members = NULL;
            if (new_capacity < capacity || !json_size_multiply(new_capacity, sizeof(*members), &new_capacity)) {
                json_value_free_members(members, count);
                return false;
            }
            new_members = (json_member_t *)realloc(members, new_capacity);
            if (new_members == NULL) {
                json_value_free_members(members, count);
                return false;
            }
            members = new_members;
            capacity = new_capacity / sizeof(*members);
        }
        if (!json_parse_string(parser, &key)) {
            json_value_free_members(members, count);
            return false;
        }
        json_skip_space(parser);
        if (parser->position >= parser->length || parser->text[parser->position] != ':') {
            free(key);
            json_value_free_members(members, count);
            return false;
        }
        parser->position += 1U;
        json_skip_space(parser);
        json_value_init(&value);
        if (!json_parse_value(parser, &value)) {
            free(key);
            json_value_free(&value);
            json_value_free_members(members, count);
            return false;
        }
        members[count].key = key;
        members[count].value = value;
        count += 1U;
        json_skip_space(parser);
        if (parser->position < parser->length && parser->text[parser->position] == '}') {
            parser->position += 1U;
            output->kind = JSON_VALUE_OBJECT;
            output->as.object.items = members;
            output->as.object.count = count;
            return true;
        }
        if (parser->position >= parser->length || parser->text[parser->position] != ',') {
            json_value_free_members(members, count);
            return false;
        }
        parser->position += 1U;
        json_skip_space(parser);
    }
    json_value_free_members(members, count);
    return false;
}

static bool json_parse_number(json_parser_t *parser, json_value_t *output)
{
    size_t start = 0U;
    size_t length = 0U;
    char *token = NULL;
    char *end = NULL;
    double real = 0.0;
    bool is_integer = true;
    int64_t integer = 0;
    if (parser == NULL || output == NULL) {
        return false;
    }
    start = parser->position;
    if (parser->position < parser->length && parser->text[parser->position] == '-') {
        parser->position += 1U;
    }
    if (parser->position >= parser->length) {
        return false;
    }
    if (parser->text[parser->position] == '0') {
        parser->position += 1U;
        if (parser->position < parser->length &&
            parser->text[parser->position] >= '0' && parser->text[parser->position] <= '9') {
            return false;
        }
    } else if (parser->text[parser->position] >= '1' &&
               parser->text[parser->position] <= '9') {
        do {
            parser->position += 1U;
        } while (parser->position < parser->length &&
                 parser->text[parser->position] >= '0' &&
                 parser->text[parser->position] <= '9');
    } else {
        return false;
    }
    if (parser->position < parser->length && parser->text[parser->position] == '.') {
        is_integer = false;
        parser->position += 1U;
        if (parser->position >= parser->length ||
            parser->text[parser->position] < '0' || parser->text[parser->position] > '9') {
            return false;
        }
        do {
            parser->position += 1U;
        } while (parser->position < parser->length &&
                 parser->text[parser->position] >= '0' &&
                 parser->text[parser->position] <= '9');
    }
    if (parser->position < parser->length &&
        (parser->text[parser->position] == 'e' || parser->text[parser->position] == 'E')) {
        is_integer = false;
        parser->position += 1U;
        if (parser->position < parser->length &&
            (parser->text[parser->position] == '+' || parser->text[parser->position] == '-')) {
            parser->position += 1U;
        }
        if (parser->position >= parser->length ||
            parser->text[parser->position] < '0' || parser->text[parser->position] > '9') {
            return false;
        }
        do {
            parser->position += 1U;
        } while (parser->position < parser->length &&
                 parser->text[parser->position] >= '0' &&
                 parser->text[parser->position] <= '9');
    }
    length = parser->position - start;
    if (length == 0U) {
        return false;
    }
    token = (char *)malloc(length + 1U);
    if (token == NULL) {
        return false;
    }
    memcpy(token, parser->text + start, length);
    token[length] = '\0';
    errno = 0;
    real = strtod(token, &end);
    if (end == token || *end != '\0' || errno == ERANGE || !isfinite(real)) {
        free(token);
        return false;
    }
    if (is_integer) {
        long long parsed = 0LL;
        char *integer_end = NULL;
        errno = 0;
        parsed = strtoll(token, &integer_end, 10);
        if (integer_end != NULL && *integer_end == '\0' && errno != ERANGE) {
            integer = (int64_t)parsed;
        } else {
            is_integer = false;
        }
    }
    output->kind = JSON_VALUE_NUMBER;
    output->as.number.real = real;
    output->as.number.integer = integer;
    output->as.number.is_integer = is_integer;
    output->as.number.lexeme = token;
    return true;
}

static bool json_parse_value(json_parser_t *parser, json_value_t *output)
{
    if (parser == NULL || output == NULL) {
        return false;
    }
    json_skip_space(parser);
    if (parser->position >= parser->length) {
        return false;
    }
    if (parser->text[parser->position] == 'n' && parser->length - parser->position >= 4U &&
        memcmp(parser->text + parser->position, "null", 4U) == 0) {
        parser->position += 4U;
        output->kind = JSON_VALUE_NULL;
        return true;
    }
    if (parser->text[parser->position] == 't' && parser->length - parser->position >= 4U &&
        memcmp(parser->text + parser->position, "true", 4U) == 0) {
        parser->position += 4U;
        output->kind = JSON_VALUE_BOOL;
        output->as.boolean = true;
        return true;
    }
    if (parser->text[parser->position] == 'f' && parser->length - parser->position >= 5U &&
        memcmp(parser->text + parser->position, "false", 5U) == 0) {
        parser->position += 5U;
        output->kind = JSON_VALUE_BOOL;
        output->as.boolean = false;
        return true;
    }
    if (parser->text[parser->position] == '"') {
        output->kind = JSON_VALUE_STRING;
        return json_parse_string(parser, &output->as.string);
    }
    if (parser->text[parser->position] == '[') {
        return json_parse_array(parser, output);
    }
    if (parser->text[parser->position] == '{') {
        return json_parse_object(parser, output);
    }
    return json_parse_number(parser, output);
}

static JSON_GENERATED_UNUSED const json_member_t *json_object_get(const json_value_t *object, const char *key)
{
    size_t index = 0U;
    if (object == NULL || key == NULL || object->kind != JSON_VALUE_OBJECT) {
        return NULL;
    }
    for (index = 0U; index < object->as.object.count; ++index) {
        if (strcmp(object->as.object.items[index].key, key) == 0) {
            return &object->as.object.items[index];
        }
    }
    return NULL;
}

static JSON_GENERATED_UNUSED int json_copy_string(const char *input, char **output)
{
    size_t length = 0U;
    char *copy = NULL;
    if (input == NULL || output == NULL) {
        return -1;
    }
    length = strlen(input);
    copy = (char *)malloc(length + 1U);
    if (copy == NULL) {
        return -1;
    }
    memcpy(copy, input, length + 1U);
    *output = copy;
    return 0;
}

static JSON_GENERATED_UNUSED int json_value_clone(json_value_t *output, const json_value_t *input)
{
    size_t index = 0U;
    if (output == NULL || input == NULL) {
        return -1;
    }
    json_value_init(output);
    output->kind = input->kind;
    if (input->kind == JSON_VALUE_STRING) {
        return json_copy_string(input->as.string, &output->as.string);
    }
    if (input->kind == JSON_VALUE_BOOL || input->kind == JSON_VALUE_NULL) {
        output->as = input->as;
        return 0;
    }
    if (input->kind == JSON_VALUE_NUMBER) {
        output->as.number.integer = input->as.number.integer;
        output->as.number.real = input->as.number.real;
        output->as.number.is_integer = input->as.number.is_integer;
        if (input->as.number.lexeme == NULL) {
            return 0;
        }
        return json_copy_string(input->as.number.lexeme, &output->as.number.lexeme);
    }
    if (input->kind == JSON_VALUE_ARRAY) {
        output->as.array.count = input->as.array.count;
        if (input->as.array.count == 0U) {
            return 0;
        }
        output->as.array.items = (json_value_t *)calloc(input->as.array.count,
                                                         sizeof(*output->as.array.items));
        if (output->as.array.items == NULL) {
            json_value_free(output);
            return -1;
        }
        for (index = 0U; index < input->as.array.count; ++index) {
            if (json_value_clone(&output->as.array.items[index], &input->as.array.items[index]) != 0) {
                json_value_free(output);
                return -1;
            }
        }
        return 0;
    }
    output->as.object.count = input->as.object.count;
    if (input->as.object.count == 0U) {
        return 0;
    }
    output->as.object.items = (json_member_t *)calloc(input->as.object.count,
                                                       sizeof(*output->as.object.items));
    if (output->as.object.items == NULL) {
        json_value_free(output);
        return -1;
    }
    for (index = 0U; index < input->as.object.count; ++index) {
        if (json_copy_string(input->as.object.items[index].key,
                             &output->as.object.items[index].key) != 0 ||
            json_value_clone(&output->as.object.items[index].value,
                             &input->as.object.items[index].value) != 0) {
            json_value_free(output);
            return -1;
        }
    }
    return 0;
}

static JSON_GENERATED_UNUSED int json_decode_string(const json_value_t *input, char **output)
{
    if (input == NULL || output == NULL || input->kind != JSON_VALUE_STRING) {
        return -1;
    }
    return json_copy_string(input->as.string, output);
}

static JSON_GENERATED_UNUSED int json_decode_bool(const json_value_t *input, bool *output)
{
    if (input == NULL || output == NULL || input->kind != JSON_VALUE_BOOL) {
        return -1;
    }
    *output = input->as.boolean;
    return 0;
}

static JSON_GENERATED_UNUSED int json_decode_int(const json_value_t *input, int64_t *output)
{
    if (input == NULL || output == NULL || input->kind != JSON_VALUE_NUMBER ||
        !input->as.number.is_integer) {
        return -1;
    }
    *output = input->as.number.integer;
    return 0;
}

static JSON_GENERATED_UNUSED int json_decode_double(const json_value_t *input, double *output)
{
    if (input == NULL || output == NULL || input->kind != JSON_VALUE_NUMBER) {
        return -1;
    }
    *output = input->as.number.real;
    return 0;
}

static bool writer_put(json_writer_t *writer, const char *text, size_t length)
{
    if (writer == NULL || (text == NULL && length != 0U)) {
        return false;
    }
    if (writer->required > SIZE_MAX - length) {
        writer->failed = true;
        return false;
    }
    writer->required += length;
    if (writer->buffer != NULL) {
        if (writer->position > writer->capacity ||
            length >= writer->capacity - writer->position) {
            writer->failed = true;
            return false;
        }
        memcpy(writer->buffer + writer->position, text, length);
        writer->position += length;
    }
    return true;
}

static bool writer_char(json_writer_t *writer, char value)
{
    return writer_put(writer, &value, 1U);
}

static bool writer_string(json_writer_t *writer, const char *value)
{
    static const char hex[] = "0123456789abcdef";
    const unsigned char *cursor = (const unsigned char *)value;
    char escaped[7] = {0};
    if (writer == NULL || value == NULL || !writer_char(writer, '"')) {
        return false;
    }
    while (*cursor != 0U) {
        if (*cursor == '"' || *cursor == '\\') {
            if (!writer_char(writer, '\\') || !writer_char(writer, (char)*cursor)) {
                return false;
            }
        } else if (*cursor == '\b' || *cursor == '\f' || *cursor == '\n' ||
                   *cursor == '\r' || *cursor == '\t') {
            const char escapes[] = {'b', 'f', 'n', 'r', 't'};
            unsigned int offset = (*cursor == '\b') ? 0U : (*cursor == '\f') ? 1U :
                                  (*cursor == '\n') ? 2U : (*cursor == '\r') ? 3U : 4U;
            if (!writer_char(writer, '\\') || !writer_char(writer, escapes[offset])) {
                return false;
            }
        } else if (*cursor < 0x20U) {
            escaped[0] = '\\'; escaped[1] = 'u'; escaped[2] = '0'; escaped[3] = '0';
            escaped[4] = hex[*cursor >> 4U]; escaped[5] = hex[*cursor & 0x0FU];
            if (!writer_put(writer, escaped, 6U)) {
                return false;
            }
        } else if (!writer_char(writer, (char)*cursor)) {
            return false;
        }
        cursor += 1;
    }
    return writer_char(writer, '"');
}

static bool writer_int(json_writer_t *writer, int64_t value)
{
    char text[32] = {0};
    int written = snprintf(text, sizeof(text), "%lld", (long long)value);
    return written > 0 && (size_t)written < sizeof(text) &&
           writer_put(writer, text, (size_t)written);
}

static bool writer_double(json_writer_t *writer, double value)
{
    char text[64] = {0};
    int written = 0;
    if (!isfinite(value)) {
        return false;
    }
    written = snprintf(text, sizeof(text), "%.17g", value);
    return written > 0 && (size_t)written < sizeof(text) &&
           writer_put(writer, text, (size_t)written);
}

static JSON_GENERATED_UNUSED int encode_json_value(const json_value_t *value, json_writer_t *writer)
{
    size_t index = 0U;
    bool first = true;
    if (value == NULL || writer == NULL) {
        return -1;
    }
    if (value->kind == JSON_VALUE_NULL) {
        return writer_put(writer, "null", 4U) ? 0 : -1;
    }
    if (value->kind == JSON_VALUE_BOOL) {
        return writer_put(writer, value->as.boolean ? "true" : "false",
                          value->as.boolean ? 4U : 5U) ? 0 : -1;
    }
    if (value->kind == JSON_VALUE_NUMBER) {
        if (value->as.number.lexeme != NULL) {
            return writer_put(writer, value->as.number.lexeme,
                              strlen(value->as.number.lexeme)) ? 0 : -1;
        }
        return value->as.number.is_integer ? (writer_int(writer, value->as.number.integer) ? 0 : -1) :
                                             (writer_double(writer, value->as.number.real) ? 0 : -1);
    }
    if (value->kind == JSON_VALUE_STRING) {
        return writer_string(writer, value->as.string) ? 0 : -1;
    }
    if (!writer_char(writer, value->kind == JSON_VALUE_ARRAY ? '[' : '{')) {
        return -1;
    }
    if (value->kind == JSON_VALUE_ARRAY) {
        for (index = 0U; index < value->as.array.count; ++index) {
            if (!first && !writer_char(writer, ',')) {
                return -1;
            }
            first = false;
            if (encode_json_value(&value->as.array.items[index], writer) != 0) {
                return -1;
            }
        }
    } else if (value->kind == JSON_VALUE_OBJECT) {
        for (index = 0U; index < value->as.object.count; ++index) {
            if (!first && !writer_char(writer, ',')) {
                return -1;
            }
            first = false;
            if (!writer_string(writer, value->as.object.items[index].key) ||
                !writer_char(writer, ':') ||
                encode_json_value(&value->as.object.items[index].value, writer) != 0) {
                return -1;
            }
        }
    }
    return writer_char(writer, value->kind == JSON_VALUE_ARRAY ? ']' : '}') ? 0 : -1;
}

'''.splitlines()


def emit_object_functions(node: NodeModel) -> list[str]:
    """生成一个 object 的 decode/free/encode 函数。"""
    name = node.type_name[:-2]
    lines = [f"static void free_{name}({node.type_name} *value);",
             f"static int decode_{name}(const json_value_t *node, {node.type_name} *out);",
             f"static int encode_{name}(const {node.type_name} *value, json_writer_t *writer);", ""]
    lines += [f"static void free_{name}({node.type_name} *value)", "{", "    if (value == NULL) {", "        return;", "    }"]
    for item in node.fields:
        lines += [*field_free_lines(item.node, f"value->{item.c_name}")]
    lines += [f"    memset(value, 0, sizeof(*value));", "}", ""]
    lines += [f"static int decode_{name}(const json_value_t *node, {node.type_name} *out)", "{"]
    if node.fields:
        lines.append("    const json_member_t *member = NULL;")
    lines += ["    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {",
              "        return -1;", "    }",
              f"    if (node->as.object.count != {len(node.fields)}U) {{", "        return -1;", "    }",
              "    memset(out, 0, sizeof(*out));"]
    for item in node.fields:
        lines += [f"    member = json_object_get(node, {c_string(item.json_name)});",
                  "    if (member == NULL) {", "        goto fail;", "    }"]
        for line in field_decode_expr(item.node, "member->value", f"out->{item.c_name}"):
            lines.append("    " + line if line else line)
    lines += ["    return 0;"]
    if node.fields:
        lines += ["fail:", f"    free_{name}(out);", "    return -1;"]
    lines += ["}", ""]
    lines += [f"static int encode_{name}(const {node.type_name} *value, json_writer_t *writer)", "{"]
    if node.fields:
        lines.append("    bool first = true;")
    lines += ["    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {", "        return -1;", "    }"]
    for item in node.fields:
        lines += ["    if (!first && !writer_char(writer, ',')) {", "        return -1;", "    }",
                  "    first = false;",
                  f"    if (!writer_string(writer, {c_string(item.json_name)}) || !writer_char(writer, ':')) {{",
                  "        return -1;", "    }"]
        lines += ["    " + line for line in encode_expr(item.node, f"value->{item.c_name}")]
    lines += ["    return writer_char(writer, '}') ? 0 : -1;", "}", ""]
    return lines


def encode_expr(node: NodeModel, value: str) -> list[str]:
    if node.kind == "scalar":
        if node.scalar == "string":
            return [f"if (!writer_string(writer, {value})) {{", "    return -1;", "}"]
        if node.scalar == "bool":
            return [f"if (!writer_put(writer, {value} ? \"true\" : \"false\", {value} ? 4U : 5U)) {{",
                    "    return -1;", "}"]
        if node.scalar == "int":
            return [f"if (!writer_int(writer, {value})) {{", "    return -1;", "}"]
        return [f"if (!writer_double(writer, {value})) {{", "    return -1;", "}"]
    if node.kind == "generic":
        return [f"if (encode_json_value(&{value}, writer) != 0) {{", "    return -1;", "}"]
    if node.kind == "object":
        return [f"if (encode_{node.type_name[:-2]}(&{value}, writer) != 0) {{", "    return -1;", "}"]
    if node.kind == "array":
        return [f"if (encode_{node.array_type[:-2]}(&{value}, writer) != 0) {{", "    return -1;", "}"]
    raise ValueError("不支持的编码类型")


def emit_array_functions(node: NodeModel) -> list[str]:
    name = node.array_type[:-2]
    lines = [f"static void free_{name}({node.array_type} *value);",
             f"static int decode_{name}(const json_value_t *node, {node.array_type} *out);",
             f"static int encode_{name}(const {node.array_type} *value, json_writer_t *writer);", ""]
    item_kind = node.item or (node.variants[0] if node.variants else None)
    lines += [f"static void free_{name}({node.array_type} *value)", "{",
              "    if (value == NULL) {", "        return;", "    }"]
    if node.item is not None:
        free_item = field_free_lines(node.item, f"value->items[index]")
        if free_item:
            lines += ["    size_t index = 0U;",
                      "    for (index = 0U; index < value->count; ++index) {",
                      *["        " + line for line in free_item], "    }"]
    elif node.variants:
        lines += ["    size_t index = 0U;",
                  "    for (index = 0U; index < value->count; ++index) {",
                  f"        free_{node.union_item_name[:-2]}(&value->items[index]);", "    }"]
    else:
        lines += ["    size_t index = 0U;",
                  "    for (index = 0U; index < value->count; ++index) {",
                  "        json_value_free(&value->items[index]);", "    }"]
    lines += ["    free(value->items);", "    value->items = NULL;", "    value->count = 0U;", "}", ""]
    lines += [f"static int decode_{name}(const json_value_t *node, {node.array_type} *out)", "{",
              "    size_t index = 0U;", "    if (node == NULL || out == NULL || node->kind != JSON_VALUE_ARRAY) {",
              "        return -1;", "    }", "    memset(out, 0, sizeof(*out));", "    out->count = node->as.array.count;"]
    lines += ["    if (out->count == 0U) {", "        return 0;", "    }",
              "    out->items = calloc(out->count, sizeof(*out->items));", "    if (out->items == NULL) {", "        out->count = 0U;", "        return -1;", "    }"]
    lines += ["    for (index = 0U; index < out->count; ++index) {"]
    if node.item is not None:
        lines += ["        if (" + array_item_decode_condition(node.item) + ") {", "            goto fail;", "        }"]
    elif node.variants:
        lines += [f"        if (decode_{node.union_item_name[:-2]}(&node->as.array.items[index], &out->items[index]) != 0) {{",
                  "            goto fail;", "        }"]
    else:
        lines += ["        if (json_value_clone(&out->items[index], &node->as.array.items[index]) != 0) {",
                  "            goto fail;", "        }"]
    lines += ["    }", "    return 0;", "fail:", f"    free_{name}(out);", "    return -1;", "}", ""]
    lines += [f"static int encode_{name}(const {node.array_type} *value, json_writer_t *writer)", "{",
              "    size_t index = 0U;", "    if (value == NULL || writer == NULL || !writer_char(writer, '[')) {", "        return -1;", "    }",
              "    for (index = 0U; index < value->count; ++index) {", "        if (index != 0U && !writer_char(writer, ',')) {", "            return -1;", "        }"]
    if node.item is not None:
        lines += ["        " + line for line in encode_expr(node.item, "value->items[index]")]
    elif node.variants:
        lines += [f"        if (encode_{node.union_item_name[:-2]}(&value->items[index], writer) != 0) {{", "            return -1;", "        }"]
    else:
        lines += ["        if (encode_json_value(&value->items[index], writer) != 0) {", "            return -1;", "        }"]
    lines += ["    }", "    return writer_char(writer, ']') ? 0 : -1;", "}", ""]
    return lines


def array_item_decode_condition(node: NodeModel) -> str:
    target = "out->items[index]"
    if node.kind == "scalar":
        helper = {"string": "json_decode_string", "bool": "json_decode_bool",
                  "int": "json_decode_int", "double": "json_decode_double"}[node.scalar or ""]
        return f"{helper}(&node->as.array.items[index], &{target}) != 0"
    if node.kind == "generic":
        return f"json_value_clone(&{target}, &node->as.array.items[index]) != 0"
    if node.kind == "object":
        return f"decode_{node.type_name[:-2]}(&node->as.array.items[index], &{target}) != 0"
    if node.kind == "array":
        return f"decode_{node.array_type[:-2]}(&node->as.array.items[index], &{target}) != 0"
    raise ValueError("不支持的数组元素类型")


def emit_union_functions(node: NodeModel) -> list[str]:
    if not node.variants:
        return []
    name = node.union_item_name[:-2]
    lines = [f"static void free_{name}({node.union_item_name} *value);",
             f"static int decode_{name}(const json_value_t *node, {node.union_item_name} *out);",
             f"static int encode_{name}(const {node.union_item_name} *value, json_writer_t *writer);", ""]
    lines += [f"static void free_{name}({node.union_item_name} *value)", "{", "    if (value == NULL) {", "        return;", "    }", "    switch (value->kind) {"]
    for index, variant in enumerate(node.variants, start=1):
        lines += [f"    case {node.union_kind_name.upper()}_VARIANT_{index}:",
                  f"        free_{variant.type_name[:-2]}(&value->value.variant_{index});", "        break;"]
    lines += ["    default:", "        break;", "    }", "    memset(value, 0, sizeof(*value));", "}", ""]
    lines += [f"static int decode_{name}(const json_value_t *node, {node.union_item_name} *out)", "{",
              "    if (node == NULL || out == NULL) {", "        return -1;", "    }", "    memset(out, 0, sizeof(*out));"]
    for index, variant in enumerate(node.variants, start=1):
        lines += [f"    if (decode_{variant.type_name[:-2]}(node, &out->value.variant_{index}) == 0) {{",
                  f"        out->kind = {node.union_kind_name.upper()}_VARIANT_{index};", "        return 0;", "    }"]
    lines += ["    return -1;", "}", ""]
    lines += [f"static int encode_{name}(const {node.union_item_name} *value, json_writer_t *writer)", "{",
              "    if (value == NULL || writer == NULL) {", "        return -1;", "    }", "    switch (value->kind) {"]
    for index, variant in enumerate(node.variants, start=1):
        lines += [f"    case {node.union_kind_name.upper()}_VARIANT_{index}:",
                  f"        return encode_{variant.type_name[:-2]}(&value->value.variant_{index}, writer);"]
    lines += ["    default:", "        return -1;", "    }", "}", ""]
    return lines


def emit_type_functions(root: NodeModel) -> list[str]:
    lines: list[str] = []
    emitted: set[int] = set()

    def emit(node: NodeModel) -> None:
        if id(node) in emitted:
            return
        if node.kind == "object":
            for item in node.fields:
                emit(item.node)
            lines.extend(emit_object_functions(node))
        elif node.kind == "array":
            if node.item is not None:
                emit(node.item)
            else:
                for variant in node.variants:
                    emit(variant)
                lines.extend(emit_union_functions(node))
            lines.extend(emit_array_functions(node))
        emitted.add(id(node))

    emit(root)
    return lines


def emit_source(root: NodeModel, header_file_name: str, overview: str, root_base: str) -> str:
    lines = source_header_comment(root_base + ".c", overview)
    lines += [f'#include "{header_file_name}"', "", "#include <errno.h>", "#include <math.h>",
              "#include <stdio.h>", "#include <stdlib.h>", "#include <string.h>", "",
              "#if defined(__GNUC__) || defined(__clang__)",
              "#define JSON_GENERATED_UNUSED __attribute__((unused))",
              "#else",
              "#define JSON_GENERATED_UNUSED",
              "#endif", "#define JSON_GENERATED_OK 0", ""]
    lines += emit_generic_helpers()
    lines += emit_type_functions(root)
    root_type = c_type(root)
    root_lower = root_base
    root_name = root_type[:-2]
    lines += [f"int {root_lower}_from_json(const char *json_text, size_t json_length, {root_type} *out_value)", "{",
              "    json_parser_t parser = {0};", "    json_value_t document;", "    int result = -1;",
              "    if (json_text == NULL || out_value == NULL) {", "        return -1;", "    }", "    json_value_init(&document);",
              "    parser.text = json_text;", "    parser.length = json_length;", "    parser.position = 0U;",
              "    if (!json_parse_value(&parser, &document)) {", "        json_value_free(&document);", "        return -1;", "    }",
              "    json_skip_space(&parser);", "    if (parser.position != parser.length) {", "        json_value_free(&document);", "        return -1;", "    }"]
    if root.kind == "object":
        lines += [f"    result = decode_{root_name}(&document, out_value);"]
    elif root.kind == "array":
        lines += [f"    result = decode_{root_name}(&document, out_value);"]
    elif root.kind == "generic":
        lines += ["    result = json_value_clone(out_value, &document);"]
    elif root.scalar == "string":
        lines += ["    result = json_decode_string(&document, out_value);"]
    elif root.scalar == "bool":
        lines += ["    result = json_decode_bool(&document, out_value);"]
    elif root.scalar == "int":
        lines += ["    result = json_decode_int(&document, out_value);"]
    else:
        lines += ["    result = json_decode_double(&document, out_value);"]
    lines += ["    json_value_free(&document);", "    return result;", "}", ""]
    lines += [f"int {root_lower}_to_json(const {root_type} *value, char *buffer, size_t buffer_size, size_t *written)", "{",
              "    json_writer_t writer = {0};", "    int result = -1;",
              "    if (value == NULL || written == NULL || (buffer == NULL && buffer_size != 0U)) {", "        return -1;", "    }",
              "    writer.buffer = buffer;", "    writer.capacity = buffer_size;"]
    if root.kind == "object":
        lines += [f"    result = encode_{root_name}(value, &writer);"]
    elif root.kind == "array":
        lines += [f"    result = encode_{root_name}(value, &writer);"]
    elif root.kind == "generic":
        lines += ["    result = encode_json_value(value, &writer);"]
    elif root.scalar == "string":
        lines += ["    result = writer_string(&writer, *value) ? 0 : -1;"]
    elif root.scalar == "bool":
        lines += ["    result = writer_put(&writer, *value ? \"true\" : \"false\", *value ? 4U : 5U) ? 0 : -1;"]
    elif root.scalar == "int":
        lines += ["    result = writer_int(&writer, *value) ? 0 : -1;"]
    else:
        lines += ["    result = writer_double(&writer, *value) ? 0 : -1;"]
    lines += ["    if (writer.failed && buffer != NULL) {", "        *written = writer.required;", "        return -2;", "    }", "    if (result != 0 || writer.failed) {", "        *written = writer.required;", "        return -1;", "    }", "    if (buffer != NULL) {", "        buffer[writer.position] = '\\0';", "    }", "    *written = writer.required;", "    return JSON_GENERATED_OK;", "}", ""]
    if root.kind in {"object", "array"}:
        lines += [f"void {root_lower}_free({root_type} *value)", "{", f"    free_{root_name}(value);", "}", ""]
    elif root.kind == "generic":
        lines += [f"void {root_lower}_free({root_type} *value)", "{", "    json_value_free(value);", "}", ""]
    elif root.scalar == "string":
        lines += [f"void {root_lower}_free({root_type} *value)", "{", "    if (value != NULL) {", "        free(*value);", "        *value = NULL;", "    }", "}", ""]
    else:
        lines += [f"void {root_lower}_free({root_type} *value)", "{", "    (void)value;", "}", ""]
    return "\n".join(lines)


def generate(input_value: Any, root_name: str, header_name: str, source_name: str) -> tuple[str, str]:
    root_base = type_identifier(root_name)
    allocator = NameAllocator()
    root = infer_node(input_value, root_base, allocator)
    assign_names(root, root_base, is_root=True)
    guard = re.sub(r"[^A-Za-z0-9]", "_", root_base.upper()) + "_H"
    overview = "由 JSON 示例自动生成的结构体和 JSON 编解码接口。"
    header = emit_header(root, guard, overview, root_base)
    source = emit_source(root, header_name, overview, root_base)
    return header, source


def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    parser = argparse.ArgumentParser(
        description="从 JSON 示例生成 C 结构体以及 JSON 编解码 .h/.c 文件。")
    parser.add_argument("input", type=Path, help="输入 JSON 文件")
    parser.add_argument("-o", "--output-dir", type=Path, default=Path("."), help="输出目录，默认当前目录")
    parser.add_argument("--base-name", help="输出文件基础名，默认使用输入文件名")
    parser.add_argument("--root-name", help="根 C 类型/接口名，默认使用 base-name")
    parser.add_argument("--header", type=Path, help="自定义头文件路径")
    parser.add_argument("--source", type=Path, help="自定义源文件路径")
    return parser.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    args = parse_args(argv)
    try:
        with args.input.open("r", encoding="utf-8") as input_file:
            data = json.load(input_file)
    except (OSError, json.JSONDecodeError) as error:
        print(f"读取 JSON 失败：{error}", file=sys.stderr)
        return 2
    base_name = type_identifier(args.base_name or args.input.stem)
    root_name = type_identifier(args.root_name or base_name)
    header_path = args.header or (args.output_dir / f"{base_name}.h")
    source_path = args.source or (args.output_dir / f"{base_name}.c")
    try:
        header, source = generate(data, root_name, header_path.name, source_path.name)
        header_path.parent.mkdir(parents=True, exist_ok=True)
        source_path.parent.mkdir(parents=True, exist_ok=True)
        header_path.write_text(header, encoding="utf-8")
        source_path.write_text(source, encoding="utf-8")
    except (OSError, TypeError, ValueError) as error:
        print(f"生成 C 代码失败：{error}", file=sys.stderr)
        return 3
    print(f"已生成：{header_path}")
    print(f"已生成：{source_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
