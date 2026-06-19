$ErrorActionPreference = "Stop"

$root = Split-Path -Parent $PSScriptRoot
$launcherName = (-join [char[]](0x542F, 0x52A8, 0x7CFB, 0x7EDF)) + ".exe"
$launcher = Join-Path $root $launcherName
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

if (-not (Test-Path -LiteralPath $launcher)) {
    throw "Portable launcher is missing: $launcher"
}

if (Test-ApiAvailable) {
    throw "Port 8080 is already in use. Stop the running system before executing this test."
}

$env:STATIONERY_SKIP_BROWSER = "1"

try {
    $first = Start-Process -FilePath $launcher -WorkingDirectory $root -PassThru
    if (-not $first.WaitForExit(5000)) {
        throw "First launcher process did not exit."
    }
    Wait-ForApi $true

    $second = Start-Process -FilePath $launcher -WorkingDirectory $root -PassThru
    if (-not $second.WaitForExit(5000)) {
        throw "Second launcher process did not exit."
    }
    Wait-ForApi $true

    $response = Invoke-RestMethod -Method Post -Uri "$baseUrl/api/system/shutdown" -TimeoutSec 2
    if ($response.message -ne "System stopped") {
        throw "Unexpected shutdown response: $($response | ConvertTo-Json -Compress)"
    }
    Wait-ForApi $false

    Write-Host "portable launcher integration test passed"
} finally {
    Remove-Item Env:STATIONERY_SKIP_BROWSER -ErrorAction SilentlyContinue
    if (Test-ApiAvailable) {
        try {
            Invoke-RestMethod -Method Post -Uri "$baseUrl/api/system/shutdown" -TimeoutSec 2 | Out-Null
            Wait-ForApi $false
        } catch {
            Write-Warning "Could not stop test server automatically: $($_.Exception.Message)"
        }
    }
}
