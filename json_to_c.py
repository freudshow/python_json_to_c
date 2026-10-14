#!/usr/bin/env python3
from __future__ import annotations
"""
json_to_c.py - 从 JSON 示例生成 C 结构体和 JSON 编解码代码

整体架构：
----------
本程序是一个单文件的 Python 3.7+ 工具，无第三方依赖。它的工作流程分为四个主要阶段：

1. Schema 推导阶段 (infer_node)
   - 读取输入 JSON，递归分析每个值的类型
   - 区分标量类型、对象、数组、通用值
   - 处理异构数组、大整数等特殊情况

2. 名称分配阶段 (assign_names)
   - 为每个需要在 C 头文件中暴露的类型分配唯一的 C 标识符
   - 处理命名冲突、C 关键字避让、snake_case 规范化

3. 头文件生成阶段 (emit_header)
   - 生成通用 JSON DOM 定义 (json_value_t, json_member_t)
   - 生成类型化结构体定义 (typedef struct ... _t)
   - 生成公共 API 声明 (from_json, to_json, free)

4. 源文件生成阶段 (emit_source)
   - 生成通用 JSON 解析器、写出器、内存管理代码
   - 为每个类型生成 decode/encode/free 函数
   - 支持两种后端：内置解析器、cJSON 库

核心数据结构：
--------------
- NodeModel: 表示推导出的 schema 节点，树状结构
- FieldModel: 对象字段，包含 JSON 名、C 名、子节点
- NameAllocator: 确保生成的 C 标识符唯一

关键算法：
---------
1. 异构数组处理：检查数组元素签名，相同则用单一 item 类型，
   不同则生成 union 类型 (kind + union value)

2. 大整数处理：超出 int64_t 范围的整数标记为 generic，
   保留原始数字文本 (lexeme) 实现无损 round-trip

3. 内存管理：所有动态分配配对 free 函数，错误路径 goto fail 清理

4. 确定性输出：遍历顺序固定，生成的 C 代码顺序可预测，便于 diff
"""

import argparse
import datetime as _datetime
import json
import keyword
import re
import sys
from dataclasses import dataclass, field
from pathlib import Path
from typing import Any, Iterable, Optional


# =============================================================================
# C 语言关键字集合（C99/C11），用于避免生成的标识符与关键字冲突
# =============================================================================
C_KEYWORDS = {
    "auto", "break", "case", "char", "const", "continue", "default", "do",
    "double", "else", "enum", "extern", "float", "for", "goto", "if", "int",
    "long", "register", "return", "short", "signed", "sizeof", "static",
    "struct", "switch", "typedef", "union", "unsigned", "void", "volatile",
    "while", "_Alignas", "_Alignof", "_Atomic", "_Bool", "_Complex",
    "_Generic", "_Imaginary", "_Noreturn", "_Static_assert", "_Thread_local",
}


# =============================================================================
# 标识符规范化工具函数
# =============================================================================
def snake_identifier(value: str, fallback: str = "value") -> str:
    """
    将任意字符串转换为合法的 C 标识符（snake_case）。

    处理步骤：
    1. 非字母数字下划线替换为下划线
    2. 连续下划线折叠为单个，去首尾下划线，转小写
    3. 空结果使用 fallback
    4. 数字开头加 "field_" 前缀
    5. C 关键字/Python 关键字加 "_value" 后缀

    参数:
        value: 原始字符串（如 JSON 字段名）
        fallback: 结果为空时使用的默认名

    返回:
        合法的 C 标识符字符串
    """
    # 步骤 1: 非法字符替换为下划线
    result = re.sub(r"[^0-9A-Za-z_]", "_", value)
    # 步骤 2: 折叠连续下划线、去首尾、转小写
    result = re.sub(r"_+", "_", result).strip("_").lower()
    # 步骤 3: 空字符串处理
    if not result:
        result = fallback
    # 步骤 4: 数字开头加前缀（C 标识符不能以数字开头）
    if result[0].isdigit():
        result = "field_" + result
    # 步骤 5: 关键字冲突处理
    if result in C_KEYWORDS or keyword.iskeyword(result):
        result += "_value"
    return result


def type_identifier(value: str, fallback: str = "root") -> str:
    """
    生成用于类型名、文件名的基础标识符。

    本质上是 snake_identifier 的封装，语义上区分"类型标识"与"字段标识"。
    """
    result = snake_identifier(value, fallback)
    return result


def c_string(value: str) -> str:
    r"""
    将 Python 字符串转换为 UTF-8 C 字符串字面量。

    转义规则：
    - "  -> \"
    - \  -> \\
    - 换行/回车/制表符 -> \n \r \t
    - 可打印 ASCII (0x20-0x7E) 直接输出
    - 其他字节 -> 三位八进制 \ooo（避免十六进制转义吞噬后续字符）

    参数:
        value: Python 字符串

    返回:
        带双引号的 C 字符串字面量，如 "hello\nworld"
    """
    encoded = value.encode("utf-8")
    chunks: list[str] = []
    for byte in encoded:
        if byte == 0x22:          # "
            chunks.append(r"\"")
        elif byte == 0x5C:        # \
            chunks.append(r"\\")
        elif byte == 0x0A:        # \n
            chunks.append(r"\n")
        elif byte == 0x0D:        # \r
            chunks.append(r"\r")
        elif byte == 0x09:        # \t
            chunks.append(r"\t")
        elif 0x20 <= byte <= 0x7E:  # 可打印 ASCII
            chunks.append(chr(byte))
        else:
            # 固定三位八进制，避免 \xHH 吞掉后续十六进制字符
            chunks.append(f"\\{byte:03o}")
    return '"' + "".join(chunks) + '"'


# =============================================================================
# Schema 数据模型
# =============================================================================
@dataclass
class FieldModel:
    """
    对象字段模型。

    属性:
        json_name: JSON 中的原始键名（保留原始大小写、特殊字符，用于编解码时的键匹配）
        c_name: 转换后的 C 字段名（snake_case，合法标识符）
        node: 该字段值对应的 schema 节点
    """
    json_name: str
    c_name: str
    node: "NodeModel"


@dataclass
class NodeModel:
    """
    Schema 树节点，表示推导出的 JSON 类型结构。

    kind 取值:
        "scalar"  - 标量（bool/int/double/string），scalar 字段记录具体类型
        "object"  - 对象，fields 列表包含所有字段
        "array"   - 数组，三种子模式互斥：
                    - item: 同构数组，所有元素同一类型
                    - variants: 异构数组，元素为有限种对象类型之一（union）
                    - 空/通用: item/variants 均为 None，退化为 json_value_t[]
        "generic" - 通用 JSON 值（null、大整数、无法静态类型化的值），用 json_value_t 表示

    命名字段（由 assign_names 填充）:
        type_name: 对象类型的 C 类型名（如 fixture_t）
        array_type: 数组类型的 C 类型名（如 fixture_array_t）
        array_item_name: 数组元素类型名
        union_kind_name: union 判别枚举类型名
        union_item_name: union 容器类型名
    """
    kind: str                      # 节点种类
    path: str                      # 推导时的路径（用于调试/生成唯一基础名）
    scalar: Optional[str] = None   # 标量子类型："bool" | "int" | "double" | "string"
    fields: list[FieldModel] = field(default_factory=list)  # 对象字段列表
    item: Optional["NodeModel"] = None                        # 同构数组元素类型
    variants: list["NodeModel"] = field(default_factory=list) # 异构数组变体列表
    type_name: Optional[str] = None          # 对象类型名
    array_type: Optional[str] = None         # 数组类型名
    array_item_name: Optional[str] = None    # 数组元素类型名
    union_kind_name: Optional[str] = None    # union 判别枚举名
    union_item_name: Optional[str] = None    # union 容器类型名


