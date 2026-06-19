$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$data = Join-Path $root "data"

$products = @(Import-Csv (Join-Path $data "products.txt") -Delimiter "|")
$sales = @(Import-Csv (Join-Path $data "sales.txt") -Delimiter "|")
$purchases = @(Import-Csv (Join-Path $data "purchases.txt") -Delimiter "|")

if (-not $products.Count) {
    throw "Generated product catalog is empty."
}

$duplicateIds = @($products | Group-Object id | Where-Object Count -gt 1)
if ($duplicateIds.Count) {
    throw "Duplicate product IDs: $($duplicateIds.Name -join ', ')"
}

$duplicateNames = @($products | Group-Object name | Where-Object Count -gt 1)
if ($duplicateNames.Count) {
    throw "Duplicate product names: $($duplicateNames.Name -join ', ')"
}

$productsById = @{}
foreach ($product in $products) {
    if (-not $product.id -or -not $product.name -or -not $product.category) {
        throw "Product catalog contains an incomplete row."
    }
    if ([int]$product.stock -lt 0) {
        throw "Product $($product.id) has negative stock."
    }
    $productsById[$product.id] = $product
}

foreach ($recordSet in @(
    @{ Label = "sale"; Rows = $sales },
    @{ Label = "purchase"; Rows = $purchases }
)) {
    foreach ($record in $recordSet.Rows) {
        $product = $productsById[$record.product_id]
        if (-not $product) {
            throw "$($recordSet.Label) $($record.id) references missing product $($record.product_id)."
        }
        if ($record.name -ne $product.name) {
            throw "$($recordSet.Label) $($record.id) has stale product name '$($record.name)'."
        }
        if ($record.category -ne $product.category) {
            throw "$($recordSet.Label) $($record.id) has stale category '$($record.category)'."
        }
        if ([int]$record.quantity -le 0) {
            throw "$($recordSet.Label) $($record.id) has a non-positive quantity."
        }
    }
}

Write-Host "generated data integrity test passed: $($products.Count) products, $($sales.Count) sales, $($purchases.Count) purchases"
