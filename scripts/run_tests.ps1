<#
.SYNOPSIS
  Runs PlatformIO native unit tests, guardrails, and firmware compile smoke.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

Write-Host "==> Guardrails" -ForegroundColor Cyan
& "$PSScriptRoot\check_guardrails.ps1"
if ($LASTEXITCODE -ne 0) {
  exit $LASTEXITCODE
}

Write-Host "==> Unit tests (native - hub parsers/auth/codec)" -ForegroundColor Cyan
pio test -e native
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Firmware compile smoke (hub_m5 + bridges; hub_ttgo legacy; ecoflow S3)" -ForegroundColor Cyan
foreach ($name in @(
    "WIFI_SSID", "WIFI_PASS", "BMS_BLE_ADDRESS", "ESPNOW_PEER_MAC", "ESPNOW_BRIDGE_MAC",
    "ESPNOW_BMS_BRIDGE_MAC", "ESPNOW_PMK", "XT369P_BT_ADDRESS", "ESPNOW_CHANNEL",
    "ECOFLOW_BLE_ADDRESS", "ECOFLOW_SERIAL", "ECOFLOW_USER_ID"
  )) {
  $userVal = [Environment]::GetEnvironmentVariable($name, "User")
  if (-not [string]::IsNullOrWhiteSpace($userVal)) {
    Set-Item -Path "Env:$name" -Value $userVal
  }
}
if (-not $env:WIFI_SSID) { $env:WIFI_SSID = "buildcheck" }
if (-not $env:WIFI_PASS) { $env:WIFI_PASS = "buildcheck" }
if (-not $env:BMS_BLE_ADDRESS) { $env:BMS_BLE_ADDRESS = "AA:BB:CC:DD:EE:FF" }
if (-not $env:ESPNOW_PEER_MAC) { $env:ESPNOW_PEER_MAC = "AA:BB:CC:DD:EE:FF" }
if (-not $env:ESPNOW_BMS_BRIDGE_MAC) { $env:ESPNOW_BMS_BRIDGE_MAC = "AA:BB:CC:DD:EE:01" }
if ($null -eq $env:ESPNOW_BRIDGE_MAC) { $env:ESPNOW_BRIDGE_MAC = "AA:BB:CC:DD:EE:02" }
if ($null -eq $env:ESPNOW_PMK) { $env:ESPNOW_PMK = "" }
if ($null -eq $env:XT369P_BT_ADDRESS) { $env:XT369P_BT_ADDRESS = "" }
if ($null -eq $env:ESPNOW_CHANNEL) { $env:ESPNOW_CHANNEL = "" }
if ($null -eq $env:ECOFLOW_BLE_ADDRESS) { $env:ECOFLOW_BLE_ADDRESS = "" }
if ($null -eq $env:ECOFLOW_SERIAL) { $env:ECOFLOW_SERIAL = "" }
if ($null -eq $env:ECOFLOW_USER_ID) { $env:ECOFLOW_USER_ID = "" }

pio run -e hub_m5_wattcycle -e bridge_ttgo_wattcycle -e bridge_xt369p -e bridge_ecoflow_delta3 -e hub_ttgo_wattcycle
exit $LASTEXITCODE

