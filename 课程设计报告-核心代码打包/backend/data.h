/**
 * ============================================================
 *  data.h — 数据层核心头文件
 *  作用: 定义整个系统的数据结构和数据操作接口
 * ============================================================
 *
 *  【设计理念】
 *   系统采用"结构体数组 + 文本文件"的方式实现数据持久化:
 *   - 三个全局数组: products[]、sales[]、purchases[] 分别存放三类数据
 *   - 数据文件使用管道符(|)分隔的文本格式,方便人类阅读和调试
 *   - 每次增删改操作后立即写回文件,保证数据不丢失
 *
 *  【文件格式示例 — products.txt】
 *   id|name|category|manufacturer|model|stock|price
 *   P001|晨光中性笔|笔|晨光文具|K35|150|2.50
 *   P007|晨光A5笔记本|本|晨光文具|B5120|200|5.00
 *
 *  【文件格式示例 — sales.txt】
 *   id|product_id|name|category|date|quantity|price|total
 *   S001|P001|晨光中性笔|笔|2026-05-27|10|3.00|30.00
 */

#ifndef DATA_H
#define DATA_H

/* ========== 常量定义 ========== */

#define MAX_PRODUCTS  200   /* 商品数组最大容量,防止内存溢出 */
#define MAX_SALES     500   /* 销售记录最大容量 */
#define MAX_PURCHASES 200   /* 进货记录最大容量 */
#define STR_LEN       64    /* 通用字符串字段最大长度(含末尾'\0') */

/* ========== 数据结构定义 ========== */

/**
 * 【商品库存信息】对应需求中的"文具商品库存信息管理"
 * 字段说明:
 *   id           — 商品编号,格式P001~P999,自动递增生成
 *   name         — 商品名称,如"晨光中性笔"
 *   category     — 商品类别: 笔、本、尺、橡皮、修正、胶水、胶带、工具
 *   manufacturer — 生产厂家,如"晨光文具"
 *   model        — 型号规格,如"K35"
 *   stock        — 当前库存数量,进货时增加,销售时扣减
 *   price        — 商品单价(元),作为默认售价参考
 */
typedef struct {
    char id[STR_LEN];           /* 商品编号 P001 */
    char name[STR_LEN];         /* 商品名称 */
    char category[STR_LEN];     /* 类别 */
    char manufacturer[STR_LEN]; /* 厂家 */
    char model[STR_LEN];        /* 型号 */
    int  stock;                 /* 库存数量 */
    double price;               /* 单价 */
} Product;

/**
 * 【销售交易记录】对应需求中的"文具商品交易信息管理"
 * 字段说明:
 *   id         — 交易编号,格式S001~S999,自动递增
 *   product_id — 关联的商品编号,用于库存联动
 *   name       — 商品名称(冗余存储,避免查询时再关联)
 *   category   — 商品类别(冗余存储,便于筛选)
 *   date       — 交易日期,格式YYYY-MM-DD
 *   quantity   — 本次交易数量
 *   price      — 实际售价(可能与商品单价不同,支持议价)
 *   total      — 交易总金额 = quantity × price
 */
typedef struct {
    char id[STR_LEN];           /* 交易编号 S001 */
    char product_id[STR_LEN];   /* 商品编号 */
    char name[STR_LEN];         /* 商品名称 */
    char category[STR_LEN];     /* 类别 */
    char date[STR_LEN];         /* 交易日期 YYYY-MM-DD */
    int  quantity;              /* 数量 */
    double price;               /* 售价 */
    double total;               /* 总金额 */
} Sale;

/**
 * 【进货记录】对应需求中的"文具商品进货信息管理"
 * 字段说明:
 *   id         — 进货编号,格式B001~B999,自动递增
 *   product_id — 关联的商品编号
 *   name       — 商品名称(冗余存储)
 *   category   — 商品类别(冗余存储)
 *   date       — 进货日期,格式YYYY-MM-DD
 *   quantity   — 本次进货数量
 *   price      — 进货单价(采购成本价)
 *   total      — 进货总金额 = quantity × price
 */
typedef struct {
    char id[STR_LEN];           /* 进货编号 B001 */
    char product_id[STR_LEN];   /* 商品编号 */
    char name[STR_LEN];         /* 商品名称 */
    char category[STR_LEN];     /* 类别 */
    char date[STR_LEN];         /* 进货日期 YYYY-MM-DD */
    int  quantity;              /* 数量 */
    double price;               /* 进价 */
    double total;               /* 总金额 */
} Purchase;

/* ========== 全局数据存储 ========== */
/* 使用 extern 声明,实际定义在 data.c 中 */
/* 其他模块通过 #include "data.h" 即可访问这些数据 */

extern Product  products[MAX_PRODUCTS];     /* 商品数组 */
extern int      product_count;              /* 当前商品数量 */

extern Sale     sales[MAX_SALES];           /* 销售记录数组 */
extern int      sale_count;                 /* 当前销售记录数 */

extern Purchase purchases[MAX_PURCHASES];   /* 进货记录数组 */
extern int      purchase_count;             /* 当前进货记录数 */

/* ========== 文件读写函数声明 ========== */

/**
 * load_products() — 从 data/products.txt 加载商品数据到内存数组
 * save_products() — 将内存数组中的商品数据写回 data/products.txt
 *
 * 加载流程: 打开文件 → 跳过表头 → 逐行解析 → 填入数组
 * 保存流程: 打开文件 → 写入表头 → 逐行写出数组内容
 */
void load_products(void);
void save_products(void);

/** 同上,销售记录的加载与保存 */
void load_sales(void);
void save_sales(void);

/** 同上,进货记录的加载与保存 */
void load_purchases(void);
void save_purchases(void);

/* ========== ID生成函数声明 ========== */

/**
 * next_product_id() — 生成下一个商品编号
 * 算法: 遍历所有商品找到最大编号,加1后格式化为P001格式
 * 输出示例: 如果已有P001~P025,则生成P026
 */
void next_product_id(char *buf);

/** next_sale_id() — 生成下一个销售编号,格式S001 */
void next_sale_id(char *buf);

/** next_purchase_id() — 生成下一个进货编号,格式B001 */
void next_purchase_id(char *buf);

/* ========== 查询函数声明 ========== */

/**
 * find_product_by_id() — 按商品编号查找商品
 * @id: 商品编号字符串,如"P001"
 * 返回值: 商品在数组中的下标(0开始),未找到返回-1
 */
int find_product_by_id(const char *id);

#endif /* DATA_H */
