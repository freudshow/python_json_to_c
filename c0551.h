/**
 * 文件名：c0551.h
 * 作者：json_to_c.py 自动生成
 * 版本：1.0.0
 * 生成日期：2026-10-11
 * 概述：由 JSON 示例自动生成的结构体和 JSON 编解码接口。
 * 修改记录：首次生成。
 */
#ifndef C0551_H
#define C0551_H

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

typedef struct c0551_youyahzconfig_youyahezha_mianban_t {
    int64_t enable;  // JSON 字段 'Enable'
    int64_t ykon;  // JSON 字段 'YkOn'
    int64_t switchyx;  // JSON 字段 'SwitchYX'
    int64_t ykcontinuetime;  // JSON 字段 'YkContinueTime'
    int64_t chonghezha_failcount;  // JSON 字段 'Chonghezha_failcount'
    int64_t chonghezha_interval_s;  // JSON 字段 'ChongHeZha_Interval_s'
    int64_t diyabeizitouenable;  // JSON 字段 'diyabeizitouEnable'
} c0551_youyahzconfig_youyahezha_mianban_t;

typedef struct c0551_youyahzconfig_diyabeizitouset_t {
    int64_t yx_qf1_rtdbno;  // JSON 字段 'YX_QF1_RtdbNo'
    int64_t ua_qf1_rtdbno;  // JSON 字段 'UA_QF1_RtdbNo'
    int64_t ub_qf1_rtdbno;  // JSON 字段 'UB_QF1_RtdbNo'
    int64_t uc_qf1_rtdbno;  // JSON 字段 'UC_QF1_RtdbNo'
    int64_t yk_qf1on_rtdbno;  // JSON 字段 'YK_QF1On_RtdbNo'
    int64_t yk_qf1off_rtdbno;  // JSON 字段 'YK_QF1Off_RtdbNo'
    int64_t yx_qfll_rtdb;  // JSON 字段 'YX_QFLL_Rtdb'
    int64_t ykon_qfll_rtdb;  // JSON 字段 'YKOn_QFLL_Rtdb'
    int64_t ykoff_qfll_rtdb;  // JSON 字段 'YKOff_QFLL_Rtdb'
    int64_t latertime;  // JSON 字段 'LaterTime'
    int64_t lessvoltage;  // JSON 字段 'LessVoltage'
    int64_t afterlessvoltimeexeyktime;  // JSON 字段 'AfterLessVolTimeExeYkTime'
    int64_t aftercheckyxtime;  // JSON 字段 'AfterCheckYxTime'
    int64_t yx_qf2_rtdbno;  // JSON 字段 'YX_QF2_RtdbNo'
    int64_t ua_qf2_rtdbno;  // JSON 字段 'UA_QF2_RtdbNo'
    int64_t ub_qf2_rtdbno;  // JSON 字段 'UB_QF2_RtdbNo'
    int64_t uc_qf2_rtdbno;  // JSON 字段 'UC_QF2_RtdbNo'
    int64_t yk_qf2on_rtdbno;  // JSON 字段 'YK_QF2On_RtdbNo'
    int64_t yk_qf2off_rtdbno;  // JSON 字段 'YK_QF2Off_RtdbNo'
    int64_t qf2commcheckrtdbno;  // JSON 字段 'QF2CommCheckRtdbNo'
    int64_t qfllcommcheckrtdbno;  // JSON 字段 'QFLLCommCheckRtdbNo'
    int64_t qf1commcheckrtdbno;  // JSON 字段 'QF1CommCheckRtdbNo'
    int64_t qf1tqerrcheckenable;  // JSON 字段 'QF1TQErrCheckEnable'
    int64_t qf1tqerrcheckrtdb;  // JSON 字段 'QF1TQErrCheckRtdb'
    int64_t qf2tqerrcheckenable;  // JSON 字段 'QF2TQErrCheckEnable'
    int64_t qf2tqerrcheckrtdb;  // JSON 字段 'QF2TQErrCheckRtdb'
    int64_t modeselect;  // JSON 字段 'ModeSelect'
    int64_t qfll_logic_enable;  // JSON 字段 'QFLL_Logic_Enable'
    int64_t bzthardplateenable;  // JSON 字段 'BZTHardPlateEnable'
    int64_t bzthardplatertdb;  // JSON 字段 'BZTHardPlateRtdb'
    int64_t extyxresetenable;  // JSON 字段 'ExtYXResetEnable'
    int64_t extyxresetrtdb;  // JSON 字段 'ExtYXResetRtdb'
    int64_t yxhz_rtdbno;  // JSON 字段 'YxHz_RtdbNo'
    int64_t exit_logic_enable;  // JSON 字段 'Exit_Logic_Enable'
    int64_t exit_logic_exeqfllyktime;  // JSON 字段 'Exit_Logic_ExeQFLLYkTime'
    int64_t qfll_hz_bisuoenable;  // JSON 字段 'QFLL_HZ_BiSuoEnable'
} c0551_youyahzconfig_diyabeizitouset_t;

