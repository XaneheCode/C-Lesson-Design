/**
 * ============================================================
 *  purchase.c — 进货管理模块实现
 *  作用: 进货记录的增删改查,含库存联动逻辑
 * ============================================================
 *
 *  【核心业务: 库存联动 — 与sale.c相反】
 *   创建进货 → 商品库存 stock += quantity (入库,增加库存)
 *   修改进货 → 先撤销旧增量,再应用新增量
 *   删除进货 → 商品库存 stock -= quantity (撤销入库,减少库存)
 *
 *  【与销售模块的对称关系】
 *   销售 = 出库(stock减少)  ←→  进货 = 入库(stock增加)
 *   销售删除 = 退货(stock恢复)  ←→  进货删除 = 退购(stock扣回)
 *
 *  【排序功能 — 对应需求基本功能7】
 *   支持按日期升序/降序排列进货记录
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "utils.h"
#include "product.h"
#include "purchase.h"

/* ========== 内部辅助函数 ========== */

/** purchase_to_json() — 将进货记录序列化为JSON对象 */
static void purchase_to_json(const Purchase *p, JsonBuilder *jb) {
    json_begin_obj(jb);
    json_add_str(jb, "id", p->id);
    json_add_str(jb, "product_id", p->product_id);
    json_add_str(jb, "name", p->name);
    json_add_str(jb, "category", p->category);
    json_add_str(jb, "date", p->date);
    json_add_int(jb, "quantity", p->quantity);
    json_add_dbl(jb, "price", p->price);
    json_add_dbl(jb, "total", p->total);
    json_end_obj(jb);
}

/**
 * cmp_purchase_date_asc() — qsort比较函数: 按日期升序(早→晚)
 *
 * 日期格式 YYYY-MM-DD 的字典序比较即日期比较
 */
static int cmp_purchase_date_asc(const void *a, const void *b) {
    return strcmp(((Purchase *)a)->date, ((Purchase *)b)->date);
}

/** cmp_purchase_date_desc() — qsort比较函数: 按日期降序(晚→早) */
static int cmp_purchase_date_desc(const void *a, const void *b) {
    return strcmp(((Purchase *)b)->date, ((Purchase *)a)->date);  /* 注意b在前a在后 */
}

/* ========== API处理函数 ========== */

/**
 * handle_get_purchases() — 获取进货记录列表(支持多条件筛选+排序)
 *
 * @query: URL查询字符串
 *   支持参数:
 *     category=笔       — 按商品类别筛选
 *     date_from=2026-05-01  — 起始日期(含)
 *     date_to=2026-05-31    — 结束日期(含)
 *     product_id=P001       — 按商品编号筛选
 *     sort=asc|desc         — 排序方向,默认desc(降序)
 *
 * 【多条件组合查询 — 对应拓展功能2】
 *   所有条件用"与"逻辑: 同时满足才返回
 *   实现逻辑与 handle_get_sales() 完全对称
 */
void handle_get_purchases(const char *query, char *response_body) {
    /* 解析查询参数 */
    char category[STR_LEN] = {0};
    char date_from[STR_LEN] = {0};   /* 起始日期 */
    char date_to[STR_LEN] = {0};     /* 结束日期 */
    char product_id[STR_LEN] = {0};
    char sort[16] = "desc";          /* 默认降序(最新在前) */
    int has_category = 0, has_from = 0, has_to = 0, has_pid = 0;

    if (query && strlen(query) > 0) {
        char qcopy[1024];
        strncpy(qcopy, query, sizeof(qcopy) - 1);
        qcopy[sizeof(qcopy) - 1] = '\0';

        char *tok = strtok(qcopy, "&");
        while (tok) {
            url_decode(tok);
            if (strncmp(tok, "category=", 9) == 0) {
                strncpy(category, tok + 9, STR_LEN - 1); has_category = 1;
            } else if (strncmp(tok, "date_from=", 10) == 0) {
                strncpy(date_from, tok + 10, STR_LEN - 1); has_from = 1;
            } else if (strncmp(tok, "date_to=", 8) == 0) {
                strncpy(date_to, tok + 8, STR_LEN - 1); has_to = 1;
            } else if (strncmp(tok, "product_id=", 11) == 0) {
                strncpy(product_id, tok + 11, STR_LEN - 1); has_pid = 1;
            } else if (strncmp(tok, "sort=", 5) == 0) {
                strncpy(sort, tok + 5, 15);
            }
            tok = strtok(NULL, "&");
        }
    }

    /* 过滤: 多条件"与"逻辑 */
    Purchase filtered[MAX_PURCHASES];
    int fcount = 0;

    for (int i = 0; i < purchase_count; i++) {
        int match = 1;
        if (has_category && strcmp(purchases[i].category, category) != 0) match = 0;
        if (has_from && strcmp(purchases[i].date, date_from) < 0) match = 0;
        if (has_to && strcmp(purchases[i].date, date_to) > 0) match = 0;
        if (has_pid && strcmp(purchases[i].product_id, product_id) != 0) match = 0;
        if (match) filtered[fcount++] = purchases[i];
    }

    /* 排序: 使用标准库 qsort */
    if (strcmp(sort, "asc") == 0)
        qsort(filtered, fcount, sizeof(Purchase), cmp_purchase_date_asc);
    else
        qsort(filtered, fcount, sizeof(Purchase), cmp_purchase_date_desc);

    /* 构建JSON响应 */
    char buf[131072];  /* 128KB */
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);
    json_add_int(&jb, "total", fcount);
    json_begin_array(&jb, "data");

    for (int i = 0; i < fcount; i++) {
        char item_buf[512];
        JsonBuilder item_jb;
        json_init(&item_jb, item_buf, sizeof(item_buf));
        purchase_to_json(&filtered[i], &item_jb);
        if (!jb.first) json_append_raw(&jb, ",");
        json_append_raw(&jb, item_buf);
        jb.first = 0;
    }

    json_end_array(&jb);
    json_end_obj(&jb);

    strncpy(response_body, buf, 8191);
    response_body[8191] = '\0';
}

