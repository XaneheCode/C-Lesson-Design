/**
 * ============================================================
 *  stats.c — 统计分析模块实现
 *  作用: 仪表盘统计、销售排行、库存预警、周类别统计、库存报告
 * ============================================================
 *
 *  【对应需求】
 *   拓展功能5: 指定商品专项统计(近一周某类别销售总额+库存)
 *   拓展功能6: 通用统计分析(销售排行、库存预警等)
 *
 *  【统计方法论】
 *   所有统计都是"实时计算": 遍历内存数组,按需聚合
 *   不维护单独的统计表,因为数据量小(<200条),实时计算足够快
 *
 *  【时间处理】
 *   使用C标准库 <time.h> 的 time() + localtime() 获取当前日期
 *   日期格式 YYYY-MM-DD 的字符串比较即日期比较
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "data.h"
#include "utils.h"
#include "product.h"
#include "stats.h"

/**
 * handle_dashboard_stats() — 仪表盘综合统计
 *
 * 【统计内容一览】
 *   商品相关: 种类数、总库存、库存总价值、库存预警数
 *   销售相关: 历史总销售额、今日/本周/本月销售额
 *   进货相关: 历史总进货额
 *   利润:    总销售额 - 总进货额 (简化计算)
 *
 * 【时间窗口计算】
 *   今日: 日期字符串 == today
 *   本周: 日期 >= 本周一 且 日期 <= today
 *   本月: 日期前缀 == "YYYY-MM"
 *
 * 答辩要点: 利润为什么不精确?
 *   → 简化模型: 利润 = 销售额 - 进货额
 *   → 实际还应考虑期初库存、损耗等,但课程设计简化处理
 */
void handle_dashboard_stats(char *response_body) {
    double total_sales = 0, total_purchases = 0, total_profit = 0;
    int total_sale_qty = 0;

    /* 统计历史总销售额和总销售量 */
    for (int i = 0; i < sale_count; i++) {
        total_sales += sales[i].total;
        total_sale_qty += sales[i].quantity;
    }
    /* 统计历史总进货额 */
    for (int i = 0; i < purchase_count; i++) {
        total_purchases += purchases[i].total;
    }
    /* 简化利润 = 销售额 - 进货额 */
    total_profit = total_sales - total_purchases;

    /* ===== 获取今日日期 ===== */
    time_t now = time(NULL);           /* 获取当前时间戳(秒数) */
    struct tm *t = localtime(&now);    /* 转换为本地时间结构体 */
    char today[16];
    sprintf(today, "%04d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);

    /* 今日销售额: 遍历销售记录,日期等于今天的累加 */
    double today_sales = 0;
    int today_qty = 0;
    for (int i = 0; i < sale_count; i++) {
        if (strcmp(sales[i].date, today) == 0) {
            today_sales += sales[i].total;
            today_qty += sales[i].quantity;
        }
    }

    /* ===== 本周销售额 ===== */
    /* tm_wday: 0=周日,1=周一,...,6=周六 */
    int wday = t->tm_wday;
    if (wday == 0) wday = 7;  /* 将周日从0转换为7,方便计算偏移量 */

    /* 计算本周一的日期 */
    struct tm week_start = *t;
    week_start.tm_mday -= (wday - 1);  /* 回退到周一: 偏移 = wday-1 天 */
    mktime(&week_start);               /* mktime会自动规范化日期溢出 */
    char week_start_str[16];
    sprintf(week_start_str, "%04d-%02d-%02d",
        week_start.tm_year + 1900, week_start.tm_mon + 1, week_start.tm_mday);

    /* 遍历销售记录,日期在[本周一,今天]范围内则累加 */
    double week_sales = 0;
    for (int i = 0; i < sale_count; i++) {
        if (strcmp(sales[i].date, week_start_str) >= 0 &&
            strcmp(sales[i].date, today) <= 0) {
            week_sales += sales[i].total;
        }
    }

    /* ===== 本月销售额 ===== */
    /* 用"YYYY-MM"前缀匹配本月所有记录 */
    char month_prefix[8];
    sprintf(month_prefix, "%04d-%02d", t->tm_year + 1900, t->tm_mon + 1);
    double month_sales = 0;
    for (int i = 0; i < sale_count; i++) {
        if (strncmp(sales[i].date, month_prefix, 7) == 0) {
            month_sales += sales[i].total;
        }
    }

    /* ===== 库存统计 ===== */
    int total_stock = 0;          /* 总库存量 */
    double inventory_value = 0;   /* 库存总价值 */
    int low_stock_count = 0;      /* 库存预警数 */
    for (int i = 0; i < product_count; i++) {
        total_stock += products[i].stock;
        inventory_value += products[i].stock * products[i].price;  /* 库存×单价=价值 */
        if (products[i].stock < 20) low_stock_count++;             /* 低于20件为预警 */
    }

    /* 构建JSON响应 */
    char buf[4096];
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);
    json_add_int(&jb, "product_count", product_count);     /* 商品种类数 */
    json_add_int(&jb, "total_stock", total_stock);         /* 总库存 */
    json_add_dbl(&jb, "inventory_value", inventory_value); /* 库存总价值 */
    json_add_int(&jb, "low_stock_count", low_stock_count); /* 库存预警数 */
    json_add_int(&jb, "sale_count", sale_count);           /* 销售记录数 */
    json_add_int(&jb, "purchase_count", purchase_count);   /* 进货记录数 */
    json_add_dbl(&jb, "total_sales", total_sales);         /* 历史总销售额 */
    json_add_dbl(&jb, "total_purchases", total_purchases); /* 历史总进货额 */
    json_add_dbl(&jb, "total_profit", total_profit);       /* 简化利润 */
    json_add_int(&jb, "total_sale_qty", total_sale_qty);   /* 历史总销售量 */
    json_add_dbl(&jb, "today_sales", today_sales);         /* 今日销售额 */
    json_add_int(&jb, "today_qty", today_qty);             /* 今日销售量 */
    json_add_dbl(&jb, "week_sales", week_sales);           /* 本周销售额 */
    json_add_dbl(&jb, "month_sales", month_sales);         /* 本月销售额 */
    json_end_obj(&jb);

    strncpy(response_body, buf, 4095);
    response_body[4095] = '\0';
}

