/**
 * 文件名：fixture.h
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-08-12
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#ifndef FIXTURE_H
#define FIXTURE_H

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

typedef struct fixture_normal_key_unicode_field_1start_t {
    int64_t unicode;  // JSON 字段 'unicode键'
    int64_t value;  // JSON 字段 'value'
} fixture_normal_key_unicode_field_1start_t;

typedef struct fixture_normal_key_unicode_a_b_t {
    int64_t class_value;  // JSON 字段 'class'
} fixture_normal_key_unicode_a_b_t;

typedef struct fixture_normal_key_unicode_t {
    fixture_normal_key_unicode_field_1start_t field_1start;  // JSON 字段 '1start'
    fixture_normal_key_unicode_a_b_t a_b;  // JSON 字段 'a_b'
} fixture_normal_key_unicode_t;

typedef struct fixture_normal_key_t {
    fixture_normal_key_unicode_t unicode;  // JSON 字段 'unicode键'
    char * a_b;  // JSON 字段 'a_b'
} fixture_normal_key_t;

typedef struct {
    json_value_t *items;  // 数组元素
    size_t count;  // 元素数量
} fixture_class_value_array_t;

typedef struct fixture_t {
    fixture_normal_key_t normal_key;  // JSON 字段 'normal-key'
    char * type;  // JSON 字段 'type'
    fixture_class_value_array_t class_value;  // JSON 字段 'class'
    char * field_1start;  // JSON 字段 '1start'
    bool value;  // JSON 字段 'value'
} fixture_t;

/** 将 JSON 文本解码为 fixture_t。返回 0 表示成功。 */
int fixture_from_json(const char *json_text, size_t json_length, fixture_t *out_value);
/** 将 fixture_t 编码为 JSON 文本；空间不足返回 -2。 */
int fixture_to_json(const fixture_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 fixture_t 内部动态内存。 */
void fixture_free(fixture_t *value);

#endif /* FIXTURE_H */
