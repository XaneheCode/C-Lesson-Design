/**
 * ============================================================
 *  product.c — 商品管理模块实现
 *  作用: 商品库存的增删改查(CRUD)和统计
 * ============================================================
 *
 *  【模块职责】
 *   处理所有 /api/products 相关的HTTP请求
 *   每个函数对应一种HTTP操作(GET/POST/PUT/DELETE)
 *
 *  【数据流】
 *   HTTP请求 → server.c路由 → 本模块处理 → 读写products[]数组 → 保存到文件 → 返回JSON
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "data.h"
#include "utils.h"
#include "product.h"

/* ========== JSON解析器实现 ========== */

/**
 * json_get_str() — 从JSON字符串中提取字符串字段值
 *
 * @json:    JSON源字符串,如 {"name":"晨光中性笔","price":2.5}
 * @key:     要提取的字段名,如 "name"
 * @out:     输出缓冲区
 * @out_len: 缓冲区大小
 * 返回:     1=成功提取, 0=字段不存在
 *
 * 【解析原理】
 *   1. 构造搜索模式 "\"key\":\"" (JSON字符串值的标准格式)
 *   2. 用 strstr() 在源字符串中定位该模式
 *   3. 跳过模式后,逐字符复制直到遇到结束引号 "
 *   4. 处理转义字符(\\n → 换行等)
 *
 * 【为什么要支持无引号模式?】
 *   数值类型的值在JSON中可以不加引号: {"stock":150}
 *   第一次搜索带引号的没找到,再搜不带引号的
 */
int json_get_str(const char *json, const char *key, char *out, int out_len) {
    /* 第1步: 尝试匹配带引号的值 "key":"value" */
    char search[128];
    sprintf(search, "\"%s\":\"", key);  /* 如 "name":" */
    const char *p = strstr(json, search);

    if (!p) {
        /* 第2步: 如果没找到,尝试无引号模式 "key": */
        /* 这种情况适用于数值型值: {"stock":150} */
        sprintf(search, "\"%s\":", key);
        p = strstr(json, search);
        if (!p) return 0;  /* 字段不存在 */

        p += strlen(search);  /* 跳过 "key": 定位到值的开始 */
        while (*p == ' ') p++;  /* 跳过可能的空格 */

        /* 提取值直到遇到逗号或右花括号 */
        int i = 0;
        while (*p && *p != ',' && *p != '}' && i < out_len - 1) {
            out[i++] = *p++;
        }
        /* 去除末尾空格 */
        while (i > 0 && out[i-1] == ' ') i--;
        out[i] = '\0';
        return i > 0;  /* 非空字符串表示成功 */
    }

    /* 第3步: 提取带引号的字符串值 */
    p += strlen(search);  /* 跳过 "key":" 定位到值的内容 */
    int i = 0;
    while (*p && *p != '"' && i < out_len - 1) {
        /* 处理转义字符 */
        if (*p == '\\' && p[1]) {
            p++;  /* 跳过反斜杠 */
            switch (*p) {
                case 'n': out[i++] = '\n'; break;  /* \n → 换行 */
                case 't': out[i++] = '\t'; break;  /* \t → 制表 */
                default:  out[i++] = *p; break;     /* 其他: 原样保留 */
            }
            p++;
        } else {
            out[i++] = *p++;  /* 普通字符 */
        }
    }
    out[i] = '\0';
    return 1;
}

/**
 * json_get_int() — 从JSON中提取整数字段
 * 示例: {"stock":150} 中提取 stock → 返回 150
 * 直接跳到冒号后面的数字,用 atoi() 转换
 */
