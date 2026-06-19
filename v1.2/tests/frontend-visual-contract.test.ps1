$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$welcome = Get-Content -Raw -Encoding UTF8 (Join-Path $root "frontend\js\views-welcome.jsx")
$stats = Get-Content -Raw -Encoding UTF8 (Join-Path $root "frontend\js\views-query-stats.jsx")
$styles = Get-Content -Raw -Encoding UTF8 (Join-Path $root "frontend\css\styles.css")
$app = Get-Content -Raw -Encoding UTF8 (Join-Path $root "frontend\js\app.jsx")
$index = Get-Content -Raw -Encoding UTF8 (Join-Path $root "frontend\index.html")
$ui = Get-Content -Raw -Encoding UTF8 (Join-Path $root "frontend\js\ui.jsx")
$tables = Get-Content -Raw -Encoding UTF8 (Join-Path $root "frontend\js\views-tables.jsx")
$assistant = Get-Content -Raw -Encoding UTF8 (Join-Path $root "frontend\js\assistant.jsx")

function Assert-BlockDoesNotMatch {
    param(
        [string]$Text,
        [string]$SelectorPattern,
        [string]$ForbiddenPattern,
        [string]$Message
    )

    if ($Text -notmatch "$SelectorPattern\s*\{(?<body>[^}]*)\}") {
        throw "Missing CSS rule: $SelectorPattern"
    }

    if ($Matches.body -match $ForbiddenPattern) {
        throw $Message
    }
}

if ($welcome -notmatch '<span className="ch">[^<]+</span>\s*<span className="ch hero-room">[^<]+</span>') {
    throw "The welcome title must render the room character without a nested seal."
}

if ($welcome -match 'className="seal"') {
    throw "The welcome title must not render the seal character."
}

if ($styles -notmatch '\.hero-title \.hero-room\s*\{[^}]*color:\s*var\(--vermillion\)') {
    throw "The room character must use the existing vermillion accent."
}

if ($styles -match '\.hero-title \.seal\s*\{') {
    throw "The unused welcome seal CSS rule must be removed."
}

