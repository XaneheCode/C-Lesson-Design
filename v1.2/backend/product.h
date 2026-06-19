/**
 * ============================================================
 *  product.h — 商品管理模块头文件
 *  作用: 声明商品CRUD操作的API接口
 * ============================================================
 *
 *  【对应需求】
 *   基本功能2: 文具商品库存信息管理 — 增删改查
 *   基本功能6: 库存信息查询 — 按商品编号查询
 *   拓展功能1: 多维度信息查询 — 按名称/类别/厂家筛选
 *
 *  【API路由对应关系】(定义在 server.c 的路由表中)
 *   GET    /api/products        → handle_get_products  获取商品列表(支持筛选)
 *   GET    /api/products/{id}   → handle_get_product   获取单个商品详情
 *   POST   /api/products        → handle_create_product 新增商品
 *   PUT    /api/products/{id}   → handle_update_product 修改商品
 *   DELETE /api/products/{id}   → handle_delete_product 删除商品
 *   GET    /api/products/stats  → handle_product_stats  获取商品统计数据
 */

#ifndef PRODUCT_H
#define PRODUCT_H

/* 商品列表查询(支持 category类别筛选、keyword关键字搜索) */
void handle_get_products(const char *query, char *response_body);

/* 按商品编号获取单个商品详情 */
void handle_get_product(const char *id, char *response_body);

/* 新增商品(从请求体JSON中解析字段) */
void handle_create_product(const char *body, char *response_body);

/* 修改商品信息(部分字段更新) */
void handle_update_product(const char *id, const char *body, char *response_body);

/* 删除商品 */
void handle_delete_product(const char *id, char *response_body);

/* 获取商品统计概览(总数、总库存、各类别数量等) */
void handle_product_stats(char *response_body);

/* ========== JSON解析辅助函数 ========== */
/* (放在product.h是因为首次使用在product.c,其他模块也复用) */

/**
 * json_get_str() — 从JSON字符串中提取指定key的字符串值
 * @json: JSON字符串,如 {"name":"张三"}
 * @key:  要提取的字段名,如 "name"
 * @out:  输出缓冲区
 * @out_len: 缓冲区大小
 * 返回: 1=成功, 0=字段不存在
 */
int json_get_str(const char *json, const char *key, char *out, int out_len);

/**
 * json_get_int() — 从JSON中提取整数值
 * 返回: 字段值,不存在返回0
 */
int json_get_int(const char *json, const char *key);

/**
 * json_get_dbl() — 从JSON中提取浮点值
 * 返回: 字段值,不存在返回0.0
 */
double json_get_dbl(const char *json, const char *key);

#endif /* PRODUCT_H */
