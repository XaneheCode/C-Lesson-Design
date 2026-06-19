#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <windows.h>
#include <winhttp.h>

#include "assistant.h"
#include "data.h"
#include "product.h"
#include "sale.h"
#include "utils.h"

#define ASSISTANT_MODEL "qwen3.6-flash"
#define ASSISTANT_HOST L"dashscope.aliyuncs.com"
#define ASSISTANT_PATH L"/compatible-mode/v1/chat/completions"
#define ASSISTANT_MAX_KEY 256
#define ASSISTANT_MAX_ITEMS 12
#define ASSISTANT_HTTP_LIMIT 1048576
#define ASSISTANT_RESTOCK_GUARD "RESTOCK_IS_NOT_SALE: Restock, purchase, or inbound inventory requests are never sales. Never call draft_sale for these intents. Explain that direct inbound recording is unavailable in the assistant and offer restock_advice or the purchase-entry screen."

typedef struct {
    char product_id[STR_LEN];
    int quantity;
} DraftItem;

typedef struct {
    int active;
    int serial;
    char id[32];
    DraftItem items[ASSISTANT_MAX_ITEMS];
    int count;
} PendingSale;

static char assistant_api_key[ASSISTANT_MAX_KEY] = "";
static PendingSale pending_sale = {0};
static int draft_serial = 0;

static void reply_error(char *response_body, const char *message) {
    char escaped[1024];
    json_escape(message, escaped, sizeof(escaped));
    sprintf(response_body, "{\"error\":\"%s\"}", escaped);
}

static void reply_message(char *response_body, const char *message) {
    char escaped[8192];
    json_escape(message, escaped, sizeof(escaped));
    sprintf(response_body, "{\"type\":\"message\",\"message\":\"%s\"}", escaped);
}

static void utf8_to_wide(const char *src, wchar_t *dst, int dst_len) {
    MultiByteToWideChar(CP_UTF8, 0, src, -1, dst, dst_len);
}

static int append_text(char **buf, int *len, int *cap, const char *text, int amount) {
    if (*len + amount + 1 > *cap) {
        int next = *cap;
        while (*len + amount + 1 > next) next *= 2;
        if (next > ASSISTANT_HTTP_LIMIT) return 0;
        char *grown = (char *)realloc(*buf, next);
        if (!grown) return 0;
        *buf = grown;
        *cap = next;
    }
    memcpy(*buf + *len, text, amount);
    *len += amount;
    (*buf)[*len] = '\0';
    return 1;
}

