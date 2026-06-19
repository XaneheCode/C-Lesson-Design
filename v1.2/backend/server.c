/**
 * ============================================================
 *  server.c — HTTP服务器主程序
 *  作用: 启动TCP服务器,监听端口8080,解析HTTP请求,路由分发
 * ============================================================
 *
 *  【整体架构】
 *   HTTP请求 → 接收TCP连接 → 解析请求行(方法+路径) → 路由匹配 → 调用处理函数 → 返回响应
 *
 *  【依赖的技术】
 *   Winsock2: Windows平台的Socket API,用于TCP网络通信
 *   HTTP/1.1: 简化实现,只支持GET/POST/PUT/DELETE/OPTIONS
 *   CORS: 跨域资源共享,允许前端页面访问API
 *
 *  【启动流程】
 *   1. 加载数据文件(商品/销售/进货)
 *   2. 初始化Winsock
 *   3. 创建Socket → 绑定端口 → 开始监听
 *   4. 进入无限循环,每次accept一个连接
 *   5. 解析HTTP请求 → 路由分发 → 发送响应 → 关闭连接
 *
 * 答辩要点: 为什么不用多线程/多进程?
 *   → 课程设计数据量小,单线程足够
 *   → 保持简单,避免线程同步的复杂性
 *   → 每次请求-响应后关闭连接(Connection: close)
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <winsock2.h>
#include <ws2tcpip.h>

#include "data.h"
#include "utils.h"
#include "product.h"
#include "sale.h"
#include "purchase.h"
#include "stats.h"
#include "assistant.h"

#pragma comment(lib, "ws2_32.lib")  /* 链接Winsock库(MSVC编译器专用) */

#define PORT 8080      /* HTTP服务监听端口 */
#define BUF_SIZE 65536 /* 请求缓冲区大小(64KB) */

static volatile int shutdown_requested = 0;

/* ========== HTTP响应发送 ========== */

/**
 * send_response() — 发送JSON格式的HTTP响应
 *
 * @sock:  客户端socket连接
 * @code:  HTTP状态码(200=成功,201=已创建,404=未找到,400=错误请求)
 * @body:  响应体(JSON字符串)
 *
 * 【HTTP响应格式】
 *   HTTP/1.1 200 OK              ← 状态行
 *   Content-Type: application/json; charset=utf-8  ← 响应头
 *   Content-Length: 123
 *   Access-Control-Allow-Origin: *    ← CORS头,允许跨域
 *   Connection: close
 *                                 ← 空行分隔头和体
 *   {"message":"success"}         ← 响应体
 *
 * 【为什么需要CORS头?】
 *   前端页面通过浏览器的fetch()调用API时,浏览器会实施同源策略
 *   如果前端文件(file://)和API(http://localhost:8080)不同源,
 *   浏览器会阻止JavaScript读取响应
 *   CORS头告诉浏览器: "允许所有来源访问"
 */
static void send_response(SOCKET sock, int code, const char *body) {
    char header[512];
    /* 根据状态码选择状态文本 */
    const char *status_text = (code == 200) ? "OK" :
                              (code == 201) ? "Created" :
                              (code == 403) ? "Forbidden" :
                              (code == 404) ? "Not Found" :
                              (code == 400) ? "Bad Request" : "Internal Server Error";

    /* sprintf拼接HTTP响应头 */
    int hlen = sprintf(header,
        "HTTP/1.1 %d %s\r\n"
        "Content-Type: application/json; charset=utf-8\r\n"
        "Content-Length: %d\r\n"
        "Access-Control-Allow-Origin: *\r\n"                    /* 允许任意来源跨域访问 */
        "Access-Control-Allow-Methods: GET, POST, PUT, DELETE, OPTIONS\r\n"  /* 允许的HTTP方法 */
        "Access-Control-Allow-Headers: Content-Type\r\n"        /* 允许的请求头 */
        "Connection: close\r\n"                                  /* 每次请求后关闭连接 */
        "\r\n",                                                  /* 空行: 头部结束 */
        code, status_text, (int)strlen(body));

    /* 分两次发送: 先发头部,再发响应体 */
    send(sock, header, hlen, 0);
    send(sock, body, (int)strlen(body), 0);
}