/** handle_get_purchase() — 获取单条进货记录详情 */
void handle_get_purchase(const char *id, char *response_body) {
    int idx = -1;
    for (int i = 0; i < purchase_count; i++) {
        if (strcmp(purchases[i].id, id) == 0) { idx = i; break; }
    }
    if (idx < 0) {
        sprintf(response_body, "{\"error\":\"未找到进货记录\"}");
        return;
    }
    JsonBuilder jb;
    json_init(&jb, response_body, 8192);
    purchase_to_json(&purchases[idx], &jb);
}

/**
 * handle_create_purchase() — 创建进货记录(进货入库)
 *
 * 【核心业务流程 — 对应拓展功能3: 进货流程管理】
 *
 *   请求体: {"product_id":"P001", "date":"2026-05-27", "quantity":100, "price":1.80}
 *
 *   处理步骤:
 *   ① 解析请求 → 获取商品编号、日期、数量、进价
 *   ② 校验商品是否存在(查products数组)
 *   ③ 校验数量是否大于0
 *   ④ 自动生成进货编号(如B016)
 *   ⑤ 填充进货记录(从商品信息冗余复制名称/类别)
 *   ⑥ 增加商品库存: stock += quantity  ★关键步骤★
 *   ⑦ 保存进货文件 + 商品文件(两份文件都要更新)
 *   ⑧ 返回成功信息(含进货金额和当前库存)
 *
 * 【与销售创建的对比】
 *   销售创建: stock -= quantity (出库扣减)
 *   进货创建: stock += quantity (入库增加)
 *   两者方向相反,但流程结构完全一致
 */
void handle_create_purchase(const char *body, char *response_body) {
    /* 步骤1: 容量检查 */
    if (purchase_count >= MAX_PURCHASES) {
        sprintf(response_body, "{\"error\":\"进货记录数量已达上限\"}");
        return;
    }

    /* 步骤2: 解析并校验商品编号 */
    char pid[STR_LEN];
    if (!json_get_str(body, "product_id", pid, STR_LEN)) {
        sprintf(response_body, "{\"error\":\"缺少商品编号\"}");
        return;
    }

    /* 步骤3: 查找商品是否存在 */
    int pidx = find_product_by_id(pid);
    if (pidx < 0) {
        sprintf(response_body, "{\"error\":\"商品不存在\"}");
        return;
    }

    /* 步骤4: 校验数量 */
    int qty = json_get_int(body, "quantity");
    if (qty <= 0) {
        sprintf(response_body, "{\"error\":\"进货数量必须大于0\"}");
        return;
    }

    /* 步骤5: 填充进货记录 */
    Purchase *p = &purchases[purchase_count];  /* 指向数组末尾的空位 */
    next_purchase_id(p->id);                    /* 自动生成B编号 */
    strncpy(p->product_id, pid, STR_LEN - 1);
    /* 从商品信息冗余复制名称和类别(避免查询时再关联) */
    strncpy(p->name, products[pidx].name, STR_LEN - 1);
    strncpy(p->category, products[pidx].category, STR_LEN - 1);
    json_get_str(body, "date", p->date, STR_LEN);
    p->quantity = qty;
    p->price = json_get_dbl(body, "price");
    /* 如果没传进价或进价为0,使用商品默认单价 */
    if (p->price <= 0) p->price = products[pidx].price;
    p->total = p->quantity * p->price;  /* 进货总额 = 数量 × 进价 */

    /* ★步骤6: 增加库存★ — 与销售创建(stock -= qty)方向相反 */
    products[pidx].stock += qty;

    /* 步骤7: 更新计数并保存两份文件 */
    purchase_count++;
    save_purchases();    /* 保存进货记录 */
    save_products();     /* 保存更新后的商品库存 */

    /* 步骤8: 返回成功信息 */
    JsonBuilder jb;
    json_init(&jb, response_body, 8192);
    json_begin_obj(&jb);
    json_add_str(&jb, "message", "进货记录添加成功");
    json_add_str(&jb, "id", p->id);
    json_add_dbl(&jb, "total", p->total);
    json_add_int(&jb, "current_stock", products[pidx].stock);  /* 告知前端当前库存 */
    json_end_obj(&jb);
}