# =============================================================================
# 名称分配器：确保生成的 C 标识符全局唯一
# =============================================================================
class NameAllocator:
    """
    避免不同 JSON 名称经 snake_identifier 清洗后产生相同 C 标识符的冲突。

    策略：基础名冲突时追加 _2, _3, ... 后缀。
    """

    def __init__(self) -> None:
        self.used: set[str] = set()  # 已分配的标识符集合

    def allocate(self, base: str) -> str:
        """
        分配一个唯一标识符。

        参数:
            base: 基础名（已通过 snake_identifier 规范化）

        返回:
            唯一的标识符字符串
        """
        candidate = snake_identifier(base)
        original = candidate
        suffix = 2
        # 循环直到找到未使用的名字
        while candidate in self.used:
            candidate = f"{original}_{suffix}"
            suffix += 1
        self.used.add(candidate)
        return candidate


# =============================================================================
# Schema 推导：从 JSON 值递归构建 NodeModel 树
# =============================================================================
def scalar_kind(value: Any) -> Optional[str]:
    """
    识别 JSON 标量值的具体类型。

    注意：Python bool 是 int 的子类，必须先判断 bool 再判断 int。

    返回:
        "bool" | "int" | "double" | "string" | None（非标量）
    """
    if value is None:
        return None  # null -> generic
    if isinstance(value, bool):
        return "bool"
    if isinstance(value, int):
        # int64_t 范围：-2^63 .. 2^63-1
        if -(2**63) <= value <= 2**63 - 1:
            return "int"
        return "double"  # 超大整数用 double 近似（实际会走 generic 保留 lexeme）
    if isinstance(value, float):
        return "double"
    if isinstance(value, str):
        return "string"
    return None


def node_signature(node: NodeModel) -> tuple[Any, ...]:
    """
    计算节点的结构签名，用于判断两个节点是否类型等价。

    签名是一个嵌套元组，完全捕获类型结构，忽略字段名、路径等非类型信息。
    用于：
    - 数组同构性检查（所有元素签名相同 -> 单一 item 类型）
    - 异构数组去重（相同签名的变体合并为一个）

    返回:
        递归元组签名
    """
    if node.kind == "scalar":
        return ("scalar", node.scalar)
    if node.kind == "generic":
        return ("generic",)
    if node.kind == "object":
        # 对象签名：字段名 + 子节点签名（按顺序）
        return (
            "object",
            tuple((item.json_name, node_signature(item.node)) for item in node.fields),
        )
    if node.kind == "array":
        if node.item is not None:
            return ("array", node_signature(node.item))
        # variants 数组：每个变体的签名组成元组
        return ("array", tuple(node_signature(item) for item in node.variants))
    raise ValueError(f"未知节点类型: {node.kind}")


def infer_node(value: Any, path: str, allocator: NameAllocator) -> NodeModel:
    """
    从单个 JSON 值递归推导 schema（核心推导算法）。

    算法要点：
    1. 大整数（超出 int64_t）标记为 generic，保留原始数字文本
    2. 标量直接映射 scalar 类型
    3. null 映射为 generic
    4. 对象：递归推导每个字段，字段名去重（同名字段取最后一个）
    5. 数组：
       - 空数组 -> 通用数组（json_value_t[]）
       - 全对象元素：按签名分组，单一签名 -> 同构；多签名 -> variants（union）
       - 非对象元素：签名全同 -> 同构；否则 -> generic 数组（避免 int/double 混合丢精度）

    参数:
        value: JSON 值（dict/list/str/int/float/bool/None）
        path: 当前路径字符串（用于生成唯一基础名）
        allocator: 名称分配器（此处仅用于对象字段名去重）

    返回:
        根 NodeModel
    """
    # 大整数检测：超出 int64_t 无法无损存入 C 固定类型，标记为 generic
    if (
        isinstance(value, int)
        and not isinstance(value, bool)
        and not (-(2**63) <= value <= 2**63 - 1)
    ):
        return NodeModel("generic", path)

    kind = scalar_kind(value)
    if kind is not None:
        return NodeModel("scalar", path, scalar=kind)

    if value is None:
        return NodeModel("generic", path)

    if isinstance(value, dict):
        result = NodeModel("object", path)
        used_fields: set[str] = set()  # 本对象内字段 C 名去重
        for key, child in value.items():
            base = snake_identifier(str(key), "field")
            c_name = base
            suffix = 2
            # 同一对象内字段名冲突加后缀
            while c_name in used_fields:
                c_name = f"{base}_{suffix}"
                suffix += 1
            used_fields.add(c_name)
            result.fields.append(
                FieldModel(
                    str(key),
                    c_name,
                    infer_node(child, f"{path}_{c_name}", allocator),
                )
            )
        return result

    if isinstance(value, list):
        if not value:
            # 空数组：无法推导元素类型，退化为 generic 数组
            return NodeModel("array", path)

        # 递归推导每个元素
        children = [
            infer_node(item, f"{path}_item_{index + 1}", allocator)
            for index, item in enumerate(value)
        ]

        # 全部为对象元素：尝试按结构分组生成 union
        if all(item.kind == "object" for item in children):
            grouped: list[NodeModel] = []
            signatures: list[tuple[Any, ...]] = []
            for child in children:
                signature = node_signature(child)
                if signature not in signatures:
                    signatures.append(signature)
                    grouped.append(child)
            if len(grouped) == 1:
                # 所有对象结构相同 -> 同构数组
                return NodeModel("array", path, item=grouped[0])
            # 多种结构 -> variants（union 类型）
            return NodeModel("array", path, variants=grouped)

        # 非对象元素或混合：检查签名是否完全一致
        child_signatures = {node_signature(item) for item in children}
        if len(child_signatures) == 1:
            return NodeModel("array", path, item=children[0])

        # 签名不一致（如 int/double 混合）：不能统一为 double（大整数丢精度）
        # 退化为 generic 数组，保留每个数字的原始文本
        return NodeModel("array", path)

    raise TypeError(f"不支持的 JSON 值: {type(value)!r}")


