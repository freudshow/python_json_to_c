/**
 * 文件名：root.c
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-10-11
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#include "root.h"

#include <errno.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if defined(__GNUC__) || defined(__clang__)
#define JSON_GENERATED_UNUSED __attribute__((unused))
#else
#define JSON_GENERATED_UNUSED
#endif
#define JSON_GENERATED_OK 0

/* ---------- 通用 JSON DOM、解析器和写出器 ---------- */
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

static void free_root_components_variant_1_data(root_components_variant_1_data_t *value);
static int decode_root_components_variant_1_data(const json_value_t *node, root_components_variant_1_data_t *out);
static int encode_root_components_variant_1_data(const root_components_variant_1_data_t *value, json_writer_t *writer);

static void free_root_components_variant_1_data(root_components_variant_1_data_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_1_data(const json_value_t *node, root_components_variant_1_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 2U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\347\224\265\345\216\213(kV)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->kv) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\351\242\221\347\216\207(Hz)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->hz) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_1_data(out);
    return -1;
}

static int encode_root_components_variant_1_data(const root_components_variant_1_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\347\224\265\345\216\213(kV)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->kv)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\351\242\221\347\216\207(Hz)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->hz)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_1(root_components_variant_1_t *value);
static int decode_root_components_variant_1(const json_value_t *node, root_components_variant_1_t *out);
static int encode_root_components_variant_1(const root_components_variant_1_t *value, json_writer_t *writer);

static void free_root_components_variant_1(root_components_variant_1_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_1_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_1(const json_value_t *node, root_components_variant_1_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_1_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_1(out);
    return -1;
}

static int encode_root_components_variant_1(const root_components_variant_1_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_1_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_2_data(root_components_variant_2_data_t *value);
static int decode_root_components_variant_2_data(const json_value_t *node, root_components_variant_2_data_t *out);
static int encode_root_components_variant_2_data(const root_components_variant_2_data_t *value, json_writer_t *writer);

static void free_root_components_variant_2_data(root_components_variant_2_data_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_2_data(const json_value_t *node, root_components_variant_2_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 3U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\351\242\235\345\256\232\347\224\265\346\265\201(A)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->a) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\351\242\235\345\256\232\347\224\265\345\216\213(V)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->v) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\350\204\261\346\211\243\346\227\266\351\227\264(ms)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ms) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_2_data(out);
    return -1;
}

static int encode_root_components_variant_2_data(const root_components_variant_2_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\351\242\235\345\256\232\347\224\265\346\265\201(A)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->a)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\351\242\235\345\256\232\347\224\265\345\216\213(V)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->v)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\350\204\261\346\211\243\346\227\266\351\227\264(ms)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ms)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_2(root_components_variant_2_t *value);
static int decode_root_components_variant_2(const json_value_t *node, root_components_variant_2_t *out);
static int encode_root_components_variant_2(const root_components_variant_2_t *value, json_writer_t *writer);

