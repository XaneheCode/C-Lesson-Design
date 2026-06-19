/**
 * ============================================================
 *  purchase.h — 进货管理模块头文件
 *  作用: 声明进货记录CRUD操作的API接口
 * ============================================================
 *
 *  【对应需求】
 *   基本功能4: 文具商品进货信息管理
 *   拓展功能4: 进货流程管理(进货时自动增加库存)
 *
 *  【业务逻辑 — 进货入库流程】
 *   1. 前端选择商品,输入进货数量和进价
 *   2. 后端校验商品是否存在
 *   3. 增加商品库存 stock += quantity
 *   4. 生成进货记录并保存
 *   5. 返回成功信息(含当前库存)
 *
 *  【与销售模块的对称关系】
 *   销售: stock -= quantity (出库)
 *   进货: stock += quantity (入库)
 */

#ifndef PURCHASE_H
#define PURCHASE_H

/** 获取进货记录列表(支持类别、日期范围筛选) */
void handle_get_purchases(const char *query, char *response_body);

void handle_get_purchase(const char *id, char *response_body);

/**
 * handle_create_purchase() — 创建进货记录(进货入库)
 *
 * 请求体JSON示例:
 *   {"product_id":"P001", "date":"2026-05-25", "quantity":100, "price":1.80}
 *
 * 业务流程:
 *   1. 解析请求 → 2. 校验商品存在 → 3. 校验数量>0
 *   → 4. 生成B编号 → 5. 增加库存 → 6. 保存两份文件
 */
void handle_create_purchase(const char *body, char *response_body);

void handle_update_purchase(const char *id, const char *body, char *response_body);
void handle_delete_purchase(const char *id, char *response_body);

#endif /* PURCHASE_H */
