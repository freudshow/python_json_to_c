/**
 * 文件名：demo_config_example.h
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-10-11
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#ifndef DEMO_CONFIG_EXAMPLE_H
#define DEMO_CONFIG_EXAMPLE_H

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

typedef struct demo_config_example_broker_t {
    char * ip;  // JSON 字段 'ip'
    char * port;  // JSON 字段 'port'
    char * my_app;  // JSON 字段 'my_app'
    char * target_app;  // JSON 字段 'target_app'
    int64_t msg_qid;  // JSON 字段 'msg_qid'
} demo_config_example_broker_t;

typedef struct demo_config_example_device_t {
    char * model;  // JSON 字段 'model'
    char * port;  // JSON 字段 'port'
    char * addr;  // JSON 字段 'addr'
    char * desc;  // JSON 字段 'desc'
} demo_config_example_device_t;

typedef struct demo_config_example_self_device_t {
    char * model;  // JSON 字段 'model'
    char * port;  // JSON 字段 'port'
    char * addr;  // JSON 字段 'addr'
    char * desc;  // JSON 字段 'desc'
} demo_config_example_self_device_t;

typedef struct demo_config_example_sequence_variant_1_t {
    char * op;  // JSON 字段 'op'
} demo_config_example_sequence_variant_1_t;

typedef struct demo_config_example_sequence_variant_2_points_item_t {
    char * name;  // JSON 字段 'name'
    char * type;  // JSON 字段 'type'
    char * unit;  // JSON 字段 'unit'
    int64_t deadzone;  // JSON 字段 'deadzone'
    int64_t ratio;  // JSON 字段 'ratio'
    int64_t isreport;  // JSON 字段 'isReport'
} demo_config_example_sequence_variant_2_points_item_t;

typedef struct {
    demo_config_example_sequence_variant_2_points_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_2_points_array_t;

typedef struct demo_config_example_sequence_variant_2_t {
    char * op;  // JSON 字段 'op'
    char * model;  // JSON 字段 'model'
    demo_config_example_sequence_variant_2_points_array_t points;  // JSON 字段 'points'
} demo_config_example_sequence_variant_2_t;

typedef struct demo_config_example_sequence_variant_3_t {
    char * op;  // JSON 字段 'op'
    char * model;  // JSON 字段 'model'
} demo_config_example_sequence_variant_3_t;

typedef struct demo_config_example_sequence_variant_4_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
} demo_config_example_sequence_variant_4_t;

typedef struct demo_config_example_sequence_variant_5_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    char * point;  // JSON 字段 'point'
} demo_config_example_sequence_variant_5_t;

typedef struct demo_config_example_sequence_variant_6_points_item_t {
    char * name;  // JSON 字段 'name'
    char * type;  // JSON 字段 'type'
} demo_config_example_sequence_variant_6_points_item_t;

typedef struct {
    demo_config_example_sequence_variant_6_points_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_6_points_array_t;

typedef struct demo_config_example_sequence_variant_6_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    demo_config_example_sequence_variant_6_points_array_t points;  // JSON 字段 'points'
} demo_config_example_sequence_variant_6_t;

typedef struct demo_config_example_sequence_variant_7_points_item_t {
    char * name;  // JSON 字段 'name'
    double val;  // JSON 字段 'val'
} demo_config_example_sequence_variant_7_points_item_t;

typedef struct {
    demo_config_example_sequence_variant_7_points_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_7_points_array_t;

typedef struct demo_config_example_sequence_variant_7_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    char * quality;  // JSON 字段 'quality'
    demo_config_example_sequence_variant_7_points_array_t points;  // JSON 字段 'points'
} demo_config_example_sequence_variant_7_t;

typedef struct demo_config_example_sequence_variant_8_points_variant_1_t {
    char * name;  // JSON 字段 'name'
    char * kind;  // JSON 字段 'kind'
    double val;  // JSON 字段 'val'
    char * unit;  // JSON 字段 'unit'
    int64_t id;  // JSON 字段 'id'
} demo_config_example_sequence_variant_8_points_variant_1_t;

typedef struct demo_config_example_sequence_variant_8_points_variant_2_t {
    char * name;  // JSON 字段 'name'
    char * kind;  // JSON 字段 'kind'
    int64_t val;  // JSON 字段 'val'
    char * unit;  // JSON 字段 'unit'
    int64_t id;  // JSON 字段 'id'
} demo_config_example_sequence_variant_8_points_variant_2_t;

typedef enum {
    DEMO_CONFIG_EXAMPLE_SEQUENCE_VARIANT_8_POINTS_KIND_T_VARIANT_1 = 0,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_VARIANT_8_POINTS_KIND_T_VARIANT_2 = 1,
} demo_config_example_sequence_variant_8_points_kind_t;

typedef struct {
    demo_config_example_sequence_variant_8_points_kind_t kind;  // 当前 union 成员
    union {
        demo_config_example_sequence_variant_8_points_variant_1_t variant_1;
        demo_config_example_sequence_variant_8_points_variant_2_t variant_2;
    } value;
} demo_config_example_sequence_variant_8_points_item_t;

typedef struct {
    demo_config_example_sequence_variant_8_points_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_8_points_array_t;

typedef struct demo_config_example_sequence_variant_8_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    demo_config_example_sequence_variant_8_points_array_t points;  // JSON 字段 'points'
} demo_config_example_sequence_variant_8_t;

typedef struct demo_config_example_sequence_variant_9_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    bool total;  // JSON 字段 'total'
} demo_config_example_sequence_variant_9_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_10_names_array_t;

typedef struct demo_config_example_sequence_variant_10_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    demo_config_example_sequence_variant_10_names_array_t names;  // JSON 字段 'names'
} demo_config_example_sequence_variant_10_t;

typedef struct demo_config_example_sequence_variant_11_points_item_t {
    char * name;  // JSON 字段 'name'
    int64_t val;  // JSON 字段 'val'
    char * datatype;  // JSON 字段 'datatype'
} demo_config_example_sequence_variant_11_points_item_t;

typedef struct {
    demo_config_example_sequence_variant_11_points_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_11_points_array_t;

typedef struct demo_config_example_sequence_variant_11_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    demo_config_example_sequence_variant_11_points_array_t points;  // JSON 字段 'points'
} demo_config_example_sequence_variant_11_t;

typedef struct demo_config_example_sequence_variant_12_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    char * point;  // JSON 字段 'point'
    char * cmd;  // JSON 字段 'cmd'
    char * token;  // JSON 字段 'token'
    int64_t delay_ms;  // JSON 字段 'delay_ms'
    char * dev;  // JSON 字段 'dev'
} demo_config_example_sequence_variant_12_t;

typedef struct demo_config_example_sequence_variant_13_t {
    char * op;  // JSON 字段 'op'
    char * name;  // JSON 字段 'name'
    char * value;  // JSON 字段 'value'
    char * datatype;  // JSON 字段 'datatype'
    char * token;  // JSON 字段 'token'
    int64_t delay_ms;  // JSON 字段 'delay_ms'
} demo_config_example_sequence_variant_13_t;

typedef struct demo_config_example_sequence_variant_14_extdata_item_t {
    char * name;  // JSON 字段 'name'
    char * value;  // JSON 字段 'value'
} demo_config_example_sequence_variant_14_extdata_item_t;

typedef struct {
    demo_config_example_sequence_variant_14_extdata_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_14_extdata_array_t;

typedef struct demo_config_example_sequence_variant_14_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    char * yx_mode;  // JSON 字段 'yx_mode'
    demo_config_example_sequence_variant_14_extdata_array_t extdata;  // JSON 字段 'extdata'
} demo_config_example_sequence_variant_14_t;

typedef struct demo_config_example_sequence_variant_15_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    char * yx_mode;  // JSON 字段 'yx_mode'
} demo_config_example_sequence_variant_15_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_16_names_array_t;

typedef struct demo_config_example_sequence_variant_16_t {
    char * op;  // JSON 字段 'op'
    demo_config_example_sequence_variant_16_names_array_t names;  // JSON 字段 'names'
} demo_config_example_sequence_variant_16_t;

typedef struct {
    int64_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_17_start_array_t;

typedef struct {
    int64_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_17_end_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_variant_17_names_array_t;

typedef struct demo_config_example_sequence_variant_17_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    demo_config_example_sequence_variant_17_start_array_t start;  // JSON 字段 'start'
    demo_config_example_sequence_variant_17_end_array_t end;  // JSON 字段 'end'
    int64_t time_span;  // JSON 字段 'time_span'
    char * frozen_type;  // JSON 字段 'frozen_type'
    demo_config_example_sequence_variant_17_names_array_t names;  // JSON 字段 'names'
} demo_config_example_sequence_variant_17_t;

typedef struct demo_config_example_sequence_variant_18_t {
    char * op;  // JSON 字段 'op'
    char * device_ref;  // JSON 字段 'device_ref'
    char * model;  // JSON 字段 'model'
} demo_config_example_sequence_variant_18_t;

typedef enum {
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_1 = 0,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_2 = 1,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_3 = 2,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_4 = 3,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_5 = 4,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_6 = 5,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_7 = 6,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_8 = 7,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_9 = 8,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_10 = 9,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_11 = 10,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_12 = 11,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_13 = 12,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_14 = 13,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_15 = 14,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_16 = 15,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_17 = 16,
    DEMO_CONFIG_EXAMPLE_SEQUENCE_KIND_T_VARIANT_18 = 17,
} demo_config_example_sequence_kind_t;

typedef struct {
    demo_config_example_sequence_kind_t kind;  // 当前 union 成员
    union {
        demo_config_example_sequence_variant_1_t variant_1;
        demo_config_example_sequence_variant_2_t variant_2;
        demo_config_example_sequence_variant_3_t variant_3;
        demo_config_example_sequence_variant_4_t variant_4;
        demo_config_example_sequence_variant_5_t variant_5;
        demo_config_example_sequence_variant_6_t variant_6;
        demo_config_example_sequence_variant_7_t variant_7;
        demo_config_example_sequence_variant_8_t variant_8;
        demo_config_example_sequence_variant_9_t variant_9;
        demo_config_example_sequence_variant_10_t variant_10;
        demo_config_example_sequence_variant_11_t variant_11;
        demo_config_example_sequence_variant_12_t variant_12;
        demo_config_example_sequence_variant_13_t variant_13;
        demo_config_example_sequence_variant_14_t variant_14;
        demo_config_example_sequence_variant_15_t variant_15;
        demo_config_example_sequence_variant_16_t variant_16;
        demo_config_example_sequence_variant_17_t variant_17;
        demo_config_example_sequence_variant_18_t variant_18;
    } value;
} demo_config_example_sequence_item_t;

typedef struct {
    demo_config_example_sequence_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} demo_config_example_sequence_array_t;

typedef struct demo_config_example_t {
    char * comment;  // JSON 字段 'comment'
    demo_config_example_broker_t broker;  // JSON 字段 'broker'
    int64_t verbose;  // JSON 字段 'verbose'
    demo_config_example_device_t device;  // JSON 字段 'device'
    demo_config_example_self_device_t self_device;  // JSON 字段 'self_device'
    int64_t default_delay_ms;  // JSON 字段 'default_delay_ms'
    demo_config_example_sequence_array_t sequence;  // JSON 字段 'sequence'
} demo_config_example_t;

/** 将 JSON 文本解码为 demo_config_example_t。返回 0 表示成功。 */
int demo_config_example_from_json(const char *json_text, size_t json_length, demo_config_example_t *out_value);
/** 将 demo_config_example_t 编码为 JSON 文本；空间不足返回 -2。 */
int demo_config_example_to_json(const demo_config_example_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 demo_config_example_t 内部动态内存。 */
void demo_config_example_free(demo_config_example_t *value);

#endif /* DEMO_CONFIG_EXAMPLE_H */