static void free_root_components_variant_2(root_components_variant_2_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_2_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_2(const json_value_t *node, root_components_variant_2_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_2_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_2(out);
    return -1;
}

static int encode_root_components_variant_2(const root_components_variant_2_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_2_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_3_data(root_components_variant_3_data_t *value);
static int decode_root_components_variant_3_data(const json_value_t *node, root_components_variant_3_data_t *out);
static int encode_root_components_variant_3_data(const root_components_variant_3_data_t *value, json_writer_t *writer);

static void free_root_components_variant_3_data(root_components_variant_3_data_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_3_data(const json_value_t *node, root_components_variant_3_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 2U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\345\217\230\346\257\224");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->field) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\351\207\217\347\250\213(A)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->a) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_3_data(out);
    return -1;
}

static int encode_root_components_variant_3_data(const root_components_variant_3_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\217\230\346\257\224") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->field)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\351\207\217\347\250\213(A)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->a)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_3(root_components_variant_3_t *value);
static int decode_root_components_variant_3(const json_value_t *node, root_components_variant_3_t *out);
static int encode_root_components_variant_3(const root_components_variant_3_t *value, json_writer_t *writer);

static void free_root_components_variant_3(root_components_variant_3_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_3_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_3(const json_value_t *node, root_components_variant_3_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_3_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_3(out);
    return -1;
}

static int encode_root_components_variant_3(const root_components_variant_3_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_3_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_4_data(root_components_variant_4_data_t *value);
static int decode_root_components_variant_4_data(const json_value_t *node, root_components_variant_4_data_t *out);
static int encode_root_components_variant_4_data(const root_components_variant_4_data_t *value, json_writer_t *writer);

static void free_root_components_variant_4_data(root_components_variant_4_data_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_4_data(const json_value_t *node, root_components_variant_4_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 3U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\345\234\260\345\235\200");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->field) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\345\200\215\347\216\207");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->field_2) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\347\224\265\350\203\275(kWh)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->kwh) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_4_data(out);
    return -1;
}

static int encode_root_components_variant_4_data(const root_components_variant_4_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\234\260\345\235\200") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->field)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\200\215\347\216\207") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->field_2)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\347\224\265\350\203\275(kWh)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->kwh)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_4(root_components_variant_4_t *value);
static int decode_root_components_variant_4(const json_value_t *node, root_components_variant_4_t *out);
static int encode_root_components_variant_4(const root_components_variant_4_t *value, json_writer_t *writer);

static void free_root_components_variant_4(root_components_variant_4_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_4_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_4(const json_value_t *node, root_components_variant_4_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_4_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_4(out);
    return -1;
}

static int encode_root_components_variant_4(const root_components_variant_4_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_4_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_5_data(root_components_variant_5_data_t *value);
static int decode_root_components_variant_5_data(const json_value_t *node, root_components_variant_5_data_t *out);
static int encode_root_components_variant_5_data(const root_components_variant_5_data_t *value, json_writer_t *writer);

static void free_root_components_variant_5_data(root_components_variant_5_data_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->field);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_5_data(const json_value_t *node, root_components_variant_5_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 1U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\351\200\273\350\276\221");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->field) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_5_data(out);
    return -1;
}

static int encode_root_components_variant_5_data(const root_components_variant_5_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\351\200\273\350\276\221") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->field)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_5(root_components_variant_5_t *value);
static int decode_root_components_variant_5(const json_value_t *node, root_components_variant_5_t *out);
static int encode_root_components_variant_5(const root_components_variant_5_t *value, json_writer_t *writer);

static void free_root_components_variant_5(root_components_variant_5_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_5_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_5(const json_value_t *node, root_components_variant_5_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_5_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_5(out);
    return -1;
}

static int encode_root_components_variant_5(const root_components_variant_5_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_5_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_6_data(root_components_variant_6_data_t *value);
static int decode_root_components_variant_6_data(const json_value_t *node, root_components_variant_6_data_t *out);
static int encode_root_components_variant_6_data(const root_components_variant_6_data_t *value, json_writer_t *writer);

static void free_root_components_variant_6_data(root_components_variant_6_data_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_6_data(const json_value_t *node, root_components_variant_6_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 2U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\345\273\266\346\227\266(s)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->s) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\345\275\223\345\211\215(s)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->s_2) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_6_data(out);
    return -1;
}

static int encode_root_components_variant_6_data(const root_components_variant_6_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\273\266\346\227\266(s)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->s)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\275\223\345\211\215(s)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->s_2)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_6(root_components_variant_6_t *value);
static int decode_root_components_variant_6(const json_value_t *node, root_components_variant_6_t *out);
static int encode_root_components_variant_6(const root_components_variant_6_t *value, json_writer_t *writer);

static void free_root_components_variant_6(root_components_variant_6_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_6_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_6(const json_value_t *node, root_components_variant_6_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_6_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_6(out);
    return -1;
}

static int encode_root_components_variant_6(const root_components_variant_6_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_6_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_7_data(root_components_variant_7_data_t *value);
static int decode_root_components_variant_7_data(const json_value_t *node, root_components_variant_7_data_t *out);
static int encode_root_components_variant_7_data(const root_components_variant_7_data_t *value, json_writer_t *writer);

static void free_root_components_variant_7_data(root_components_variant_7_data_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->field);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_7_data(const json_value_t *node, root_components_variant_7_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 2U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\347\261\273\345\236\213");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->field) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\344\275\277\350\203\275");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_bool(&member->value, &out->field_2) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_7_data(out);
    return -1;
}

static int encode_root_components_variant_7_data(const root_components_variant_7_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\347\261\273\345\236\213") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->field)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\344\275\277\350\203\275") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_put(writer, value->field_2 ? "true" : "false", value->field_2 ? 4U : 5U)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_7(root_components_variant_7_t *value);
static int decode_root_components_variant_7(const json_value_t *node, root_components_variant_7_t *out);
static int encode_root_components_variant_7(const root_components_variant_7_t *value, json_writer_t *writer);

static void free_root_components_variant_7(root_components_variant_7_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_7_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_7(const json_value_t *node, root_components_variant_7_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_7_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_7(out);
    return -1;
}

static int encode_root_components_variant_7(const root_components_variant_7_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_7_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_8_data(root_components_variant_8_data_t *value);
static int decode_root_components_variant_8_data(const json_value_t *node, root_components_variant_8_data_t *out);
static int encode_root_components_variant_8_data(const root_components_variant_8_data_t *value, json_writer_t *writer);

static void free_root_components_variant_8_data(root_components_variant_8_data_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_8_data(const json_value_t *node, root_components_variant_8_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 3U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\344\270\212\351\231\220(%)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->field) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\344\270\213\351\231\220(%)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->field_2) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\345\234\260\345\235\200");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->field_3) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_8_data(out);
    return -1;
}

static int encode_root_components_variant_8_data(const root_components_variant_8_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\344\270\212\351\231\220(%)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->field)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\344\270\213\351\231\220(%)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->field_2)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\234\260\345\235\200") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->field_3)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_8(root_components_variant_8_t *value);
static int decode_root_components_variant_8(const json_value_t *node, root_components_variant_8_t *out);
static int encode_root_components_variant_8(const root_components_variant_8_t *value, json_writer_t *writer);

static void free_root_components_variant_8(root_components_variant_8_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_8_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_8(const json_value_t *node, root_components_variant_8_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_8_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_8(out);
    return -1;
}

static int encode_root_components_variant_8(const root_components_variant_8_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_8_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_9_data(root_components_variant_9_data_t *value);
static int decode_root_components_variant_9_data(const json_value_t *node, root_components_variant_9_data_t *out);
static int encode_root_components_variant_9_data(const root_components_variant_9_data_t *value, json_writer_t *writer);

static void free_root_components_variant_9_data(root_components_variant_9_data_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_9_data(const json_value_t *node, root_components_variant_9_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 3U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\345\234\260\345\235\200");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->field) != 0) {
        goto fail;
    }
    member = json_object_get(node, "IP");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ip) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\345\221\250\346\234\237(ms)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ms) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_9_data(out);
    return -1;
}

static int encode_root_components_variant_9_data(const root_components_variant_9_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\234\260\345\235\200") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->field)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "IP") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ip)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\221\250\346\234\237(ms)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ms)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_9(root_components_variant_9_t *value);
static int decode_root_components_variant_9(const json_value_t *node, root_components_variant_9_t *out);
static int encode_root_components_variant_9(const root_components_variant_9_t *value, json_writer_t *writer);

static void free_root_components_variant_9(root_components_variant_9_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_9_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_9(const json_value_t *node, root_components_variant_9_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_9_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_9(out);
    return -1;
}

static int encode_root_components_variant_9(const root_components_variant_9_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_9_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_10_data(root_components_variant_10_data_t *value);
static int decode_root_components_variant_10_data(const json_value_t *node, root_components_variant_10_data_t *out);
static int encode_root_components_variant_10_data(const root_components_variant_10_data_t *value, json_writer_t *writer);

static void free_root_components_variant_10_data(root_components_variant_10_data_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->field);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_10_data(const json_value_t *node, root_components_variant_10_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 1U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\351\242\234\350\211\262");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->field) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_10_data(out);
    return -1;
}

static int encode_root_components_variant_10_data(const root_components_variant_10_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\351\242\234\350\211\262") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->field)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_10(root_components_variant_10_t *value);
static int decode_root_components_variant_10(const json_value_t *node, root_components_variant_10_t *out);
static int encode_root_components_variant_10(const root_components_variant_10_t *value, json_writer_t *writer);

static void free_root_components_variant_10(root_components_variant_10_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_10_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_10(const json_value_t *node, root_components_variant_10_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_10_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_10(out);
    return -1;
}

static int encode_root_components_variant_10(const root_components_variant_10_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_10_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_11_data(root_components_variant_11_data_t *value);
static int decode_root_components_variant_11_data(const json_value_t *node, root_components_variant_11_data_t *out);
static int encode_root_components_variant_11_data(const root_components_variant_11_data_t *value, json_writer_t *writer);

static void free_root_components_variant_11_data(root_components_variant_11_data_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->kw);
free(value->field);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_11_data(const json_value_t *node, root_components_variant_11_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 3U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\345\212\237\347\216\207(kW)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kw) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\350\275\254\351\200\237(rpm)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->rpm) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\347\212\266\346\200\201");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->field) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_11_data(out);
    return -1;
}

static int encode_root_components_variant_11_data(const root_components_variant_11_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\212\237\347\216\207(kW)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kw)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\350\275\254\351\200\237(rpm)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->rpm)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\347\212\266\346\200\201") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->field)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_11(root_components_variant_11_t *value);
static int decode_root_components_variant_11(const json_value_t *node, root_components_variant_11_t *out);
static int encode_root_components_variant_11(const root_components_variant_11_t *value, json_writer_t *writer);

static void free_root_components_variant_11(root_components_variant_11_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_11_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_11(const json_value_t *node, root_components_variant_11_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_11_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_11(out);
    return -1;
}

static int encode_root_components_variant_11(const root_components_variant_11_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_11_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_12_data(root_components_variant_12_data_t *value);
static int decode_root_components_variant_12_data(const json_value_t *node, root_components_variant_12_data_t *out);
static int encode_root_components_variant_12_data(const root_components_variant_12_data_t *value, json_writer_t *writer);

static void free_root_components_variant_12_data(root_components_variant_12_data_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_12_data(const json_value_t *node, root_components_variant_12_data_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 3U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "\344\270\212\351\231\220(\302\260C)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->c) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\344\270\213\351\231\220(\302\260C)");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->c_2) != 0) {
        goto fail;
    }
    member = json_object_get(node, "\345\234\260\345\235\200");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->field) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_12_data(out);
    return -1;
}

static int encode_root_components_variant_12_data(const root_components_variant_12_data_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\344\270\212\351\231\220(\302\260C)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->c)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\344\270\213\351\231\220(\302\260C)") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->c_2)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "\345\234\260\345\235\200") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->field)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_variant_12(root_components_variant_12_t *value);
static int decode_root_components_variant_12(const json_value_t *node, root_components_variant_12_t *out);
static int encode_root_components_variant_12(const root_components_variant_12_t *value, json_writer_t *writer);

static void free_root_components_variant_12(root_components_variant_12_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->id);
free(value->kind);
free(value->name);
free_root_components_variant_12_data(&value->data);
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_variant_12(const json_value_t *node, root_components_variant_12_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 6U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Id");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->id) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Kind");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->kind) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "X");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->x) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Y");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->y) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Data");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_variant_12_data(&member->value, &out->data) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_components_variant_12(out);
    return -1;
}

static int encode_root_components_variant_12(const root_components_variant_12_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Id") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->id)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Kind") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->kind)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Name") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->name)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "X") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->x)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Y") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->y)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Data") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_variant_12_data(&value->data, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_components_item(root_components_item_t *value);
static int decode_root_components_item(const json_value_t *node, root_components_item_t *out);
static int encode_root_components_item(const root_components_item_t *value, json_writer_t *writer);

