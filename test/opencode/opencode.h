/**
 * 文件名：opencode.h
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-08-12
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#ifndef OPENCODE_H
#define OPENCODE_H

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

typedef struct opencode_provider_ollama_options_t {
    char * baseurl;  // JSON 字段 'baseURL'
} opencode_provider_ollama_options_t;

typedef struct opencode_provider_ollama_models_nemotron_3_ultra_cloud_t {
    char * name;  // JSON 字段 'name'
} opencode_provider_ollama_models_nemotron_3_ultra_cloud_t;

typedef struct opencode_provider_ollama_models_gemma4_31b_cloud_t {
    char * name;  // JSON 字段 'name'
} opencode_provider_ollama_models_gemma4_31b_cloud_t;

typedef struct opencode_provider_ollama_models_nemotron_3_super_cloud_t {
    char * name;  // JSON 字段 'name'
} opencode_provider_ollama_models_nemotron_3_super_cloud_t;

typedef struct opencode_provider_ollama_models_t {
    opencode_provider_ollama_models_nemotron_3_ultra_cloud_t nemotron_3_ultra_cloud;  // JSON 字段 'nemotron-3-ultra:cloud'
    opencode_provider_ollama_models_gemma4_31b_cloud_t gemma4_31b_cloud;  // JSON 字段 'gemma4:31b-cloud'
    opencode_provider_ollama_models_nemotron_3_super_cloud_t nemotron_3_super_cloud;  // JSON 字段 'nemotron-3-super:cloud'
} opencode_provider_ollama_models_t;

typedef struct opencode_provider_ollama_t {
    char * npm;  // JSON 字段 'npm'
    char * name;  // JSON 字段 'name'
    opencode_provider_ollama_options_t options;  // JSON 字段 'options'
    opencode_provider_ollama_models_t models;  // JSON 字段 'models'
} opencode_provider_ollama_t;

typedef struct opencode_provider_bailian_token_plan_personal_options_t {
    char * baseurl;  // JSON 字段 'baseURL'
    char * apikey;  // JSON 字段 'apiKey'
} opencode_provider_bailian_token_plan_personal_options_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_modalities_input_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_modalities_output_array_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_modalities_t {
    opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_modalities_input_array_t input;  // JSON 字段 'input'
    opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_modalities_output_array_t output;  // JSON 字段 'output'
} opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_modalities_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_options_thinking_t {
    char * type;  // JSON 字段 'type'
    int64_t budgettokens;  // JSON 字段 'budgetTokens'
} opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_options_thinking_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_options_t {
    opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_options_thinking_t thinking;  // JSON 字段 'thinking'
    bool reasoning;  // JSON 字段 'reasoning'
} opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_options_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_t {
    char * name;  // JSON 字段 'name'
    int64_t contextwindow;  // JSON 字段 'contextWindow'
    int64_t maxoutputtokens;  // JSON 字段 'maxOutputTokens'
    opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_modalities_t modalities;  // JSON 字段 'modalities'
    opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_options_t options;  // JSON 字段 'options'
} opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_options_thinking_t {
    char * type;  // JSON 字段 'type'
    int64_t budgettokens;  // JSON 字段 'budgetTokens'
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_options_thinking_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_options_t {
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_options_thinking_t thinking;  // JSON 字段 'thinking'
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_options_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_t {
    char * name;  // JSON 字段 'name'
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_options_t options;  // JSON 字段 'options'
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_modalities_input_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_modalities_output_array_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_modalities_t {
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_modalities_input_array_t input;  // JSON 字段 'input'
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_modalities_output_array_t output;  // JSON 字段 'output'
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_modalities_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_options_thinking_t {
    char * type;  // JSON 字段 'type'
    int64_t budgettokens;  // JSON 字段 'budgetTokens'
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_options_thinking_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_options_t {
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_options_thinking_t thinking;  // JSON 字段 'thinking'
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_options_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_t {
    char * name;  // JSON 字段 'name'
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_modalities_t modalities;  // JSON 字段 'modalities'
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_options_t options;  // JSON 字段 'options'
} opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_modalities_input_array_t;

typedef struct {
    char * *items;  // 数组元素
    size_t count;  // 元素数量
} opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_modalities_output_array_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_modalities_t {
    opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_modalities_input_array_t input;  // JSON 字段 'input'
    opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_modalities_output_array_t output;  // JSON 字段 'output'
} opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_modalities_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_options_thinking_t {
    char * type;  // JSON 字段 'type'
    int64_t budgettokens;  // JSON 字段 'budgetTokens'
} opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_options_thinking_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_options_t {
    opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_options_thinking_t thinking;  // JSON 字段 'thinking'
} opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_options_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_t {
    char * name;  // JSON 字段 'name'
    opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_modalities_t modalities;  // JSON 字段 'modalities'
    opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_options_t options;  // JSON 字段 'options'
} opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_glm_5_2_options_thinking_t {
    char * type;  // JSON 字段 'type'
    int64_t budgettokens;  // JSON 字段 'budgetTokens'
} opencode_provider_bailian_token_plan_personal_models_glm_5_2_options_thinking_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_glm_5_2_options_t {
    opencode_provider_bailian_token_plan_personal_models_glm_5_2_options_thinking_t thinking;  // JSON 字段 'thinking'
} opencode_provider_bailian_token_plan_personal_models_glm_5_2_options_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_glm_5_2_t {
    char * name;  // JSON 字段 'name'
    opencode_provider_bailian_token_plan_personal_models_glm_5_2_options_t options;  // JSON 字段 'options'
} opencode_provider_bailian_token_plan_personal_models_glm_5_2_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_deepseek_v4_pro_t {
    char * name;  // JSON 字段 'name'
} opencode_provider_bailian_token_plan_personal_models_deepseek_v4_pro_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_deepseek_v4_flash_0731_t {
    char * name;  // JSON 字段 'name'
} opencode_provider_bailian_token_plan_personal_models_deepseek_v4_flash_0731_t;

typedef struct opencode_provider_bailian_token_plan_personal_models_t {
    opencode_provider_bailian_token_plan_personal_models_qwen3_8_max_t qwen3_8_max;  // JSON 字段 'qwen3.8-max'
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_max_t qwen3_7_max;  // JSON 字段 'qwen3.7-max'
    opencode_provider_bailian_token_plan_personal_models_qwen3_7_plus_t qwen3_7_plus;  // JSON 字段 'qwen3.7-plus'
    opencode_provider_bailian_token_plan_personal_models_qwen3_6_flash_t qwen3_6_flash;  // JSON 字段 'qwen3.6-flash'
    opencode_provider_bailian_token_plan_personal_models_glm_5_2_t glm_5_2;  // JSON 字段 'glm-5.2'
    opencode_provider_bailian_token_plan_personal_models_deepseek_v4_pro_t deepseek_v4_pro;  // JSON 字段 'deepseek-v4-pro'
    opencode_provider_bailian_token_plan_personal_models_deepseek_v4_flash_0731_t deepseek_v4_flash_0731;  // JSON 字段 'deepseek-v4-flash-0731'
} opencode_provider_bailian_token_plan_personal_models_t;

typedef struct opencode_provider_bailian_token_plan_personal_t {
    char * npm;  // JSON 字段 'npm'
    char * name;  // JSON 字段 'name'
    opencode_provider_bailian_token_plan_personal_options_t options;  // JSON 字段 'options'
    opencode_provider_bailian_token_plan_personal_models_t models;  // JSON 字段 'models'
} opencode_provider_bailian_token_plan_personal_t;

typedef struct opencode_provider_t {
    opencode_provider_ollama_t ollama;  // JSON 字段 'ollama'
    opencode_provider_bailian_token_plan_personal_t bailian_token_plan_personal;  // JSON 字段 'bailian-token-plan-personal'
} opencode_provider_t;

typedef struct opencode_t {
    char * schema;  // JSON 字段 '$schema'
    opencode_provider_t provider;  // JSON 字段 'provider'
} opencode_t;

/** 将 JSON 文本解码为 opencode_t。返回 0 表示成功。 */
int opencode_from_json(const char *json_text, size_t json_length, opencode_t *out_value);
/** 将 opencode_t 编码为 JSON 文本；空间不足返回 -2。 */
int opencode_to_json(const opencode_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 opencode_t 内部动态内存。 */
void opencode_free(opencode_t *value);

#endif /* OPENCODE_H */
