<#
.SYNOPSIS
  First-time ESP-NOW pairing: read both board MACs, set env vars, optionally flash both.

.DESCRIPTION
  Two boards, two roles - nothing else to memorize:

    BRIDGE = XT369P solar ESP (usually COM22)
    HUB    = Wattcycle BMS ESP (usually COM19)

    BRIDGE  --ESP-NOW-->  HUB

    ESPNOW_PEER_MAC   (on BRIDGE) = HUB MAC
    ESPNOW_BRIDGE_MAC (on HUB)    = BRIDGE MAC
    ESPNOW_PMK        (on BOTH)   = same 32-char hex secret

  This script reads MACs with esptool (USB), writes User env vars, and can flash both repos.

.EXAMPLE
  .\scripts\pair_espnow_link.ps1
.EXAMPLE
  .\scripts\pair_espnow_link.ps1 -HubPort COM19 -BridgePort COM22 -Flash
#>
[CmdletBinding()]
param(
  [string]$HubPort = "COM19",
  [string]$BridgePort = "COM22",
  [string]$HubRepo = "",
  [string]$BridgeRepo = "",
  [switch]$Flash,
  [switch]$RegeneratePmk
)

$ErrorActionPreference = "Stop"

function Resolve-RepoRoots {
  param([string]$Hub, [string]$Bridge)
  $here = Split-Path -Parent $PSScriptRoot
  $parent = Split-Path -Parent $here
  if ([string]::IsNullOrWhiteSpace($Hub)) {
    if (Test-Path (Join-Path $here "src\Bms")) { $Hub = $here }
    else { $Hub = Join-Path $parent "WattcycleEsp32Gateway" }
  }
  if ([string]::IsNullOrWhiteSpace($Bridge)) {
    if (Test-Path (Join-Path $here "src\Meter")) { $Bridge = $here }
    else { $Bridge = Join-Path $parent "XT369P_SPP_Gateway" }
  }
  if (-not (Test-Path (Join-Path $Hub "scripts\flash_usb.ps1"))) {
    throw "Wattcycle hub repo not found: $Hub"
  }
  if (-not (Test-Path (Join-Path $Bridge "scripts\flash_usb.ps1"))) {
    throw "XT369P bridge repo not found: $Bridge"
  }
  return @{ Hub = $Hub; Bridge = $Bridge }
}

function Get-EsptoolPath {
  $path = Join-Path $env:USERPROFILE ".platformio\packages\tool-esptoolpy\esptool.py"
  if (-not (Test-Path $path)) {
    throw "esptool.py not found at $path (install PlatformIO / build once first)."
  }
  return $path
}

function Read-Esp32StaMac {
  param([Parameter(Mandatory)][string]$Port, [Parameter(Mandatory)][string]$Label)
  Write-Host "Reading $Label STA MAC on $Port ..." -ForegroundColor Cyan
  $esptool = Get-EsptoolPath
  $raw = & python $esptool --chip esp32 --port $Port read_mac 2>&1 | Out-String
  if ($raw -notmatch 'MAC:\s*([0-9A-Fa-f]{2}(?::[0-9A-Fa-f]{2}){5})') {
    throw "Could not read MAC on $Port ($Label). Is USB connected? Output:`n$raw"
  }
  return $Matches[1].ToUpperInvariant()
}

function Set-UserEnv {
  param([Parameter(Mandatory)][string]$Name, [Parameter(Mandatory)][string]$Value)
  [Environment]::SetEnvironmentVariable($Name, $Value, "User")
  Set-Item -Path "Env:$Name" -Value $Value
}

function Get-OrCreatePmk {
  param([switch]$ForceNew)
  $existing = [Environment]::GetEnvironmentVariable("ESPNOW_PMK", "User")
  if (-not $ForceNew -and -not [string]::IsNullOrWhiteSpace($existing) -and $existing.Length -ge 16) {
    Write-Host "Reusing existing User ESPNOW_PMK ($($existing.Length) chars)." -ForegroundColor DarkGray
    return $existing
  }
  $buffer = New-Object byte[] 16
  $rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
  try { $rng.GetBytes($buffer) } finally { $rng.Dispose() }
  $pmk = -join ($buffer | ForEach-Object { $_.ToString("x2") })
  Write-Host "Generated new ESPNOW_PMK." -ForegroundColor Green
  return $pmk
}

$roots = Resolve-RepoRoots -Hub $HubRepo -Bridge $BridgeRepo

Write-Host ""
Write-Host "ESP-NOW pairing (first-time setup)" -ForegroundColor Cyan
Write-Host @"

  BRIDGE = XT369P solar ESP     ($BridgePort)
  HUB    = Wattcycle BMS ESP    ($HubPort)

  BRIDGE  ===== ESP-NOW =====>  HUB

  On BRIDGE:  ESPNOW_PEER_MAC   = HUB MAC
  On HUB:     ESPNOW_BRIDGE_MAC = BRIDGE MAC
  On BOTH:    ESPNOW_PMK        = same secret

"@

if ($HubPort -eq $BridgePort) {
  throw "HubPort and BridgePort must be different (got $HubPort for both)."
}

$hubMac = Read-Esp32StaMac -Port $HubPort -Label "HUB (Wattcycle)"
$bridgeMac = Read-Esp32StaMac -Port $BridgePort -Label "BRIDGE (XT369P)"
$pmk = Get-OrCreatePmk -ForceNew:$RegeneratePmk

Write-Host ""
Write-Host "Discovered:" -ForegroundColor Yellow
Write-Host "  HUB    (Wattcycle) MAC = $hubMac"
Write-Host "  BRIDGE (XT369P)   MAC = $bridgeMac"
Write-Host ""

Set-UserEnv -Name "ESPNOW_PEER_MAC" -Value $hubMac
Set-UserEnv -Name "ESPNOW_BRIDGE_MAC" -Value $bridgeMac
Set-UserEnv -Name "ESPNOW_PMK" -Value $pmk

Write-Host "Saved Windows User env:" -ForegroundColor Green
Write-Host "  ESPNOW_PEER_MAC   = $hubMac     (for XT369P bridge flash)"
Write-Host "  ESPNOW_BRIDGE_MAC = $bridgeMac  (for Wattcycle hub flash)"
Write-Host "  ESPNOW_PMK        = (32-char secret, not printed again here)"
Write-Host ""

if (-not $Flash) {
  Write-Host "Next: flash both boards (env is already set):" -ForegroundColor Yellow
  Write-Host "  cd $($roots.Hub);    .\scripts\flash_usb.ps1 -Port $HubPort"
  Write-Host "  cd $($roots.Bridge); .\scripts\flash_usb.ps1 -Port $BridgePort"
  Write-Host ""
  Write-Host "Or re-run with -Flash to do both now." -ForegroundColor DarkGray
  exit 0
}

Write-Host "==> Flashing HUB (Wattcycle) on $HubPort" -ForegroundColor Cyan
& (Join-Path $roots.Hub "scripts\flash_usb.ps1") -Port $HubPort
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host "==> Flashing BRIDGE (XT369P) on $BridgePort" -ForegroundColor Cyan
& (Join-Path $roots.Bridge "scripts\flash_usb.ps1") -Port $BridgePort
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

Write-Host ""
Write-Host "Done. Serial should show enc=1 on both boards." -ForegroundColor Green
Write-Host "  HUB:    ESP-NOW RX listening (enc=1)"
Write-Host "  BRIDGE: ESP-NOW TX peer ... enc=1"
exit 0
