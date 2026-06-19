/**
 * ============================================================
 *  sale.c — 销售管理模块实现
 *  作用: 销售记录的增删改查,含库存联动逻辑
 * ============================================================
 *
 *  【核心业务: 库存联动】
 *   创建销售 → 商品库存 stock -= quantity (出库)
 *   修改销售 → 先恢复旧扣减,再应用新扣减
 *   删除销售 → 商品库存 stock += quantity (退货,恢复库存)
 *
 *   这保证了数据一致性: 销售记录和商品库存始终同步
 *
 *  【排序功能 — 对应需求基本功能7】
 *   支持按日期升序/降序排列交易记录
 *   使用标准库 qsort() 配合自定义比较函数实现
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "utils.h"
#include "product.h"
#include "sale.h"

/* ========== 内部辅助函数 ========== */

/** sale_to_json() — 将销售记录序列化为JSON对象 */
static void sale_to_json(const Sale *s, JsonBuilder *jb) {
    json_begin_obj(jb);
    json_add_str(jb, "id", s->id);
    json_add_str(jb, "product_id", s->product_id);
    json_add_str(jb, "name", s->name);
    json_add_str(jb, "category", s->category);
    json_add_str(jb, "date", s->date);
    json_add_int(jb, "quantity", s->quantity);
    json_add_dbl(jb, "price", s->price);
    json_add_dbl(jb, "total", s->total);
    json_end_obj(jb);
}

/**
 * cmp_sale_date_asc() — qsort比较函数: 按日期升序(早→晚)
 *
 * qsort需要的比较函数:
 *   返回<0: a排前面
 *   返回>0: b排前面
 *   返回=0: 顺序不变
 *
 * 日期格式是 YYYY-MM-DD,字典序比较即日期比较(因为年月日从左到右递增)
 */
static int cmp_sale_date_asc(const void *a, const void *b) {
    return strcmp(((Sale *)a)->date, ((Sale *)b)->date);
}

/** cmp_sale_date_desc() — qsort比较函数: 按日期降序(晚→早) */
static int cmp_sale_date_desc(const void *a, const void *b) {
    return strcmp(((Sale *)b)->date, ((Sale *)a)->date);  /* 注意b在前a在后 */
}

/* ========== API处理函数 ========== */

/**
 * handle_get_sales() — 获取销售记录列表(支持多条件筛选+排序)
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
 *   例如 category=笔&date_from=2026-05-20 → 类别为笔 且 日期>=5月20日
 */
void handle_get_sales(const char *query, char *response_body) {
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
    Sale filtered[MAX_SALES];
    int fcount = 0;
    for (int i = 0; i < sale_count; i++) {
        int match = 1;
        if (has_category && strcmp(sales[i].category, category) != 0) match = 0;
        /* 日期比较: YYYY-MM-DD格式的字符串比较等价于日期比较 */
        if (has_from && strcmp(sales[i].date, date_from) < 0) match = 0;  /* 早于起始日期 */
        if (has_to && strcmp(sales[i].date, date_to) > 0) match = 0;      /* 晚于结束日期 */
        if (has_pid && strcmp(sales[i].product_id, product_id) != 0) match = 0;
        if (match) filtered[fcount++] = sales[i];
    }

    /* 排序: 使用标准库 qsort(快速排序) */
    if (strcmp(sort, "asc") == 0)
        qsort(filtered, fcount, sizeof(Sale), cmp_sale_date_asc);
    else
        qsort(filtered, fcount, sizeof(Sale), cmp_sale_date_desc);

    /* 构建JSON响应 */
    char buf[131072];  /* 128KB,销售记录可能较多 */
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);
    json_add_int(&jb, "total", fcount);
    json_begin_array(&jb, "data");
    for (int i = 0; i < fcount; i++) {
        char item_buf[512];
        JsonBuilder item_jb;
        json_init(&item_jb, item_buf, sizeof(item_buf));
        sale_to_json(&filtered[i], &item_jb);
        if (!jb.first) json_append_raw(&jb, ",");
        json_append_raw(&jb, item_buf);
        jb.first = 0;
    }
    json_end_array(&jb);
    json_end_obj(&jb);

    strncpy(response_body, buf, 8191);
    response_body[8191] = '\0';
}

/** handle_get_sale() — 获取单条销售记录详情 */
void handle_get_sale(const char *id, char *response_body) {
    int idx = -1;
    for (int i = 0; i < sale_count; i++) {
        if (strcmp(sales[i].id, id) == 0) { idx = i; break; }
    }
    if (idx < 0) {
        sprintf(response_body, "{\"error\":\"未找到交易记录\"}");
        return;
    }
    JsonBuilder jb;
    json_init(&jb, response_body, 8192);
    sale_to_json(&sales[idx], &jb);
}

/**
 * handle_create_sale() — 创建销售记录(销售出库)
 *
 * 【核心业务流程 — 对应拓展功能3: 销售流程管理】
 *
 *   请求体: {"product_id":"P001", "date":"2026-05-27", "quantity":10, "price":3.00}
 *
 *   处理步骤:
 *   ① 解析请求 → 获取商品编号、日期、数量、售价
 *   ② 校验商品是否存在(查products数组)
 *   ③ 校验库存是否充足(stock >= quantity)
 *   ④ 自动生成销售编号(如S037)
 *   ⑤ 填充销售记录(从商品信息冗余复制名称/类别)
 *   ⑥ 扣减商品库存: stock -= quantity  ★关键步骤★
 *   ⑦ 保存销售文件 + 商品文件(两份文件都要更新)
 *   ⑧ 返回成功信息(含交易金额和剩余库存)
 *
 * 答辩要点: 为什么要同时保存两个文件?
 *   → 销售记录新增了,商品库存也变了
 *   → 如果只保存一个,重启后数据不一致
 */