/**
 * handle_sales_ranking() — 销售排行榜
 *
 * @query: 支持 date_from/date_to 日期范围筛选
 *
 * 【算法 — 按商品分组汇总 → 冒泡排序降序 → 取前20名】
 *   1. 遍历销售记录,按product_id分组,累加销售量和金额
 *   2. 用"手动分组"(线性查找已有分组)实现,因为C没有map
 *   3. 对分组结果按金额降序排列(冒泡排序)
 *   4. 取前20名输出
 *
 * 【为什么用冒泡排序而不是qsort?】
 *   → 数据量很小(最多25种商品),冒泡排序足够
 *   → 不需要定义额外的比较函数
 */
void handle_sales_ranking(const char *query, char *response_body) {
    /* 解析日期范围参数 */
    char date_from[STR_LEN] = {0};
    char date_to[STR_LEN] = {0};
    int has_from = 0, has_to = 0;

    if (query && strlen(query) > 0) {
        char qcopy[1024];
        strncpy(qcopy, query, sizeof(qcopy) - 1);
        qcopy[sizeof(qcopy) - 1] = '\0';
        char *tok = strtok(qcopy, "&");
        while (tok) {
            url_decode(tok);
            if (strncmp(tok, "date_from=", 10) == 0) {
                strncpy(date_from, tok + 10, STR_LEN - 1); has_from = 1;
            } else if (strncmp(tok, "date_to=", 8) == 0) {
                strncpy(date_to, tok + 8, STR_LEN - 1); has_to = 1;
            }
            tok = strtok(NULL, "&");
        }
    }

    /* ===== 按商品分组汇总 ===== */
    typedef struct {
        char product_id[STR_LEN];
        char name[STR_LEN];
        char category[STR_LEN];
        int total_qty;       /* 该商品总销售量 */
        double total_amount;  /* 该商品总销售金额 */
    } RankItem;

    RankItem ranks[200];  /* 最多200种商品的排行数据 */
    int rcount = 0;       /* 已分组的商品数 */

    /* 遍历所有销售记录,按商品编号归组 */
    for (int i = 0; i < sale_count; i++) {
        /* 日期范围过滤 */
        if (has_from && strcmp(sales[i].date, date_from) < 0) continue;
        if (has_to && strcmp(sales[i].date, date_to) > 0) continue;

        /* 查找该商品是否已在排行数组中 */
        int found = -1;
        for (int j = 0; j < rcount; j++) {
            if (strcmp(ranks[j].product_id, sales[i].product_id) == 0) {
                found = j;
                break;
            }
        }
        if (found < 0) {
            /* 新商品: 追加到排行数组 */
            strncpy(ranks[rcount].product_id, sales[i].product_id, STR_LEN - 1);
            strncpy(ranks[rcount].name, sales[i].name, STR_LEN - 1);
            strncpy(ranks[rcount].category, sales[i].category, STR_LEN - 1);
            ranks[rcount].total_qty = sales[i].quantity;
            ranks[rcount].total_amount = sales[i].total;
            rcount++;
        } else {
            /* 已有商品: 累加销售量和金额 */
            ranks[found].total_qty += sales[i].quantity;
            ranks[found].total_amount += sales[i].total;
        }
    }

    /* ===== 冒泡排序: 按金额降序 ===== */
    for (int i = 0; i < rcount - 1; i++) {
        for (int j = i + 1; j < rcount; j++) {
            if (ranks[j].total_amount > ranks[i].total_amount) {
                RankItem tmp = ranks[i];
                ranks[i] = ranks[j];
                ranks[j] = tmp;
            }
        }
    }

    /* 构建JSON响应: 取前20名 */
    char buf[16384];
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);
    json_add_int(&jb, "total", rcount);
    json_begin_array(&jb, "ranking");

    for (int i = 0; i < rcount && i < 20; i++) {
        char item[512];
        JsonBuilder ij;
        json_init(&ij, item, sizeof(item));
        json_begin_obj(&ij);
        json_add_int(&ij, "rank", i + 1);                  /* 排名 */
        json_add_str(&ij, "product_id", ranks[i].product_id);
        json_add_str(&ij, "name", ranks[i].name);
        json_add_str(&ij, "category", ranks[i].category);
        json_add_int(&ij, "total_qty", ranks[i].total_qty);       /* 总销售量 */
        json_add_dbl(&ij, "total_amount", ranks[i].total_amount);  /* 总销售额 */
        /* 附加当前库存(方便前端一并展示) */
        int pidx = find_product_by_id(ranks[i].product_id);
        if (pidx >= 0) json_add_int(&ij, "current_stock", products[pidx].stock);
        json_end_obj(&ij);

        if (!jb.first) json_append_raw(&jb, ",");
        json_append_raw(&jb, item);
        jb.first = 0;
    }

    json_end_array(&jb);
    json_end_obj(&jb);

    strncpy(response_body, buf, 8191);
    response_body[8191] = '\0';
}

