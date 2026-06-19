$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$backend = Join-Path $root "backend"
$frontend = Join-Path $root "frontend"

$assistantCPath = Join-Path $backend "assistant.c"
$assistantHPath = Join-Path $backend "assistant.h"
if (-not (Test-Path -LiteralPath $assistantCPath) -or -not (Test-Path -LiteralPath $assistantHPath)) {
    throw "Missing C assistant module."
}

$assistantC = Get-Content -Raw -Encoding UTF8 $assistantCPath
$server = Get-Content -Raw -Encoding UTF8 (Join-Path $backend "server.c")
$build = Get-Content -Raw -Encoding UTF8 (Join-Path $backend "build_eng.bat")
$api = Get-Content -Raw -Encoding UTF8 (Join-Path $frontend "js\api.js")
$assistantUiPath = Join-Path $frontend "js\assistant.jsx"
if (-not (Test-Path -LiteralPath $assistantUiPath)) {
    throw "Missing Bai Ze frontend assistant panel."
}
$assistantUi = Get-Content -Raw -Encoding UTF8 $assistantUiPath

foreach ($marker in @(
    "qwen3.6-flash",
    "dashscope.aliyuncs.com",
    "WinHttpOpen",
    "assistant_set_api_key",
    "assistant_has_api_key",
    "assistant_handle_chat",
  "assistant_confirm_sale",
  "assistant_is_recent_sale",
  "assistant_today",
  "merge_draft_item",
    "normalize_draft_items_text",
    "resolve_product_reference",
    "RESTOCK_IS_NOT_SALE",
    "query_inventory",
    "low_stock_alerts",
    "draft_sale",
    "restock_advice",
    "weekly_sales_analysis",
    "weekly_business_report"
)) {
    if ($assistantC -notmatch [regex]::Escape($marker)) {
        throw "Missing assistant backend marker: $marker"
    }
}

foreach ($route in @(
    "/api/assistant/status",
    "/api/assistant/key",
    "/api/assistant/chat",
    "/api/assistant/confirm-sale"
)) {
    if ($server -notmatch [regex]::Escape($route)) {
        throw "Missing assistant API route: $route"
    }
}

if ($server -notmatch 'is_local_client') {
    throw "Assistant routes must use localhost protection."
}

if ($build -notmatch 'assistant\.c' -or $build -notmatch '\-lwinhttp') {
    throw "Backend build must compile assistant.c and link WinHTTP."
}

foreach ($marker in @("getAssistantStatus", "setAssistantKey", "askAssistant", "confirmAssistantSale")) {
    if ($api -notmatch [regex]::Escape($marker)) {
        throw "Missing frontend assistant API method: $marker"
    }
}

foreach ($marker in @("BaiZeAssistant", "baize-assistant.png", "assistant-fab", "assistant-panel")) {
    if ($assistantUi -notmatch [regex]::Escape($marker)) {
        throw "Missing Bai Ze UI marker: $marker"
    }
}

Write-Host "assistant contract test passed"