# =============================================================================
# 名称分配：为每个需要暴露的类型生成 C 类型名
# =============================================================================
def assign_names(node: NodeModel, base: str, is_root: bool = False) -> None:
    """
    后序遍历 schema 树，为每个复合类型分配 C 类型名。

    命名规则：
    - 对象: {base}_t
    - 根数组: {base}_t；非根数组: {base}_array_t
    - 同构数组元素: 递归分配，base + "_item"
    - 异构数组变体: base + "_variant_{index}"，并生成 union 相关名字
    - generic/scalar: 无需命名，直接用内置类型

    参数:
        node: 当前节点
        base: 基础名（已规范化）
        is_root: 是否为根节点（影响数组命名）
    """
    base = type_identifier(base)
    if node.kind == "object":
        node.type_name = base + "_t"
        for item in node.fields:
            assign_names(item.node, base + "_" + item.c_name)
    elif node.kind == "array":
        # 根数组用 {base}_t，嵌套数组用 {base}_array_t
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
            # 通用数组元素类型
            node.array_item_name = "json_value_t"
    elif node.kind == "generic":
        return  # 使用 json_value_t，无需命名
    elif node.kind == "scalar":
        return  # 使用内置标量类型，无需命名


def c_type(node: NodeModel) -> str:
    """
    根据节点类型返回对应的 C 类型字符串（用于结构体字段声明、函数签名等）。

    映射表：
        object  -> type_name (如 fixture_t)
        array   -> array_type (如 fixture_array_t)
        generic -> "json_value_t"
        string  -> "char *"
        bool    -> "bool"
        int     -> "int64_t"
        double  -> "double"
    """
    if node.kind == "object":
        if node.type_name is None:
            raise ValueError("对象类型尚未命名")
        return node.type_name
    if node.kind == "array":
        if node.array_type is None:
            # 命名阶段尚未完成时的占位；真正生成前会再次计算
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
    """
    深度优先遍历 schema 树，产出所有节点（用于去重发射定义）。

    产出顺序：先当前节点，再递归子节点（对象字段序、数组 item、variants 序）。
    """
    yield node
    if node.kind == "object":
        for item in node.fields:
            yield from all_nodes(item.node)
    elif node.kind == "array":
        if node.item is not None:
            yield from all_nodes(node.item)
        for variant in node.variants:
            yield from all_nodes(variant)


# =============================================================================
# 头文件生成：类型定义与公共 API 声明
# =============================================================================
def emit_node_definitions(root: NodeModel) -> list[str]:
    """
    生成所有复合类型的 C 定义（typedef struct/enum）。

    使用 id(node) 去重，确保共享子类型只定义一次。
    产出顺序：依赖先行（先递归子节点，再定义当前节点）。

    返回:
        C 代码行列表
    """
    lines: list[str] = []
    emitted: set[int] = set()

    def emit(node: NodeModel) -> None:
        if id(node) in emitted:
            return
        if node.kind == "object":
            # 先发射所有字段类型
            for item in node.fields:
                emit(item.node)
            lines.append(f"typedef struct {node.type_name} {{")
            if not node.fields:
                # 空结构体在 C 中非法，加占位字段
                lines.append("    uint8_t _unused;  // 空对象占位字段")
            else:
                for item in node.fields:
                    lines.append(
                        f"    {c_type(item.node)} {item.c_name};  // JSON 字段 {item.json_name!r}"
                    )
            lines.append(f"}} {node.type_name};")
            lines.append("")
        elif node.kind == "array":
            if node.item is not None:
                emit(node.item)
            else:
                for variant in node.variants:
                    emit(variant)
                if node.variants:
                    # 生成 union 判别枚举
                    lines.append("typedef enum {")
                    for index, variant in enumerate(node.variants):
                        lines.append(
                            f"    {node.union_kind_name.upper()}_VARIANT_{index + 1} = {index},"
                        )
                    lines.append(f"}} {node.union_kind_name};")
                    lines.append("")
                    # 生成 union 容器结构体
                    lines.append("typedef struct {")
                    lines.append(
                        f"    {node.union_kind_name} kind;  // 当前 union 成员"
                    )
                    lines.append("    union {")
                    for index, variant in enumerate(node.variants):
                        lines.append(
                            f"        {variant.type_name} variant_{index + 1};"
                        )
                    lines.append("    } value;")
                    lines.append(f"}} {node.union_item_name};")
                    lines.append("")
            # 生成数组结构体：items 指针 + count
            lines.append("typedef struct {")
            lines.append(f"    {node.array_item_name} *items;  // 数组元素")
            lines.append("    size_t count;  // 元素数量")
            lines.append(f"}} {node.array_type};")
            lines.append("")
        emitted.add(id(node))

    emit(root)
    return lines


def source_header_comment(file_name: str, overview: str) -> list[str]:
    """
    生成标准文件头注释块。

    包含：文件名、作者、版本、生成日期、概述、修改记录。
    """
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
    """
    生成完整的 .h 头文件内容。

    结构：
    1. 文件头注释
    2. 头文件保护宏
    3. 标准头文件引用
    4. 通用 JSON DOM 定义（json_value_t, json_member_t, json_value_kind_t）
    5. 通用 DOM 内存管理声明（json_value_free）
    6. 类型化结构体定义（emit_node_definitions）
    7. 根类型的公共 API 声明：from_json / to_json / free
    8. 头文件保护尾
    """
    lines = source_header_comment(root_base + ".h", overview)
    lines += [
        f"#ifndef {guard}",
        f"#define {guard}",
        "",
        "#include <stdbool.h>",
        "#include <stddef.h>",
        "#include <stdint.h>",
        "",
        # 通用 JSON 值类型枚举
        "typedef enum {",
        "    JSON_VALUE_NULL = 0,",
        "    JSON_VALUE_BOOL,",
        "    JSON_VALUE_NUMBER,",
        "    JSON_VALUE_STRING,",
        "    JSON_VALUE_ARRAY,",
        "    JSON_VALUE_OBJECT",
        "} json_value_kind_t;",
        "",
        # 前向声明
        "typedef struct json_value json_value_t;",
        "typedef struct json_member json_member_t;",
        "",
        # json_value_t：标签联合，支持所有 JSON 类型
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
        # json_member_t：对象成员（键值对）
        "struct json_member {",
        "    char *key;",
        "    json_value_t value;",
        "};",
        "",
        "/** 释放通用 JSON 值及其递归分配的内存。 */",
        "void json_value_free(json_value_t *value);",
        "",
    ]
    # 类型化结构体定义
    lines += emit_node_definitions(root)
    root_type = c_type(root)
    root_lower = root_base
    # 公共 API 声明
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


