<#
.SYNOPSIS
  Flash one or both monorepo firmwares (distinct COM ports).

.EXAMPLE
  .\scripts\flash.ps1 -Target HubTtgoWattcycle -Port COM19
.EXAMPLE
  .\scripts\flash.ps1 -Target BridgeXt369p -Port COM22
.EXAMPLE
  .\scripts\flash.ps1 -Target Both -HubPort COM19 -BridgePort COM22
#>
[CmdletBinding()]
param(
  [Parameter(Mandatory)]
  [ValidateSet("HubTtgoWattcycle", "BridgeXt369p", "Both")]
  [string]$Target,

  [string]$Port = "",
  [string]$HubPort = "COM19",
  [string]$BridgePort = "COM22"
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

function Import-UserEnv {
  param([string[]]$Names)
  foreach ($name in $Names) {
    $userVal = [Environment]::GetEnvironmentVariable($name, "User")
    if (-not [string]::IsNullOrWhiteSpace($userVal)) {
      Set-Item -Path "Env:$name" -Value $userVal
    }
  }
}

function Flash-HubTtgoWattcycle {
  param([string]$UploadPort)
  Import-UserEnv @("WIFI_SSID", "WIFI_PASS", "BMS_BLE_ADDRESS", "ESPNOW_PMK", "ESPNOW_BRIDGE_MAC")
  if (-not $env:WIFI_SSID -or -not $env:WIFI_PASS) {
    Write-Error "WIFI_SSID and WIFI_PASS must be set before building the hub."
  }
  if ($env:WIFI_SSID.Length -lt 2 -or $env:WIFI_SSID -in @("x", "placeholder", "YourWifiName")) {
    Write-Error "WIFI_SSID='$env:WIFI_SSID' looks invalid."
  }
  if (-not $env:BMS_BLE_ADDRESS) {
    Write-Error "BMS_BLE_ADDRESS must be set (BLE MAC of the Wattcycle pack)."
  }
  if (-not [string]::IsNullOrWhiteSpace($env:ESPNOW_PMK) -and
      [string]::IsNullOrWhiteSpace($env:ESPNOW_BRIDGE_MAC)) {
    Write-Warning "ESPNOW_PMK set but ESPNOW_BRIDGE_MAC empty -- ESP-NOW stays disabled."
  }
  Write-Host "Hub build: WIFI_SSID='$env:WIFI_SSID' BMS='$env:BMS_BLE_ADDRESS' BRIDGE='$env:ESPNOW_BRIDGE_MAC'" -ForegroundColor DarkGray

  Write-Host "==> Bundle web assets" -ForegroundColor Cyan
  & "$PSScriptRoot\bundle_web.ps1"
  if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Write-Host "==> Build + upload hub firmware on $UploadPort" -ForegroundColor Cyan
  pio run -e hub_ttgo_wattcycle -t upload --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

  Write-Host "==> Upload LittleFS web assets" -ForegroundColor Cyan
  pio run -e hub_ttgo_wattcycle -t uploadfs --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

function Flash-BridgeXt369p {
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
  Write-Host "Bridge build: WIFI_SSID='$env:WIFI_SSID' ESPNOW_PEER_MAC='$env:ESPNOW_PEER_MAC'" -ForegroundColor DarkGray

  Write-Host "==> Build + upload bridge firmware on $UploadPort" -ForegroundColor Cyan
  pio run -e bridge_xt369p -t upload --upload-port $UploadPort
  if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

switch ($Target) {
  "HubTtgoWattcycle" {
    $p = if ($Port) { $Port } else { $HubPort }
    Flash-HubTtgoWattcycle -UploadPort $p
  }
  "BridgeXt369p" {
    $p = if ($Port) { $Port } else { $BridgePort }
    Flash-BridgeXt369p -UploadPort $p
  }
  "Both" {
    if ($HubPort -eq $BridgePort) {
      Write-Error "HubPort and BridgePort must differ (got $HubPort)."
    }
    Flash-HubTtgoWattcycle -UploadPort $HubPort
    Flash-BridgeXt369p -UploadPort $BridgePort
  }
}

Write-Host "Flash complete ($Target)." -ForegroundColor Green
exit 0