/**
 * handle_low_stock_alerts() — 库存预警
 *
 * 【预警逻辑】
 *   遍历所有商品,筛选 stock < 20 的商品
 *   20是经验值,可根据实际业务调整
 *
 * 【应用场景】
 *   仪表盘显示预警数量,点击可查看具体哪些商品库存不足
 *   提醒店主及时补货
 */
void handle_low_stock_alerts(char *response_body) {
    char buf[16384];
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);
    json_begin_array(&jb, "alerts");

    int count = 0;
    for (int i = 0; i < product_count; i++) {
        if (products[i].stock < 20) {
            char item[512];
            JsonBuilder ij;
            json_init(&ij, item, sizeof(item));
            json_begin_obj(&ij);
            json_add_str(&ij, "id", products[i].id);
            json_add_str(&ij, "name", products[i].name);
            json_add_str(&ij, "category", products[i].category);
            json_add_int(&ij, "stock", products[i].stock);
            json_add_dbl(&ij, "price", products[i].price);
            json_end_obj(&ij);

            if (!jb.first) json_append_raw(&jb, ",");
            json_append_raw(&jb, item);
            jb.first = 0;
            count++;
        }
    }

    json_end_array(&jb);
    json_add_int(&jb, "total", count);  /* 预警商品总数 */
    json_end_obj(&jb);

    strncpy(response_body, buf, 8191);
    response_body[8191] = '\0';
}

