/**
 * ============================================================
 *  data.c — 数据层核心实现
 *  作用: 数据的加载、保存、ID生成、查询
 * ============================================================
 *
 *  【文件存储格式说明】
 *   本系统采用"管道符分隔文本文件"(类似CSV)存储数据:
 *
 *   products.txt 示例:
 *   ┌────────────────────────────────────────────────────────────┐
 *   │ id|name|category|manufacturer|model|stock|price           │  ← 表头行
 *   │ P001|晨光中性笔|笔|晨光文具|K35|150|2.50                  │  ← 数据行
 *   │ P007|晨光A5笔记本|本|晨光文具|B5120|200|5.00              │
 *   └────────────────────────────────────────────────────────────┘
 *
 *   sales.txt 示例:
 *   ┌──────────────────────────────────────────────────────────────┐
 *   │ id|product_id|name|category|date|quantity|price|total       │
 *   │ S001|P001|晨光中性笔|笔|2026-05-27|10|3.00|30.00           │
 *   └──────────────────────────────────────────────────────────────┘
 *
 *  【为什么选择这种格式?】
 *   1. 纯文本,用记事本就能打开查看和修改
 *   2. 格式简单,C语言 sscanf/strtok 就能解析,不需要JSON库
 *   3. 管道符"|"极少出现在中文商品名中,作为分隔符很安全
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"

/* ========== 全局数据定义 ========== */
/* 这三个数组是系统的核心数据仓库,所有模块共享访问 */

Product  products[MAX_PRODUCTS];    /* 商品数组: 存放所有商品信息 */
int      product_count = 0;         /* 当前商品数量(有效数据条数) */

Sale     sales[MAX_SALES];          /* 销售记录数组 */
int      sale_count = 0;

Purchase purchases[MAX_PURCHASES];  /* 进货记录数组 */
int      purchase_count = 0;

/* ========== 数据文件路径 ========== */
/* 使用相对路径,要求程序从项目根目录启动 */

static const char *PRODUCTS_FILE  = "data/products.txt";   /* 商品数据文件 */
static const char *SALES_FILE     = "data/sales.txt";      /* 销售记录文件 */
static const char *PURCHASES_FILE = "data/purchases.txt";  /* 进货记录文件 */

/* ========== 内部辅助函数(仅本文件使用) ========== */

/**
 * parse_product() — 解析一行文本,填充Product结构体
 *
 * @line: 一行文本,如 "P001|晨光中性笔|笔|晨光文具|K35|150|2.50"
 * @p:    输出参数,解析结果写入此结构体
 * 返回:   1=解析成功, 0=格式错误(字段不足)
 *
 * 【解析原理 — strtok逐字段切割】
 *   strtok(line, "|") 用"|"作为分隔符,将字符串切分为多个token
 *   第一次调用传入字符串,后续调用传入NULL继续切分
 *   每个token依次对应结构体的一个字段
 */