# =============================================================================
# 字段级编解码代码片段生成
# =============================================================================
def field_decode_expr(node: NodeModel, source: str, target: str) -> list[str]:
    """
    生成单字段解码代码片段：将 json_value_t 解码为类型化字段。

    参数:
        node: 字段的 schema 节点
        source: 源表达式（如 "member->value"）
        target: 目标表达式（如 "out->field_name"）

    返回:
        C 代码行列表，使用 goto fail 错误处理模式
    """
    if node.kind == "scalar":
        helper = {
            "string": "json_decode_string",
            "bool": "json_decode_bool",
            "int": "json_decode_int",
            "double": "json_decode_double",
        }[node.scalar or ""]
        return [
            f"if ({helper}(&{source}, &{target}) != 0) {{",
            "    goto fail;",
            "}"
        ]
    if node.kind == "generic":
        return [
            f"if (json_value_clone(&{target}, &{source}) != 0) {{",
            "    goto fail;",
            "}"
        ]
    if node.kind == "object":
        return [
            f"if (decode_{node.type_name[:-2]}(&{source}, &{target}) != 0) {{",
            "    goto fail;",
            "}"
        ]
    if node.kind == "array":
        return [
            f"if (decode_{node.array_type[:-2]}(&{source}, &{target}) != 0) {{",
            "    goto fail;",
            "}"
        ]
    raise ValueError("不支持的字段类型")


def field_free_lines(node: NodeModel, target: str) -> list[str]:
    """
    生成单字段释放代码片段。

    标量 string 需要 free；generic 调用 json_value_free；
    object/array 调用对应的 free_X 函数；其他标量无需释放。
    """
    if node.kind == "scalar":
        return [f"free({target});"] if node.scalar == "string" else []
    if node.kind == "generic":
        return [f"json_value_free(&{target});"]
    if node.kind == "object":
        return [f"free_{node.type_name[:-2]}(&{target});"]
    if node.kind == "array":
        return [f"free_{node.array_type[:-2]}(&{target});"]
    raise ValueError("不支持的字段类型")


# =============================================================================
# 通用 JSON DOM、解析器、写出器（内置后端核心实现）
# =============================================================================
def emit_generic_helpers() -> list[str]:
    """
    生成内置 JSON 后端的核心运行时代码（约 800 行 C 代码）。

    包含模块：
    1. json_parser_t / json_writer_t - 解析/写出状态机
    2. json_value_init / json_value_free - DOM 内存管理
    3. json_parse_string / _array / _object / _number / _value - 递归下降解析器
    4. json_object_get - 对象键查找
    5. json_copy_string / json_value_clone - 深拷贝
    6. json_decode_* - 从通用 DOM 解码到类型化值
    7. json_writer_t + writer_put/char/string/int/double - 写出器
    8. encode_json_value - 通用 DOM 编码为 JSON 文本

    设计特点：
    - 无递归深度限制（C 栈处理嵌套）
    - 解析器保留数字原文 - 支持大整数无损 round-trip
    - 写出器支持两阶段：先计算所需长度，再写入缓冲区（buffer 为 NULL 时仅计算长度）
    - 所有分配检查返回值，错误路径清理已分配内存
    """
    return r"""/* ---------- 通用 JSON DOM、解析器和写出器 ---------- */
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
""".splitlines()


# =============================================================================
# 类型化对象/数组/Union 的 encode/decode/free 函数生成
# =============================================================================
def emit_object_functions(node: NodeModel) -> list[str]:
    """
    为单个 object 类型生成三个静态函数：
    - free_X: 释放对象内部动态内存（字符串、嵌套对象/数组）
    - decode_X: 从通用 json_value_t 解码为类型化结构体
    - encode_X: 将类型化结构体编码为 JSON（通过 json_writer_t）

    使用 goto fail 模式处理错误：失败时调用 free_X 清理已分配内存。
    """
    name = node.type_name[:-2]  # 去掉 "_t" 后缀得到基础名
    lines = [
        f"static void free_{name}({node.type_name} *value);",
        f"static int decode_{name}(const json_value_t *node, {node.type_name} *out);",
        f"static int encode_{name}(const {node.type_name} *value, json_writer_t *writer);",
        "",
    ]
    # free 函数
    lines += [
        f"static void free_{name}({node.type_name} *value)",
        "{",
        "    if (value == NULL) {",
        "        return;",
        "    }",
    ]
    for item in node.fields:
        lines += [*field_free_lines(item.node, f"value->{item.c_name}")]
    lines += [f"    memset(value, 0, sizeof(*value));", "}", ""]
    # decode 函数
    lines += [
        f"static int decode_{name}(const json_value_t *node, {node.type_name} *out)",
        "{",
    ]
    if node.fields:
        lines.append("    const json_member_t *member = NULL;")
    lines += [
        "    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {",
        "        return -1;",
        "    }",
        f"    if (node->as.object.count != {len(node.fields)}U) {{",
        "        return -1;",
        "    }",
        "    memset(out, 0, sizeof(*out));",
    ]
    for item in node.fields:
        lines += [
            f"    member = json_object_get(node, {c_string(item.json_name)});",
            "    if (member == NULL) {",
            "        goto fail;",
            "    }",
        ]
        for line in field_decode_expr(
            item.node, "member->value", f"out->{item.c_name}"
        ):
            lines.append("    " + line if line else line)
    lines += ["    return 0;"]
    if node.fields:
        lines += ["fail:", f"    free_{name}(out);", "    return -1;"]
    lines += ["}", ""]
    # encode 函数
    lines += [
        f"static int encode_{name}(const {node.type_name} *value, json_writer_t *writer)",
        "{",
    ]
    if node.fields:
        lines.append("    bool first = true;")
    lines += [
        "    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {",
        "        return -1;",
        "    }",
    ]
    for item in node.fields:
        lines += [
            "    if (!first && !writer_char(writer, ',')) {",
            "        return -1;",
            "    }",
            "    first = false;",
            f"    if (!writer_string(writer, {c_string(item.json_name)}) || !writer_char(writer, ':')) {{",
            "        return -1;",
            "    }",
        ]
        lines += [
            "    " + line for line in encode_expr(item.node, f"value->{item.c_name}")
        ]
    lines += ["    return writer_char(writer, '}') ? 0 : -1;", "}", ""]
    return lines


def encode_expr(node: NodeModel, value: str) -> list[str]:
    """
    生成表达式编码代码片段：将类型化值写入 json_writer_t。

    参数:
        node: 值的 schema 节点
        value: C 表达式字符串（如 "value->field"）

    返回:
        C 代码行列表
    """
    if node.kind == "scalar":
        if node.scalar == "string":
            return [f"if (!writer_string(writer, {value})) {{", "    return -1;", "}"]
        if node.scalar == "bool":
            return [
                f'if (!writer_put(writer, {value} ? "true" : "false", {value} ? 4U : 5U)) {{',
                "    return -1;",
                "}",
            ]
        if node.scalar == "int":
            return [f"if (!writer_int(writer, {value})) {{", "    return -1;", "}"]
        return [f"if (!writer_double(writer, {value})) {{", "    return -1;", "}"]
    if node.kind == "generic":
        return [
            f"if (encode_json_value(&{value}, writer) != 0) {{",
            "    return -1;",
            "}",
        ]
    if node.kind == "object":
        return [
            f"if (encode_{node.type_name[:-2]}(&{value}, writer) != 0) {{",
            "    return -1;",
            "}",
        ]
    if node.kind == "array":
        return [
            f"if (encode_{node.array_type[:-2]}(&{value}, writer) != 0) {{",
            "    return -1;",
            "}",
        ]
    raise ValueError("不支持的编码类型")