int json_get_int(const char *json, const char *key) {
    char search[128];
    sprintf(search, "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return 0;  /* 字段不存在,返回默认值0 */
    p += strlen(search);
    while (*p == ' ') p++;  /* 跳过空格 */
    return atoi(p);         /* atoi自动遇到非数字字符停止 */
}

/**
 * json_get_dbl() — 从JSON中提取浮点数字段
 * 示例: {"price":2.50} 中提取 price → 返回 2.50
 */
double json_get_dbl(const char *json, const char *key) {
    char search[128];
    sprintf(search, "\"%s\":", key);
    const char *p = strstr(json, search);
    if (!p) return 0.0;
    p += strlen(search);
    while (*p == ' ') p++;
    return atof(p);
}

/* ========== 内部辅助函数 ========== */

/**
 * product_to_json() — 将一个Product结构体序列化为JSON对象
 *
 * @p:  商品数据指针
 * @jb: JSON构建器(输出)
 *
 * 输出示例: {"id":"P001","name":"晨光中性笔","category":"笔",...}
 *
 * 答辩要点: 为什么用json_add_int而不是sprintf直接拼?
 *   → JsonBuilder自动处理字段间逗号,不用手动管理
 *   → 自动处理字符串转义,防止商品名含引号时JSON出错
 */
static void product_to_json(const Product *p, JsonBuilder *jb) {
    json_begin_obj(jb);                    /* { */
    json_add_str(jb, "id", p->id);         /* "id":"P001" */
    json_add_str(jb, "name", p->name);     /* ,"name":"晨光中性笔" */
    json_add_str(jb, "category", p->category);
    json_add_str(jb, "manufacturer", p->manufacturer);
    json_add_str(jb, "model", p->model);
    json_add_int(jb, "stock", p->stock);   /* ,"stock":150 */
    json_add_dbl(jb, "price", p->price);   /* ,"price":2.50 */
    json_end_obj(jb);                      /* } */
}

/* ========== API处理函数 ========== */

/**
 * handle_get_products() — 处理"获取商品列表"请求
 *
 * @query: URL查询字符串,如 "category=笔&keyword=晨光"
 * @response_body: 输出缓冲区,写入JSON响应
 *
 * 【支持的查询参数】
 *   category=笔  — 按类别筛选(精确匹配)
 *   keyword=晨光  — 按关键字搜索(模糊匹配名称/编号/厂家)
 *
 * 【处理流程】
 *   1. 解析query参数 → 2. 逐商品判断是否匹配 → 3. 构建JSON数组响应
 *
 * 【为什么用数组过滤而不是直接修改全局数据?】
 *   → 不修改原始数据,查询是无副作用的(幂等性)
 *   → 同一个数据可以同时被多个筛选条件查询
 */
void handle_get_products(const char *query, char *response_body) {
    /* 解析查询参数 */
    char category[STR_LEN] = {0};
    char keyword[STR_LEN] = {0};
    int has_category = 0;  /* 标记: 是否提供了类别筛选 */
    int has_keyword = 0;   /* 标记: 是否提供了关键字搜索 */

    if (query && strlen(query) > 0) {
        /* 复制query,因为strtok会修改原字符串 */
        char qcopy[1024];
        strncpy(qcopy, query, sizeof(qcopy) - 1);
        qcopy[sizeof(qcopy) - 1] = '\0';

        /* 按 & 分割查询参数: "category=笔&keyword=晨光" → "category=笔", "keyword=晨光" */
        char *tok = strtok(qcopy, "&");
        while (tok) {
            url_decode(tok);  /* URL解码,如 %E7%AC%94 → 笔 */
            if (strncmp(tok, "category=", 9) == 0) {
                strncpy(category, tok + 9, STR_LEN - 1);
                has_category = 1;
            } else if (strncmp(tok, "keyword=", 8) == 0) {
                strncpy(keyword, tok + 8, STR_LEN - 1);
                has_keyword = 1;
            }
            tok = strtok(NULL, "&");  /* 继续下一个参数 */
        }
    }

    /* 过滤商品: 遍历全部商品,只保留满足筛选条件的 */
    Product filtered[MAX_PRODUCTS];  /* 过滤结果临时数组 */
    int fcount = 0;                  /* 过滤结果数量 */

    for (int i = 0; i < product_count; i++) {
        int match = 1;  /* 假设匹配,逐条件检查 */

        /* 条件1: 类别筛选 — 类别必须完全一致 */
        if (has_category && strcmp(products[i].category, category) != 0) {
            match = 0;
        }
        /* 条件2: 关键字搜索 — 名称/编号/厂家任一包含关键字即匹配 */
        if (has_keyword && match) {
            if (strstr(products[i].name, keyword) == NULL &&          /* 名称中找不到 */
                strstr(products[i].id, keyword) == NULL &&            /* 编号中找不到 */
                strstr(products[i].manufacturer, keyword) == NULL) {  /* 厂家中找不到 */
                match = 0;
            }
        }
        if (match) {
            filtered[fcount++] = products[i];  /* 匹配成功,加入结果 */
        }
    }

    /* 构建JSON响应: {"total":5,"data":[{...},{...}]} */
    char buf[65536];  /* 64KB缓冲区,足够存放所有商品的JSON */
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);                    /* { */
    json_add_int(&jb, "total", fcount);     /* "total":5 */
    json_begin_array(&jb, "data");          /* "data":[ */

    /* 遍历过滤结果,逐个序列化为JSON对象 */
    for (int i = 0; i < fcount; i++) {
        char item_buf[512];  /* 单个商品的JSON缓冲 */
        JsonBuilder item_jb;
        json_init(&item_jb, item_buf, sizeof(item_buf));
        product_to_json(&filtered[i], &item_jb);  /* 序列化单个商品 */

        /* 将子对象JSON追加到主JSON数组 */
        if (!jb.first) json_append_raw(&jb, ",");  /* 非第一个元素加逗号 */
        json_append_raw(&jb, item_buf);
        jb.first = 0;
    }

    json_end_array(&jb);  /* ] */
    json_end_obj(&jb);    /* } */

    /* 复制到输出缓冲区 */
    strncpy(response_body, buf, 8191);
    response_body[8191] = '\0';
}