/**
 * handle_weekly_category_sales() — 近一周类别销售统计
 *
 * @query: 支持 category 类别筛选(如"笔"、"橡皮")
 *
 * 【对应拓展功能5: 指定商品专项统计】
 *   可以查询"近一周橡皮类销售总额+当前库存"
 *
 * 【算法 — 7天每日明细】
 *   1. 计算日期范围: [今天-6天, 今天] (共7天)
 *   2. 生成7个日期字符串,用于每日归组
 *   3. 遍历销售记录,日期在范围内且类别匹配的累加
 *   4. 同时按日期归入daily[7]数组
 *   5. 统计该类别当前总库存
 */
void handle_weekly_category_sales(const char *query, char *response_body) {
    /* 解析类别筛选参数 */
    char category[STR_LEN] = "";
    if (query && strlen(query) > 0) {
        char qcopy[1024];
        strncpy(qcopy, query, sizeof(qcopy) - 1);
        char *tok = strtok(qcopy, "&");
        while (tok) {
            url_decode(tok);
            if (strncmp(tok, "category=", 9) == 0) {
                strncpy(category, tok + 9, STR_LEN - 1);
            }
            tok = strtok(NULL, "&");
        }
    }

    /* ===== 计算日期范围: 近7天 ===== */
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char today[16];
    sprintf(today, "%04d-%02d-%02d", t->tm_year + 1900, t->tm_mon + 1, t->tm_mday);

    /* 7天前的日期(含今天,共7天) */
    struct tm week_ago = *t;
    week_ago.tm_mday -= 6;       /* 回退6天 */
    mktime(&week_ago);            /* 规范化日期(自动处理跨月) */
    char week_ago_str[16];
    sprintf(week_ago_str, "%04d-%02d-%02d",
        week_ago.tm_year + 1900, week_ago.tm_mon + 1, week_ago.tm_mday);

    double total_amount = 0;  /* 总销售额 */
    int total_qty = 0;        /* 总销售量 */

    /* ===== 生成7天日期数组,用于每日归组 ===== */
    double daily[7] = {0};     /* 每天的销售额 */
    char dates[7][16];         /* 每天的日期字符串 */
    for (int i = 0; i < 7; i++) {
        struct tm d = week_ago;
        d.tm_mday += i;        /* 从7天前开始,逐天递增 */
        mktime(&d);            /* 规范化日期 */
        sprintf(dates[i], "%04d-%02d-%02d", d.tm_year + 1900, d.tm_mon + 1, d.tm_mday);
    }

    /* ===== 遍历销售记录,累加统计 ===== */
    for (int i = 0; i < sale_count; i++) {
        /* 日期范围过滤 */
        if (strcmp(sales[i].date, week_ago_str) < 0) continue;
        if (strcmp(sales[i].date, today) > 0) continue;
        /* 类别筛选(空类别表示全部) */
        if (strlen(category) > 0 && strcmp(sales[i].category, category) != 0) continue;

        total_amount += sales[i].total;
        total_qty += sales[i].quantity;

        /* 按日期归入对应的daily数组 */
        for (int d = 0; d < 7; d++) {
            if (strcmp(sales[i].date, dates[d]) == 0) {
                daily[d] += sales[i].total;
            }
        }
    }

    /* ===== 统计该类别当前总库存 ===== */
    int cat_stock = 0;
    for (int i = 0; i < product_count; i++) {
        if (strlen(category) == 0 || strcmp(products[i].category, category) == 0) {
            cat_stock += products[i].stock;
        }
    }

    /* 构建JSON响应 */
    char buf[4096];
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);
    json_add_str(&jb, "category", category[0] ? category : "全部");
    json_add_str(&jb, "date_from", week_ago_str);
    json_add_str(&jb, "date_to", today);
    json_add_dbl(&jb, "total_amount", total_amount);
    json_add_int(&jb, "total_qty", total_qty);
    json_add_int(&jb, "current_stock", cat_stock);

    /* 每日明细数组 */
    json_begin_array(&jb, "daily");
    for (int i = 0; i < 7; i++) {
        char item[256];
        JsonBuilder ij;
        json_init(&ij, item, sizeof(item));
        json_begin_obj(&ij);
        json_add_str(&ij, "date", dates[i]);
        json_add_dbl(&ij, "amount", daily[i]);
        json_end_obj(&ij);

        if (!jb.first) json_append_raw(&jb, ",");
        json_append_raw(&jb, item);
        jb.first = 0;
    }
    json_end_array(&jb);
    json_end_obj(&jb);

    strncpy(response_body, buf, 4095);
    response_body[4095] = '\0';
}

