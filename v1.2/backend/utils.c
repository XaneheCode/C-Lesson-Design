/**
 * ============================================================
 *  utils.c — 工具函数实现
 *  作用: JSON构建器、URL解码、字符串处理
 * ============================================================
 *
 *  【JsonBuilder 设计思路】
 *   由于C语言没有原生JSON支持,我们用"字符串拼接"的方式构建JSON:
 *   - 维护一个字符缓冲区 buf
 *   - 每次添加字段时,在末尾追加 "\"key\":value" 片段
 *   - 用 first 标记控制是否插入逗号分隔符
 *
 *   构建过程示例:
 *     初始:  (空)
 *     begin_obj → {
 *     add_str("name","张三") → {"name":"张三"
 *     add_int("age",20)      → {"name":"张三","age":20
 *     end_obj                 → {"name":"张三","age":20}
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "utils.h"

/* ========== JSON构建器实现 ========== */

/**
 * json_init() — 初始化JSON构建器
 *
 * @jb:  构建器指针
 * @buf: 用户提供的输出缓冲区
 * @cap: 缓冲区容量(字节数)
 *
 * 初始化后,buf为空字符串,first=1表示还没有任何字段
 */
void json_init(JsonBuilder *jb, char *buf, int cap) {
    jb->buf = buf;    /* 指向输出缓冲区 */
    jb->cap = cap;    /* 缓冲区总大小 */
    jb->len = 0;      /* 当前写入位置(0字节) */
    jb->first = 1;    /* 标记: 尚无任何字段 */
    buf[0] = '\0';    /* 清空缓冲区 */
}

/**
 * json_append_raw() — 向构建器追加原始字符串
 *
 * 这是构建器最底层的操作: 直接把字符串追加到buf末尾
 * 包含容量检查,防止缓冲区溢出
 */
void json_append_raw(JsonBuilder *jb, const char *s) {
    int slen = (int)strlen(s);
    /* 安全检查: 如果追加后会溢出,则放弃 */
    if (jb->len + slen >= jb->cap) return;
    /* memcpy 比 strcat 更高效(不需要再扫描找末尾) */
    memcpy(jb->buf + jb->len, s, slen);
    jb->len += slen;       /* 更新写入位置 */
    jb->buf[jb->len] = '\0'; /* 确保字符串以\0结尾 */
}

/**
 * json_add_str() — 添加字符串键值对
 *
 * 示例: json_add_str(&jb, "name", "晨光中性笔")
 * 输出: "name":"晨光中性笔"
 *
 * 如果不是第一个字段,前面会自动加逗号:
 *   第1个字段: "name":"晨光中性笔"
 *   第2个字段: ,"category":"笔"
 *
 * 值会经过 json_escape() 转义,防止JSON语法错误
 */
void json_add_str(JsonBuilder *jb, const char *key, const char *val) {
    char escaped[512];
    /* 先对值进行JSON转义(处理引号、反斜杠等特殊字符) */
    json_escape(val, escaped, sizeof(escaped));

    char tmp[1024];
    /* 如果不是第一个字段,先输出逗号分隔符 */
    if (!jb->first) json_append_raw(jb, ",");
    /* 拼接 "key":"escaped_value" */
    sprintf(tmp, "\"%s\":\"%s\"", key, escaped);
    json_append_raw(jb, tmp);
    jb->first = 0;  /* 标记: 已有字段,后续需要加逗号 */
}

/**
 * json_add_int() — 添加整数键值对
 * 示例: json_add_int(&jb, "stock", 150) → ,"stock":150
 */
void json_add_int(JsonBuilder *jb, const char *key, int val) {
    char tmp[256];
    if (!jb->first) json_append_raw(jb, ",");
    sprintf(tmp, "\"%s\":%d", key, val);  /* 整数不需要引号 */
    json_append_raw(jb, tmp);
    jb->first = 0;
}

/**
 * json_add_dbl() — 添加浮点数键值对
 * 示例: json_add_dbl(&jb, "price", 2.5) → ,"price":2.50
 * %.2f 保留两位小数,统一金额格式
 */
void json_add_dbl(JsonBuilder *jb, const char *key, double val) {
    char tmp[256];
    if (!jb->first) json_append_raw(jb, ",");
    sprintf(tmp, "\"%s\":%.2f", key, val);
    json_append_raw(jb, tmp);
    jb->first = 0;
}

/**
 * json_add_raw() — 添加原始值(不加引号)
 * 用于添加布尔值、null等非字符串类型
 * 示例: json_add_raw(&jb, "active", "true") → ,"active":true
 */
void json_add_raw(JsonBuilder *jb, const char *key, const char *raw_val) {
    char tmp[256];
    if (!jb->first) json_append_raw(jb, ",");
    sprintf(tmp, "\"%s\":%s", key, raw_val);
    json_append_raw(jb, tmp);
    jb->first = 0;
}

/**
 * json_begin_array() — 开始一个数组字段
 * 示例: json_begin_array(&jb, "data") → 输出 "data":[
 * 注意: 进入数组后,first重置为1,数组内的对象之间也需要逗号
 */
