<#
.SYNOPSIS
  Builds firmware and uploads it over ArduinoOTA (no USB cable required after first flash).

.PARAMETER Hostname
  OTA hostname (default: wattcycle-gateway). Must match OTA_HOSTNAME build flag.

.PARAMETER Ip
  Optional IP address if mDNS hostname resolution fails on Windows.
#>
[CmdletBinding()]
param(
  [string]$Hostname = "wattcycle-gateway",
  [string]$Ip = ""
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

if (-not $env:WIFI_SSID -or -not $env:WIFI_PASS) {
  Write-Error "WIFI_SSID and WIFI_PASS environment variables must be set before building."
}

Write-Host "==> Building firmware" -ForegroundColor Cyan
pio run -e hub_ttgo_wattcycle
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

$target = if ($Ip) { $Ip } else { $Hostname }
Write-Host "==> OTA upload to $target" -ForegroundColor Cyan
pio run -e hub_ttgo_wattcycle -t upload --upload-port $target
exit $LASTEXITCODE