static void free_root_components_item(root_components_item_t *value)
{
    if (value == NULL) {
        return;
    }
    switch (value->kind) {
    case ROOT_COMPONENTS_KIND_T_VARIANT_1:
        free_root_components_variant_1(&value->value.variant_1);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_2:
        free_root_components_variant_2(&value->value.variant_2);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_3:
        free_root_components_variant_3(&value->value.variant_3);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_4:
        free_root_components_variant_4(&value->value.variant_4);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_5:
        free_root_components_variant_5(&value->value.variant_5);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_6:
        free_root_components_variant_6(&value->value.variant_6);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_7:
        free_root_components_variant_7(&value->value.variant_7);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_8:
        free_root_components_variant_8(&value->value.variant_8);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_9:
        free_root_components_variant_9(&value->value.variant_9);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_10:
        free_root_components_variant_10(&value->value.variant_10);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_11:
        free_root_components_variant_11(&value->value.variant_11);
        break;
    case ROOT_COMPONENTS_KIND_T_VARIANT_12:
        free_root_components_variant_12(&value->value.variant_12);
        break;
    default:
        break;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_root_components_item(const json_value_t *node, root_components_item_t *out)
{
    if (node == NULL || out == NULL) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    if (decode_root_components_variant_1(node, &out->value.variant_1) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_1;
        return 0;
    }
    if (decode_root_components_variant_2(node, &out->value.variant_2) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_2;
        return 0;
    }
    if (decode_root_components_variant_3(node, &out->value.variant_3) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_3;
        return 0;
    }
    if (decode_root_components_variant_4(node, &out->value.variant_4) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_4;
        return 0;
    }
    if (decode_root_components_variant_5(node, &out->value.variant_5) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_5;
        return 0;
    }
    if (decode_root_components_variant_6(node, &out->value.variant_6) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_6;
        return 0;
    }
    if (decode_root_components_variant_7(node, &out->value.variant_7) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_7;
        return 0;
    }
    if (decode_root_components_variant_8(node, &out->value.variant_8) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_8;
        return 0;
    }
    if (decode_root_components_variant_9(node, &out->value.variant_9) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_9;
        return 0;
    }
    if (decode_root_components_variant_10(node, &out->value.variant_10) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_10;
        return 0;
    }
    if (decode_root_components_variant_11(node, &out->value.variant_11) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_11;
        return 0;
    }
    if (decode_root_components_variant_12(node, &out->value.variant_12) == 0) {
        out->kind = ROOT_COMPONENTS_KIND_T_VARIANT_12;
        return 0;
    }
    return -1;
}

static int encode_root_components_item(const root_components_item_t *value, json_writer_t *writer)
{
    if (value == NULL || writer == NULL) {
        return -1;
    }
    switch (value->kind) {
    case ROOT_COMPONENTS_KIND_T_VARIANT_1:
        return encode_root_components_variant_1(&value->value.variant_1, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_2:
        return encode_root_components_variant_2(&value->value.variant_2, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_3:
        return encode_root_components_variant_3(&value->value.variant_3, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_4:
        return encode_root_components_variant_4(&value->value.variant_4, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_5:
        return encode_root_components_variant_5(&value->value.variant_5, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_6:
        return encode_root_components_variant_6(&value->value.variant_6, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_7:
        return encode_root_components_variant_7(&value->value.variant_7, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_8:
        return encode_root_components_variant_8(&value->value.variant_8, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_9:
        return encode_root_components_variant_9(&value->value.variant_9, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_10:
        return encode_root_components_variant_10(&value->value.variant_10, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_11:
        return encode_root_components_variant_11(&value->value.variant_11, writer);
    case ROOT_COMPONENTS_KIND_T_VARIANT_12:
        return encode_root_components_variant_12(&value->value.variant_12, writer);
    default:
        return -1;
    }
}

static void free_root_components_array(root_components_array_t *value);
static int decode_root_components_array(const json_value_t *node, root_components_array_t *out);
static int encode_root_components_array(const root_components_array_t *value, json_writer_t *writer);

static void free_root_components_array(root_components_array_t *value)
{
    if (value == NULL) {
        return;
    }
    size_t index = 0U;
    for (index = 0U; index < value->count; ++index) {
        free_root_components_item(&value->items[index]);
    }
    free(value->items);
    value->items = NULL;
    value->count = 0U;
}

static int decode_root_components_array(const json_value_t *node, root_components_array_t *out)
{
    size_t index = 0U;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_ARRAY) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->count = node->as.array.count;
    if (out->count == 0U) {
        return 0;
    }
    out->items = calloc(out->count, sizeof(*out->items));
    if (out->items == NULL) {
        out->count = 0U;
        return -1;
    }
    for (index = 0U; index < out->count; ++index) {
        if (decode_root_components_item(&node->as.array.items[index], &out->items[index]) != 0) {
            goto fail;
        }
    }
    return 0;
fail:
    free_root_components_array(out);
    return -1;
}

static int encode_root_components_array(const root_components_array_t *value, json_writer_t *writer)
{
    size_t index = 0U;
    if (value == NULL || writer == NULL || !writer_char(writer, '[')) {
        return -1;
    }
    for (index = 0U; index < value->count; ++index) {
        if (index != 0U && !writer_char(writer, ',')) {
            return -1;
        }
        if (encode_root_components_item(&value->items[index], writer) != 0) {
            return -1;
        }
    }
    return writer_char(writer, ']') ? 0 : -1;
}

static void free_root_connections_item(root_connections_item_t *value);
static int decode_root_connections_item(const json_value_t *node, root_connections_item_t *out);
static int encode_root_connections_item(const root_connections_item_t *value, json_writer_t *writer);

static void free_root_connections_item(root_connections_item_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->s);
free(value->st);
free(value->t);
free(value->tt);
    memset(value, 0, sizeof(*value));
}

static int decode_root_connections_item(const json_value_t *node, root_connections_item_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 4U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "S");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->s) != 0) {
        goto fail;
    }
    member = json_object_get(node, "ST");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->st) != 0) {
        goto fail;
    }
    member = json_object_get(node, "T");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->t) != 0) {
        goto fail;
    }
    member = json_object_get(node, "TT");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->tt) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root_connections_item(out);
    return -1;
}

static int encode_root_connections_item(const root_connections_item_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "S") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->s)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ST") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->st)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "T") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->t)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "TT") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->tt)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_root_connections_array(root_connections_array_t *value);
static int decode_root_connections_array(const json_value_t *node, root_connections_array_t *out);
static int encode_root_connections_array(const root_connections_array_t *value, json_writer_t *writer);

static void free_root_connections_array(root_connections_array_t *value)
{
    if (value == NULL) {
        return;
    }
    size_t index = 0U;
    for (index = 0U; index < value->count; ++index) {
        free_root_connections_item(&value->items[index]);
    }
    free(value->items);
    value->items = NULL;
    value->count = 0U;
}

static int decode_root_connections_array(const json_value_t *node, root_connections_array_t *out)
{
    size_t index = 0U;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_ARRAY) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    out->count = node->as.array.count;
    if (out->count == 0U) {
        return 0;
    }
    out->items = calloc(out->count, sizeof(*out->items));
    if (out->items == NULL) {
        out->count = 0U;
        return -1;
    }
    for (index = 0U; index < out->count; ++index) {
        if (decode_root_connections_item(&node->as.array.items[index], &out->items[index]) != 0) {
            goto fail;
        }
    }
    return 0;
fail:
    free_root_connections_array(out);
    return -1;
}