typedef struct c0551_youyahzconfig_twopowerchange_t {
    int64_t modeselect;  // JSON 字段 'ModeSelect'
    int64_t bzthardplateenable;  // JSON 字段 'BZTHardPlateEnable'
    int64_t bzthardplatertdb;  // JSON 字段 'BZTHardPlateRtdb'
    int64_t extyxresetenable;  // JSON 字段 'ExtYXResetEnable'
    int64_t extyxresetrtdb;  // JSON 字段 'ExtYXResetRtdb'
    int64_t recoverplateenable;  // JSON 字段 'RecoverPlateEnable'
    int64_t qf1reclosingplateenable;  // JSON 字段 'QF1ReclosingPlateEnable'
    int64_t latertime;  // JSON 字段 'LaterTime'
    int64_t lessvoltage;  // JSON 字段 'LessVoltage'
    int64_t havevoltage;  // JSON 字段 'HaveVoltage'
    int64_t canreadytime;  // JSON 字段 'CanReadyTime'
    int64_t canexedelaytime;  // JSON 字段 'CanExeDelayTime'
    int64_t reclosingmaxtimes;  // JSON 字段 'ReclosingMaxTimes'
    int64_t yx_qf1_rtdbno;  // JSON 字段 'YX_QF1_RtdbNo'
    int64_t ua_qf1_rtdbno;  // JSON 字段 'UA_QF1_RtdbNo'
    int64_t ub_qf1_rtdbno;  // JSON 字段 'UB_QF1_RtdbNo'
    int64_t uc_qf1_rtdbno;  // JSON 字段 'UC_QF1_RtdbNo'
    int64_t ykno_qf1_on;  // JSON 字段 'YKNo_QF1_On'
    int64_t ykno_qf1_off;  // JSON 字段 'YKNo_QF1_Off'
    int64_t qf1commcheckrtdbno;  // JSON 字段 'QF1CommCheckRtdbNo'
    int64_t qf1tqerrcheckenable;  // JSON 字段 'QF1TQErrCheckEnable'
    int64_t qf1tqerrcheckrtdb;  // JSON 字段 'QF1TQErrCheckRtdb'
    int64_t yx_qf2_rtdbno;  // JSON 字段 'YX_QF2_RtdbNo'
    int64_t ua_qf2_rtdbno;  // JSON 字段 'UA_QF2_RtdbNo'
    int64_t ub_qf2_rtdbno;  // JSON 字段 'UB_QF2_RtdbNo'
    int64_t uc_qf2_rtdbno;  // JSON 字段 'UC_QF2_RtdbNo'
    int64_t ykno_qf2_on;  // JSON 字段 'YKNo_QF2_On'
    int64_t ykno_qf2_off;  // JSON 字段 'YKNo_QF2_Off'
    int64_t qf2commcheckrtdbno;  // JSON 字段 'QF2CommCheckRtdbNo'
    int64_t qf2tqerrcheckenable;  // JSON 字段 'QF2TQErrCheckEnable'
    int64_t qf2tqerrcheckrtdb;  // JSON 字段 'QF2TQErrCheckRtdb'
} c0551_youyahzconfig_twopowerchange_t;

typedef struct c0551_youyahzconfig_youyahezha_tablelist_item_t {
    int64_t no;  // JSON 字段 'No'
    char * name;  // JSON 字段 'Name'
    int64_t value;  // JSON 字段 'Value'
    char * unit;  // JSON 字段 'Unit'
    int64_t rtdbno;  // JSON 字段 'RtDbNo'
    double ratio;  // JSON 字段 'Ratio'
    int64_t ischeck;  // JSON 字段 'IsCheck'
    char * comment;  // JSON 字段 'Comment'
} c0551_youyahzconfig_youyahezha_tablelist_item_t;

typedef struct {
    c0551_youyahzconfig_youyahezha_tablelist_item_t *items;  // 数组元素
    size_t count;  // 元素数量
} c0551_youyahzconfig_youyahezha_tablelist_array_t;

typedef struct c0551_youyahzconfig_t {
    c0551_youyahzconfig_youyahezha_mianban_t youyahezha_mianban;  // JSON 字段 'youyahezha_mianban'
    c0551_youyahzconfig_diyabeizitouset_t diyabeizitouset;  // JSON 字段 'DiyabeizitouSet'
    c0551_youyahzconfig_twopowerchange_t twopowerchange;  // JSON 字段 'TwoPowerChange'
    c0551_youyahzconfig_youyahezha_tablelist_array_t youyahezha_tablelist;  // JSON 字段 'youyahezha_tablelist'
} c0551_youyahzconfig_t;

typedef struct c0551_t {
    c0551_youyahzconfig_t youyahzconfig;  // JSON 字段 'YouYaHzConfig'
} c0551_t;

/** 将 JSON 文本解码为 c0551_t。返回 0 表示成功。 */
int c0551_from_json(const char *json_text, size_t json_length, c0551_t *out_value);
/** 将 c0551_t 编码为 JSON 文本；空间不足返回 -2。 */
int c0551_to_json(const c0551_t *value, char *buffer, size_t buffer_size, size_t *written);
/** 释放 c0551_t 内部动态内存。 */
void c0551_free(c0551_t *value);

#endif /* C0551_H */
