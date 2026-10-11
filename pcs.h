/**
 * 文件名：pcs.h
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-10-11
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#ifndef PCS_H
#define PCS_H

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

typedef struct pcs_target_t {
    char * device_family;  // JSON 字段 'device_family'
    char * min_runtime_version;  // JSON 字段 'min_runtime_version'
} pcs_target_t;

typedef struct pcs_scan_groups_item_t {
    char * id;  // JSON 字段 'id'
    int64_t period_ms;  // JSON 字段 'period_ms'
    int64_t priority;  // JSON 字段 'priority'
    int64_t max_execution_ms;  // JSON 字段 'max_execution_ms'
    char * bad_quality_policy;  // JSON 字段 'bad_quality_policy'
} pcs_scan_groups_item_t;

typedef struct {
    pcs_scan_groups_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_scan_groups_array_t;

typedef struct pcs_points_variant_1_address_t {
    int64_t function;  // JSON 字段 'function'
    int64_t register_value;  // JSON 字段 'register'
    int64_t count;  // JSON 字段 'count'
} pcs_points_variant_1_address_t;

typedef struct pcs_points_variant_1_t {
    char * id;  // JSON 字段 'id'
    char * direction;  // JSON 字段 'direction'
    char * data_type;  // JSON 字段 'data_type'
    char * unit;  // JSON 字段 'unit'
    char * protocol_instance;  // JSON 字段 'protocol_instance'
    pcs_points_variant_1_address_t address;  // JSON 字段 'address'
} pcs_points_variant_1_t;

typedef struct pcs_points_variant_2_address_t {
    int64_t function;  // JSON 字段 'function'
    int64_t register_value;  // JSON 字段 'register'
    int64_t count;  // JSON 字段 'count'
} pcs_points_variant_2_address_t;

typedef struct pcs_points_variant_2_limits_t {
    int64_t min;  // JSON 字段 'min'
    int64_t max;  // JSON 字段 'max'
    int64_t max_slew_per_second;  // JSON 字段 'max_slew_per_second'
} pcs_points_variant_2_limits_t;

typedef struct pcs_points_variant_2_t {
    char * id;  // JSON 字段 'id'
    char * direction;  // JSON 字段 'direction'
    char * data_type;  // JSON 字段 'data_type'
    char * unit;  // JSON 字段 'unit'
    char * protocol_instance;  // JSON 字段 'protocol_instance'
    pcs_points_variant_2_address_t address;  // JSON 字段 'address'
    pcs_points_variant_2_limits_t limits;  // JSON 字段 'limits'
} pcs_points_variant_2_t;

typedef enum {
    PCS_POINTS_KIND_T_VARIANT_1 = 0,
    PCS_POINTS_KIND_T_VARIANT_2 = 1,
} pcs_points_kind_t;

typedef struct {
    pcs_points_kind_t kind;  // 当前 union 成员
    union {
        pcs_points_variant_1_t variant_1;
        pcs_points_variant_2_t variant_2;
    } value;
} pcs_points_item_t;

typedef struct {
    pcs_points_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_points_array_t;

typedef struct pcs_logic_nodes_variant_1_params_t {
    char * point_id;  // JSON 字段 'point_id'
} pcs_logic_nodes_variant_1_params_t;

typedef struct pcs_logic_nodes_variant_1_t {
    char * id;  // JSON 字段 'id'
    char * type_id;  // JSON 字段 'type_id'
    int64_t type_version;  // JSON 字段 'type_version'
    char * scan_group;  // JSON 字段 'scan_group'
    pcs_logic_nodes_variant_1_params_t params;  // JSON 字段 'params'
} pcs_logic_nodes_variant_1_t;

typedef struct pcs_logic_nodes_variant_2_params_t {
    int64_t value;  // JSON 字段 'value'
    char * unit;  // JSON 字段 'unit'
} pcs_logic_nodes_variant_2_params_t;

typedef struct pcs_logic_nodes_variant_2_t {
    char * id;  // JSON 字段 'id'
    char * type_id;  // JSON 字段 'type_id'
    int64_t type_version;  // JSON 字段 'type_version'
    char * scan_group;  // JSON 字段 'scan_group'
    pcs_logic_nodes_variant_2_params_t params;  // JSON 字段 'params'
} pcs_logic_nodes_variant_2_t;

typedef struct pcs_logic_nodes_variant_3_params_t {
    uint8_t _unused;  // 空对象占位字段
} pcs_logic_nodes_variant_3_params_t;

typedef struct pcs_logic_nodes_variant_3_t {
    char * id;  // JSON 字段 'id'
    char * type_id;  // JSON 字段 'type_id'
    int64_t type_version;  // JSON 字段 'type_version'
    char * scan_group;  // JSON 字段 'scan_group'
    pcs_logic_nodes_variant_3_params_t params;  // JSON 字段 'params'
} pcs_logic_nodes_variant_3_t;

typedef struct pcs_logic_nodes_variant_4_params_t {
    int64_t min;  // JSON 字段 'min'
    int64_t max;  // JSON 字段 'max'
} pcs_logic_nodes_variant_4_params_t;

typedef struct pcs_logic_nodes_variant_4_t {
    char * id;  // JSON 字段 'id'
    char * type_id;  // JSON 字段 'type_id'
    int64_t type_version;  // JSON 字段 'type_version'
    char * scan_group;  // JSON 字段 'scan_group'
    pcs_logic_nodes_variant_4_params_t params;  // JSON 字段 'params'
} pcs_logic_nodes_variant_4_t;

typedef struct pcs_logic_nodes_variant_5_params_t {
    char * point_id;  // JSON 字段 'point_id'
    char * interlock_id;  // JSON 字段 'interlock_id'
} pcs_logic_nodes_variant_5_params_t;

typedef struct pcs_logic_nodes_variant_5_t {
    char * id;  // JSON 字段 'id'
    char * type_id;  // JSON 字段 'type_id'
    int64_t type_version;  // JSON 字段 'type_version'
    char * scan_group;  // JSON 字段 'scan_group'
    pcs_logic_nodes_variant_5_params_t params;  // JSON 字段 'params'
} pcs_logic_nodes_variant_5_t;

typedef enum {
    PCS_LOGIC_NODES_KIND_T_VARIANT_1 = 0,
    PCS_LOGIC_NODES_KIND_T_VARIANT_2 = 1,
    PCS_LOGIC_NODES_KIND_T_VARIANT_3 = 2,
    PCS_LOGIC_NODES_KIND_T_VARIANT_4 = 3,
    PCS_LOGIC_NODES_KIND_T_VARIANT_5 = 4,
} pcs_logic_nodes_kind_t;

typedef struct {
    pcs_logic_nodes_kind_t kind;  // 当前 union 成员
    union {
        pcs_logic_nodes_variant_1_t variant_1;
        pcs_logic_nodes_variant_2_t variant_2;
        pcs_logic_nodes_variant_3_t variant_3;
        pcs_logic_nodes_variant_4_t variant_4;
        pcs_logic_nodes_variant_5_t variant_5;
    } value;
} pcs_logic_nodes_item_t;

typedef struct {
    pcs_logic_nodes_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_logic_nodes_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_logic_data_edges_item_from_value_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_logic_data_edges_item_to_array_t;

typedef struct pcs_logic_data_edges_item_t {
    pcs_logic_data_edges_item_from_value_array_t from_value;  // JSON 字段 'from'
    pcs_logic_data_edges_item_to_array_t to;  // JSON 字段 'to'
} pcs_logic_data_edges_item_t;

typedef struct {
    pcs_logic_data_edges_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_logic_data_edges_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_logic_exec_edges_item_from_value_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_logic_exec_edges_item_to_array_t;

typedef struct pcs_logic_exec_edges_item_t {
    pcs_logic_exec_edges_item_from_value_array_t from_value;  // JSON 字段 'from'
    pcs_logic_exec_edges_item_to_array_t to;  // JSON 字段 'to'
} pcs_logic_exec_edges_item_t;

typedef struct {
    pcs_logic_exec_edges_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_logic_exec_edges_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} pcs_logic_entry_nodes_array_t;

typedef struct pcs_logic_t {
    pcs_logic_nodes_array_t nodes;  // JSON 字段 'nodes'
    pcs_logic_data_edges_array_t data_edges;  // JSON 字段 'data_edges'
    pcs_logic_exec_edges_array_t exec_edges;  // JSON 字段 'exec_edges'
    pcs_logic_entry_nodes_array_t entry_nodes;  // JSON 字段 'entry_nodes'
} pcs_logic_t;

typedef struct pcs_visual_nodes_read_power_t {
    int64_t x;  // JSON 字段 'x'
    int64_t y;  // JSON 字段 'y'
    int64_t width;  // JSON 字段 'width'
    int64_t height;  // JSON 字段 'height'
} pcs_visual_nodes_read_power_t;

typedef struct pcs_visual_nodes_upper_limit_t {
    int64_t x;  // JSON 字段 'x'
    int64_t y;  // JSON 字段 'y'
    int64_t width;  // JSON 字段 'width'
    int64_t height;  // JSON 字段 'height'
} pcs_visual_nodes_upper_limit_t;

typedef struct pcs_visual_nodes_over_limit_t {
    int64_t x;  // JSON 字段 'x'
    int64_t y;  // JSON 字段 'y'
    int64_t width;  // JSON 字段 'width'
    int64_t height;  // JSON 字段 'height'
} pcs_visual_nodes_over_limit_t;

typedef struct pcs_visual_nodes_select_action_t {
    int64_t x;  // JSON 字段 'x'
    int64_t y;  // JSON 字段 'y'
    int64_t width;  // JSON 字段 'width'
    int64_t height;  // JSON 字段 'height'
} pcs_visual_nodes_select_action_t;

typedef struct pcs_visual_nodes_reduce_power_t {
    int64_t x;  // JSON 字段 'x'
    int64_t y;  // JSON 字段 'y'
    int64_t width;  // JSON 字段 'width'
    int64_t height;  // JSON 字段 'height'
} pcs_visual_nodes_reduce_power_t;

typedef struct pcs_visual_nodes_clamp_setpoint_t {
    int64_t x;  // JSON 字段 'x'
    int64_t y;  // JSON 字段 'y'
    int64_t width;  // JSON 字段 'width'
    int64_t height;  // JSON 字段 'height'
} pcs_visual_nodes_clamp_setpoint_t;

typedef struct pcs_visual_nodes_request_setpoint_t {
    int64_t x;  // JSON 字段 'x'
    int64_t y;  // JSON 字段 'y'
    int64_t width;  // JSON 字段 'width'
    int64_t height;  // JSON 字段 'height'
} pcs_visual_nodes_request_setpoint_t;

typedef struct pcs_visual_nodes_t {
    pcs_visual_nodes_read_power_t read_power;  // JSON 字段 'read-power'
    pcs_visual_nodes_upper_limit_t upper_limit;  // JSON 字段 'upper-limit'
    pcs_visual_nodes_over_limit_t over_limit;  // JSON 字段 'over-limit'
    pcs_visual_nodes_select_action_t select_action;  // JSON 字段 'select-action'
    pcs_visual_nodes_reduce_power_t reduce_power;  // JSON 字段 'reduce-power'
    pcs_visual_nodes_clamp_setpoint_t clamp_setpoint;  // JSON 字段 'clamp-setpoint'
    pcs_visual_nodes_request_setpoint_t request_setpoint;  // JSON 字段 'request-setpoint'
} pcs_visual_nodes_t;

typedef struct pcs_visual_t {
    pcs_visual_nodes_t nodes;  // JSON 字段 'nodes'
} pcs_visual_t;

typedef struct pcs_t {
    char * schema_version;  // JSON 字段 'schema_version'
    char * project_id;  // JSON 字段 'project_id'
    int64_t revision;  // JSON 字段 'revision'
    pcs_target_t target;  // JSON 字段 'target'
    pcs_scan_groups_array_t scan_groups;  // JSON 字段 'scan_groups'
    pcs_points_array_t points;  // JSON 字段 'points'
    pcs_logic_t logic;  // JSON 字段 'logic'
    pcs_visual_t visual;  // JSON 字段 'visual'
} pcs_t;

/** 将 JSON 文本解码为 pcs_t。返回 0 表示成功。 */
int pcs_from_json(const char *json_text, size_t json_length, pcs_t *out_value);
/** 将 pcs_t 编码为 JSON 文本；空间不足返回 -2。 */
int pcs_to_json(const pcs_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 pcs_t 内部动态内存。 */
void pcs_free(pcs_t *value);

#endif /* PCS_H */