static int parse_product(const char *line, Product *p) {
    /* 复制输入行,strtok会修改原字符串,所以不能直接用const参数 */
    char tmp[512];
    strncpy(tmp, line, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    /* 第1个字段: 商品编号 (如 P001) */
    char *tok = strtok(tmp, "|");
    if (!tok) return 0;  /* 字段不足,格式错误 */
    strncpy(p->id, tok, STR_LEN - 1);
    p->id[STR_LEN - 1] = '\0';  /* 手动补'\0',防止strncpy不终止 */

    /* 第2个字段: 商品名称 (如 晨光中性笔) */
    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(p->name, tok, STR_LEN - 1);
    p->name[STR_LEN - 1] = '\0';

    /* 第3个字段: 类别 (如 笔) */
    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(p->category, tok, STR_LEN - 1);
    p->category[STR_LEN - 1] = '\0';

    /* 第4个字段: 厂家 (如 晨光文具) */
    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(p->manufacturer, tok, STR_LEN - 1);
    p->manufacturer[STR_LEN - 1] = '\0';

    /* 第5个字段: 型号 (如 K35) */
    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(p->model, tok, STR_LEN - 1);
    p->model[STR_LEN - 1] = '\0';

    /* 第6个字段: 库存数量 — atoi将字符串转整数 */
    tok = strtok(NULL, "|");
    if (!tok) return 0;
    p->stock = atoi(tok);

    /* 第7个字段: 单价 — atof将字符串转浮点数 */
    /* 注意: 最后一个字段的分隔符可能是 |、\n 或 \r */
    tok = strtok(NULL, "|\n\r");
    if (!tok) return 0;
    p->price = atof(tok);

    return 1; /* 解析成功 */
}

/**
 * parse_sale() — 解析一行销售记录文本
 * 格式: id|product_id|name|category|date|quantity|price|total (共8个字段)
 */
static int parse_sale(const char *line, Sale *s) {
    char tmp[512];
    strncpy(tmp, line, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    /* 逐字段解析,每个字段用strtok切分 */
    char *tok = strtok(tmp, "|");
    if (!tok) return 0;
    strncpy(s->id, tok, STR_LEN - 1); s->id[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(s->product_id, tok, STR_LEN - 1); s->product_id[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(s->name, tok, STR_LEN - 1); s->name[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(s->category, tok, STR_LEN - 1); s->category[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(s->date, tok, STR_LEN - 1); s->date[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    s->quantity = atoi(tok);

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    s->price = atof(tok);

    tok = strtok(NULL, "|\n\r");
    if (!tok) return 0;
    s->total = atof(tok);

    return 1;
}

/**
 * parse_purchase() — 解析一行进货记录文本
 * 格式与销售记录相同: id|product_id|name|category|date|quantity|price|total
 */
static int parse_purchase(const char *line, Purchase *p) {
    char tmp[512];
    strncpy(tmp, line, sizeof(tmp) - 1);
    tmp[sizeof(tmp) - 1] = '\0';

    char *tok = strtok(tmp, "|");
    if (!tok) return 0;
    strncpy(p->id, tok, STR_LEN - 1); p->id[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(p->product_id, tok, STR_LEN - 1); p->product_id[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(p->name, tok, STR_LEN - 1); p->name[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(p->category, tok, STR_LEN - 1); p->category[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    strncpy(p->date, tok, STR_LEN - 1); p->date[STR_LEN - 1] = '\0';

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    p->quantity = atoi(tok);

    tok = strtok(NULL, "|");
    if (!tok) return 0;
    p->price = atof(tok);

    tok = strtok(NULL, "|\n\r");
    if (!tok) return 0;
    p->total = atof(tok);

    return 1;
}

/* ========== 公开的数据加载/保存函数 ========== */

/**
 * load_products() — 从文件加载商品数据到内存
 *
 * 加载流程:
 *   1. 打开 data/products.txt
 *   2. 读取并跳过第一行(表头)
 *   3. 逐行读取,每行调用 parse_product() 解析
 *   4. 解析成功则 product_count++
 *   5. 文件不存在则 product_count = 0 (空数据)
 *
 * 答辩要点: 为什么跳过第一行?
 *   → 第一行是字段名(id|name|category...),不是数据
 */
void load_products(void) {
    FILE *fp = fopen(PRODUCTS_FILE, "r");  /* 以只读模式打开文件 */
    if (!fp) {
        /* 文件不存在(首次运行),初始化为空 */
        product_count = 0;
        return;
    }

    char line[512];  /* 每行最大512字节,足够存放一条记录 */
    product_count = 0;

    /* 跳过第一行表头: "id|name|category|manufacturer|model|stock|price" */
    if (fgets(line, sizeof(line), fp) == NULL) { fclose(fp); return; }

    /* 循环读取每一行数据 */
    while (fgets(line, sizeof(line), fp) && product_count < MAX_PRODUCTS) {
        /* 跳过空行或过短的行(可能是文件末尾的空行) */
        if (strlen(line) < 5) continue;

        /* 解析该行,成功则计数+1 */
        if (parse_product(line, &products[product_count])) {
            product_count++;
        }
    }
    fclose(fp);  /* 关闭文件,释放资源 */
}

/**
 * save_products() — 将内存中的商品数据保存到文件
 *
 * 保存流程:
 *   1. 以写模式打开文件(会清空原内容)
 *   2. 写入表头行
 *   3. 遍历数组,逐行写出每个商品
 *
 * 为什么用"w"模式而非"r+"?
 *   → 简单直接:先清空再写入,保证数据一致性
 *   → 数据量小(<200条),性能影响可忽略
 */
void save_products(void) {
    FILE *fp = fopen(PRODUCTS_FILE, "w");
    if (!fp) return;  /* 打开失败(如目录不存在),静默返回 */

    /* 先写表头行 */
    fprintf(fp, "id|name|category|manufacturer|model|stock|price\n");

    /* 遍历数组,逐行写出 */
    for (int i = 0; i < product_count; i++) {
        fprintf(fp, "%s|%s|%s|%s|%s|%d|%.2f\n",
            products[i].id,
            products[i].name,
            products[i].category,
            products[i].manufacturer,
            products[i].model,
            products[i].stock,
            products[i].price);       /* %.2f 保留2位小数 */
    }
    fclose(fp);
}

/** load_sales() — 加载销售记录,流程与 load_products() 相同 */
void load_sales(void) {
    FILE *fp = fopen(SALES_FILE, "r");
    if (!fp) { sale_count = 0; return; }

    char line[512];
    sale_count = 0;
    /* 跳过表头: "id|product_id|name|category|date|quantity|price|total" */
    if (fgets(line, sizeof(line), fp) == NULL) { fclose(fp); return; }

    while (fgets(line, sizeof(line), fp) && sale_count < MAX_SALES) {
        if (strlen(line) < 5) continue;
        if (parse_sale(line, &sales[sale_count])) {
            sale_count++;
        }
    }
    fclose(fp);
}

/** save_sales() — 保存销售记录 */
void save_sales(void) {
    FILE *fp = fopen(SALES_FILE, "w");
    if (!fp) return;

    fprintf(fp, "id|product_id|name|category|date|quantity|price|total\n");
    for (int i = 0; i < sale_count; i++) {
        fprintf(fp, "%s|%s|%s|%s|%s|%d|%.2f|%.2f\n",
            sales[i].id, sales[i].product_id, sales[i].name,
            sales[i].category, sales[i].date,
            sales[i].quantity, sales[i].price, sales[i].total);
    }
    fclose(fp);
}

/** load_purchases() — 加载进货记录 */
void load_purchases(void) {
    FILE *fp = fopen(PURCHASES_FILE, "r");
    if (!fp) { purchase_count = 0; return; }

    char line[512];
    purchase_count = 0;
    if (fgets(line, sizeof(line), fp) == NULL) { fclose(fp); return; }

    while (fgets(line, sizeof(line), fp) && purchase_count < MAX_PURCHASES) {
        if (strlen(line) < 5) continue;
        if (parse_purchase(line, &purchases[purchase_count])) {
            purchase_count++;
        }
    }
    fclose(fp);
}

/** save_purchases() — 保存进货记录 */
void save_purchases(void) {
    FILE *fp = fopen(PURCHASES_FILE, "w");
    if (!fp) return;

    fprintf(fp, "id|product_id|name|category|date|quantity|price|total\n");
    for (int i = 0; i < purchase_count; i++) {
        fprintf(fp, "%s|%s|%s|%s|%s|%d|%.2f|%.2f\n",
            purchases[i].id, purchases[i].product_id, purchases[i].name,
            purchases[i].category, purchases[i].date,
            purchases[i].quantity, purchases[i].price, purchases[i].total);
    }
    fclose(fp);
}

/* ========== ID生成函数 ========== */

/**
 * next_product_id() — 自动递增生成下一个商品编号
 *
 * 算法:
 *   1. 遍历所有商品,找到编号中数字部分最大的值
 *   2. 最大值+1,格式化为P%03d(如P026)
 *
 * 示例: 已有P001,P005,P025 → max_num=25 → 生成P026
 *
 * 答辩要点: 为什么不直接用 count+1?
 *   → 因为删除商品后会有间隙(如删了P003,count变24,但P025仍存在)
 *   → 必须找最大值才能保证不重复
 */
void next_product_id(char *buf) {
    int max_num = 0;  /* 记录已找到的最大编号数字 */
    for (int i = 0; i < product_count; i++) {
        /* 只处理以"P"开头的编号 */
        if (strncmp(products[i].id, "P", 1) == 0) {
            /* atoi(products[i].id + 1) 提取"P001"中的"001"→整数1 */
            int num = atoi(products[i].id + 1);
            if (num > max_num) max_num = num;
        }
    }
    /* %03d 表示至少3位,不足前面补0: 1→001, 26→026 */
    sprintf(buf, "P%03d", max_num + 1);
}

/** next_sale_id() — 生成下一个销售编号S001,逻辑同上 */
void next_sale_id(char *buf) {
    int max_num = 0;
    for (int i = 0; i < sale_count; i++) {
        if (strncmp(sales[i].id, "S", 1) == 0) {
            int num = atoi(sales[i].id + 1);
            if (num > max_num) max_num = num;
        }
    }
    sprintf(buf, "S%03d", max_num + 1);
}

/** next_purchase_id() — 生成下一个进货编号B001,逻辑同上 */
void next_purchase_id(char *buf) {
    int max_num = 0;
    for (int i = 0; i < purchase_count; i++) {
        if (strncmp(purchases[i].id, "B", 1) == 0) {
            int num = atoi(purchases[i].id + 1);
            if (num > max_num) max_num = num;
        }
    }
    sprintf(buf, "B%03d", max_num + 1);
}

/* ========== 查询函数 ========== */

/**
 * find_product_by_id() — 按编号查找商品
 *
 * @id: 商品编号,如 "P001"
 * 返回值: 商品在数组中的下标(0,1,2...),未找到返回-1
 *
 * 使用场景: 销售/进货时需要根据商品编号找到对应商品,
 *           以便获取名称、类别、当前库存等信息
 */
int find_product_by_id(const char *id) {
    /* 线性查找: 遍历数组,逐个比较编号 */
    for (int i = 0; i < product_count; i++) {
        if (strcmp(products[i].id, id) == 0) {
            return i;  /* 找到了,返回下标 */
        }
    }
    return -1;  /* 遍历完都没找到 */
}