/**
 * handle_get_product() — 处理"获取单个商品"请求
 *
 * @id: 商品编号(从URL路径中提取),如 "P001"
 * @response_body: 输出JSON
 *
 * 返回示例: {"id":"P001","name":"晨光中性笔","category":"笔",...}
 */
void handle_get_product(const char *id, char *response_body) {
    int idx = find_product_by_id(id);  /* 按编号查找 */
    if (idx < 0) {
        /* 未找到,返回错误JSON */
        sprintf(response_body, "{\"error\":\"未找到商品\"}");
        return;
    }

    /* 找到,直接序列化为JSON */
    JsonBuilder jb;
    json_init(&jb, response_body, 8192);
    product_to_json(&products[idx], &jb);
}

/**
 * handle_create_product() — 处理"新增商品"请求
 *
 * @body: HTTP请求体,JSON格式
 *   示例: {"name":"百乐钢笔","category":"笔","manufacturer":"百乐","model":"FP-78G","stock":50,"price":35.00}
 * @response_body: 输出成功/失败信息
 *
 * 【业务流程】
 *   1. 检查数组是否已满
 *   2. 生成新编号(如P026)
 *   3. 从请求体JSON解析各字段值
 *   4. 写入数组末尾
 *   5. 保存到文件
 *   6. 返回成功信息
 */
void handle_create_product(const char *body, char *response_body) {
    /* 步骤1: 容量检查 */
    if (product_count >= MAX_PRODUCTS) {
        sprintf(response_body, "{\"error\":\"商品数量已达上限\"}");
        return;
    }

    /* 步骤2: 准备新商品的位置(数组末尾) */
    Product *p = &products[product_count];

    /* 步骤3: 自动生成编号 */
    next_product_id(p->id);

    /* 步骤4: 从JSON请求体解析各字段 */
    json_get_str(body, "name", p->name, STR_LEN);
    json_get_str(body, "category", p->category, STR_LEN);
    json_get_str(body, "manufacturer", p->manufacturer, STR_LEN);
    json_get_str(body, "model", p->model, STR_LEN);
    p->stock = json_get_int(body, "stock");
    p->price = json_get_dbl(body, "price");

    /* 步骤5: 更新计数并保存 */
    product_count++;
    save_products();  /* 立即写回文件,防止数据丢失 */

    /* 步骤6: 返回成功信息 */
    JsonBuilder jb;
    json_init(&jb, response_body, 8192);
    json_begin_obj(&jb);
    json_add_str(&jb, "message", "商品添加成功");
    json_add_str(&jb, "id", p->id);
    json_end_obj(&jb);
}

/**
 * handle_update_product() — 处理"修改商品"请求
 *
 * @id: 要修改的商品编号
 * @body: JSON,只包含需要修改的字段(部分更新)
 *
 * 【部分更新逻辑】
 *   只修改请求体中出现的字段,未出现的保持原值
 *   例如只传了 {"price":3.00},则只更新价格,其他不变
 */
void handle_update_product(const char *id, const char *body, char *response_body) {
    int idx = find_product_by_id(id);
    if (idx < 0) {
        sprintf(response_body, "{\"error\":\"未找到商品\"}");
        return;
    }

    Product *p = &products[idx];
    char tmp[STR_LEN];

    /* 只更新请求体中包含的字段 */
    if (json_get_str(body, "name", tmp, STR_LEN))
        strncpy(p->name, tmp, STR_LEN - 1);
    if (json_get_str(body, "category", tmp, STR_LEN))
        strncpy(p->category, tmp, STR_LEN - 1);
    if (json_get_str(body, "manufacturer", tmp, STR_LEN))
        strncpy(p->manufacturer, tmp, STR_LEN - 1);
    if (json_get_str(body, "model", tmp, STR_LEN))
        strncpy(p->model, tmp, STR_LEN - 1);

    /* stock和price: JSON中出现了就更新(即使值为0) */
    int stock = json_get_int(body, "stock");
    if (stock > 0 || strstr(body, "stock"))  /* body中有"stock"关键字 */
        p->stock = stock;

    double price = json_get_dbl(body, "price");
    if (price > 0 || strstr(body, "price"))
        p->price = price;

    save_products();  /* 保存修改 */
    sprintf(response_body, "{\"message\":\"商品更新成功\"}");
}

