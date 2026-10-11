/**
 * 文件名：c0916.h
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-10-11
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#ifndef C0916_H
#define C0916_H

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

typedef struct c0916_dataitem_item_t {
    int64_t rtdbno;  // JSON 字段 'RtDBNo'
    double ratio;  // JSON 字段 'Ratio'
    char * desc;  // JSON 字段 'desc'
    char * unit;  // JSON 字段 'Unit'
    double dead;  // JSON 字段 'dead'
    int64_t type;  // JSON 字段 'type'
} c0916_dataitem_item_t;

typedef struct {
    c0916_dataitem_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} c0916_dataitem_array_t;

typedef struct c0916_menu_root_children_variant_1_children_variant_1_children_item_t {
    char * name;  // JSON 字段 'name'
    char * action;  // JSON 字段 'action'
    int64_t passwd;  // JSON 字段 'passwd'
} c0916_menu_root_children_variant_1_children_variant_1_children_item_t;

typedef struct {
    c0916_menu_root_children_variant_1_children_variant_1_children_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} c0916_menu_root_children_variant_1_children_variant_1_children_array_t;

typedef struct c0916_menu_root_children_variant_1_children_variant_1_t {
    char * name;  // JSON 字段 'name'
    int64_t passwd;  // JSON 字段 'passwd'
    c0916_menu_root_children_variant_1_children_variant_1_children_array_t children;  // JSON 字段 'children'
} c0916_menu_root_children_variant_1_children_variant_1_t;

typedef struct c0916_menu_root_children_variant_1_children_variant_2_t {
    char * name;  // JSON 字段 'name'
    char * action;  // JSON 字段 'action'
    int64_t passwd;  // JSON 字段 'passwd'
} c0916_menu_root_children_variant_1_children_variant_2_t;

typedef enum {
    C0916_MENU_ROOT_CHILDREN_VARIANT_1_CHILDREN_KIND_T_VARIANT_1 = 0,
    C0916_MENU_ROOT_CHILDREN_VARIANT_1_CHILDREN_KIND_T_VARIANT_2 = 1,
} c0916_menu_root_children_variant_1_children_kind_t;

typedef struct {
    c0916_menu_root_children_variant_1_children_kind_t kind;  // 当前 union 成员
    union {
        c0916_menu_root_children_variant_1_children_variant_1_t variant_1;
        c0916_menu_root_children_variant_1_children_variant_2_t variant_2;
    } value;
} c0916_menu_root_children_variant_1_children_item_t;

typedef struct {
    c0916_menu_root_children_variant_1_children_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} c0916_menu_root_children_variant_1_children_array_t;

typedef struct c0916_menu_root_children_variant_1_t {
    char * name;  // JSON 字段 'name'
    int64_t passwd;  // JSON 字段 'passwd'
    c0916_menu_root_children_variant_1_children_array_t children;  // JSON 字段 'children'
} c0916_menu_root_children_variant_1_t;

typedef struct c0916_menu_root_children_variant_2_t {
    char * name;  // JSON 字段 'name'
    char * action;  // JSON 字段 'action'
    int64_t passwd;  // JSON 字段 'passwd'
} c0916_menu_root_children_variant_2_t;

typedef struct c0916_menu_root_children_variant_3_children_item_t {
    char * name;  // JSON 字段 'name'
    char * action;  // JSON 字段 'action'
    int64_t passwd;  // JSON 字段 'passwd'
} c0916_menu_root_children_variant_3_children_item_t;

typedef struct {
    c0916_menu_root_children_variant_3_children_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} c0916_menu_root_children_variant_3_children_array_t;

typedef struct c0916_menu_root_children_variant_3_t {
    char * name;  // JSON 字段 'name'
    int64_t passwd;  // JSON 字段 'passwd'
    c0916_menu_root_children_variant_3_children_array_t children;  // JSON 字段 'children'
} c0916_menu_root_children_variant_3_t;

typedef struct c0916_menu_root_children_variant_4_children_variant_1_t {
    char * name;  // JSON 字段 'name'
    char * action;  // JSON 字段 'action'
    int64_t passwd;  // JSON 字段 'passwd'
} c0916_menu_root_children_variant_4_children_variant_1_t;

typedef struct c0916_menu_root_children_variant_4_children_variant_2_children_item_t {
    char * name;  // JSON 字段 'name'
    char * action;  // JSON 字段 'action'
    int64_t passwd;  // JSON 字段 'passwd'
} c0916_menu_root_children_variant_4_children_variant_2_children_item_t;

typedef struct {
    c0916_menu_root_children_variant_4_children_variant_2_children_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} c0916_menu_root_children_variant_4_children_variant_2_children_array_t;

typedef struct c0916_menu_root_children_variant_4_children_variant_2_t {
    char * name;  // JSON 字段 'name'
    int64_t passwd;  // JSON 字段 'passwd'
    c0916_menu_root_children_variant_4_children_variant_2_children_array_t children;  // JSON 字段 'children'
} c0916_menu_root_children_variant_4_children_variant_2_t;

typedef enum {
    C0916_MENU_ROOT_CHILDREN_VARIANT_4_CHILDREN_KIND_T_VARIANT_1 = 0,
    C0916_MENU_ROOT_CHILDREN_VARIANT_4_CHILDREN_KIND_T_VARIANT_2 = 1,
} c0916_menu_root_children_variant_4_children_kind_t;

typedef struct {
    c0916_menu_root_children_variant_4_children_kind_t kind;  // 当前 union 成员
    union {
        c0916_menu_root_children_variant_4_children_variant_1_t variant_1;
        c0916_menu_root_children_variant_4_children_variant_2_t variant_2;
    } value;
} c0916_menu_root_children_variant_4_children_item_t;

typedef struct {
    c0916_menu_root_children_variant_4_children_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} c0916_menu_root_children_variant_4_children_array_t;

typedef struct c0916_menu_root_children_variant_4_t {
    char * name;  // JSON 字段 'name'
    int64_t passwd;  // JSON 字段 'passwd'
    c0916_menu_root_children_variant_4_children_array_t children;  // JSON 字段 'children'
} c0916_menu_root_children_variant_4_t;

typedef enum {
    C0916_MENU_ROOT_CHILDREN_KIND_T_VARIANT_1 = 0,
    C0916_MENU_ROOT_CHILDREN_KIND_T_VARIANT_2 = 1,
    C0916_MENU_ROOT_CHILDREN_KIND_T_VARIANT_3 = 2,
    C0916_MENU_ROOT_CHILDREN_KIND_T_VARIANT_4 = 3,
} c0916_menu_root_children_kind_t;

typedef struct {
    c0916_menu_root_children_kind_t kind;  // 当前 union 成员
    union {
        c0916_menu_root_children_variant_1_t variant_1;
        c0916_menu_root_children_variant_2_t variant_2;
        c0916_menu_root_children_variant_3_t variant_3;
        c0916_menu_root_children_variant_4_t variant_4;
    } value;
} c0916_menu_root_children_item_t;

typedef struct {
    c0916_menu_root_children_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} c0916_menu_root_children_array_t;

typedef struct c0916_menu_root_t {
    char * name;  // JSON 字段 'name'
    int64_t passwd;  // JSON 字段 'passwd'
    c0916_menu_root_children_array_t children;  // JSON 字段 'children'
} c0916_menu_root_t;

typedef struct c0916_menu_t {
    int64_t version;  // JSON 字段 'version'
    c0916_menu_root_t root;  // JSON 字段 'root'
} c0916_menu_t;

typedef struct c0916_t {
    c0916_dataitem_array_t dataitem;  // JSON 字段 'DataItem'
    int64_t onpagekeep;  // JSON 字段 'OnPageKeep'
    int64_t loopfreq;  // JSON 字段 'LoopFreq'
    int64_t suphardkey;  // JSON 字段 'SupHardKey'
    int64_t splitrow;  // JSON 字段 'SplitRow'
    int64_t descwidth;  // JSON 字段 'DescWidth'
    c0916_menu_t menu;  // JSON 字段 'menu'
} c0916_t;

/** 将 JSON 文本解码为 c0916_t。返回 0 表示成功。 */
int c0916_from_json(const char *json_text, size_t json_length, c0916_t *out_value);
/** 将 c0916_t 编码为 JSON 文本；空间不足返回 -2。 */
int c0916_to_json(const c0916_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 c0916_t 内部动态内存。 */
void c0916_free(c0916_t *value);

#endif /* C0916_H */