static int encode_root_connections_array(const root_connections_array_t *value, json_writer_t *writer)
{
    size_t index = 0U;
    if (value == NULL || writer == NULL || !writer_char(writer, '[')) {
        return -1;
    }
    for (index = 0U; index < value->count; ++index) {
        if (index != 0U && !writer_char(writer, ',')) {
            return -1;
        }
        if (encode_root_connections_item(&value->items[index], writer) != 0) {
            return -1;
        }
    }
    return writer_char(writer, ']') ? 0 : -1;
}

static void free_root(root_t *value);
static int decode_root(const json_value_t *node, root_t *out);
static int encode_root(const root_t *value, json_writer_t *writer);

static void free_root(root_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->schema);
free_root_components_array(&value->components);
free_root_connections_array(&value->connections);
    memset(value, 0, sizeof(*value));
}

static int decode_root(const json_value_t *node, root_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 3U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Schema");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->schema) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Components");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_components_array(&member->value, &out->components) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Connections");
    if (member == NULL) {
        goto fail;
    }
    if (decode_root_connections_array(&member->value, &out->connections) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_root(out);
    return -1;
}

static int encode_root(const root_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Schema") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->schema)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Components") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_components_array(&value->components, writer) != 0) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Connections") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_root_connections_array(&value->connections, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