static int dashscope_chat(const char *payload, char **response_out, char *error, int error_len) {
    HINTERNET session = NULL, connection = NULL, request = NULL;
    wchar_t auth_header[ASSISTANT_MAX_KEY + 32];
    wchar_t key_wide[ASSISTANT_MAX_KEY];
    char *response = NULL;
    int response_len = 0, response_cap = 16384;
    int ok = 0;

    utf8_to_wide(assistant_api_key, key_wide, ASSISTANT_MAX_KEY);
    swprintf(auth_header, ASSISTANT_MAX_KEY + 32, L"Authorization: Bearer %ls", key_wide);

    session = WinHttpOpen(L"Wenfang-BaiZe/1.0", WINHTTP_ACCESS_TYPE_DEFAULT_PROXY,
                          WINHTTP_NO_PROXY_NAME, WINHTTP_NO_PROXY_BYPASS, 0);
    if (!session) goto cleanup;
    WinHttpSetTimeouts(session, 5000, 5000, 12000, 20000);

    connection = WinHttpConnect(session, ASSISTANT_HOST, INTERNET_DEFAULT_HTTPS_PORT, 0);
    if (!connection) goto cleanup;

    request = WinHttpOpenRequest(connection, L"POST", ASSISTANT_PATH, NULL,
                                 WINHTTP_NO_REFERER, WINHTTP_DEFAULT_ACCEPT_TYPES,
                                 WINHTTP_FLAG_SECURE);
    if (!request) goto cleanup;

    if (!WinHttpAddRequestHeaders(request, L"Content-Type: application/json; charset=utf-8",
                                  -1L, WINHTTP_ADDREQ_FLAG_ADD | WINHTTP_ADDREQ_FLAG_REPLACE)) goto cleanup;
    if (!WinHttpAddRequestHeaders(request, auth_header, -1L, WINHTTP_ADDREQ_FLAG_ADD)) goto cleanup;

    if (!WinHttpSendRequest(request, WINHTTP_NO_ADDITIONAL_HEADERS, 0,
                            (LPVOID)payload, (DWORD)strlen(payload),
                            (DWORD)strlen(payload), 0)) goto cleanup;
    if (!WinHttpReceiveResponse(request, NULL)) goto cleanup;

    DWORD status = 0, status_size = sizeof(status);
    WinHttpQueryHeaders(request, WINHTTP_QUERY_STATUS_CODE | WINHTTP_QUERY_FLAG_NUMBER,
                        WINHTTP_HEADER_NAME_BY_INDEX, &status, &status_size, WINHTTP_NO_HEADER_INDEX);

    response = (char *)malloc(response_cap);
    if (!response) goto cleanup;
    response[0] = '\0';

    while (1) {
        DWORD available = 0, read = 0;
        if (!WinHttpQueryDataAvailable(request, &available)) goto cleanup;
        if (available == 0) break;
        char *chunk = (char *)malloc(available);
        if (!chunk) goto cleanup;
        if (!WinHttpReadData(request, chunk, available, &read)) {
            free(chunk);
            goto cleanup;
        }
        if (!append_text(&response, &response_len, &response_cap, chunk, (int)read)) {
            free(chunk);
            goto cleanup;
        }
        free(chunk);
    }

    if (status < 200 || status >= 300) {
        snprintf(error, error_len, "千问服务返回 HTTP %lu，请检查密钥或稍后重试", status);
        goto cleanup;
    }

    *response_out = response;
    response = NULL;
    ok = 1;

cleanup:
    if (!ok && error[0] == '\0') {
        snprintf(error, error_len, "无法连接千问服务，请检查网络后重试");
    }
    if (response) free(response);
    if (request) WinHttpCloseHandle(request);
    if (connection) WinHttpCloseHandle(connection);
    if (session) WinHttpCloseHandle(session);
    SecureZeroMemory(key_wide, sizeof(key_wide));
    SecureZeroMemory(auth_header, sizeof(auth_header));
    return ok;
}

int assistant_has_api_key(void) {
    return assistant_api_key[0] != '\0';
}

void assistant_clear_api_key(void) {
    SecureZeroMemory(assistant_api_key, sizeof(assistant_api_key));
    SecureZeroMemory(&pending_sale, sizeof(pending_sale));
}

void assistant_get_status(char *response_body) {
    sprintf(response_body,
            "{\"configured\":%s,\"model\":\"%s\",\"storage\":\"memory-only\"}",
            assistant_has_api_key() ? "true" : "false", ASSISTANT_MODEL);
}

void assistant_set_api_key(const char *body, char *response_body) {
    char key[ASSISTANT_MAX_KEY];
    if (!json_get_str(body, "api_key", key, sizeof(key)) || strlen(key) < 12) {
        reply_error(response_body, "请输入有效的阿里云百炼 API Key");
        return;
    }
    SecureZeroMemory(assistant_api_key, sizeof(assistant_api_key));
    strncpy(assistant_api_key, key, sizeof(assistant_api_key) - 1);
    assistant_api_key[sizeof(assistant_api_key) - 1] = '\0';
    SecureZeroMemory(key, sizeof(key));
    sprintf(response_body, "{\"configured\":true,\"message\":\"密钥仅保存在当前 C 服务进程内存中\"}");
}

static void build_inventory_snapshot(char *out, int out_len) {
    JsonBuilder jb;
    json_init(&jb, out, out_len);
    json_begin_obj(&jb);
    json_begin_array(&jb, "products");
    for (int i = 0; i < product_count; i++) {
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
    }
    json_end_array(&jb);
    json_end_obj(&jb);
}

