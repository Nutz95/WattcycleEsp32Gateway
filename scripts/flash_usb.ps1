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

# Prefer persistent User env over a polluted process shell (e.g. WIFI_SSID=x leftovers).
foreach ($name in @('WIFI_SSID', 'WIFI_PASS', 'BMS_BLE_ADDRESS', 'ESPNOW_PMK', 'ESPNOW_BRIDGE_MAC')) {
  $userVal = [Environment]::GetEnvironmentVariable($name, 'User')
  if (-not [string]::IsNullOrWhiteSpace($userVal)) {
    Set-Item -Path "Env:$name" -Value $userVal
  }
}

if (-not $env:WIFI_SSID -or -not $env:WIFI_PASS) {
  Write-Error "WIFI_SSID and WIFI_PASS environment variables must be set before building."
}
if ($env:WIFI_SSID.Length -lt 2 -or $env:WIFI_SSID -in @('x', 'placeholder', 'YourWifiName')) {
  Write-Error "WIFI_SSID='$env:WIFI_SSID' looks invalid. Set your User env WIFI_SSID (real AP name) and retry."
}
if (-not [string]::IsNullOrWhiteSpace($env:ESPNOW_PMK) -and
    [string]::IsNullOrWhiteSpace($env:ESPNOW_BRIDGE_MAC)) {
  Write-Warning "ESPNOW_PMK is set but ESPNOW_BRIDGE_MAC is empty -- ESP-NOW stays disabled (prevents boot loops)."
}
Write-Host "Build credentials: WIFI_SSID='$env:WIFI_SSID' BMS='$env:BMS_BLE_ADDRESS' BRIDGE='$env:ESPNOW_BRIDGE_MAC'" -ForegroundColor DarkGray

if (-not $env:BMS_BLE_ADDRESS) {
  $env:BMS_BLE_ADDRESS = [Environment]::GetEnvironmentVariable("BMS_BLE_ADDRESS", "User")
}
if (-not $env:BMS_BLE_ADDRESS) {
  Write-Error "BMS_BLE_ADDRESS must be set (BLE MAC of the Wattcycle pack)."
}

Write-Host "==> Bundle web assets" -ForegroundColor Cyan
& "$PSScriptRoot\bundle_web.ps1"
if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Build + upload firmware on $Port" -ForegroundColor Cyan
pio run -e ttgo-tdisplay -t upload --upload-port $Port
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Upload LittleFS web assets" -ForegroundColor Cyan
pio run -e ttgo-tdisplay -t uploadfs --upload-port $Port
exit $LASTEXITCODE