/**
 * handle_inventory_report() — 库存全景报告
 *
 * 【报告内容 — 每个商品一行】
 *   基本信息: 编号、名称、类别、单价
 *   库存信息: 当前库存、库存价值(库存×单价)
 *   销售统计: 历史销售量、历史销售额
 *   进货统计: 历史进货量、历史进货额
 *
 * 【算法 — 双重嵌套遍历】
 *   外层: 遍历每个商品
 *   内层: 遍历所有销售/进货记录,按product_id匹配累加
 *
 *   时间复杂度: O(P × (S + B)) — 商品数 × (销售数 + 进货数)
 *   数据量很小时,这种暴力遍历完全可行
 *
 * 【应用场景】
 *   一键查看所有商品的进销存全景,辅助库存管理决策
 */
void handle_inventory_report(char *response_body) {
    char buf[32768];
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);
    json_add_int(&jb, "total_products", product_count);
    json_begin_array(&jb, "products");

    for (int i = 0; i < product_count; i++) {
        /* ===== 统计该商品的销售总量和总额 ===== */
        int sold_qty = 0;
        double sold_amount = 0;
        for (int j = 0; j < sale_count; j++) {
            if (strcmp(sales[j].product_id, products[i].id) == 0) {
                sold_qty += sales[j].quantity;
                sold_amount += sales[j].total;
            }
        }

        /* ===== 统计该商品的进货总量和总额 ===== */
        int bought_qty = 0;
        double bought_cost = 0;
        for (int j = 0; j < purchase_count; j++) {
            if (strcmp(purchases[j].product_id, products[i].id) == 0) {
                bought_qty += purchases[j].quantity;
                bought_cost += purchases[j].total;
            }
        }

        /* 构建单个商品的JSON对象 */
        char item[1024];
        JsonBuilder ij;
        json_init(&ij, item, sizeof(item));
        json_begin_obj(&ij);
        json_add_str(&ij, "id", products[i].id);
        json_add_str(&ij, "name", products[i].name);
        json_add_str(&ij, "category", products[i].category);
        json_add_int(&ij, "stock", products[i].stock);                     /* 当前库存 */
        json_add_dbl(&ij, "price", products[i].price);                    /* 单价 */
        json_add_dbl(&ij, "inventory_value", products[i].stock * products[i].price); /* 库存价值 */
        json_add_int(&ij, "sold_qty", sold_qty);                          /* 历史销售量 */
        json_add_dbl(&ij, "sold_amount", sold_amount);                    /* 历史销售额 */
        json_add_int(&ij, "bought_qty", bought_qty);                      /* 历史进货量 */
        json_add_dbl(&ij, "bought_cost", bought_cost);                    /* 历史进货额 */
        json_end_obj(&ij);

        if (!jb.first) json_append_raw(&jb, ",");
        json_append_raw(&jb, item);
        jb.first = 0;
    }

    json_end_array(&jb);
    json_end_obj(&jb);

    strncpy(response_body, buf, 32767);
    response_body[32767] = '\0';
}