void json_begin_array(JsonBuilder *jb, const char *key) {
    char tmp[256];
    if (!jb->first) json_append_raw(jb, ",");
    sprintf(tmp, "\"%s\":[", key);
    json_append_raw(jb, tmp);
    jb->first = 1;  /* 重置: 数组内的第一个元素不需要逗号 */
}

/** json_end_array() — 结束数组: 输出 ] */
void json_end_array(JsonBuilder *jb) {
    json_append_raw(jb, "]");
    jb->first = 0;  /* 数组本身算一个字段,后续字段需要逗号 */
}

/** json_begin_obj() — 开始对象: 输出 { */
void json_begin_obj(JsonBuilder *jb) {
    json_append_raw(jb, "{");
    jb->first = 1;  /* 对象内的第一个字段不需要逗号 */
}

/** json_end_obj() — 结束对象: 输出 } */
void json_end_obj(JsonBuilder *jb) {
    json_append_raw(jb, "}");
    jb->first = 0;  /* 对象本身算一个字段 */
}

/** json_get() — 获取构建完成的JSON字符串 */
const char *json_get(JsonBuilder *jb) {
    return jb->buf;
}

/* ========== URL解码 ========== */

/**
 * url_decode() — URL百分号解码
 *
 * 浏览器发送HTTP请求时,URL中的非ASCII字符会被编码:
 *   中文"笔" → %E7%AC%94 (UTF-8编码的十六进制表示)
 *   空格 → %20 或 +
 *
 * 解码算法(原地修改):
 *   遍历字符串,遇到%则读取后2个字符作为16进制数转换
 *   遇到+则替换为空格
 *   其他字符原样保留
 *
 * 示例: "category=%E7%AC%94" → "category=笔"
 */
void url_decode(char *src) {
    char *dst = src;  /* 读写指针分离,实现原地解码 */
    while (*src) {
        if (*src == '%' && src[1] && src[2]) {
            /* 遇到 %XX,将XX解析为16进制 */
            char hex[3] = {src[1], src[2], 0};  /* 提取两个十六进制字符 */
            *dst++ = (char)strtol(hex, NULL, 16); /* strtol将16进制字符串转为整数 */
            src += 3;  /* 跳过 %XX 三个字符 */
        } else if (*src == '+') {
            /* + 号表示空格(HTML表单的约定) */
            *dst++ = ' ';
            src++;
        } else {
            /* 普通字符,直接复制 */
            *dst++ = *src++;
        }
    }
    *dst = '\0';  /* 确保输出字符串结尾 */
}

/* ========== 字符串工具 ========== */

/**
 * trim() — 去除字符串首尾空白
 *
 * @str: 输入字符串(原地修改)
 * 返回值: 指向去空白后的起始位置
 *
 * 使用场景: 用户输入可能包含多余空格,如 "  P001  "
 * 去空白后: "P001"
 */
char *trim(char *str) {
    /* 去除开头空白: 指针向后移动,跳过所有空白字符 */
    while (*str && isspace((unsigned char)*str)) str++;

    /* 去除末尾空白: 从后往前,将空白替换为\0 */
    char *end = str + strlen(str) - 1;
    while (end > str && isspace((unsigned char)*end)) *end-- = '\0';

    return str;  /* 返回去空白后的起始位置 */
}

/**
 * json_escape() — JSON字符串转义
 *
 * @src:      原始字符串
 * @dst:      输出缓冲区(转义后的字符串)
 * @dst_size: 输出缓冲区大小
 *
 * JSON中以下字符有特殊含义,必须转义:
 *   "  → \"   (引号,JSON的字符串界定符)
 *   \  → \\   (反斜杠,转义起始符)
 *   换行 → \n (避免JSON中出现实际换行)
 *   回车 → \r
 *   制表 → \t
 *
 * 示例: 晨光"中性笔\n → 晨光\"中性笔\\n
 */
void json_escape(const char *src, char *dst, int dst_size) {
    int j = 0;  /* 输出位置索引 */
    for (int i = 0; src[i] && j < dst_size - 2; i++) {
        switch (src[i]) {
            case '"':  /* 引号: 输出 \" */
                if (j + 2 < dst_size) { dst[j++] = '\\'; dst[j++] = '"'; }
                break;
            case '\\': /* 反斜杠: 输出 \\ */
                if (j + 2 < dst_size) { dst[j++] = '\\'; dst[j++] = '\\'; }
                break;
            case '\n': /* 换行符: 输出 \n */
                if (j + 2 < dst_size) { dst[j++] = '\\'; dst[j++] = 'n'; }
                break;
            case '\r': /* 回车符: 输出 \r */
                if (j + 2 < dst_size) { dst[j++] = '\\'; dst[j++] = 'r'; }
                break;
            case '\t': /* 制表符: 输出 \t */
                if (j + 2 < dst_size) { dst[j++] = '\\'; dst[j++] = 't'; }
                break;
            default:   /* 普通字符: 原样保留(包括中文UTF-8多字节) */
                dst[j++] = src[i];
                break;
        }
    }
    dst[j] = '\0';  /* 确保输出字符串结尾 */
}