def emit_array_functions(node: NodeModel) -> list[str]:
    """
    为数组类型生成 free/decode/encode 函数。

    三种情况：
    1. 同构数组: item != None，元素类型统一
    2. 异构数组: variants 非空，元素为 union 类型
    3. 通用数组: item/variants 均为 None，元素为 json_value_t
    """
    name = node.array_type[:-2]  # 去掉 "_t"
    lines = [
        f"static void free_{name}({node.array_type} *value);",
        f"static int decode_{name}(const json_value_t *node, {node.array_type} *out);",
        f"static int encode_{name}(const {node.array_type} *value, json_writer_t *writer);",
        "",
    ]
    item_kind = node.item or (node.variants[0] if node.variants else None)
    lines += [
        f"static void free_{name}({node.array_type} *value)",
        "{",
        "    if (value == NULL) {",
        "        return;",
        "    }",
    ]
    if node.item is not None:
        free_item = field_free_lines(node.item, f"value->items[index]")
        if free_item:
            lines += [
                "    size_t index = 0U;",
                "    for (index = 0U; index < value->count; ++index) {",
                *["        " + line for line in free_item],
                "    }",
            ]
    elif node.variants:
        lines += [
            "    size_t index = 0U;",
            "    for (index = 0U; index < value->count; ++index) {",
            f"        free_{node.union_item_name[:-2]}(&value->items[index]);",
            "    }",
        ]
    else:
        lines += [
            "    size_t index = 0U;",
            "    for (index = 0U; index < value->count; ++index) {",
            "        json_value_free(&value->items[index]);",
            "    }",
        ]
    lines += [
        "    free(value->items);",
        "    value->items = NULL;",
        "    value->count = 0U;",
        "}",
        "",
    ]
    # decode
    lines += [
        f"static int decode_{name}(const json_value_t *node, {node.array_type} *out)",
        "{",
        "    size_t index = 0U;",
        "    if (node == NULL || out == NULL || node->kind != JSON_VALUE_ARRAY) {",
        "        return -1;",
        "    }",
        "    memset(out, 0, sizeof(*out));",
        "    out->count = node->as.array.count;",
    ]
    lines += [
        "    if (out->count == 0U) {",
        "        return 0;",
        "    }",
        "    out->items = calloc(out->count, sizeof(*out->items));",
        "    if (out->items == NULL) {",
        "        out->count = 0U;",
        "        return -1;",
        "    }",
    ]
    lines += ["    for (index = 0U; index < out->count; ++index) {"]
    if node.item is not None:
        lines += [
            "        if (" + array_item_decode_condition(node.item) + ") {",
            "            goto fail;",
            "        }",
        ]
    elif node.variants:
        lines += [
            f"        if (decode_{node.union_item_name[:-2]}(&node->as.array.items[index], &out->items[index]) != 0) {{",
            "            goto fail;",
            "        }",
        ]
    else:
        lines += [
            "        if (json_value_clone(&out->items[index], &node->as.array.items[index]) != 0) {",
            "            goto fail;",
            "        }",
        ]
    lines += [
        "    }",
        "    return 0;",
        "fail:",
        f"    free_{name}(out);",
        "    return -1;",
        "}",
        "",
    ]
    # encode
    lines += [
        f"static int encode_{name}(const {node.array_type} *value, json_writer_t *writer)",
        "{",
        "    size_t index = 0U;",
        "    if (value == NULL || writer == NULL || !writer_char(writer, '[')) {",
        "        return -1;",
        "    }",
        "    for (index = 0U; index < value->count; ++index) {",
        "        if (index != 0U && !writer_char(writer, ',')) {",
        "            return -1;",
        "        }",
    ]
    if node.item is not None:
        lines += [
            "        " + line for line in encode_expr(node.item, "value->items[index]")
        ]
    elif node.variants:
        lines += [
            f"        if (encode_{node.union_item_name[:-2]}(&value->items[index], writer) != 0) {{",
            "            return -1;",
            "        }",
        ]
    else:
        lines += [
            "        if (encode_json_value(&value->items[index], writer) != 0) {",
            "            return -1;",
            "        }",
        ]
    lines += ["    }", "    return writer_char(writer, ']') ? 0 : -1;", "}", ""]
    return lines


def array_item_decode_condition(node: NodeModel) -> str:
    """
    生成数组元素解码的条件表达式字符串（用于 if 语句中）。

    根据元素类型调用对应的 decode_X 或 json_value_clone。
    """
    target = "out->items[index]"
    if node.kind == "scalar":
        helper = {
            "string": "json_decode_string",
            "bool": "json_decode_bool",
            "int": "json_decode_int",
            "double": "json_decode_double",
        }[node.scalar or ""]
        return f"{helper}(&node->as.array.items[index], &{target}) != 0"
    if node.kind == "generic":
        return f"json_value_clone(&{target}, &node->as.array.items[index]) != 0"
    if node.kind == "object":
        return f"decode_{node.type_name[:-2]}(&node->as.array.items[index], &{target}) != 0"
    if node.kind == "array":
        return f"decode_{node.array_type[:-2]}(&node->as.array.items[index], &{target}) != 0"
    raise ValueError("不支持的数组元素类型")


def emit_union_functions(node: NodeModel) -> list[str]:
    """
    为异构数组的 union 类型生成 free/decode/encode 函数。

    union 结构：
        typedef enum { KIND_VARIANT_1, KIND_VARIANT_2, ... } kind_t;
        typedef struct { kind_t kind; union { T1 v1; T2 v2; ... } value; } item_t;

    decode 策略：尝试按变体顺序解码，第一个成功的即为匹配类型。
    """
    if not node.variants:
        return []
    name = node.union_item_name[:-2]
    lines = [
        f"static void free_{name}({node.union_item_name} *value);",
        f"static int decode_{name}(const json_value_t *node, {node.union_item_name} *out);",
        f"static int encode_{name}(const {node.union_item_name} *value, json_writer_t *writer);",
        "",
    ]
    # free: switch 判别 kind，释放对应变体
    lines += [
        f"static void free_{name}({node.union_item_name} *value)",
        "{",
        "    if (value == NULL) {",
        "        return;",
        "    }",
        "    switch (value->kind) {",
    ]
    for index, variant in enumerate(node.variants, start=1):
        lines += [
            f"    case {node.union_kind_name.upper()}_VARIANT_{index}:",
            f"        free_{variant.type_name[:-2]}(&value->value.variant_{index});",
            "        break;",
        ]
    lines += [
        "    default:",
        "        break;",
        "    }",
        "    memset(value, 0, sizeof(*value));",
        "}",
        "",
    ]
    # decode: 顺序尝试每个变体的 decode
    lines += [
        f"static int decode_{name}(const json_value_t *node, {node.union_item_name} *out)",
        "{",
        "    if (node == NULL || out == NULL) {",
        "        return -1;",
        "    }",
        "    memset(out, 0, sizeof(*out));",
    ]
    for index, variant in enumerate(node.variants, start=1):
        lines += [
            f"    if (decode_{variant.type_name[:-2]}(node, &out->value.variant_{index}) == 0) {{",
            f"        out->kind = {node.union_kind_name.upper()}_VARIANT_{index};",
            "        return 0;",
            "    }",
        ]
    lines += ["    return -1;", "}", ""]
    # encode: switch 判别 kind，调用对应 encode
    lines += [
        f"static int encode_{name}(const {node.union_item_name} *value, json_writer_t *writer)",
        "{",
        "    if (value == NULL || writer == NULL) {",
        "        return -1;",
        "    }",
        "    switch (value->kind) {",
    ]
    for index, variant in enumerate(node.variants, start=1):
        lines += [
            f"    case {node.union_kind_name.upper()}_VARIANT_{index}:",
            f"        return encode_{variant.type_name[:-2]}(&value->value.variant_{index}, writer);",
        ]
    lines += ["    default:", "        return -1;", "    }", "}", ""]
    return lines


