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

typedef struct fixture_class_value_variant_1_value_t {
    char * a_b;  // JSON 字段 'a-b'
} fixture_class_value_variant_1_value_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} fixture_class_value_variant_1_normal_key_array_t;

typedef struct fixture_class_value_variant_1_t {
    fixture_class_value_variant_1_value_t value;  // JSON 字段 'value'
    fixture_class_value_variant_1_normal_key_array_t normal_key;  // JSON 字段 'normal-key'
    int64_t a_b;  // JSON 字段 'a-b'
} fixture_class_value_variant_1_t;

typedef struct {
    json_value_t *items;  // 数组元素
    size_t count;  // 元素数量
} fixture_class_value_variant_2_a_b_array_t;

typedef struct {
    json_value_t *items;  // 数组元素
    size_t count;  // 元素数量
} fixture_class_value_variant_2_normal_key_array_t;

typedef struct fixture_class_value_variant_2_type_t {
    int64_t class_value;  // JSON 字段 'class'
    char * a_b;  // JSON 字段 'a_b'
} fixture_class_value_variant_2_type_t;

typedef struct fixture_class_value_variant_2_t {
    fixture_class_value_variant_2_a_b_array_t a_b;  // JSON 字段 'a-b'
    fixture_class_value_variant_2_normal_key_array_t normal_key;  // JSON 字段 'normal-key'
    fixture_class_value_variant_2_type_t type;  // JSON 字段 'type'
} fixture_class_value_variant_2_t;

typedef struct fixture_class_value_variant_3_t {
    uint8_t _unused;  // 空对象占位字段
} fixture_class_value_variant_3_t;

typedef enum {
    FIXTURE_CLASS_VALUE_KIND_T_VARIANT_1 = 0,
    FIXTURE_CLASS_VALUE_KIND_T_VARIANT_2 = 1,
    FIXTURE_CLASS_VALUE_KIND_T_VARIANT_3 = 2,
} fixture_class_value_kind_t;

typedef struct {
    fixture_class_value_kind_t kind;  // 当前 union 成员
    union {
        fixture_class_value_variant_1_t variant_1;
        fixture_class_value_variant_2_t variant_2;
        fixture_class_value_variant_3_t variant_3;
    } value;
} fixture_class_value_item_t;

typedef struct {
    fixture_class_value_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} fixture_class_value_array_t;

typedef struct {
    bool *items;  // 数组元素
    size_t count;  // 元素数量
} fixture_unicode_array_t;

typedef struct fixture_t {
    int64_t value;  // JSON 字段 'value'
    fixture_class_value_array_t class_value;  // JSON 字段 'class'
    int64_t a_b;  // JSON 字段 'a_b'
    fixture_unicode_array_t unicode;  // JSON 字段 'unicode键'
} fixture_t;

/** 将 JSON 文本解码为 fixture_t。返回 0 表示成功。 */
int fixture_from_json(const char *json_text, size_t json_length, fixture_t *out_value);
/** 将 fixture_t 编码为 JSON 文本；空间不足返回 -2。 */
int fixture_to_json(const fixture_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 fixture_t 内部动态内存。 */
void fixture_free(fixture_t *value);

#endif /* FIXTURE_H */