static void tool_query_inventory(const char *args, char *response_body) {
    char keyword[STR_LEN] = "", category[STR_LEN] = "";
    int max_stock = json_get_int(args, "max_stock");
    int min_stock = json_get_int(args, "min_stock");
    json_get_str(args, "keyword", keyword, sizeof(keyword));
    json_get_str(args, "category", category, sizeof(category));

    char text[8192] = "库存查询结果：";
    int count = 0;
    for (int i = 0; i < product_count; i++) {
        if (keyword[0] && !strstr(products[i].name, keyword) && !strstr(products[i].id, keyword)) continue;
        if (category[0] && strcmp(products[i].category, category) != 0) continue;
        if (max_stock > 0 && products[i].stock >= max_stock) continue;
        if (min_stock > 0 && products[i].stock < min_stock) continue;
        char line[256];
        snprintf(line, sizeof(line), "\n- %.*s（%.*s）：库存 %d，单价 %.2f 元",
                 STR_LEN - 1, products[i].name, STR_LEN - 1, products[i].id,
                 products[i].stock, products[i].price);
        strncat(text, line, sizeof(text) - strlen(text) - 1);
        count++;
    }
    if (!count) strcat(text, "\n未找到符合条件的商品。");
    reply_message(response_body, text);
}

static void tool_low_stock(const char *args, char *response_body) {
    int threshold = json_get_int(args, "threshold");
    if (threshold <= 0) threshold = 20;
    char text[8192];
    snprintf(text, sizeof(text), "库存低于 %d 的商品：", threshold);
    int count = 0;
    for (int i = 0; i < product_count; i++) {
        if (products[i].stock >= threshold) continue;
        char line[256];
        snprintf(line, sizeof(line), "\n- %.*s（%.*s）：当前 %d 件，建议关注补货",
                 STR_LEN - 1, products[i].name, STR_LEN - 1, products[i].id, products[i].stock);
        strncat(text, line, sizeof(text) - strlen(text) - 1);
        count++;
    }
    if (!count) strcat(text, "\n暂无低库存商品。");
    reply_message(response_body, text);
}

static int total_sold_for_product(const char *product_id) {
    int qty = 0;
    for (int i = 0; i < sale_count; i++) {
        if (strcmp(sales[i].product_id, product_id) == 0) qty += sales[i].quantity;
    }
    return qty;
}

static int assistant_is_recent_sale(const char *date) {
    time_t now = time(NULL);
    time_t week_ago = now - 6 * 24 * 60 * 60;
    struct tm *start = localtime(&week_ago);
    char start_date[16];
    strftime(start_date, sizeof(start_date), "%Y-%m-%d", start);
    return strcmp(date, start_date) >= 0;
}

static void tool_restock_advice(char *response_body) {
    char text[8192] = "补货建议（结合当前库存与历史销量）：";
    int count = 0;
    for (int i = 0; i < product_count; i++) {
        int sold = total_sold_for_product(products[i].id);
        int target = sold > 20 ? sold : 20;
        if (products[i].stock >= target) continue;
        char line[256];
        snprintf(line, sizeof(line), "\n- %s：库存 %d，历史销量 %d，建议补货 %d 件",
                 products[i].name, products[i].stock, sold, target - products[i].stock);
        strncat(text, line, sizeof(text) - strlen(text) - 1);
        count++;
    }
    if (!count) strcat(text, "\n当前库存整体充足，暂无优先补货项。");
    reply_message(response_body, text);
}

static void tool_weekly_analysis(const char *args, char *response_body) {
    char category[STR_LEN] = "";
    json_get_str(args, "category", category, sizeof(category));
    if (!category[0]) strcpy(category, "全部");
    double amount = 0;
    int qty = 0, stock = 0;
    for (int i = 0; i < sale_count; i++) {
        if (!assistant_is_recent_sale(sales[i].date)) continue;
        if (strcmp(category, "全部") == 0 || strcmp(sales[i].category, category) == 0) {
            amount += sales[i].total;
            qty += sales[i].quantity;
        }
    }
    for (int i = 0; i < product_count; i++) {
        if (strcmp(category, "全部") == 0 || strcmp(products[i].category, category) == 0) stock += products[i].stock;
    }
    char text[512];
    snprintf(text, sizeof(text), "%s品类近一周分析：销售数量 %d 件，销售额 %.2f 元，当前库存 %d 件。",
             category, qty, amount, stock);
    reply_message(response_body, text);
}