/**
 * send_file_response() — 发送文件内容的HTTP响应(用于前端页面)
 *
 * @sock:     客户端socket
 * @filepath: 文件路径,如 "frontend/index.html"
 *
 * 【文件服务功能】
 *   浏览器访问 http://localhost:8080/ 时,服务器返回前端HTML页面
 *   支持HTML、CSS、JS三种文件类型的MIME类型识别
 */
static void send_file_response(SOCKET sock, const char *filepath) {
    /* 以二进制模式打开文件 */
    FILE *fp = fopen(filepath, "rb");
    if (!fp) {
        send_response(sock, 404, "{\"error\":\"File not found\"}");
        return;
    }

    /* 获取文件大小: fseek到末尾 → ftell → fseek回开头 */
    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    /* 分配缓冲区并读取文件内容 */
    char *content = (char *)malloc(fsize + 1);
    fread(content, 1, fsize, fp);
    content[fsize] = '\0';
    fclose(fp);

    /* 根据文件扩展名确定MIME类型 */
    const char *ctype = "text/html; charset=utf-8";      /* 默认: HTML */
    if (strstr(filepath, ".css")) ctype = "text/css; charset=utf-8";
    if (strstr(filepath, ".js")) ctype = "application/javascript; charset=utf-8";
    if (strstr(filepath, ".jsx")) ctype = "application/javascript; charset=utf-8";
    if (strstr(filepath, ".png")) ctype = "image/png";

    /* 发送HTTP响应 */
    char header[512];
    int hlen = sprintf(header,
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: %s\r\n"
        "Content-Length: %ld\r\n"
        "Connection: close\r\n"
        "\r\n",
        ctype, fsize);

    send(sock, header, hlen, 0);
    send(sock, content, (int)fsize, 0);
    free(content);  /* 释放文件内容缓冲区 */
}

/* ========== URL路由辅助函数 ========== */

/**
 * path_match() — 检查URL路径是否匹配某个模式
 *
 * @path:    请求路径,如 "/api/products/P001"
 * @pattern: 匹配模式,如 "/api/products"
 * 返回值:   匹配成功则返回路径剩余部分("/P001"),失败返回NULL
 *
 * 【匹配规则】
 *   路径必须以pattern开头
 *   pattern后面必须是 '\0'(结尾)、'?'(查询参数) 或 '/'(子路径)
 *
 *   示例:
 *   path="/api/products/P001", pattern="/api/products" → 返回 "/P001"
 *   path="/api/products?category=笔", pattern="/api/products" → 返回 "?category=笔"
 *   path="/api/sales", pattern="/api/products" → 返回 NULL (不匹配)
 */
static const char *path_match(const char *path, const char *pattern) {
    int plen = (int)strlen(pattern);
    if (strncmp(path, pattern, plen) == 0) {
        /* pattern后面是结尾或查询参数 → 匹配整个路径 */
        if (path[plen] == '\0' || path[plen] == '?') return path + plen;
        /* pattern后面是/ → 匹配子路径 */
        if (path[plen] == '/') return path + plen + 1;
    }
    return NULL;  /* 不匹配 */
}

/**
 * get_query() — 从URL路径中提取查询字符串
 *
 * @path: "/api/products?category=笔&keyword=晨光"
 * 返回:  "category=笔&keyword=晨光" (跳过问号)
 * 如果没有查询参数,返回NULL
 */
static const char *get_query(const char *path) {
    const char *q = strchr(path, '?');
    return q ? q + 1 : NULL;
}

/**
 * get_path_id() — 从路径剩余部分提取资源ID
 *
 * @path_remainder: path_match返回的剩余路径,如 "/P001"
 * @id: 输出缓冲区
 * @id_len: 缓冲区大小
 *
 * 提取规则: 复制字符直到遇到 '/'、'?' 或结尾
 * 示例: "/P001" → "P001"
 */
static void get_path_id(const char *path_remainder, char *id, int id_len) {
    int i = 0;
    while (path_remainder[i] && path_remainder[i] != '/' &&
           path_remainder[i] != '?' && i < id_len - 1) {
        id[i] = path_remainder[i];
        i++;
    }
    id[i] = '\0';
}

/* ========== HTTP请求路由分发 ========== */

