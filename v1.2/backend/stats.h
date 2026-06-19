/**
 * ============================================================
 *  stats.h — 统计分析模块头文件
 *  作用: 声明各类统计分析功能的API接口
 * ============================================================
 *
 *  【对应需求 — 拓展功能】
 *   拓展功能5: 指定商品专项统计(如近一周橡皮类销售总额+库存)
 *   拓展功能6: 通用统计分析(销售排行、库存预警等)
 *
 *  【统计功能一览】
 *   仪表盘统计   — 今日/本周/本月销售、库存总价值等核心指标
 *   销售排行榜   — 按商品汇总销售金额,降序排列
 *   库存预警     — 筛选库存低于阈值(20件)的商品
 *   周类别统计   — 按类别统计近7天销售,含每日明细
 *   库存总报告   — 每个商品的库存、已售、已购全景视图
 */

#ifndef STATS_H
#define STATS_H

/**
 * handle_dashboard_stats() — 仪表盘综合统计
 *
 * 返回数据示例:
 *   {
 *     "product_count": 25,      商品种类数
 *     "total_stock": 2150,      总库存
 *     "inventory_value": 8500,  库存总价值
 *     "today_sales": 78.50,     今日销售额
 *     "week_sales": 523.00,     本周销售额
 *     "month_sales": 3200.00,   本月销售额
 *     "total_sales": 15000.00,  历史总销售额
 *     "low_stock_count": 3      库存预警数量
 *   }
 */
void handle_dashboard_stats(char *response_body);

/**
 * handle_sales_ranking() — 销售排行榜
 * @query: 支持 date_from/date_to 日期范围筛选
 * 算法: 按商品分组汇总销售金额 → 降序排列 → 返回前20名
 */
void handle_sales_ranking(const char *query, char *response_body);

/** 库存预警: 筛选 stock < 20 的商品 */
void handle_low_stock_alerts(char *response_body);

/**
 * handle_weekly_category_sales() — 近一周类别销售统计
 * @query: 支持 category 类别筛选(如"橡皮")
 * 返回: 按类别汇总近7天销售额,含每日明细
 */
void handle_weekly_category_sales(const char *query, char *response_body);

/**
 * handle_inventory_report() — 库存全景报告
 * 返回: 每个商品的当前库存、库存价值、历史销售量/额、历史进货量/额
 */
void handle_inventory_report(char *response_body);

#endif /* STATS_H */