Assert-BlockDoesNotMatch $styles '\.stopped-mark' 'transform:\s*rotate' `
    "The stopped-system character must be upright."

if ($styles -notmatch '\.trend-summary\s*\{[^}]*grid-template-columns:\s*1fr;') {
    throw "The trend summary must use one column so the chart sits below the total."
}

if ($styles -notmatch '\.category-share-card\s*\{[^}]*display:\s*flex;[^}]*flex-direction:\s*column;') {
    throw "The category share card must use a vertical flex layout."
}

if ($styles -notmatch '\.category-share-body\s*\{[^}]*flex:\s*1;[^}]*justify-content:\s*center;') {
    throw "The category share body must center the chart group in the available area."
}

if ($styles -notmatch '\.sales-donut\s*\{[^}]*width:\s*min\(100%,\s*280px\);') {
    throw "The category share donut must be enlarged to 280px."
}

if ($styles -notmatch '\.func-grid\s*\{[^}]*grid-template-columns:\s*repeat\(10,\s*minmax\(0,\s*1fr\)\);') {
    throw "The desktop welcome grid must use ten columns so all tiles align in one row."
}

if ($styles -notmatch '@media\s*\(max-width:\s*1120px\)\s*\{[^}]*\.func-grid\s*\{[^}]*grid-template-columns:\s*repeat\(4,\s*minmax\(0,\s*1fr\)\);') {
    throw "The narrow welcome grid must use four columns so tiles align in two rows."
}

if ($styles -notmatch 'leaf-vein-watermark\.png') {
    throw "Decorative layers must use the generated leaf-vein watermark asset."
}

if ($styles -notmatch '\.paper-bg::before\s*\{') {
    throw "Paper background should include a subtle cloud-water pattern layer."
}

if ($styles -notmatch '\.sidebar::after\s*\{') {
    throw "Sidebar should include a bottom embossed pattern layer."
}

if ($styles -notmatch '\.welcome::before\s*\{') {
    throw "Welcome page should include a local flowing pattern layer."
}

if ($styles -notmatch '\.sales-trend-card::after\s*\{') {
    throw "Sales trend card should include a local flowing watermark."
}

if ($welcome -notmatch 'welcomeRef' -or $welcome -notmatch 'window\.gsap\.context' -or $welcome -notmatch 'prefers-reduced-motion') {
    throw "Welcome page should use GSAP entrance motion with reduced-motion support."
}

if ($app -notmatch 'transitionRoute' -or $app -notmatch 'mainRef' -or $app -notmatch 'prefers-reduced-motion') {
    throw "App shell should use a GSAP route transition with reduced-motion support."
}

if ($app -match 'requestAnimationFrame') {
    throw "Route entrance motion must not depend on a delayed animation-frame callback."
}

if ($app -notmatch 'React\.useLayoutEffect') {
    throw "Route entrance motion must begin in a layout effect before paint."
}

if ($app -notmatch 'pendingRouteRef' -or $app -notmatch 'routeTweenRef') {
    throw "Route switching must track the pending destination and cancel stale tweens."
}

if ($index -notmatch 'js/assistant\.jsx') {
    throw "The frontend must load the Bai Ze assistant panel."
}

foreach ($selector in @(".assistant-fab", ".assistant-panel", ".assistant-key-form", ".assistant-sale-card")) {
    if ($styles -notmatch [regex]::Escape($selector)) {
        throw "Missing Bai Ze assistant style: $selector"
    }
}

if ($styles -notmatch '(?s)\.assistant-shortcuts\s*\{[^}]*display:\s*grid;[^}]*grid-template-columns:\s*repeat\(2,\s*minmax\(0,\s*1fr\)\);') {
    throw "Bai Ze shortcut prompts must wrap into a readable two-column grid."
}

if ($ui -notmatch 'const TABLE_PAGE_SIZE = 10;' -or
    $ui -notmatch 'function Pagination\(' -or
    $ui -notmatch 'function useTablePagination\(') {
    throw "Ledger tables must share a ten-row pagination primitive."
}

if ([regex]::Matches($tables, 'useTablePagination\(').Count -lt 3 -or
    [regex]::Matches($tables, '<Pagination').Count -lt 3 -or
    [regex]::Matches($tables, 'pageItems\.map').Count -lt 3) {
    throw "Inventory, transactions, and purchases must all render paginated rows."
}

if ($styles -notmatch '(?s)\.modal\s*\{[^}]*position:\s*fixed;[^}]*top:\s*50%;[^}]*left:\s*50%;[^}]*transform:\s*translate\(-50%,\s*-50%\);') {
    throw "Ledger edit dialogs must stay centered in the visible viewport."
}

if ($ui -notmatch 'ReactDOM\.createPortal\(') {
    throw "Modals must render at the document root so shell motion cannot offset viewport centering."
}

if ($assistant -match '<span>\s*白泽\s*</span>' -or $styles -match '\.assistant-fab span\s*\{') {
    throw "The floating Bai Ze emblem must not include a visible text label."
}

foreach ($marker in @(
    "confirmingDraftIdsRef",
    "updateDraftStatus",
    "SALE_DRAFT_CONFIRMING",
    "SALE_DRAFT_CONFIRMED"
)) {
    if ($assistant -notmatch [regex]::Escape($marker)) {
        throw "Bai Ze sale drafts must lock duplicate confirmations and render the final ledger state: $marker"
    }
}

if ($stats -notmatch 'StationeryApi\.getWeeklyStats\(\)') {
    throw "The weekly chart must load sales for all products."
}

if ($stats -notmatch 'const \[statsLoading, setStatsLoading\] = useState\(true\);') {
    throw "Statistics charts must begin in loading state so GSAP waits for rendered data nodes."
}

foreach ($marker in @("sales-donut", "ranking-bar-fill", "sales-detail-table", "window.gsap")) {
    if ($stats -notmatch [regex]::Escape($marker)) {
        throw "Missing sales overview marker: $marker"
    }
}

$allProductsTitle = -join [char[]](0x6240, 0x6709, 0x5546, 0x54C1, 0x9500, 0x552E, 0x60C5, 0x51B5)
if ([regex]::Matches($stats, [regex]::Escape($allProductsTitle)).Count -lt 2) {
    throw "Both statistics views must use the all-products sales overview title."
}

Write-Host "frontend visual contract test passed"