/**
 * handle_request() — 核心路由函数: 将HTTP请求分发到对应的处理函数
 *
 * @sock:   客户端socket(用于发送响应)
 * @method: HTTP方法("GET"/"POST"/"PUT"/"DELETE"/"OPTIONS")
 * @path:   请求路径("/api/products?category=笔")
 * @body:   请求体(POST/PUT的JSON数据)
 *
 * 【路由表 — 完整的API映射】
 *   ┌──────────────┬────────┬─────────────────────────────┐
 *   │ 路径          │ 方法    │ 处理函数                     │
 *   ├──────────────┼────────┼─────────────────────────────┤
 *   │ /            │ GET    │ send_file(index.html)       │
 *   │ /api/products│ GET    │ handle_get_products()       │
 *   │ /api/products│ POST   │ handle_create_product()     │
 *   │ /api/products/:id │ GET/PUT/DELETE │ 单个商品CRUD  │
 *   │ /api/products/stats │ GET │ handle_product_stats()  │
 *   │ /api/sales   │ GET    │ handle_get_sales()          │
 *   │ /api/sales   │ POST   │ handle_create_sale()        │
 *   │ /api/sales/:id │ GET/PUT/DELETE │ 单个销售CRUD     │
 *   │ /api/purchases │ GET  │ handle_get_purchases()      │
 *   │ /api/purchases │ POST │ handle_create_purchase()    │
 *   │ /api/purchases/:id │ GET/PUT/DELETE │ 单个进货CRUD │
 *   │ /api/stats/* │ GET    │ 各统计接口                   │
 *   └──────────────┴────────┴─────────────────────────────┘
 *
 * 【路由匹配顺序】
 *   1. 先匹配静态文件(前端页面)
 *   2. 处理CORS预检请求(OPTIONS)
 *   3. 按前缀匹配API路径(stats → products → sales → purchases)
 *   4. 全部不匹配则返回404
 *
 * 【注意: 路径匹配顺序很重要!】
 *   /api/products/stats 必须在 /api/products 之前匹配
 *   否则 "stats" 会被当成商品ID
 */
