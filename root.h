/**
 * 文件名：root.h
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-10-11
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#ifndef ROOT_H
#define ROOT_H

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

typedef struct root_components_variant_1_data_t {
    int64_t kv;  // JSON 字段 '电压(kV)'
    int64_t hz;  // JSON 字段 '频率(Hz)'
} root_components_variant_1_data_t;

typedef struct root_components_variant_1_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_1_data_t data;  // JSON 字段 'Data'
} root_components_variant_1_t;

typedef struct root_components_variant_2_data_t {
    int64_t a;  // JSON 字段 '额定电流(A)'
    int64_t v;  // JSON 字段 '额定电压(V)'
    int64_t ms;  // JSON 字段 '脱扣时间(ms)'
} root_components_variant_2_data_t;

typedef struct root_components_variant_2_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_2_data_t data;  // JSON 字段 'Data'
} root_components_variant_2_t;

typedef struct root_components_variant_3_data_t {
    int64_t field;  // JSON 字段 '变比'
    int64_t a;  // JSON 字段 '量程(A)'
} root_components_variant_3_data_t;

typedef struct root_components_variant_3_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_3_data_t data;  // JSON 字段 'Data'
} root_components_variant_3_t;

typedef struct root_components_variant_4_data_t {
    int64_t field;  // JSON 字段 '地址'
    int64_t field_2;  // JSON 字段 '倍率'
    int64_t kwh;  // JSON 字段 '电能(kWh)'
} root_components_variant_4_data_t;

typedef struct root_components_variant_4_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_4_data_t data;  // JSON 字段 'Data'
} root_components_variant_4_t;

typedef struct root_components_variant_5_data_t {
    char * field;  // JSON 字段 '逻辑'
} root_components_variant_5_data_t;

typedef struct root_components_variant_5_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_5_data_t data;  // JSON 字段 'Data'
} root_components_variant_5_t;

typedef struct root_components_variant_6_data_t {
    int64_t s;  // JSON 字段 '延时(s)'
    int64_t s_2;  // JSON 字段 '当前(s)'
} root_components_variant_6_data_t;

typedef struct root_components_variant_6_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_6_data_t data;  // JSON 字段 'Data'
} root_components_variant_6_t;

typedef struct root_components_variant_7_data_t {
    char * field;  // JSON 字段 '类型'
    bool field_2;  // JSON 字段 '使能'
} root_components_variant_7_data_t;

typedef struct root_components_variant_7_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_7_data_t data;  // JSON 字段 'Data'
} root_components_variant_7_t;

typedef struct root_components_variant_8_data_t {
    int64_t field;  // JSON 字段 '上限(%)'
    int64_t field_2;  // JSON 字段 '下限(%)'
    int64_t field_3;  // JSON 字段 '地址'
} root_components_variant_8_data_t;

typedef struct root_components_variant_8_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_8_data_t data;  // JSON 字段 'Data'
} root_components_variant_8_t;

typedef struct root_components_variant_9_data_t {
    int64_t field;  // JSON 字段 '地址'
    int64_t ip;  // JSON 字段 'IP'
    int64_t ms;  // JSON 字段 '周期(ms)'
} root_components_variant_9_data_t;

typedef struct root_components_variant_9_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_9_data_t data;  // JSON 字段 'Data'
} root_components_variant_9_t;

typedef struct root_components_variant_10_data_t {
    char * field;  // JSON 字段 '颜色'
} root_components_variant_10_data_t;

typedef struct root_components_variant_10_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_10_data_t data;  // JSON 字段 'Data'
} root_components_variant_10_t;

typedef struct root_components_variant_11_data_t {
    char * kw;  // JSON 字段 '功率(kW)'
    int64_t rpm;  // JSON 字段 '转速(rpm)'
    char * field;  // JSON 字段 '状态'
} root_components_variant_11_data_t;

typedef struct root_components_variant_11_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_11_data_t data;  // JSON 字段 'Data'
} root_components_variant_11_t;

typedef struct root_components_variant_12_data_t {
    int64_t c;  // JSON 字段 '上限(°C)'
    int64_t c_2;  // JSON 字段 '下限(°C)'
    int64_t field;  // JSON 字段 '地址'
} root_components_variant_12_data_t;

typedef struct root_components_variant_12_t {
    char * id;  // JSON 字段 'Id'
    char * kind;  // JSON 字段 'Kind'
    char * name;  // JSON 字段 'Name'
    int64_t x;  // JSON 字段 'X'
    int64_t y;  // JSON 字段 'Y'
    root_components_variant_12_data_t data;  // JSON 字段 'Data'
} root_components_variant_12_t;

typedef enum {
    ROOT_COMPONENTS_KIND_T_VARIANT_1 = 0,
    ROOT_COMPONENTS_KIND_T_VARIANT_2 = 1,
    ROOT_COMPONENTS_KIND_T_VARIANT_3 = 2,
    ROOT_COMPONENTS_KIND_T_VARIANT_4 = 3,
    ROOT_COMPONENTS_KIND_T_VARIANT_5 = 4,
    ROOT_COMPONENTS_KIND_T_VARIANT_6 = 5,
    ROOT_COMPONENTS_KIND_T_VARIANT_7 = 6,
    ROOT_COMPONENTS_KIND_T_VARIANT_8 = 7,
    ROOT_COMPONENTS_KIND_T_VARIANT_9 = 8,
    ROOT_COMPONENTS_KIND_T_VARIANT_10 = 9,
    ROOT_COMPONENTS_KIND_T_VARIANT_11 = 10,
    ROOT_COMPONENTS_KIND_T_VARIANT_12 = 11,
} root_components_kind_t;

typedef struct {
    root_components_kind_t kind;  // 当前 union 成员
    union {
        root_components_variant_1_t variant_1;
        root_components_variant_2_t variant_2;
        root_components_variant_3_t variant_3;
        root_components_variant_4_t variant_4;
        root_components_variant_5_t variant_5;
        root_components_variant_6_t variant_6;
        root_components_variant_7_t variant_7;
        root_components_variant_8_t variant_8;
        root_components_variant_9_t variant_9;
        root_components_variant_10_t variant_10;
        root_components_variant_11_t variant_11;
        root_components_variant_12_t variant_12;
    } value;
} root_components_item_t;

typedef struct {
    root_components_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} root_components_array_t;

typedef struct root_connections_item_t {
    char * s;  // JSON 字段 'S'
    char * st;  // JSON 字段 'ST'
    char * t;  // JSON 字段 'T'
    char * tt;  // JSON 字段 'TT'
} root_connections_item_t;

typedef struct {
    root_connections_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} root_connections_array_t;

typedef struct root_t {
    char * schema;  // JSON 字段 'Schema'
    root_components_array_t components;  // JSON 字段 'Components'
    root_connections_array_t connections;  // JSON 字段 'Connections'
} root_t;

/** 将 JSON 文本解码为 root_t。返回 0 表示成功。 */
int root_from_json(const char *json_text, size_t json_length, root_t *out_value);
/** 将 root_t 编码为 JSON 文本；空间不足返回 -2。 */
int root_to_json(const root_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 root_t 内部动态内存。 */
void root_free(root_t *value);

#endif /* ROOT_H */
