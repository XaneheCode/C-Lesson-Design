$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$tests = $PSScriptRoot
$backend = Join-Path $root "backend"
$exe = Join-Path $tests "assistant-draft-parser-test.exe"

try {
    & gcc -Wall -Wextra -O2 `
        -o $exe `
        (Join-Path $tests "assistant-draft-parser.test.c") `
        (Join-Path $backend "data.c") `
        (Join-Path $backend "utils.c") `
        (Join-Path $backend "product.c") `
        (Join-Path $backend "sale.c") `
        (Join-Path $backend "purchase.c") `
        (Join-Path $backend "stats.c") `
        -lwinhttp -lws2_32
    if ($LASTEXITCODE -ne 0) {
        throw "Could not compile assistant draft parser test."
    }

    & $exe
    if ($LASTEXITCODE -ne 0) {
        throw "Assistant draft parser test failed."
    }
} finally {
    Remove-Item -LiteralPath $exe -Force -ErrorAction SilentlyContinue
}