def emit_type_functions(root: NodeModel) -> list[str]:
    """
    遍历 schema 树，收集所有类型的函数定义（按依赖顺序：子类型先于父类型）。

    使用 id(node) 去重，避免共享子类型重复生成。
    """
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


# =============================================================================
# cJSON 后端辅助代码（可选）
# =============================================================================
def emit_cjson_helpers() -> list[str]:
    """
    生成 cJSON 后端所需的转换辅助代码。

    功能：
    1. cjson_has_nul_escape - 检测字符串中是否包含 \u0000（cJSON 不支持）
    2. cjson_number_is_safe - 检查数值是否在 IEEE 754 安全整数范围内（±2^53-1）
    3. cjson_tree_numbers_are_safe - 递归检查整棵树数值安全性
    4. json_value_from_cjson - 将 cJSON 树转换为内部 json_value_t DOM

    限制：
    - cJSON 用 double 存数字，大整数会丢失精度 -> 拒绝超出安全范围的数字
    - cJSON 字符串是 NUL 终止 -> 拒绝含 \u0000 的输入
    """
    return r"""/* ---------- cJSON conversion helpers ---------- */
static bool cjson_has_nul_escape(const char *text, size_t length)
{
    bool in_string = false;
    size_t index = 0U;
    if (text == NULL) {
        return false;
    }
    for (index = 0U; index < length; ++index) {
        if (!in_string) {
            if (text[index] == '"') {
                in_string = true;
            }
        } else if (text[index] == '\\') {
            if (length - index >= 6U && text[index + 1U] == 'u' &&
                text[index + 2U] == '0' && text[index + 3U] == '0' &&
                text[index + 4U] == '0' && text[index + 5U] == '0') {
                return true;
            }
            if (index + 1U < length) {
                index += 1U;
            }
        } else if (text[index] == '"') {
            in_string = false;
        }
    }
    return false;
}

static bool cjson_number_is_safe(const cJSON *value)
{
    if (value == NULL || !cJSON_IsNumber(value) || !isfinite(value->valuedouble)) {
        return false;
    }
    return fabs(value->valuedouble) <= 9007199254740991.0;
}

static bool cjson_tree_numbers_are_safe(const cJSON *value)
{
    const cJSON *child = NULL;
    if (value == NULL) {
        return false;
    }
    if (cJSON_IsNumber(value)) {
        return cjson_number_is_safe(value);
    }
    if (cJSON_IsArray(value) || cJSON_IsObject(value)) {
        for (child = value->child; child != NULL; child = child->next) {
            if (!cjson_tree_numbers_are_safe(child)) {
                return false;
            }
        }
    }
    return true;
}

static int json_value_from_cjson(json_value_t *output, const cJSON *input)
{
    const cJSON *child = NULL;
    size_t index = 0U;
    int count = 0;
    char *number_text = NULL;
    char *number_end = NULL;
    if (output == NULL || input == NULL) {
        return -1;
    }
    json_value_init(output);
    if (cJSON_IsNull(input)) {
        output->kind = JSON_VALUE_NULL;
        return 0;
    }
    if (cJSON_IsBool(input)) {
        output->kind = JSON_VALUE_BOOL;
        output->as.boolean = cJSON_IsTrue(input);
        return 0;
    }
    if (cJSON_IsNumber(input)) {
        if (!cjson_number_is_safe(input)) {
            goto fail;
        }
        output->kind = JSON_VALUE_NUMBER;
        output->as.number.real = input->valuedouble;
        number_text = cJSON_PrintUnformatted(input);
        if (number_text == NULL ||
            json_copy_string(number_text, &output->as.number.lexeme) != 0) {
            goto fail;
        }
        output->as.number.is_integer =
            strchr(number_text, '.') == NULL && strchr(number_text, 'e') == NULL &&
            strchr(number_text, 'E') == NULL;
        if (output->as.number.is_integer) {
            long long integer = 0LL;
            errno = 0;
            integer = strtoll(number_text, &number_end, 10);
            if (errno == ERANGE || number_end == number_text || *number_end != '\0') {
                cJSON_free(number_text);
                number_text = NULL;
                goto fail;
            }
            output->as.number.integer = (int64_t)integer;
        }
        cJSON_free(number_text);
        return 0;
    }
    if (cJSON_IsString(input)) {
        if (input->valuestring == NULL) {
            goto fail;
        }
        output->kind = JSON_VALUE_STRING;
        if (json_copy_string(input->valuestring, &output->as.string) != 0) {
            goto fail;
        }
        return 0;
    }
    if (cJSON_IsArray(input)) {
        count = cJSON_GetArraySize(input);
        if (count < 0) {
            goto fail;
        }
        output->kind = JSON_VALUE_ARRAY;
        output->as.array.count = 0U;
        if (count != 0) {
            output->as.array.items = (json_value_t *)calloc(
                (size_t)count, sizeof(*output->as.array.items));
            if (output->as.array.items == NULL) {
                goto fail;
            }
        }
        for (index = 0U; index < (size_t)count; ++index) {
            child = cJSON_GetArrayItem(input, (int)index);
            if (child == NULL ||
                json_value_from_cjson(&output->as.array.items[index], child) != 0) {
                goto fail;
            }
            output->as.array.count += 1U;
        }
        return 0;
    }
    if (cJSON_IsObject(input)) {
        count = cJSON_GetArraySize(input);
        if (count < 0) {
            goto fail;
        }
        output->kind = JSON_VALUE_OBJECT;
        output->as.object.count = 0U
        if (count != 0) {
            output->as.object.items = (json_member_t *)calloc(
                (size_t)count, sizeof(*output->as.object.items));
            if (output->as.object.items == NULL) {
                goto fail;
            }
        }
        child = input->child;
        for (index = 0U; index < (size_t)count; ++index) {
            if (child == NULL || child->string == NULL ||
                json_copy_string(child->string,
                                 &output->as.object.items[index].key) != 0) {
                goto fail;
            }
            output->as.object.count += 1U;
            if (json_value_from_cjson(&output->as.object.items[index].value, child) != 0) {
                goto fail;
            }
            child = child->next;
        }
        if (child != NULL) {
            goto fail;
        }
        return 0;
    }
fail:
    cJSON_free(number_text);
    json_value_free(output);
    return -1;
}
""".splitlines()


