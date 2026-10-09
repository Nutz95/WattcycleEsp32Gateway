<#
.SYNOPSIS
  Flash monorepo firmwares (distinct COM ports).

.EXAMPLE
  .\scripts\flash.ps1 -Target HubM5 -Port COM23
.EXAMPLE
  .\scripts\flash.ps1 -Target BridgeTtgoWattcycle -Port COM19
.EXAMPLE
  .\scripts\flash.ps1 -Target BridgeXt369p -Port COM22
.EXAMPLE
  .\scripts\flash.ps1 -Target BridgeEcoflowDelta3 -Port COM8
.EXAMPLE
  .\scripts\flash.ps1 -Target All -HubPort COM23 -BmsBridgePort COM19 -XtBridgePort COM22
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory)]
  [ValidateSet("HubM5", "HubTtgoWattcycle", "BridgeTtgoWattcycle", "BridgeXt369p", "BridgeEcoflowDelta3", "All", "Both")]
  [string]$Target,

  [string]$Port = "",
  [string]$HubPort = "COM23",
  [string]$BmsBridgePort = "COM19",
  [string]$XtBridgePort = "COM22",
  [string]$EcoflowBridgePort = "COM8",
  # Legacy alias used by Phase 1 Both target
  [string]$BridgePort = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

if ($BridgePort) {
  $XtBridgePort = $BridgePort
}

function Import-UserEnv {
  param([string[]]$Names)
  foreach ($name in $Names) {
    $userVal = [Environment]::GetEnvironmentVariable($name, "User")
    if (-not [string]::IsNullOrWhiteSpace($userVal)) {
      Set-Item -Path "Env:$name" -Value $userVal
    }
  }
}

