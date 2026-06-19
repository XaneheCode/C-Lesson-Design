/**
 * ============================================================
 *  sale.h — 销售管理模块头文件
 *  作用: 声明销售记录CRUD操作的API接口
 * ============================================================
 *
 *  【对应需求】
 *   基本功能3: 文具商品交易信息管理
 *   基本功能5: 基础数据操作(增删改查+保存)
 *   基本功能7: 交易信息排序展示(按日期排序)
 *   拓展功能3: 销售流程管理(销售时自动扣减库存)
 *   拓展功能2: 多条件组合查询(按日期范围/类别/商品筛选)
 *
 *  【业务逻辑 — 销售出库流程】
 *   1. 前端选择商品,输入数量和售价
 *   2. 后端校验: 商品是否存在? 库存是否充足?
 *   3. 扣减商品库存 stock -= quantity
 *   4. 生成销售记录并保存
 *   5. 返回成功信息(含剩余库存)
 *
 *  【库存联动说明】
 *   创建销售 → 扣减库存
 *   修改销售 → 先恢复旧库存,再扣减新库存
 *   删除销售 → 恢复库存
 */

#ifndef SALE_H
#define SALE_H

/**
 * handle_get_sales() — 获取销售记录列表
 * @query: 查询字符串,支持参数:
 *   category=笔       — 按类别筛选
 *   date_from=2026-05-01  — 起始日期
 *   date_to=2026-05-31    — 结束日期
 *   product_id=P001       — 按商品编号筛选
 *   sort=asc|desc         — 日期排序方向(默认降序)
 */
void handle_get_sales(const char *query, char *response_body);

void handle_get_sale(const char *id, char *response_body);

/**
 * handle_create_sale() — 创建销售记录(销售出库)
 *
 * 请求体JSON示例:
 *   {"product_id":"P001", "date":"2026-05-27", "quantity":10, "price":3.00}
 *
 * 业务流程:
 *   1. 解析请求 → 2. 校验商品存在 → 3. 校验库存充足
 *   → 4. 生成S编号 → 5. 扣减库存 → 6. 保存两份文件
 */
void handle_create_sale(const char *body, char *response_body);

void handle_update_sale(const char *id, const char *body, char *response_body);
void handle_delete_sale(const char *id, char *response_body);

#endif /* SALE_H */