# =============================================================================
# 源文件生成：组装完整的 .c 文件
# =============================================================================
def emit_source(
    root: NodeModel,
    header_file_name: str,
    overview: str,
    root_base: str,
    backend: str = "builtin",
    cjson_header: str = "cjson/cJSON.h",
) -> str:
    """
    生成完整的 .c 源文件内容。

    结构：
    1. 文件头注释
    2. #include 头文件、标准库头文件
    3. JSON_GENERATED_UNUSED 宏定义（消除未使用警告）
    4. 通用 JSON 运行时代码（emit_generic_helpers）
    5. cJSON 后端辅助代码（如果选择 cJSON）
    6. 所有类型化函数（emit_type_functions）
    7. 根类型的公共 API 实现：
       - {root}_from_json: 解析 JSON -> 通用 DOM -> 类型化结构
       - {root}_to_json: 类型化结构 -> 写出器 -> JSON 文本（或 cJSON 回写）
       - {root}_free: 释放类型化结构内存
    """
    lines = source_header_comment(root_base + ".c", overview)
    lines += [
        f'#include "{header_file_name}"',
        "",
        "#include <errno.h>",
        "#include <math.h>",
        "#include <stdio.h>",
        "#include <stdlib.h>",
        "#include <string.h>",
        "",
        "#if defined(__GNUC__) || defined(__clang__)",
        "#define JSON_GENERATED_UNUSED __attribute__((unused))",
        "#else",
        "#define JSON_GENERATED_UNUSED",
        "#endif",
        "#define JSON_GENERATED_OK 0",
        "",
    ]
    if backend == "cjson":
        # 在 errno.h 之后插入 cJSON 头文件
        lines.insert(lines.index("#include <errno.h>"), f"#include <{cjson_header}>")
    lines += emit_generic_helpers()
    if backend == "cjson":
        lines += emit_cjson_helpers()
    lines += emit_type_functions(root)
    root_type = c_type(root)
    root_lower = root_base
    root_name = root_type[:-2]  # 去掉 _t
    # from_json 实现
    lines += [
        f"int {root_lower}_from_json(const char *json_text, size_t json_length, {root_type} *out_value)",
        "{",
    ]
    if backend == "cjson":
        lines += [
            "    char *input_copy = NULL;",
            "    const char *parse_end = NULL;",
            "    cJSON *cjson_document = NULL;",
            "    bool parsed_exactly = false;",
            "    json_value_t document;",
            "    int result = -1;",
            "    if (json_text == NULL || out_value == NULL || json_length == SIZE_MAX) {",
            "        return -1;",
            "    }",
            "    if (cjson_has_nul_escape(json_text, json_length)) {",
            "        return -1;",
            "    }",
            "    json_value_init(&document);",
            "    input_copy = (char *)malloc(json_length + 1U);",
            "    if (input_copy == NULL) {",
            "        return -1;",
            "    }",
            "    memcpy(input_copy, json_text, json_length);",
            "    input_copy[json_length] = '\\0';",
            "    cjson_document = cJSON_ParseWithLengthOpts(input_copy, json_length + 1U,",
            "                                                &parse_end, 1);",
            "    parsed_exactly = cjson_document != NULL &&",
            "                    parse_end == input_copy + json_length;",
            "    free(input_copy);",
            "    if (!parsed_exactly || !cjson_tree_numbers_are_safe(cjson_document) ||",
            "        json_value_from_cjson(&document, cjson_document) != 0) {",
            "        cJSON_Delete(cjson_document);",
            "        json_value_free(&document);",
            "        return -1;",
            "    }",
        ]
    else:
        # 内置解析器
        lines += [
            "    json_parser_t parser = {0};",
            "    json_value_t document;",
            "    int result = -1;",
            "    if (json_text == NULL || out_value == NULL) {",
            "        return -1;",
            "    }",
            "    json_value_init(&document);",
            "    parser.text = json_text;",
            "    parser.length = json_length;",
            "    parser.position = 0U;",
            "    if (!json_parse_value(&parser, &document)) {",
            "        json_value_free(&document);",
            "        return -1;",
            "    }",
            "    json_skip_space(&parser);",
            "    if (parser.position != parser.length) {",
            "        json_value_free(&document);",
            "        return -1;",
            "    }",
        ]
    # 根据根类型分发到对应的 decode 函数
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
    lines.append("    json_value_free(&document);")
    if backend == "cjson":
        lines.append("    cJSON_Delete(cjson_document);")
    lines += ["    return result;", "}", ""]
    # to_json 实现
    lines += [
        f"int {root_lower}_to_json(const {root_type} *value, char *buffer, size_t buffer_size, size_t *written)",
        "{",
    ]
    if backend == "cjson":
        lines += [
            "    json_writer_t writer = {0};",
            "    cJSON *cjson_document = NULL;",
            "    char *encoded = NULL;",
            "    char *printed = NULL;",
            "    size_t encoded_length = 0U;",
            "    size_t printed_length = 0U;",
            "    int result = -1;",
            "    if (value == NULL || written == NULL || (buffer == NULL && buffer_size != 0U)) {",
            "        return -1;",
            "    }",
        ]
    else:
        lines += [
            "    json_writer_t writer = {0};",
            "    int result = -1;",
            "    if (value == NULL || written == NULL || (buffer == NULL && buffer_size != 0U)) {",
            "        return -1;",
            "    }",
            "    writer.buffer = buffer;",
            "    writer.capacity = buffer_size;",
        ]
    # 根类型分发编码
    if root.kind == "object":
        lines += [f"    result = encode_{root_name}(value, &writer);"]
    elif root.kind == "array":
        lines += [f"    result = encode_{root_name}(value, &writer);"]
    elif root.kind == "generic":
        lines += ["    result = encode_json_value(value, &writer);"]
    elif root.scalar == "string":
        lines += ["    result = writer_string(&writer, *value) ? 0 : -1;"]
    elif root.scalar == "bool":
        lines += [
            '    result = writer_put(&writer, *value ? "true" : "false", *value ? 4U : 5U) ? 0 : -1;'
        ]
    elif root.scalar == "int":
        lines += ["    result = writer_int(&writer, *value) ? 0 : -1;"]
    else:
        lines += ["    result = writer_double(&writer, *value) ? 0 : -1;"]
    if backend == "cjson":
        # cJSON 后端：两阶段写出
        # 1. 先用 writer 计算所需长度（buffer=NULL）
        # 2. 分配缓冲区，再次写入得到紧凑 JSON
        # 3. 用 cJSON 解析并重新格式化（保证输出合法、数值安全）
        lines += [
            "    if (result != 0 || writer.failed || writer.required == SIZE_MAX) {",
            "        return -1;",
            "    }",
            "    encoded_length = writer.required;",
            "    encoded = (char *)malloc(encoded_length + 1U);",
            "    if (encoded == NULL) {",
            "        return -1;",
            "    }",
            "    writer.buffer = encoded;",
            "    writer.capacity = encoded_length + 1U;",
            "    writer.position = 0U;",
            "    writer.required = 0U;",
            "    writer.failed = false;",
        ]
        if root.kind in {"object", "array"}:
            lines += [f"    result = encode_{root_name}(value, &writer);"]
        elif root.kind == "generic":
            lines += ["    result = encode_json_value(value, &writer);"]
        elif root.scalar == "string":
            lines += ["    result = writer_string(&writer, *value) ? 0 : -1;"]
        elif root.scalar == "bool":
            lines += [
                '    result = writer_put(&writer, *value ? "true" : "false", *value ? 4U : 5U) ? 0 : -1;'
            ]
        elif root.scalar == "int":
            lines += ["    result = writer_int(&writer, *value) ? 0 : -1;"]
        else:
            lines += ["    result = writer_double(&writer, *value) ? 0 : -1;"]
        lines += [
            "    if (result != 0 || writer.failed || writer.required != encoded_length) {",
            "        free(encoded);",
            "        return -1;",
            "    }",
            "    encoded[encoded_length] = '\\0';",
            "    cjson_document = cJSON_ParseWithLength(encoded, encoded_length);",
            "    free(encoded);",
            "    if (cjson_document == NULL || !cjson_tree_numbers_are_safe(cjson_document)) {",
            "        cJSON_Delete(cjson_document);",
            "        return -1;",
            "    }",
            "    printed = cJSON_PrintUnformatted(cjson_document);",
            "    cJSON_Delete(cjson_document);",
            "    if (printed == NULL) {",
            "        return -1;",
            "    }",
            "    printed_length = strlen(printed);",
            "    *written = printed_length;",
            "    if (buffer == NULL) {",
            "        cJSON_free(printed);",
            "        return 0;",
            "    }",
            "    if (buffer_size <= printed_length) {",
            "        cJSON_free(printed);",
            "        return -2;",
            "    }",
            "    memcpy(buffer, printed, printed_length + 1U);",
            "    cJSON_free(printed);",
            "    return 0;",
            "}",
            "",
        ]
    else:
        # 内置后端：writer 已直接写入用户缓冲区
        lines += [
            "    if (writer.failed && buffer != NULL) {",
            "        *written = writer.required;",
            "        return -2;",
            "    }",
            "    if (result != 0 || writer.failed) {",
            "        *written = writer.required;",
            "        return -1;",
            "    }",
            "    if (buffer != NULL) {",
            "        buffer[writer.position] = '\\0';",
            "    }",
            "    *written = writer.required;",
            "    return JSON_GENERATED_OK;",
            "}",
            "",
        ]
    # free 实现
    if root.kind in {"object", "array"}:
        lines += [
            f"void {root_lower}_free({root_type} *value)",
            "{",
            f"    free_{root_name}(value);",
            "}",
            "",
        ]
    elif root.kind == "generic":
        lines += [
            f"void {root_lower}_free({root_type} *value)",
            "{",
            "    json_value_free(value);",
            "}",
            "",
        ]
    elif root.scalar == "string":
        lines += [
            f"void {root_lower}_free({root_type} *value)",
            "{",
            "    if (value != NULL) {",
            "        free(*value);",
            "        *value = NULL;",
            "    }",
            "}",
            "",
        ]
    else:
        # bool/int/double 无动态内存
        lines += [
            f"void {root_lower}_free({root_type} *value)",
            "{",
            "    (void)value;",
            "}",
            "",
        ]
    return "\n".join(lines)