static void handle_request(SOCKET sock, const char *method,
                           const char *path, const char *body,
                           int is_local_client) {
    char response[131072];  /* 128KB响应缓冲区 */
    const char *rem;

    /* ===== 第1层: 前端静态文件服务 ===== */
    if (strcmp(path, "/") == 0 || strcmp(path, "/index.html") == 0) {
        send_file_response(sock, "frontend/index.html");
        return;
    }
    /* CSS文件: /css/xxx.css → frontend/css/xxx.css */
    if ((rem = path_match(path, "/css")) != NULL) {
        char filepath[256];
        sprintf(filepath, "frontend/css/%s", rem);
        send_file_response(sock, filepath);
        return;
    }
    /* JS文件: /js/xxx.js → frontend/js/xxx.js */
    if ((rem = path_match(path, "/js")) != NULL) {
        char filepath[256];
        sprintf(filepath, "frontend/js/%s", rem);
        send_file_response(sock, filepath);
        return;
    }

    /* ===== 第2层: CORS预检请求 ===== */
    /* 浏览器在发送跨域POST/PUT/DELETE前,会先发OPTIONS请求探查 */
    if (strcmp(method, "OPTIONS") == 0) {
        send_response(sock, 200, "");  /* 返回空响应+CORS头即可 */
        return;
    }

    /* ===== 第3层: 本机系统控制 ===== */
    if (strcmp(path, "/api/system/shutdown") == 0 && strcmp(method, "POST") == 0) {
        if (!is_local_client) {
            send_response(sock, 403, "{\"error\":\"Shutdown is only available from localhost\"}");
            return;
        }
        send_response(sock, 200, "{\"message\":\"System stopped\"}");
        assistant_clear_api_key();
        shutdown_requested = 1;
        return;
    }

    /* ===== Bai Ze assistant: localhost only, key stays in C process memory ===== */
    if (strncmp(path, "/api/assistant/", 15) == 0) {
        if (!is_local_client) {
            send_response(sock, 403, "{\"error\":\"Assistant is only available from localhost\"}");
            return;
        }
        if (strcmp(path, "/api/assistant/status") == 0 && strcmp(method, "GET") == 0) {
            assistant_get_status(response);
            send_response(sock, 200, response);
            return;
        }
        if (strcmp(path, "/api/assistant/key") == 0 && strcmp(method, "POST") == 0) {
            assistant_set_api_key(body, response);
            send_response(sock, strstr(response, "\"error\"") ? 400 : 200, response);
            return;
        }
        if (strcmp(path, "/api/assistant/chat") == 0 && strcmp(method, "POST") == 0) {
            assistant_handle_chat(body, response);
            send_response(sock, strstr(response, "\"error\"") ? 400 : 200, response);
            return;
        }
        if (strcmp(path, "/api/assistant/confirm-sale") == 0 && strcmp(method, "POST") == 0) {
            assistant_confirm_sale(body, response);
            send_response(sock, strstr(response, "\"error\"") ? 400 : 200, response);
            return;
        }
    }

    /* ===== 第4层: Products API路由 ===== */
    /* 注意: /api/products/stats 必须在 /api/products 之前匹配! */
    if ((rem = path_match(path, "/api/products/stats")) != NULL) {
        if (strcmp(method, "GET") == 0) {
            handle_product_stats(response);
            send_response(sock, 200, response);
            return;
        }
    }
    if ((rem = path_match(path, "/api/products")) != NULL) {
        if (rem[0] == '\0' || rem[0] == '?') {
            /* /api/products 或 /api/products?query — 列表操作 */
            if (strcmp(method, "GET") == 0) {
                const char *q = get_query(path);
                handle_get_products(q, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "POST") == 0) {
                handle_create_product(body, response);
                send_response(sock, 201, response);  /* 201 Created */
                return;
            }
        } else {
            /* /api/products/:id — 单个商品操作 */
            char id[STR_LEN];
            get_path_id(rem, id, STR_LEN);
            if (strcmp(method, "GET") == 0) {
                handle_get_product(id, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "PUT") == 0) {
                handle_update_product(id, body, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "DELETE") == 0) {
                handle_delete_product(id, response);
                send_response(sock, 200, response);
                return;
            }
        }
    }

    /* ===== 第4层: Sales API路由 ===== */
    if ((rem = path_match(path, "/api/sales")) != NULL) {
        if (rem[0] == '\0' || rem[0] == '?') {
            if (strcmp(method, "GET") == 0) {
                const char *q = get_query(path);
                handle_get_sales(q, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "POST") == 0) {
                handle_create_sale(body, response);
                send_response(sock, 201, response);
                return;
            }
        } else {
            char id[STR_LEN];
            get_path_id(rem, id, STR_LEN);
            if (strcmp(method, "GET") == 0) {
                handle_get_sale(id, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "PUT") == 0) {
                handle_update_sale(id, body, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "DELETE") == 0) {
                handle_delete_sale(id, response);
                send_response(sock, 200, response);
                return;
            }
        }
    }

    /* ===== 第5层: Purchases API路由 ===== */
    if ((rem = path_match(path, "/api/purchases")) != NULL) {
        if (rem[0] == '\0' || rem[0] == '?') {
            if (strcmp(method, "GET") == 0) {
                const char *q = get_query(path);
                handle_get_purchases(q, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "POST") == 0) {
                handle_create_purchase(body, response);
                send_response(sock, 201, response);
                return;
            }
        } else {
            char id[STR_LEN];
            get_path_id(rem, id, STR_LEN);
            if (strcmp(method, "GET") == 0) {
                handle_get_purchase(id, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "PUT") == 0) {
                handle_update_purchase(id, body, response);
                send_response(sock, 200, response);
                return;
            }
            if (strcmp(method, "DELETE") == 0) {
                handle_delete_purchase(id, response);
                send_response(sock, 200, response);
                return;
            }
        }
    }

    /* ===== 第6层: Stats API路由 ===== */
    if (strcmp(path, "/api/stats/dashboard") == 0 && strcmp(method, "GET") == 0) {
        handle_dashboard_stats(response);
        send_response(sock, 200, response);
        return;
    }
    if ((rem = path_match(path, "/api/stats/ranking")) != NULL) {
        if (strcmp(method, "GET") == 0) {
            const char *q = get_query(path);
            handle_sales_ranking(q, response);
            send_response(sock, 200, response);
            return;
        }
    }
    if (strcmp(path, "/api/stats/low-stock") == 0 && strcmp(method, "GET") == 0) {
        handle_low_stock_alerts(response);
        send_response(sock, 200, response);
        return;
    }
    if ((rem = path_match(path, "/api/stats/weekly")) != NULL) {
        if (strcmp(method, "GET") == 0) {
            const char *q = get_query(path);
            handle_weekly_category_sales(q, response);
            send_response(sock, 200, response);
            return;
        }
    }
    if (strcmp(path, "/api/stats/inventory") == 0 && strcmp(method, "GET") == 0) {
        handle_inventory_report(response);
        send_response(sock, 200, response);
        return;
    }

    /* ===== Static file fallback: serve any static file from frontend/ ===== */
    /* Security: reject path traversal attempts (../) */
    if (strstr(path, "..") == NULL) {
        char fpath[512];
        snprintf(fpath, sizeof(fpath), "frontend%s", path);
        /* Strip query string from filepath */
        char *qmark = strchr(fpath, '?');
        if (qmark) *qmark = '\0';
        /* Try to open the file; if exists, serve it */
        FILE *fp = fopen(fpath, "rb");
        if (fp) {
            fclose(fp);
            send_file_response(sock, fpath);
            return;
        }
    }

    /* ===== 兜底: 404 Not Found ===== */
    sprintf(response, "{\"error\":\"API endpoint not found: %s %s\"}", method, path);
    send_response(sock, 404, response);
}