/**
 * handle_delete_product() — 处理"删除商品"请求
 *
 * 【删除算法 — 数组前移覆盖】
 *   要删除下标idx的元素,将idx+1及其后的元素依次前移一位
 *   然后 product_count--
 *
 *   删除前: [P001, P002, P003, P004, P005]  (删除P003, idx=2)
 *   删除后: [P001, P002, P004, P005]         (P004覆盖P003,P005覆盖P004)
 */
void handle_delete_product(const char *id, char *response_body) {
    int idx = find_product_by_id(id);
    if (idx < 0) {
        sprintf(response_body, "{\"error\":\"未找到商品\"}");
        return;
    }

    /* 数组元素前移,覆盖被删除的元素 */
    for (int i = idx; i < product_count - 1; i++) {
        products[i] = products[i + 1];  /* 结构体整体赋值 */
    }
    product_count--;  /* 数量减1 */
    save_products();

    sprintf(response_body, "{\"message\":\"商品删除成功\"}");
}

/**
 * handle_product_stats() — 商品统计概览
 *
 * 统计内容:
 *   - 商品总种类数
 *   - 总库存量
 *   - 库存总价值(按单价计算)
 *   - 库存预警数(stock < 20)
 *   - 各类别商品数和库存量
 *
 * 【类别统计算法 — 手动分组】
 *   因为C语言没有map/dict,我们用两个并行数组模拟:
 *   categories[i] 存类别名, cat_count[i] 存该类别的商品数
 *   遇到新类别时追加到数组末尾(ncat++)
 */
void handle_product_stats(char *response_body) {
    int total = product_count;  /* 商品总种类 */
    int total_stock = 0;        /* 总库存量 */
    double total_value = 0;     /* 库存总价值 */
    int low_stock = 0;          /* 库存预警数 */

    /* 类别统计用的并行数组 */
    char categories[50][STR_LEN];  /* 类别名数组(最多50种) */
    int cat_count[50] = {0};       /* 每种的商品数 */
    int cat_stock[50] = {0};       /* 每种的库存量 */
    int ncat = 0;                  /* 已发现的类别数 */

    /* 遍历所有商品,累加统计 */
    for (int i = 0; i < product_count; i++) {
        total_stock += products[i].stock;
        total_value += products[i].stock * products[i].price;  /* 库存×单价=价值 */
        if (products[i].stock < 20) low_stock++;  /* 库存低于20件为预警 */

        /* 查找该商品的类别是否已在统计数组中 */
        int found = -1;
        for (int j = 0; j < ncat; j++) {
            if (strcmp(categories[j], products[i].category) == 0) {
                found = j;  /* 找到了,记录下标 */
                break;
            }
        }
        if (found < 0) {
            /* 新类别: 追加到数组末尾 */
            strncpy(categories[ncat], products[i].category, STR_LEN - 1);
            cat_count[ncat] = 1;
            cat_stock[ncat] = products[i].stock;
            ncat++;
        } else {
            /* 已有类别: 累加 */
            cat_count[found]++;
            cat_stock[found] += products[i].stock;
        }
    }

    /* 构建JSON响应 */
    char buf[8192];
    JsonBuilder jb;
    json_init(&jb, buf, sizeof(buf));
    json_begin_obj(&jb);
    json_add_int(&jb, "total_products", total);
    json_add_int(&jb, "total_stock", total_stock);
    json_add_dbl(&jb, "total_value", total_value);
    json_add_int(&jb, "low_stock_count", low_stock);

    /* 类别明细数组 */
    json_begin_array(&jb, "categories");
    for (int i = 0; i < ncat; i++) {
        char item[512];
        JsonBuilder ij;
        json_init(&ij, item, sizeof(item));
        json_begin_obj(&ij);
        json_add_str(&ij, "name", categories[i]);
        json_add_int(&ij, "count", cat_count[i]);
        json_add_int(&ij, "stock", cat_stock[i]);
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
