$ErrorActionPreference = "Stop"

$baseUrl = "http://localhost:8080"
$fakeKey = "sk-test-memory-only-not-real"

function Get-ErrorBody([scriptblock]$request) {
    try {
        & $request | Out-Null
        throw "Expected request to fail."
    } catch {
        if ($_.ErrorDetails -and $_.ErrorDetails.Message) {
            return $_.ErrorDetails.Message
        }
        throw
    }
}

$status = Invoke-RestMethod -Method Get -Uri "$baseUrl/api/assistant/status" -TimeoutSec 4
if ($status.configured -ne $false) {
    throw "Assistant key must be empty after server startup."
}
if ($status.model -ne "qwen3.6-flash") {
    throw "Unexpected assistant model: $($status.model)"
}

$missingKey = Get-ErrorBody {
    Invoke-RestMethod -Method Post -Uri "$baseUrl/api/assistant/chat" `
        -ContentType "application/json" -Body '{"message":"查一下库存低于20的商品"}' -TimeoutSec 4
}
if ($missingKey -notmatch "API Key") {
    throw "Chat without a key must explain that the API key is missing."
}

$configured = Invoke-RestMethod -Method Post -Uri "$baseUrl/api/assistant/key" `
    -ContentType "application/json" -Body (@{ api_key = $fakeKey } | ConvertTo-Json -Compress) -TimeoutSec 4
if ($configured.configured -ne $true) {
    throw "Assistant key was not accepted."
}

$statusAfter = Invoke-RestMethod -Method Get -Uri "$baseUrl/api/assistant/status" -TimeoutSec 4
if ($statusAfter.configured -ne $true) {
    throw "Assistant status did not retain the in-memory key."
}

$responses = @(
    ($configured | ConvertTo-Json -Compress),
    ($statusAfter | ConvertTo-Json -Compress)
)
if (($responses -join "`n") -match [regex]::Escape($fakeKey)) {
    throw "Assistant API must never echo the API key."
}

Write-Host "assistant API integration test passed"