/* ========== main() — 程序入口 ========== */

/**
 * main() — 服务器启动主函数
 *
 * 【启动流程】
 *   ① 加载数据文件(商品/销售/进货)
 *   ② 初始化Winsock库
 *   ③ 创建监听Socket
 *   ④ 设置端口复用(SO_REUSEADDR)
 *   ⑤ 绑定地址和端口
 *   ⑥ 开始监听
 *   ⑦ 进入accept循环,处理每个客户端连接
 *
 * 【HTTP请求处理流程(每次循环)】
 *   accept → recv读取请求 → 解析方法+路径+请求体 → 路由分发 → send响应 → closesocket
 *
 * 答辩要点: SO_REUSEADDR的作用?
 *   → 允许服务器重启后立即绑定同一端口
 *   → 否则Windows会等待约2分钟(TIME_WAIT)才释放端口
 */
int main(void) {
    /* ===== 步骤1: 加载数据文件 ===== */
    printf("[系统] 加载数据...\n");
    load_products();   /* 从 data/products.txt 加载商品 */
    load_sales();      /* 从 data/sales.txt 加载销售记录 */
    load_purchases();  /* 从 data/purchases.txt 加载进货记录 */
    printf("[系统] 商品: %d 条, 交易: %d 条, 进货: %d 条\n",
           product_count, sale_count, purchase_count);

    /* ===== 步骤2: 初始化Winsock ===== */
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) {
        printf("[错误] Winsock 初始化失败: %d\n", WSAGetLastError());
        return 1;
    }

    /* ===== 步骤3: 创建监听Socket ===== */
    /* AF_INET = IPv4, SOCK_STREAM = TCP, 0 = 自动选择协议 */
    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) {
        printf("[错误] 创建socket失败: %d\n", WSAGetLastError());
        WSACleanup();
        return 1;
    }

    /* ===== 步骤4: 设置端口复用 ===== */
    int opt = 1;
    setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt));

    /* ===== 步骤5: 绑定地址和端口 ===== */
    struct sockaddr_in addr;
    addr.sin_family = AF_INET;           /* IPv4地址族 */
    addr.sin_addr.s_addr = INADDR_ANY;   /* 绑定所有网卡(0.0.0.0) */
    addr.sin_port = htons(PORT);         /* 端口号,htons将主机字节序转为网络字节序 */

    if (bind(listen_sock, (struct sockaddr *)&addr, sizeof(addr)) == SOCKET_ERROR) {
        printf("[错误] 绑定端口 %d 失败: %d\n", PORT, WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    /* ===== 步骤6: 开始监听 ===== */
    /* listen(队列长度5): 最多5个连接排队等待accept */
    if (listen(listen_sock, 5) == SOCKET_ERROR) {
        printf("[错误] 监听失败: %d\n", WSAGetLastError());
        closesocket(listen_sock);
        WSACleanup();
        return 1;
    }

    /* 打印启动信息 */
    printf("\n========================================\n");
    printf("  文具店销售管理系统 - HTTP 服务器\n");
    printf("  端口: %d\n", PORT);
    printf("  访问: http://localhost:%d\n", PORT);
    printf("========================================\n\n");

    /* ===== 步骤7: 主循环 — 接受并处理客户端连接 ===== */
    while (!shutdown_requested) {
        /* accept: 阻塞等待客户端连接 */
        struct sockaddr_in client_addr;
        int client_len = sizeof(client_addr);
        SOCKET client = accept(listen_sock, (struct sockaddr *)&client_addr, &client_len);
        if (client == INVALID_SOCKET) continue;  /* 连接失败,继续等待 */

        /* 分配请求缓冲区 */
        char *req_buf = (char *)malloc(BUF_SIZE);
        if (!req_buf) { closesocket(client); continue; }
        memset(req_buf, 0, BUF_SIZE);

        int total_read = 0;
        int n;

        /* 设置接收超时(3秒),防止慢客户端阻塞服务器 */
        DWORD timeout = 3000;
        setsockopt(client, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout));

        /* 读取HTTP请求数据 */
        while (total_read < BUF_SIZE - 1) {
            n = recv(client, req_buf + total_read, BUF_SIZE - 1 - total_read, 0);
            if (n <= 0) break;        /* 连接关闭或出错 */
            total_read += n;
            /* 检测到完整的HTTP头部(以\r\n\r\n结尾)则停止 */
            req_buf[total_read] = '\0';
            if (strstr(req_buf, "\r\n\r\n")) break;
        }
        req_buf[total_read] = '\0';

        /* 过滤无效请求(太短的不是有效HTTP) */
        if (total_read < 5) {
            free(req_buf);
            closesocket(client);
            continue;
        }

        /* ===== 解析HTTP请求行 ===== */
        /* 请求行格式: "GET /api/products?category=笔 HTTP/1.1\r\n" */
        char method[16] = {0};    /* HTTP方法 */
        char path[1024] = {0};    /* 请求路径(含查询参数) */

        char *line_end = strstr(req_buf, "\r\n");
        if (line_end) {
            *line_end = '\0';     /* 临时截断,方便sscanf解析 */
            sscanf(req_buf, "%15s %1023s", method, path);  /* 提取方法和路径 */
            *line_end = '\r';     /* 恢复原字符串 */
        }

        /* ===== 定位请求体 ===== */
        /* 请求体在 \r\n\r\n 之后(空行分隔头部和体) */
        char *body = strstr(req_buf, "\r\n\r\n");
        if (body) body += 4;  /* 跳过 \r\n\r\n */

        /* ===== 处理POST/PUT的Content-Length ===== */
        /* POST/PUT请求的body可能因为TCP分包没有一次读完 */
        if ((strcmp(method, "POST") == 0 || strcmp(method, "PUT") == 0) && body) {
            char *cl = strstr(req_buf, "Content-Length:");
            if (!cl) cl = strstr(req_buf, "content-length:");  /* 大小写不敏感 */
            if (cl) {
                int expected = atoi(cl + 15);                     /* 预期body长度 */
                int body_read = total_read - (int)(body - req_buf);  /* 已读取的body长度 */
                /* 继续读取,直到读够预期长度 */
                while (body_read < expected && body_read < BUF_SIZE / 2) {
                    n = recv(client, body + body_read, BUF_SIZE / 2 - body_read, 0);
                    if (n <= 0) break;
                    body_read += n;
                    total_read += n;
                }
                body[body_read] = '\0';
            }
        }

        if (!body) body = "";  /* GET请求通常没有body */

        /* 打印请求日志 */
        printf("[%s] %s\n", method, path);

        /* ===== 路由分发: 调用对应的处理函数 ===== */
        int is_local_client = (ntohl(client_addr.sin_addr.s_addr) == INADDR_LOOPBACK);
        handle_request(client, method, path, body, is_local_client);

        /* 清理资源 */
        free(req_buf);
        closesocket(client);  /* 关闭客户端连接(Connection: close模式) */
    }

    /* 以下代码在正常运行时不会执行(无限循环) */
    closesocket(listen_sock);
    assistant_clear_api_key();
    WSACleanup();
    return 0;
}