/**
 * handle_update_purchase() — 修改进货记录
 *
 * 【库存联动逻辑 — 与handle_update_sale()对称】
 *   修改数量时,需要:
 *   1. 撤销旧增量: stock -= old_quantity (减掉之前加的)
 *   2. 应用新增量: stock += new_quantity (加上新的)
 *
 *   注意: 进货修改时不需要检查库存是否充足
 *   (因为进货是增加库存,不会导致负数 — 除非旧增量撤回后库存不够)
 *
 * 答辩要点: 为什么修改进货不需要检查库存?
 *   → 进货增加库存,修改后的数量更大只会增加更多
 *   → 但如果先撤回旧增量可能导致临时负数(本代码未处理此极端情况)
 */
void handle_update_purchase(const char *id, const char *body, char *response_body) {
    int idx = -1;
    for (int i = 0; i < purchase_count; i++) {
        if (strcmp(purchases[i].id, id) == 0) { idx = i; break; }
    }
    if (idx < 0) {
        sprintf(response_body, "{\"error\":\"未找到进货记录\"}");
        return;
    }

    Purchase *p = &purchases[idx];
    char tmp[STR_LEN];

    /* 更新日期(如果有) */
    if (json_get_str(body, "date", tmp, STR_LEN))
        strncpy(p->date, tmp, STR_LEN - 1);

    /* 更新数量(涉及库存联动) */
    int qty = json_get_int(body, "quantity");
    if (qty > 0) {
        int pidx = find_product_by_id(p->product_id);
        if (pidx >= 0) {
            /* 撤销旧增量: stock -= old_quantity */
            products[pidx].stock -= p->quantity;
            /* 应用新增量: stock += new_quantity */
            products[pidx].stock += qty;
            save_products();
        }
        p->quantity = qty;
    }

    /* 更新价格和总金额 */
    double price = json_get_dbl(body, "price");
    if (price > 0) p->price = price;
    p->total = p->quantity * p->price;

    save_purchases();
    sprintf(response_body, "{\"message\":\"进货记录更新成功\"}");
}

/**
 * handle_delete_purchase() — 删除进货记录(退购处理)
 *
 * 【库存扣回逻辑 — 与handle_delete_sale()对称】
 *   删除进货 = 退购,需要把增加的库存扣回去
 *   stock -= purchase.quantity
 *
 *   与删除销售的区别:
 *   删除销售: stock += quantity (恢复库存)
 *   删除进货: stock -= quantity (扣回库存)
 *
 *   两者方向完全相反
 */
void handle_delete_purchase(const char *id, char *response_body) {
    int idx = -1;
    for (int i = 0; i < purchase_count; i++) {
        if (strcmp(purchases[i].id, id) == 0) { idx = i; break; }
    }
    if (idx < 0) {
        sprintf(response_body, "{\"error\":\"未找到进货记录\"}");
        return;
    }

    /* ★扣回库存★: 把之前增加的数量减回去 */
    int pidx = find_product_by_id(purchases[idx].product_id);
    if (pidx >= 0) {
        products[pidx].stock -= purchases[idx].quantity;
        /* 防止库存变为负数(极端情况: 进货量大于当前库存) */
        if (products[pidx].stock < 0) products[pidx].stock = 0;
        save_products();
    }

    /* 数组前移,删除该记录 */
    for (int i = idx; i < purchase_count - 1; i++) {
        purchases[i] = purchases[i + 1];
    }
    purchase_count--;
    save_purchases();

    sprintf(response_body, "{\"message\":\"进货记录删除成功\"}");
}
