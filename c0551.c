/**
 * 文件名：c0551.c
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-10-11
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#include "c0551.h"

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

static void free_c0551_youyahzconfig_youyahezha_mianban(c0551_youyahzconfig_youyahezha_mianban_t *value);
static int decode_c0551_youyahzconfig_youyahezha_mianban(const json_value_t *node, c0551_youyahzconfig_youyahezha_mianban_t *out);
static int encode_c0551_youyahzconfig_youyahezha_mianban(const c0551_youyahzconfig_youyahezha_mianban_t *value, json_writer_t *writer);

static void free_c0551_youyahzconfig_youyahezha_mianban(c0551_youyahzconfig_youyahezha_mianban_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_c0551_youyahzconfig_youyahezha_mianban(const json_value_t *node, c0551_youyahzconfig_youyahezha_mianban_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 7U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "Enable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->enable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YkOn");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ykon) != 0) {
        goto fail;
    }
    member = json_object_get(node, "SwitchYX");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->switchyx) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YkContinueTime");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ykcontinuetime) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Chonghezha_failcount");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->chonghezha_failcount) != 0) {
        goto fail;
    }
    member = json_object_get(node, "ChongHeZha_Interval_s");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->chonghezha_interval_s) != 0) {
        goto fail;
    }
    member = json_object_get(node, "diyabeizitouEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->diyabeizitouenable) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_c0551_youyahzconfig_youyahezha_mianban(out);
    return -1;
}

static int encode_c0551_youyahzconfig_youyahezha_mianban(const c0551_youyahzconfig_youyahezha_mianban_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Enable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->enable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YkOn") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ykon)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "SwitchYX") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->switchyx)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YkContinueTime") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ykcontinuetime)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Chonghezha_failcount") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->chonghezha_failcount)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ChongHeZha_Interval_s") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->chonghezha_interval_s)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "diyabeizitouEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->diyabeizitouenable)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_c0551_youyahzconfig_diyabeizitouset(c0551_youyahzconfig_diyabeizitouset_t *value);
static int decode_c0551_youyahzconfig_diyabeizitouset(const json_value_t *node, c0551_youyahzconfig_diyabeizitouset_t *out);
static int encode_c0551_youyahzconfig_diyabeizitouset(const c0551_youyahzconfig_diyabeizitouset_t *value, json_writer_t *writer);

static void free_c0551_youyahzconfig_diyabeizitouset(c0551_youyahzconfig_diyabeizitouset_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_c0551_youyahzconfig_diyabeizitouset(const json_value_t *node, c0551_youyahzconfig_diyabeizitouset_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 36U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "YX_QF1_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yx_qf1_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UA_QF1_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ua_qf1_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UB_QF1_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ub_qf1_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UC_QF1_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->uc_qf1_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YK_QF1On_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yk_qf1on_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YK_QF1Off_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yk_qf1off_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YX_QFLL_Rtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yx_qfll_rtdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YKOn_QFLL_Rtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ykon_qfll_rtdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YKOff_QFLL_Rtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ykoff_qfll_rtdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "LaterTime");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->latertime) != 0) {
        goto fail;
    }
    member = json_object_get(node, "LessVoltage");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->lessvoltage) != 0) {
        goto fail;
    }
    member = json_object_get(node, "AfterLessVolTimeExeYkTime");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->afterlessvoltimeexeyktime) != 0) {
        goto fail;
    }
    member = json_object_get(node, "AfterCheckYxTime");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->aftercheckyxtime) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YX_QF2_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yx_qf2_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UA_QF2_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ua_qf2_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UB_QF2_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ub_qf2_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UC_QF2_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->uc_qf2_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YK_QF2On_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yk_qf2on_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YK_QF2Off_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yk_qf2off_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF2CommCheckRtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf2commcheckrtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QFLLCommCheckRtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qfllcommcheckrtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF1CommCheckRtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf1commcheckrtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF1TQErrCheckEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf1tqerrcheckenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF1TQErrCheckRtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf1tqerrcheckrtdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF2TQErrCheckEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf2tqerrcheckenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF2TQErrCheckRtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf2tqerrcheckrtdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "ModeSelect");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->modeselect) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QFLL_Logic_Enable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qfll_logic_enable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "BZTHardPlateEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->bzthardplateenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "BZTHardPlateRtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->bzthardplatertdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "ExtYXResetEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->extyxresetenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "ExtYXResetRtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->extyxresetrtdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YxHz_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yxhz_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Exit_Logic_Enable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->exit_logic_enable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Exit_Logic_ExeQFLLYkTime");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->exit_logic_exeqfllyktime) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QFLL_HZ_BiSuoEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qfll_hz_bisuoenable) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_c0551_youyahzconfig_diyabeizitouset(out);
    return -1;
}

static int encode_c0551_youyahzconfig_diyabeizitouset(const c0551_youyahzconfig_diyabeizitouset_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YX_QF1_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yx_qf1_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UA_QF1_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ua_qf1_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UB_QF1_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ub_qf1_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UC_QF1_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->uc_qf1_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YK_QF1On_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yk_qf1on_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YK_QF1Off_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yk_qf1off_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YX_QFLL_Rtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yx_qfll_rtdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YKOn_QFLL_Rtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ykon_qfll_rtdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YKOff_QFLL_Rtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ykoff_qfll_rtdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "LaterTime") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->latertime)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "LessVoltage") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->lessvoltage)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "AfterLessVolTimeExeYkTime") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->afterlessvoltimeexeyktime)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "AfterCheckYxTime") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->aftercheckyxtime)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YX_QF2_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yx_qf2_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UA_QF2_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ua_qf2_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UB_QF2_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ub_qf2_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UC_QF2_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->uc_qf2_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YK_QF2On_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yk_qf2on_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YK_QF2Off_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yk_qf2off_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF2CommCheckRtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf2commcheckrtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QFLLCommCheckRtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qfllcommcheckrtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF1CommCheckRtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf1commcheckrtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF1TQErrCheckEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf1tqerrcheckenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF1TQErrCheckRtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf1tqerrcheckrtdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF2TQErrCheckEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf2tqerrcheckenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF2TQErrCheckRtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf2tqerrcheckrtdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ModeSelect") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->modeselect)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QFLL_Logic_Enable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qfll_logic_enable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "BZTHardPlateEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->bzthardplateenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "BZTHardPlateRtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->bzthardplatertdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ExtYXResetEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->extyxresetenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ExtYXResetRtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->extyxresetrtdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YxHz_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yxhz_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Exit_Logic_Enable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->exit_logic_enable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Exit_Logic_ExeQFLLYkTime") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->exit_logic_exeqfllyktime)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QFLL_HZ_BiSuoEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qfll_hz_bisuoenable)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_c0551_youyahzconfig_twopowerchange(c0551_youyahzconfig_twopowerchange_t *value);
static int decode_c0551_youyahzconfig_twopowerchange(const json_value_t *node, c0551_youyahzconfig_twopowerchange_t *out);
static int encode_c0551_youyahzconfig_twopowerchange(const c0551_youyahzconfig_twopowerchange_t *value, json_writer_t *writer);

static void free_c0551_youyahzconfig_twopowerchange(c0551_youyahzconfig_twopowerchange_t *value)
{
    if (value == NULL) {
        return;
    }
    memset(value, 0, sizeof(*value));
}

static int decode_c0551_youyahzconfig_twopowerchange(const json_value_t *node, c0551_youyahzconfig_twopowerchange_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 31U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "ModeSelect");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->modeselect) != 0) {
        goto fail;
    }
    member = json_object_get(node, "BZTHardPlateEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->bzthardplateenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "BZTHardPlateRtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->bzthardplatertdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "ExtYXResetEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->extyxresetenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "ExtYXResetRtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->extyxresetrtdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "RecoverPlateEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->recoverplateenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF1ReclosingPlateEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf1reclosingplateenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "LaterTime");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->latertime) != 0) {
        goto fail;
    }
    member = json_object_get(node, "LessVoltage");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->lessvoltage) != 0) {
        goto fail;
    }
    member = json_object_get(node, "HaveVoltage");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->havevoltage) != 0) {
        goto fail;
    }
    member = json_object_get(node, "CanReadyTime");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->canreadytime) != 0) {
        goto fail;
    }
    member = json_object_get(node, "CanExeDelayTime");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->canexedelaytime) != 0) {
        goto fail;
    }
    member = json_object_get(node, "ReclosingMaxTimes");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->reclosingmaxtimes) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YX_QF1_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yx_qf1_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UA_QF1_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ua_qf1_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UB_QF1_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ub_qf1_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UC_QF1_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->uc_qf1_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YKNo_QF1_On");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ykno_qf1_on) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YKNo_QF1_Off");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ykno_qf1_off) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF1CommCheckRtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf1commcheckrtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF1TQErrCheckEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf1tqerrcheckenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF1TQErrCheckRtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf1tqerrcheckrtdb) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YX_QF2_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->yx_qf2_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UA_QF2_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ua_qf2_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UB_QF2_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ub_qf2_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "UC_QF2_RtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->uc_qf2_rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YKNo_QF2_On");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ykno_qf2_on) != 0) {
        goto fail;
    }
    member = json_object_get(node, "YKNo_QF2_Off");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ykno_qf2_off) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF2CommCheckRtdbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf2commcheckrtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF2TQErrCheckEnable");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf2tqerrcheckenable) != 0) {
        goto fail;
    }
    member = json_object_get(node, "QF2TQErrCheckRtdb");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->qf2tqerrcheckrtdb) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_c0551_youyahzconfig_twopowerchange(out);
    return -1;
}

static int encode_c0551_youyahzconfig_twopowerchange(const c0551_youyahzconfig_twopowerchange_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ModeSelect") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->modeselect)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "BZTHardPlateEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->bzthardplateenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "BZTHardPlateRtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->bzthardplatertdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ExtYXResetEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->extyxresetenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ExtYXResetRtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->extyxresetrtdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "RecoverPlateEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->recoverplateenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF1ReclosingPlateEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf1reclosingplateenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "LaterTime") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->latertime)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "LessVoltage") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->lessvoltage)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "HaveVoltage") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->havevoltage)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "CanReadyTime") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->canreadytime)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "CanExeDelayTime") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->canexedelaytime)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "ReclosingMaxTimes") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->reclosingmaxtimes)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YX_QF1_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yx_qf1_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UA_QF1_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ua_qf1_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UB_QF1_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ub_qf1_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UC_QF1_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->uc_qf1_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YKNo_QF1_On") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ykno_qf1_on)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YKNo_QF1_Off") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ykno_qf1_off)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF1CommCheckRtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf1commcheckrtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF1TQErrCheckEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf1tqerrcheckenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF1TQErrCheckRtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf1tqerrcheckrtdb)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YX_QF2_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->yx_qf2_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UA_QF2_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ua_qf2_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UB_QF2_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ub_qf2_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "UC_QF2_RtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->uc_qf2_rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YKNo_QF2_On") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ykno_qf2_on)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YKNo_QF2_Off") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ykno_qf2_off)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF2CommCheckRtdbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf2commcheckrtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF2TQErrCheckEnable") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf2tqerrcheckenable)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "QF2TQErrCheckRtdb") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->qf2tqerrcheckrtdb)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_c0551_youyahzconfig_youyahezha_tablelist_item(c0551_youyahzconfig_youyahezha_tablelist_item_t *value);
static int decode_c0551_youyahzconfig_youyahezha_tablelist_item(const json_value_t *node, c0551_youyahzconfig_youyahezha_tablelist_item_t *out);
static int encode_c0551_youyahzconfig_youyahezha_tablelist_item(const c0551_youyahzconfig_youyahezha_tablelist_item_t *value, json_writer_t *writer);

static void free_c0551_youyahzconfig_youyahezha_tablelist_item(c0551_youyahzconfig_youyahezha_tablelist_item_t *value)
{
    if (value == NULL) {
        return;
    }
free(value->name);
free(value->unit);
free(value->comment);
    memset(value, 0, sizeof(*value));
}

static int decode_c0551_youyahzconfig_youyahezha_tablelist_item(const json_value_t *node, c0551_youyahzconfig_youyahezha_tablelist_item_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 8U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "No");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->no) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Name");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->name) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Value");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->value) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Unit");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->unit) != 0) {
        goto fail;
    }
    member = json_object_get(node, "RtDbNo");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->rtdbno) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Ratio");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_double(&member->value, &out->ratio) != 0) {
        goto fail;
    }
    member = json_object_get(node, "IsCheck");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_int(&member->value, &out->ischeck) != 0) {
        goto fail;
    }
    member = json_object_get(node, "Comment");
    if (member == NULL) {
        goto fail;
    }
    if (json_decode_string(&member->value, &out->comment) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_c0551_youyahzconfig_youyahezha_tablelist_item(out);
    return -1;
}

static int encode_c0551_youyahzconfig_youyahezha_tablelist_item(const c0551_youyahzconfig_youyahezha_tablelist_item_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "No") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->no)) {
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
    if (!writer_string(writer, "Value") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->value)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Unit") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->unit)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "RtDbNo") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->rtdbno)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Ratio") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_double(writer, value->ratio)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "IsCheck") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_int(writer, value->ischeck)) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "Comment") || !writer_char(writer, ':')) {
        return -1;
    }
    if (!writer_string(writer, value->comment)) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_c0551_youyahzconfig_youyahezha_tablelist_array(c0551_youyahzconfig_youyahezha_tablelist_array_t *value);
static int decode_c0551_youyahzconfig_youyahezha_tablelist_array(const json_value_t *node, c0551_youyahzconfig_youyahezha_tablelist_array_t *out);
static int encode_c0551_youyahzconfig_youyahezha_tablelist_array(const c0551_youyahzconfig_youyahezha_tablelist_array_t *value, json_writer_t *writer);

static void free_c0551_youyahzconfig_youyahezha_tablelist_array(c0551_youyahzconfig_youyahezha_tablelist_array_t *value)
{
    if (value == NULL) {
        return;
    }
    size_t index = 0U;
    for (index = 0U; index < value->count; ++index) {
        free_c0551_youyahzconfig_youyahezha_tablelist_item(&value->items[index]);
    }
    free(value->items);
    value->items = NULL;
    value->count = 0U;
}

static int decode_c0551_youyahzconfig_youyahezha_tablelist_array(const json_value_t *node, c0551_youyahzconfig_youyahezha_tablelist_array_t *out)
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
        if (decode_c0551_youyahzconfig_youyahezha_tablelist_item(&node->as.array.items[index], &out->items[index]) != 0) {
            goto fail;
        }
    }
    return 0;
fail:
    free_c0551_youyahzconfig_youyahezha_tablelist_array(out);
    return -1;
}

static int encode_c0551_youyahzconfig_youyahezha_tablelist_array(const c0551_youyahzconfig_youyahezha_tablelist_array_t *value, json_writer_t *writer)
{
    size_t index = 0U;
    if (value == NULL || writer == NULL || !writer_char(writer, '[')) {
        return -1;
    }
    for (index = 0U; index < value->count; ++index) {
        if (index != 0U && !writer_char(writer, ',')) {
            return -1;
        }
        if (encode_c0551_youyahzconfig_youyahezha_tablelist_item(&value->items[index], writer) != 0) {
            return -1;
        }
    }
    return writer_char(writer, ']') ? 0 : -1;
}

static void free_c0551_youyahzconfig(c0551_youyahzconfig_t *value);
static int decode_c0551_youyahzconfig(const json_value_t *node, c0551_youyahzconfig_t *out);
static int encode_c0551_youyahzconfig(const c0551_youyahzconfig_t *value, json_writer_t *writer);

static void free_c0551_youyahzconfig(c0551_youyahzconfig_t *value)
{
    if (value == NULL) {
        return;
    }
free_c0551_youyahzconfig_youyahezha_mianban(&value->youyahezha_mianban);
free_c0551_youyahzconfig_diyabeizitouset(&value->diyabeizitouset);
free_c0551_youyahzconfig_twopowerchange(&value->twopowerchange);
free_c0551_youyahzconfig_youyahezha_tablelist_array(&value->youyahezha_tablelist);
    memset(value, 0, sizeof(*value));
}

static int decode_c0551_youyahzconfig(const json_value_t *node, c0551_youyahzconfig_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 4U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "youyahezha_mianban");
    if (member == NULL) {
        goto fail;
    }
    if (decode_c0551_youyahzconfig_youyahezha_mianban(&member->value, &out->youyahezha_mianban) != 0) {
        goto fail;
    }
    member = json_object_get(node, "DiyabeizitouSet");
    if (member == NULL) {
        goto fail;
    }
    if (decode_c0551_youyahzconfig_diyabeizitouset(&member->value, &out->diyabeizitouset) != 0) {
        goto fail;
    }
    member = json_object_get(node, "TwoPowerChange");
    if (member == NULL) {
        goto fail;
    }
    if (decode_c0551_youyahzconfig_twopowerchange(&member->value, &out->twopowerchange) != 0) {
        goto fail;
    }
    member = json_object_get(node, "youyahezha_tablelist");
    if (member == NULL) {
        goto fail;
    }
    if (decode_c0551_youyahzconfig_youyahezha_tablelist_array(&member->value, &out->youyahezha_tablelist) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_c0551_youyahzconfig(out);
    return -1;
}

static int encode_c0551_youyahzconfig(const c0551_youyahzconfig_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "youyahezha_mianban") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_c0551_youyahzconfig_youyahezha_mianban(&value->youyahezha_mianban, writer) != 0) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "DiyabeizitouSet") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_c0551_youyahzconfig_diyabeizitouset(&value->diyabeizitouset, writer) != 0) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "TwoPowerChange") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_c0551_youyahzconfig_twopowerchange(&value->twopowerchange, writer) != 0) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "youyahezha_tablelist") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_c0551_youyahzconfig_youyahezha_tablelist_array(&value->youyahezha_tablelist, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

static void free_c0551(c0551_t *value);
static int decode_c0551(const json_value_t *node, c0551_t *out);
static int encode_c0551(const c0551_t *value, json_writer_t *writer);

static void free_c0551(c0551_t *value)
{
    if (value == NULL) {
        return;
    }
free_c0551_youyahzconfig(&value->youyahzconfig);
    memset(value, 0, sizeof(*value));
}

static int decode_c0551(const json_value_t *node, c0551_t *out)
{
    const json_member_t *member = NULL;
    if (node == NULL || out == NULL || node->kind != JSON_VALUE_OBJECT) {
        return -1;
    }
    if (node->as.object.count != 1U) {
        return -1;
    }
    memset(out, 0, sizeof(*out));
    member = json_object_get(node, "YouYaHzConfig");
    if (member == NULL) {
        goto fail;
    }
    if (decode_c0551_youyahzconfig(&member->value, &out->youyahzconfig) != 0) {
        goto fail;
    }
    return 0;
fail:
    free_c0551(out);
    return -1;
}

static int encode_c0551(const c0551_t *value, json_writer_t *writer)
{
    bool first = true;
    if (value == NULL || writer == NULL || !writer_char(writer, '{')) {
        return -1;
    }
    if (!first && !writer_char(writer, ',')) {
        return -1;
    }
    first = false;
    if (!writer_string(writer, "YouYaHzConfig") || !writer_char(writer, ':')) {
        return -1;
    }
    if (encode_c0551_youyahzconfig(&value->youyahzconfig, writer) != 0) {
        return -1;
    }
    return writer_char(writer, '}') ? 0 : -1;
}

int c0551_from_json(const char *json_text, size_t json_length, c0551_t *out_value)
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
    result = decode_c0551(&document, out_value);
    json_value_free(&document);
    return result;
}

int c0551_to_json(const c0551_t *value, char *buffer, size_t buffer_size, size_t *written)
{
    json_writer_t writer = {0};
    int result = -1;
    if (value == NULL || written == NULL || (buffer == NULL && buffer_size != 0U)) {
        return -1;
    }
    writer.buffer = buffer;
    writer.capacity = buffer_size;
    result = encode_c0551(value, &writer);
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

void c0551_free(c0551_t *value)
{
    free_c0551(value);
}