static void tool_weekly_report(char *response_body) {
    double total = 0;
    int qty = 0, low = 0;
    for (int i = 0; i < sale_count; i++) {
        if (!assistant_is_recent_sale(sales[i].date)) continue;
        total += sales[i].total;
        qty += sales[i].quantity;
    }
    for (int i = 0; i < product_count; i++) if (products[i].stock < 20) low++;
    char text[1024];
    snprintf(text, sizeof(text),
             "经营周报：本期共记录销售 %d 件，累计销售额 %.2f 元。当前在售商品 %d 种，低库存预警 %d 项。建议优先检查低库存商品，并结合畅销品历史销量安排补货。",
             qty, total, product_count, low);
    reply_message(response_body, text);
}

static void send_draft_response(char *response_body) {
    char items[8192] = "";
    double grand_total = 0;
    for (int i = 0; i < pending_sale.count; i++) {
        int idx = find_product_by_id(pending_sale.items[i].product_id);
        if (idx < 0) continue;
        char item[512];
        double total = products[idx].price * pending_sale.items[i].quantity;
        grand_total += total;
        snprintf(item, sizeof(item),
                 "%s{\"product_id\":\"%s\",\"name\":\"%s\",\"quantity\":%d,\"price\":%.2f,\"total\":%.2f,\"stock_before\":%d,\"stock_after\":%d}",
                 i ? "," : "", products[idx].id, products[idx].name,
                 pending_sale.items[i].quantity, products[idx].price, total,
                 products[idx].stock, products[idx].stock - pending_sale.items[i].quantity);
        strncat(items, item, sizeof(items) - strlen(items) - 1);
    }
    snprintf(response_body, 131072,
             "{\"type\":\"sale_draft\",\"message\":\"请确认销售明细，确认后才会写入账册。\",\"draft_id\":\"%s\",\"items\":[%s],\"total\":%.2f}",
             pending_sale.id, items, grand_total);
}

static void replace_utf8_sequence(char *text, const char *needle, char replacement) {
    char *match;
    size_t needle_len = strlen(needle);
    while ((match = strstr(text, needle)) != NULL) {
        *match = replacement;
        memmove(match + 1, match + needle_len, strlen(match + needle_len) + 1);
    }
}

static void normalize_draft_items_text(char *text) {
    replace_utf8_sequence(text, "\xEF\xBC\x9A", ':');
    replace_utf8_sequence(text, "\xEF\xBC\x8C", ',');
    replace_utf8_sequence(text, "\xE3\x80\x81", ',');
}

static int resolve_product_reference(char *reference) {
    char *clean = trim(reference);
    int idx = find_product_by_id(clean);
    if (idx >= 0) return idx;

    int found = -1;
    for (int i = 0; i < product_count; i++) {
        if (strcmp(products[i].name, clean) != 0) continue;
        if (found >= 0) return -1;
        found = i;
    }
    if (found >= 0) return found;

    for (int i = 0; i < product_count; i++) {
        if (!strstr(clean, products[i].id) && !strstr(clean, products[i].name)) continue;
        if (found >= 0 && found != i) return -1;
        found = i;
    }
    return found;
}

static int merge_draft_item(const char *product_reference, int quantity) {
    char reference[STR_LEN];
    snprintf(reference, sizeof(reference), "%s", product_reference);
    int idx = resolve_product_reference(reference);
    if (idx < 0 || quantity <= 0) return 0;
    const char *product_id = products[idx].id;

    for (int i = 0; i < pending_sale.count; i++) {
        if (strcmp(pending_sale.items[i].product_id, product_id) == 0) {
            int merged = pending_sale.items[i].quantity + quantity;
            if (products[idx].stock < merged) return 0;
            pending_sale.items[i].quantity = merged;
            return 1;
        }
    }

    if (pending_sale.count >= ASSISTANT_MAX_ITEMS || products[idx].stock < quantity) return 0;
    strcpy(pending_sale.items[pending_sale.count].product_id, product_id);
    pending_sale.items[pending_sale.count].quantity = quantity;
    pending_sale.count++;
    return 1;
}