function Ensure-EspNowMacEnv {
  param(
    [string]$HubPort,
    [string]$BmsBridgePort,
    [string]$XtBridgePort
  )
  Import-UserEnv @("ESPNOW_PEER_MAC", "ESPNOW_BMS_BRIDGE_MAC", "ESPNOW_BRIDGE_MAC", "ESPNOW_PMK")
  $needPair = [string]::IsNullOrWhiteSpace($env:ESPNOW_PEER_MAC) -or
              [string]::IsNullOrWhiteSpace($env:ESPNOW_BMS_BRIDGE_MAC) -or
              [string]::IsNullOrWhiteSpace($env:ESPNOW_BRIDGE_MAC) -or
              [string]::IsNullOrWhiteSpace($env:ESPNOW_PMK)
  if (-not $needPair) {
    return
  }
  Write-Host "==> ESP-NOW MACs incomplete -- auto-detecting via pair_espnow_link.ps1" -ForegroundColor Yellow
  & "$PSScriptRoot\pair_espnow_link.ps1" -HubPort $HubPort -BmsBridgePort $BmsBridgePort `
    -XtBridgePort $XtBridgePort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  Import-UserEnv @("ESPNOW_PEER_MAC", "ESPNOW_BMS_BRIDGE_MAC", "ESPNOW_BRIDGE_MAC", "ESPNOW_PMK")
}

function Invoke-FlashHubM5 {
  param([string]$UploadPort)
  Import-UserEnv @(
    "WIFI_SSID", "WIFI_PASS", "ESPNOW_PMK", "ESPNOW_BRIDGE_MAC", "ESPNOW_BMS_BRIDGE_MAC",
    "ESPNOW_ECOFLOW_BRIDGE_MAC"
  )
  if (-not $env:WIFI_SSID -or -not $env:WIFI_PASS) {
    Write-Error "WIFI_SSID and WIFI_PASS must be set before building the M5 hub."
  }
  Write-Host ("M5 hub: WIFI_SSID='$env:WIFI_SSID' BMS='$env:ESPNOW_BMS_BRIDGE_MAC' " +
    "XT='$env:ESPNOW_BRIDGE_MAC' ECO='$env:ESPNOW_ECOFLOW_BRIDGE_MAC'") -ForegroundColor DarkGray
  if (-not $env:ESPNOW_ECOFLOW_BRIDGE_MAC) {
    Write-Warning "ESPNOW_ECOFLOW_BRIDGE_MAC unset - EcoFlow ESP-NOW peer not registered (flash S3 first, copy STA MAC)."
  }

  Write-Host "==> Bundle web assets" -ForegroundColor Cyan
  & "$PSScriptRoot\bundle_web.ps1"
  if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Write-Host "==> Build + upload hub_m5 on $UploadPort" -ForegroundColor Cyan
  pio run -e hub_m5_wattcycle -t upload --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Write-Host "==> Upload LittleFS web assets" -ForegroundColor Cyan
  pio run -e hub_m5_wattcycle -t uploadfs --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Invoke-FlashHubTtgoWattcycle {
  param([string]$UploadPort)
  Import-UserEnv @("WIFI_SSID", "WIFI_PASS", "BMS_BLE_ADDRESS", "ESPNOW_PMK", "ESPNOW_BRIDGE_MAC")
  if (-not $env:WIFI_SSID -or -not $env:WIFI_PASS) {
    Write-Error "WIFI_SSID and WIFI_PASS must be set before building the hub."
  }
  if (-not $env:BMS_BLE_ADDRESS) {
    Write-Error "BMS_BLE_ADDRESS must be set (BLE MAC of the Wattcycle pack)."
  }
  Write-Host "==> Bundle web assets" -ForegroundColor Cyan
  & "$PSScriptRoot\bundle_web.ps1"
  if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  pio run -e hub_ttgo_wattcycle -t upload --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
  pio run -e hub_ttgo_wattcycle -t uploadfs --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Invoke-FlashBridgeTtgoWattcycle {
  param([string]$UploadPort)
  Import-UserEnv @("WIFI_SSID", "BMS_BLE_ADDRESS", "ESPNOW_PEER_MAC", "ESPNOW_PMK", "ESPNOW_CHANNEL")
  if (-not $env:WIFI_SSID) {
    Write-Error "WIFI_SSID must be set (channel discovery only on the bridge)."
  }
  if (-not $env:BMS_BLE_ADDRESS) {
    Write-Error "BMS_BLE_ADDRESS must be set."
  }
  if (-not $env:ESPNOW_PEER_MAC) {
    Write-Error "ESPNOW_PEER_MAC must be set to the M5 hub STA MAC."
  }
  Write-Host "==> Build + upload BMS bridge on $UploadPort" -ForegroundColor Cyan
  pio run -e bridge_ttgo_wattcycle -t upload --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Invoke-FlashBridgeXt369p {
  param([string]$UploadPort)
  Import-UserEnv @("WIFI_SSID", "ESPNOW_PEER_MAC", "ESPNOW_PMK", "ESPNOW_CHANNEL", "XT369P_BT_ADDRESS")
  if (-not $env:WIFI_SSID) {
    Write-Error "WIFI_SSID must be set (channel discovery only on the bridge)."
  }
  if (-not $env:ESPNOW_PEER_MAC) {
    Write-Error "ESPNOW_PEER_MAC must be set to the hub STA MAC."
  }
  if (-not $env:XT369P_BT_ADDRESS) {
    Write-Warning "XT369P_BT_ADDRESS unset -- firmware will connect by name XT369P_SPP."
    $env:XT369P_BT_ADDRESS = ""
  }
  if (-not $env:ESPNOW_PMK) {
    Write-Warning "ESPNOW_PMK unset -- ESP-NOW payloads stay plaintext."
    $env:ESPNOW_PMK = ""
  }
  Write-Host "==> Build + upload XT369P bridge on $UploadPort" -ForegroundColor Cyan
  pio run -e bridge_xt369p -t upload --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Invoke-FlashBridgeEcoflowDelta3 {
  param([string]$UploadPort)
  Import-UserEnv @(
    "WIFI_SSID",
    "ECOFLOW_BLE_ADDRESS",
    "ECOFLOW_SERIAL",
    "ECOFLOW_USER_ID",
    "ESPNOW_PEER_MAC",
    "ESPNOW_PMK",
    "ESPNOW_CHANNEL"
  )
  if (-not $env:WIFI_SSID) { $env:WIFI_SSID = "" }
  if (-not $env:ECOFLOW_BLE_ADDRESS) { $env:ECOFLOW_BLE_ADDRESS = "" }
  if (-not $env:ECOFLOW_SERIAL) { $env:ECOFLOW_SERIAL = "" }
  if (-not $env:ECOFLOW_USER_ID) { $env:ECOFLOW_USER_ID = "" }
  if (-not $env:ESPNOW_PEER_MAC) { $env:ESPNOW_PEER_MAC = "" }
  if (-not $env:ESPNOW_PMK) { $env:ESPNOW_PMK = "" }
  if (-not $env:ESPNOW_CHANNEL) { $env:ESPNOW_CHANNEL = "0" }

  if (-not $env:ESPNOW_PEER_MAC) {
    Write-Warning "ESPNOW_PEER_MAC unset - ESP-NOW TX skipped until hub STA MAC is set."
  }
  Write-Host "==> Build + upload EcoFlow DELTA 3 bridge (ESP32-S3) on $UploadPort" -ForegroundColor Cyan
  Write-Host "    Copy printed STA MAC into User env ESPNOW_ECOFLOW_BRIDGE_MAC, then reflash HubM5." -ForegroundColor DarkGray
  pio run -e bridge_ecoflow_delta3 -t upload --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

switch ($Target) {
  "HubM5" {
    $p = if ($Port) { $Port } else { $HubPort }
    Invoke-FlashHubM5 -UploadPort $p
  }
  "HubTtgoWattcycle" {
    $p = if ($Port) { $Port } else { "COM19" }
    Invoke-FlashHubTtgoWattcycle -UploadPort $p
  }
  "BridgeTtgoWattcycle" {
    $p = if ($Port) { $Port } else { $BmsBridgePort }
    Invoke-FlashBridgeTtgoWattcycle -UploadPort $p
  }
  "BridgeXt369p" {
    $p = if ($Port) { $Port } else { $XtBridgePort }
    Invoke-FlashBridgeXt369p -UploadPort $p
  }
  "BridgeEcoflowDelta3" {
    $p = if ($Port) { $Port } else { $EcoflowBridgePort }
    Invoke-FlashBridgeEcoflowDelta3 -UploadPort $p
  }
  "Both" {
    Write-Warning "Target Both is legacy (hub_ttgo + XT). Prefer -Target All for Phase 2."
    Invoke-FlashHubTtgoWattcycle -UploadPort $(if ($Port) { $Port } else { "COM19" })
    Invoke-FlashBridgeXt369p -UploadPort $XtBridgePort
  }
  "All" {
    Ensure-EspNowMacEnv -HubPort $HubPort -BmsBridgePort $BmsBridgePort -XtBridgePort $XtBridgePort
    Invoke-FlashHubM5 -UploadPort $HubPort
    Invoke-FlashBridgeTtgoWattcycle -UploadPort $BmsBridgePort
    Invoke-FlashBridgeXt369p -UploadPort $XtBridgePort
    Invoke-FlashBridgeEcoflowDelta3 -UploadPort $EcoflowBridgePort
  }
}

Write-Host "Flash complete ($Target)." -ForegroundColor Green
exit 0