void handle_create_sale(const char *body, char *response_body) {
    /* 步骤1: 容量检查 */
    if (sale_count >= MAX_SALES) {
        sprintf(response_body, "{\"error\":\"交易记录数量已达上限\"}");
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
        sprintf(response_body, "{\"error\":\"销售数量必须大于0\"}");
        return;
    }

    /* 步骤5: 校验库存是否充足 */
    if (products[pidx].stock < qty) {
        sprintf(response_body, "{\"error\":\"库存不足，当前库存: %d\"}", products[pidx].stock);
        return;
    }

    /* 步骤6: 填充销售记录 */
    Sale *s = &sales[sale_count];  /* 指向数组末尾的空位 */
    next_sale_id(s->id);           /* 自动生成S编号 */
    strncpy(s->product_id, pid, STR_LEN - 1);
    /* 从商品信息冗余复制名称和类别(避免查询时再关联) */
    strncpy(s->name, products[pidx].name, STR_LEN - 1);
    strncpy(s->category, products[pidx].category, STR_LEN - 1);
    json_get_str(body, "date", s->date, STR_LEN);
    s->quantity = qty;
    s->price = json_get_dbl(body, "price");
    /* 如果没传售价或售价为0,使用商品默认单价 */
    if (s->price <= 0) s->price = products[pidx].price;
    s->total = s->quantity * s->price;  /* 总金额 = 数量 × 单价 */

    /* ★步骤7: 扣减库存★ */
    products[pidx].stock -= qty;

    /* 步骤8: 更新计数并保存两份文件 */
    sale_count++;
    save_sales();      /* 保存销售记录 */
    save_products();   /* 保存更新后的商品库存 */

    /* 步骤9: 返回成功信息 */
    JsonBuilder jb;
    json_init(&jb, response_body, 8192);
    json_begin_obj(&jb);
    json_add_str(&jb, "message", "销售记录添加成功");
    json_add_str(&jb, "id", s->id);
    json_add_dbl(&jb, "total", s->total);
    json_add_int(&jb, "remaining_stock", products[pidx].stock);  /* 告知前端剩余库存 */
    json_end_obj(&jb);
}

/**
 * handle_update_sale() — 修改销售记录
 *
 * 【库存联动逻辑】
 *   修改数量时,需要:
 *   1. 恢复旧数量: stock += old_quantity
 *   2. 检查新数量是否超出当前库存
 *   3. 扣减新数量: stock -= new_quantity
 *
 *   如果库存不足,回滚恢复操作,返回错误
 */
void handle_update_sale(const char *id, const char *body, char *response_body) {
    int idx = -1;
    for (int i = 0; i < sale_count; i++) {
        if (strcmp(sales[i].id, id) == 0) { idx = i; break; }
    }
    if (idx < 0) {
        sprintf(response_body, "{\"error\":\"未找到交易记录\"}");
        return;
    }

    Sale *s = &sales[idx];
    char tmp[STR_LEN];

    /* 更新日期(如果有) */
    if (json_get_str(body, "date", tmp, STR_LEN))
        strncpy(s->date, tmp, STR_LEN - 1);

    /* 更新数量(涉及库存联动) */
    int qty = json_get_int(body, "quantity");
    if (qty > 0) {
        int pidx = find_product_by_id(s->product_id);
        if (pidx >= 0) {
            /* 先恢复旧库存(相当于撤销旧销售) */
            products[pidx].stock += s->quantity;
            /* 检查新数量是否可行 */
            if (products[pidx].stock < qty) {
                /* 库存不足: 回滚恢复操作 */
                products[pidx].stock -= s->quantity;
                sprintf(response_body, "{\"error\":\"库存不足\"}");
                return;
            }
            /* 扣减新数量 */
            products[pidx].stock -= qty;
            save_products();
        }
        s->quantity = qty;
    }

    /* 更新价格和总金额 */
    double price = json_get_dbl(body, "price");
    if (price > 0) s->price = price;
    s->total = s->quantity * s->price;

    save_sales();
    sprintf(response_body, "{\"message\":\"交易记录更新成功\"}");
}

/**
 * handle_delete_sale() — 删除销售记录(退货处理)
 *
 * 【库存恢复逻辑】
 *   删除销售 = 退货,需要把扣减的库存加回去
 *   stock += sale.quantity
 */
void handle_delete_sale(const char *id, char *response_body) {
    int idx = -1;
    for (int i = 0; i < sale_count; i++) {
        if (strcmp(sales[i].id, id) == 0) { idx = i; break; }
    }
    if (idx < 0) {
        sprintf(response_body, "{\"error\":\"未找到交易记录\"}");
        return;
    }

    /* ★恢复库存★: 把之前扣减的数量加回去 */
    int pidx = find_product_by_id(sales[idx].product_id);
    if (pidx >= 0) {
        products[pidx].stock += sales[idx].quantity;
        save_products();
    }

    /* 数组前移,删除该记录 */
    for (int i = idx; i < sale_count - 1; i++) {
        sales[i] = sales[i + 1];
    }
    sale_count--;
    save_sales();

    sprintf(response_body, "{\"message\":\"交易记录删除成功\"}");
}
