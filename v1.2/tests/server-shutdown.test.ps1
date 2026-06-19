$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$server = Join-Path $root "backend\server.exe"
$baseUrl = "http://localhost:8080"

function Test-ApiAvailable {
    try {
        $response = Invoke-WebRequest -UseBasicParsing -Uri "$baseUrl/api/products" -TimeoutSec 4
        return $response.StatusCode -eq 200
    } catch {
        return $false
    }
}

function Wait-ForApi([bool]$available, [int]$timeoutMs = 10000) {
    $deadline = [DateTime]::UtcNow.AddMilliseconds($timeoutMs)
    do {
        if ((Test-ApiAvailable) -eq $available) {
            return
        }
        Start-Sleep -Milliseconds 100
    } while ([DateTime]::UtcNow -lt $deadline)

    throw "Timed out waiting for API available=$available"
}

if (Test-ApiAvailable) {
    throw "Port 8080 is already in use. Stop the running system before executing this test."
}

$process = Start-Process -FilePath $server -WorkingDirectory $root -WindowStyle Hidden -PassThru

try {
    Wait-ForApi $true
    $response = Invoke-RestMethod -Method Post -Uri "$baseUrl/api/system/shutdown" -TimeoutSec 2

    if ($response.message -ne "System stopped") {
        throw "Unexpected shutdown response: $($response | ConvertTo-Json -Compress)"
    }

    if (-not $process.WaitForExit(3000)) {
        throw "Server process did not exit after shutdown request."
    }

    Wait-ForApi $false
    Write-Host "server shutdown integration test passed"
} finally {
    if (-not $process.HasExited) {
        Stop-Process -Id $process.Id -Force
    }
}
