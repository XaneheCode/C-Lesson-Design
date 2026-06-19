$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$indexPath = Join-Path $root "frontend\index.html"
$index = Get-Content -Raw -Encoding UTF8 $indexPath

if ($index -match "https?://") {
    throw "frontend/index.html still depends on remote resources."
}

$requiredFiles = @(
    "frontend\vendor\react.development.js",
    "frontend\vendor\react-dom.development.js",
    "frontend\vendor\babel.min.js",
    "frontend\vendor\gsap.min.js",
    "frontend\assets\baize-assistant.png"
)

foreach ($relativePath in $requiredFiles) {
    if (-not (Test-Path -LiteralPath (Join-Path $root $relativePath))) {
        throw "Missing portable frontend asset: $relativePath"
    }
}

if ($index -notmatch 'vendor/gsap\.min\.js') {
    throw "frontend/index.html does not load the local GSAP asset."
}

Write-Host "portable frontend assets test passed"