int root_from_json(const char *json_text, size_t json_length, root_t *out_value)
{
    json_parser_t parser = {0};
    json_value_t document;
    int result = -1;
    if (json_text == NULL || out_value == NULL) {
        return -1;
    }
    json_value_init(&document);
    parser.text = json_text;
    parser.length = json_length;
    parser.position = 0U;
    if (!json_parse_value(&parser, &document)) {
        json_value_free(&document);
        return -1;
    }
    json_skip_space(&parser);
    if (parser.position != parser.length) {
        json_value_free(&document);
        return -1;
    }
    result = decode_root(&document, out_value);
    json_value_free(&document);
    return result;
}

int root_to_json(const root_t *value, char *buffer, size_t buffer_size, size_t *written)
{
    json_writer_t writer = {0};
    int result = -1;
    if (value == NULL || written == NULL || (buffer == NULL && buffer_size != 0U)) {
        return -1;
    }
    writer.buffer = buffer;
    writer.capacity = buffer_size;
    result = encode_root(value, &writer);
    if (writer.failed && buffer != NULL) {
        *written = writer.required;
        return -2;
    }
    if (result != 0 || writer.failed) {
        *written = writer.required;
        return -1;
    }
    if (buffer != NULL) {
        buffer[writer.position] = '\0';
    }
    *written = writer.required;
    return JSON_GENERATED_OK;
}

void root_free(root_t *value)
{
    free_root(value);
}
