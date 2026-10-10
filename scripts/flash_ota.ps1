<#
.SYNOPSIS
  Builds the M5 hub firmware and uploads it over ArduinoOTA.

.PARAMETER Hostname
  OTA hostname (default: wattcycle-gateway). Must match OTA_HOSTNAME build flag.

.PARAMETER Ip
  Optional IP address if mDNS hostname resolution fails on Windows.

.NOTES
  Firmware OTA does NOT refresh LittleFS web assets. After changing web/, also run:
    .\scripts\flash.ps1 -Target HubM5 -Port COM23
  (or at least `pio run -e hub_m5_wattcycle -t uploadfs`).
#>
[CmdletBinding()]
param(
  [string]$Hostname = "wattcycle-gateway",
  [string]$Ip = ""
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

Import-UserEnv @(
  "WIFI_SSID", "WIFI_PASS", "ESPNOW_PMK", "ESPNOW_BRIDGE_MAC", "ESPNOW_BMS_BRIDGE_MAC",
  "ESPNOW_ECOFLOW_BRIDGE_MAC"
)

if (-not $env:WIFI_SSID -or -not $env:WIFI_PASS) {
  Write-Error "WIFI_SSID and WIFI_PASS environment variables must be set before building."
}

Write-Host "==> Bundle web assets (embedded in next USB LittleFS flash; firmware OTA alone skips FS)" -ForegroundColor DarkGray
& "$PSScriptRoot\bundle_web.ps1"
if ($null -ne $LASTEXITCODE -and $LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Building hub_m5_wattcycle" -ForegroundColor Cyan
pio run -e hub_m5_wattcycle
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$target = if ($Ip) { $Ip } else { $Hostname }
# Windows often fails espota when host_ip=0.0.0.0 — pin the LAN interface.
$hostIp = ""
Get-NetIPAddress -AddressFamily IPv4 -ErrorAction SilentlyContinue |
  Where-Object { $_.IPAddress -match '^192\.168\.' -and $_.PrefixOrigin -ne 'WellKnown' } |
  Select-Object -First 1 | ForEach-Object { $hostIp = $_.IPAddress }

$fw = Join-Path $repoRoot ".pio\build\hub_m5_wattcycle\firmware.bin"
if (-not (Test-Path $fw)) {
  Write-Error "Missing firmware image: $fw"
}
$espota = Get-ChildItem "$env:USERPROFILE\.platformio" -Recurse -Filter espota.py -ErrorAction SilentlyContinue |
  Select-Object -First 1 -ExpandProperty FullName
if (-not $espota) {
  Write-Error "espota.py not found under ~/.platformio"
}

Write-Host "==> OTA upload to $target (host_ip=$hostIp)" -ForegroundColor Cyan
$otaArgs = @("-i", $target, "-p", "3232", "-f", $fw, "-r")
if ($hostIp) { $otaArgs += @("--host_ip", $hostIp) }
python $espota @otaArgs
exit $LASTEXITCODE