static void tool_draft_sale(const char *args, char *response_body) {
    char items_text[1024];
    if (!json_get_str(args, "items_text", items_text, sizeof(items_text))) {
        reply_error(response_body, "销售草稿缺少商品明细");
        return;
    }

    SecureZeroMemory(&pending_sale, sizeof(pending_sale));
    pending_sale.active = 1;
    pending_sale.serial = ++draft_serial;
    snprintf(pending_sale.id, sizeof(pending_sale.id), "D%04d", pending_sale.serial);

    char copy[1024];
    strncpy(copy, items_text, sizeof(copy) - 1);
    copy[sizeof(copy) - 1] = '\0';
    normalize_draft_items_text(copy);
    char *part = strtok(copy, ",");
    while (part) {
        char *colon = strchr(part, ':');
        if (!colon) {
            pending_sale.active = 0;
            reply_error(response_body, "销售草稿格式无效");
            return;
        }
        *colon = '\0';
        int qty = atoi(trim(colon + 1));
        if (!merge_draft_item(trim(part), qty)) {
            pending_sale.active = 0;
            reply_error(response_body, "销售草稿包含无效商品、数量或库存不足");
            return;
        }
        part = strtok(NULL, ",");
    }
    if (!pending_sale.count) {
        pending_sale.active = 0;
        reply_error(response_body, "销售草稿为空");
        return;
    }
    send_draft_response(response_body);
}

static void dispatch_tool(const char *name, const char *args, char *response_body) {
    if (strcmp(name, "query_inventory") == 0) tool_query_inventory(args, response_body);
    else if (strcmp(name, "low_stock_alerts") == 0) tool_low_stock(args, response_body);
    else if (strcmp(name, "draft_sale") == 0) tool_draft_sale(args, response_body);
    else if (strcmp(name, "restock_advice") == 0) tool_restock_advice(response_body);
    else if (strcmp(name, "weekly_sales_analysis") == 0) tool_weekly_analysis(args, response_body);
    else if (strcmp(name, "weekly_business_report") == 0) tool_weekly_report(response_body);
    else reply_error(response_body, "模型请求了未授权工具");
}

static void assistant_today(char *date, int date_len) {
    time_t now = time(NULL);
    struct tm *local_time = localtime(&now);
    if (!local_time || !strftime(date, date_len, "%Y-%m-%d", local_time)) {
        date[0] = '\0';
    }
}

void assistant_confirm_sale(const char *body, char *response_body) {
    char draft_id[32];
    if (!json_get_str(body, "draft_id", draft_id, sizeof(draft_id)) ||
        !pending_sale.active || strcmp(draft_id, pending_sale.id) != 0) {
        reply_error(response_body, "待确认销售草稿不存在或已失效");
        return;
    }
    if (sale_count + pending_sale.count > MAX_SALES) {
        reply_error(response_body, "交易记录数量已达上限");
        return;
    }
    for (int i = 0; i < pending_sale.count; i++) {
        int idx = find_product_by_id(pending_sale.items[i].product_id);
        if (idx < 0 || products[idx].stock < pending_sale.items[i].quantity) {
            reply_error(response_body, "库存已变化，请重新生成销售草稿");
            return;
        }
    }

    double total = 0;
    int count = pending_sale.count;
    char today[16];
    assistant_today(today, sizeof(today));
    for (int i = 0; i < pending_sale.count; i++) {
        char sale_body[256], sale_result[8192];
        int idx = find_product_by_id(pending_sale.items[i].product_id);
        total += products[idx].price * pending_sale.items[i].quantity;
        snprintf(sale_body, sizeof(sale_body),
                 "{\"product_id\":\"%s\",\"date\":\"%s\",\"quantity\":%d,\"price\":%.2f}",
                 pending_sale.items[i].product_id, today,
                 pending_sale.items[i].quantity, products[idx].price);
        handle_create_sale(sale_body, sale_result);
        if (strstr(sale_result, "\"error\"")) {
            reply_error(response_body, "销售写入失败，请检查库存后重试");
            return;
        }
    }
    SecureZeroMemory(&pending_sale, sizeof(pending_sale));
    snprintf(response_body, 131072,
             "{\"type\":\"sale_confirmed\",\"message\":\"销售已确认并写入账册。\",\"count\":%d,\"total\":%.2f}",
             count, total);
}

