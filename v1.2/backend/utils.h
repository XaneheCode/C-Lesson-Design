/**
 * ============================================================
 *  utils.h — 工具函数头文件
 *  作用: 提供JSON构建、URL解码、字符串处理等通用工具
 * ============================================================
 *
 *  【为什么需要自己写JSON构建器?】
 *   C语言没有内置JSON库,为了减少依赖(避免引第三方库),
 *   我们实现了一个轻量级的JSON构建器(JsonBuilder).
 *   它通过拼接字符串的方式逐步构建JSON,支持:
 *   - 字符串值: {"key":"value"}
 *   - 整数值:   {"key":42}
 *   - 浮点值:   {"key":3.14}
 *   - 嵌套对象: {"key":{"nested":"val"}}
 *   - 数组:     {"list":[{"a":1},{"a":2}]}
 */

#ifndef UTILS_H
#define UTILS_H

/* ========== JSON构建器 ========== */

/**
 * JsonBuilder — JSON字符串构建器
 *
 * 工作原理:
 *   buf  — 指向一个字符缓冲区,JSON内容逐步追加到此
 *   len  — 当前已写入的字符数
 *   cap  — 缓冲区总容量,防止溢出
 *   first— 标记是否为第一个字段(1=是),用于决定是否加逗号分隔符
 *
 * 使用示例:
 *   char buf[1024];
 *   JsonBuilder jb;
 *   json_init(&jb, buf, sizeof(buf));       // 初始化
 *   json_begin_obj(&jb);                     // 输出 {
 *   json_add_str(&jb, "name", "张三");       // 输出 "name":"张三"
 *   json_add_int(&jb, "age", 20);            // 输出 ,"age":20
 *   json_end_obj(&jb);                       // 输出 }
 *   // 结果: {"name":"张三","age":20}
 */
typedef struct {
    char *buf;   /* 指向JSON输出缓冲区 */
    int   len;   /* 当前写入位置(字节数) */
    int   cap;   /* 缓冲区总容量 */
    int   first; /* 1=还未写入任何字段; 0=已有字段,后续需加逗号 */
} JsonBuilder;

void json_init(JsonBuilder *jb, char *buf, int cap);      /* 初始化构建器 */
void json_add_str(JsonBuilder *jb, const char *key, const char *val);  /* 添加字符串字段,自动转义 */
void json_add_int(JsonBuilder *jb, const char *key, int val);          /* 添加整数字段 */
void json_add_dbl(JsonBuilder *jb, const char *key, double val);       /* 添加浮点字段,保留2位小数 */
void json_add_raw(JsonBuilder *jb, const char *key, const char *raw_val); /* 添加原始值(不加引号),如布尔true */
void json_begin_array(JsonBuilder *jb, const char *key);  /* 开始数组: "key":[ */
void json_end_array(JsonBuilder *jb);                      /* 结束数组: ] */
void json_begin_obj(JsonBuilder *jb);                      /* 开始对象: { */
void json_end_obj(JsonBuilder *jb);                        /* 结束对象: } */
const char *json_get(JsonBuilder *jb);                     /* 获取构建好的JSON字符串 */
void json_append_raw(JsonBuilder *jb, const char *s);     /* 直接追加原始字符串(如子对象JSON) */

/* ========== URL解码 ========== */

/**
 * url_decode() — 将URL编码的字符串解码为普通字符串
 *
 * 作用: 浏览器发送的URL中,中文等特殊字符会被编码(如%E6%96%87→文)
 * 算法:
 *   %XX  → 将XX作为16进制转为字符
 *   +    → 转为空格(表单提交约定)
 *   其他 → 原样保留
 *
 * 示例: "category=%E7%AC%94" → "category=笔"
 */
void url_decode(char *src);

/* ========== 字符串工具 ========== */

/**
 * trim() — 去除字符串首尾的空白字符(空格、制表符等)
 * @str: 输入字符串(会被原地修改)
 * 返回值: 指向去空白后的字符串起始位置
 */
char *trim(char *str);

/**
 * json_escape() — 对字符串进行JSON转义
 *
 * 作用: JSON中某些字符有特殊含义,需要转义
 * 转义规则:
 *   "  → \"
 *   \  → \\
 *   换行 → \n
 *   回车 → \r
 *   制表 → \t
 *
 * 示例: 晨光"中性笔 → 晨光\"中性笔
 */
void json_escape(const char *src, char *dst, int dst_size);

#endif /* UTILS_H */
