/**
 * 文件名：input.h
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-10-01
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#ifndef INPUT_H
#define INPUT_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

typedef enum {
    JSON_VALUE_NULL = 0,
    JSON_VALUE_BOOL,
    JSON_VALUE_NUMBER,
    JSON_VALUE_STRING,
    JSON_VALUE_ARRAY,
    JSON_VALUE_OBJECT
} json_value_kind_t;

typedef struct json_value json_value_t;
typedef struct json_member json_member_t;

struct json_value {
    json_value_kind_t kind;
    union {
        bool boolean;
        struct {
            int64_t integer;
            double real;
            bool is_integer;
            char *lexeme;  // 原始数字文本，用于通用 DOM 无损回写
        } number;
        char *string;
        struct {
            json_value_t *items;
            size_t count;
        } array;
        struct {
            json_member_t *items;
            size_t count;
        } object;
    } as;
};

struct json_member {
    char *key;
    json_value_t value;
};

/** 释放通用 JSON 值及其递归分配的内存。 */
void json_value_free(json_value_t *value);

typedef struct input_unicode_a_b_a_b_t {
    char * normal_key;  // JSON 字段 'normal-key'
    int64_t unicode;  // JSON 字段 'unicode键'
    char * type;  // JSON 字段 'type'
    int64_t a_b;  // JSON 字段 'a-b'
} input_unicode_a_b_a_b_t;

typedef struct input_unicode_a_b_t {
    input_unicode_a_b_a_b_t a_b;  // JSON 字段 'a-b'
} input_unicode_a_b_t;

typedef struct {
    json_value_t *items;  // 数组元素
    size_t count;  // 元素数量
} input_unicode_a_b_2_unicode_array_t;

typedef struct input_unicode_a_b_2_t {
    input_unicode_a_b_2_unicode_array_t unicode;  // JSON 字段 'unicode键'
} input_unicode_a_b_2_t;

typedef struct input_unicode_t {
    input_unicode_a_b_t a_b;  // JSON 字段 'a-b'
    char * unicode;  // JSON 字段 'unicode键'
    input_unicode_a_b_2_t a_b_2;  // JSON 字段 'a_b'
} input_unicode_t;

typedef struct {
    json_value_t *items;  // 数组元素
    size_t count;  // 元素数量
} input_a_b_array_t;

typedef struct input_t {
    input_unicode_t unicode;  // JSON 字段 'unicode键'
    double type;  // JSON 字段 'type'
    input_a_b_array_t a_b;  // JSON 字段 'a-b'
} input_t;

/** 将 JSON 文本解码为 input_t。返回 0 表示成功。 */
int input_from_json(const char *json_text, size_t json_length, input_t *out_value);
/** 将 input_t 编码为 JSON 文本；空间不足返回 -2。 */
int input_to_json(const input_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 input_t 内部动态内存。 */
void input_free(input_t *value);

#endif /* INPUT_H */