void assistant_handle_chat(const char *body, char *response_body) {
    char message[2048], escaped_message[4096], snapshot[32768], escaped_snapshot[65536];
    char payload[131072], error[512] = "", tool_name[128] = "", arguments[4096] = "";
    char *model_response = NULL;

    if (!assistant_has_api_key()) {
        reply_error(response_body, "请先配置阿里云百炼 API Key");
        return;
    }
    if (!json_get_str(body, "message", message, sizeof(message)) || !message[0]) {
        reply_error(response_body, "请输入问题");
        return;
    }

    build_inventory_snapshot(snapshot, sizeof(snapshot));
    json_escape(snapshot, escaped_snapshot, sizeof(escaped_snapshot));
    json_escape(message, escaped_message, sizeof(escaped_message));

    snprintf(payload, sizeof(payload),
        "{\"model\":\"" ASSISTANT_MODEL "\",\"messages\":["
        "{\"role\":\"system\",\"content\":\"" ASSISTANT_RESTOCK_GUARD "\"},"
        "{\"role\":\"system\",\"content\":\"你是文房系统中的白泽店务助手。只能使用提供的工具。涉及销售时只生成草稿，绝不直接声称已经写账。商品有歧义时请先向用户提问。当前库存快照：%s\"},"
        "{\"role\":\"user\",\"content\":\"%s\"}],"
        "\"tool_choice\":\"auto\",\"tools\":["
        "{\"type\":\"function\",\"function\":{\"name\":\"query_inventory\",\"description\":\"按条件查询库存\",\"parameters\":{\"type\":\"object\",\"properties\":{\"keyword\":{\"type\":\"string\"},\"category\":{\"type\":\"string\"},\"max_stock\":{\"type\":\"integer\"},\"min_stock\":{\"type\":\"integer\"}}}}},"
        "{\"type\":\"function\",\"function\":{\"name\":\"low_stock_alerts\",\"description\":\"列出低库存商品\",\"parameters\":{\"type\":\"object\",\"properties\":{\"threshold\":{\"type\":\"integer\"}}}}},"
        "{\"type\":\"function\",\"function\":{\"name\":\"draft_sale\",\"description\":\"生成待确认销售草稿，items_text 必须使用 商品编号:数量,商品编号:数量 格式\",\"parameters\":{\"type\":\"object\",\"properties\":{\"items_text\":{\"type\":\"string\"}},\"required\":[\"items_text\"]}}},"
        "{\"type\":\"function\",\"function\":{\"name\":\"restock_advice\",\"description\":\"根据库存和销量提供补货建议\",\"parameters\":{\"type\":\"object\",\"properties\":{}}}},"
        "{\"type\":\"function\",\"function\":{\"name\":\"weekly_sales_analysis\",\"description\":\"分析指定品类近一周销售和库存\",\"parameters\":{\"type\":\"object\",\"properties\":{\"category\":{\"type\":\"string\"}}}}},"
        "{\"type\":\"function\",\"function\":{\"name\":\"weekly_business_report\",\"description\":\"生成经营周报\",\"parameters\":{\"type\":\"object\",\"properties\":{}}}}"
        "]}",
        escaped_snapshot, escaped_message);

    if (!dashscope_chat(payload, &model_response, error, sizeof(error))) {
        reply_error(response_body, error);
        return;
    }

    if (strstr(model_response, "\"tool_calls\"") &&
        json_get_str(model_response, "name", tool_name, sizeof(tool_name)) &&
        json_get_str(model_response, "arguments", arguments, sizeof(arguments))) {
        dispatch_tool(tool_name, arguments, response_body);
    } else {
        char content[8192];
        if (!json_get_str(model_response, "content", content, sizeof(content)) || !content[0]) {
            reply_error(response_body, "千问未返回可用内容，请换一种问法");
        } else {
            reply_message(response_body, content);
        }
    }
    free(model_response);
}
