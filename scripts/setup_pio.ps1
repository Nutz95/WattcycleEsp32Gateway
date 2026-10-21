<#
.SYNOPSIS
  One-shot PlatformIO package install for every firmware env.

.DESCRIPTION
  flash.ps1 builds on demand and will otherwise download toolchains, Arduino
  cores, and libraries the first time an env is used. On a fresh PC that
  download can stall for a long time (Arduino-ESP32 is hundreds of MB).
  Run this once after cloning, before the first flash.

  Dongle envs use the already-installed espressif32 platform plus esptool 4.9.
  Do not pin espressif32@6.5.0: that pulls Arduino core 2.0.14 (~230 MB) and
  the in-tree SPI register fix already covers the ST7735 on newer cores.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

if (-not (Get-Command pio -ErrorAction SilentlyContinue)) {
  Write-Error "PlatformIO Core (pio) is not on PATH. Install it, then re-run this script."
}

Write-Host "==> Installing PlatformIO packages for all envs (first run can take a while)" -ForegroundColor Cyan
pio pkg install
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "PlatformIO packages ready. Next: set env vars, then .\scripts\flash.ps1" -ForegroundColor Green
