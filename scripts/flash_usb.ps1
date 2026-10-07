<#
.SYNOPSIS
  Builds firmware + LittleFS image and flashes over USB (COM19 by default).
#>
[CmdletBinding()]
param(
  [string]$Port = "COM19"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

if (-not $env:WIFI_SSID -or -not $env:WIFI_PASS) {
  Write-Error "WIFI_SSID and WIFI_PASS environment variables must be set before building."
}

Write-Host "==> Build + upload firmware on $Port" -ForegroundColor Cyan
pio run -e ttgo-tdisplay -t upload --upload-port $Port
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Upload LittleFS web assets" -ForegroundColor Cyan
pio run -e ttgo-tdisplay -t uploadfs --upload-port $Port
exit $LASTEXITCODE
