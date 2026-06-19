#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../backend/assistant.c"

static void setup_products(void) {
    memset(products, 0, sizeof(products));
    product_count = 2;
    strcpy(products[0].id, "P020");
    strcpy(products[0].name, "固体胶");
    strcpy(products[0].category, "胶水");
    products[0].stock = 100;
    products[0].price = 2.50;

    strcpy(products[1].id, "P002");
    strcpy(products[1].name, "真彩中性笔");
    strcpy(products[1].category, "笔");
    products[1].stock = 80;
    products[1].price = 2.00;
}

static void expect_draft(const char *label, const char *items_text, const char *product_id) {
    char body[2048];
    char response[131072];
    snprintf(body, sizeof(body), "{\"items_text\":\"%s\"}", items_text);
    tool_draft_sale(body, response);
    if (!strstr(response, "\"type\":\"sale_draft\"") ||
        !strstr(response, product_id) ||
        !strstr(response, "\"quantity\":5")) {
        fprintf(stderr, "%s failed: %s\n", label, response);
        exit(1);
    }
}

static void expect_invalid_draft(const char *label, const char *items_text) {
    char body[2048];
    char response[131072];
    snprintf(body, sizeof(body), "{\"items_text\":\"%s\"}", items_text);
    tool_draft_sale(body, response);
    if (!strstr(response, "\"error\"")) {
        fprintf(stderr, "%s unexpectedly passed: %s\n", label, response);
        exit(1);
    }
}

int main(void) {
    setup_products();
    expect_draft("product id", "P020:5", "\"product_id\":\"P020\"");
    expect_draft("product name", "固体胶:5", "\"product_id\":\"P020\"");
    expect_draft("spaces", " P020 : 5 ", "\"product_id\":\"P020\"");
    expect_draft("full-width punctuation", "P020：5", "\"product_id\":\"P020\"");
    expect_draft("decorated product reference", "商品 P002 真彩中性笔:5支", "\"product_id\":\"P002\"");
    expect_invalid_draft("ambiguous decorated reference", "P020 固体胶 P002 真彩中性笔:5");
    puts("assistant draft parser test passed");
    return 0;
}