# =============================================================================
# 主生成入口与 CLI
# =============================================================================
def generate(
    input_value: Any,
    root_name: str,
    header_name: str,
    source_name: str,
    backend: str = "builtin",
    cjson_header: str = "cjson/cJSON.h",
) -> tuple[str, str]:
    """
    完整生成流程入口。

    步骤：
    1. 校验 backend 参数
    2. 规范化根名
    3. 推导 schema (infer_node)
    4. 分配类型名 (assign_names)
    5. 生成头文件保护宏
    6. 生成头文件 (emit_header)
    7. 生成源文件 (emit_source)

    返回:
        (header_content, source_content)
    """
    if backend not in {"builtin", "cjson"}:
        raise ValueError(f"不支持的 JSON 后端: {backend}")
    root_base = type_identifier(root_name)
    allocator = NameAllocator()
    root = infer_node(input_value, root_base, allocator)
    assign_names(root, root_base, is_root=True)
    guard = re.sub(r"[^A-Za-z0-9]", "_", root_base.upper()) + "_H"
    overview = "由 JSON 示例自动生成的结构体和 JSON 编解码接口。"
    header = emit_header(root, guard, overview, root_base)
    source = emit_source(root, header_name, overview, root_base, backend, cjson_header)
    return header, source


def parse_args(argv: Optional[list[str]] = None) -> argparse.Namespace:
    """
    命令行参数解析。

    参数:
        input: 输入 JSON 文件路径（位置参数）
        -o/--output-dir: 输出目录（默认当前目录）
        --base-name: 输出文件基础名（默认输入文件名去扩展名）
        --root-name: 根类型/接口名（默认 base-name）
        --header/--source: 自定义输出文件路径
        --backend: builtin | cJSON（默认 builtin）
        --cjson-header: cJSON 头文件 include 路径
    """
    parser = argparse.ArgumentParser(
        description="从 JSON 示例生成 C 结构体以及 JSON 编解码 .h/.c 文件。"
    )
    parser.add_argument("input", type=Path, help="输入 JSON 文件")
    parser.add_argument(
        "-o",
        "--output-dir",
        type=Path,
        default=Path("."),
        help="输出目录，默认当前目录",
    )
    parser.add_argument("--base-name", help="输出文件基础名，默认使用输入文件名")
    parser.add_argument("--root-name", help="根 C 类型/接口名，默认使用 base-name")
    parser.add_argument("--header", type=Path, help="自定义头文件路径")
    parser.add_argument("--source", type=Path, help="自定义源文件路径")
    parser.add_argument(
        "--backend",
        choices=("builtin", "cjson"),
        default="builtin",
        help="JSON 实现：内置或 cJSON (默认: builtin)",
    )
    parser.add_argument(
        "--cjson-header",
        default="cjson/cJSON.h",
        help="生成的 C 源文件中 cJSON 头文件的 include 路径",
    )
    return parser.parse_args(argv)


def main(argv: Optional[list[str]] = None) -> int:
    """
    主函数。

    返回码:
        0: 成功
        2: JSON 读取/解析失败
        3: C 代码生成/写入失败
    """
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
        header, source = generate(
            data,
            root_name,
            header_path.name,
            source_path.name,
            args.backend,
            args.cjson_header,
        )
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