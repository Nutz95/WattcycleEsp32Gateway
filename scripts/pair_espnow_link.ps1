<#
.SYNOPSIS
  Auto-detect STA MACs on the three Phase 2 boards, write User env, optionally flash All.

.DESCRIPTION
  HUB         = M5Stack Basic (usually COM23) — env: hub_m5_wattcycle
  BMS_BRIDGE  = TTGO Wattcycle BLE (usually COM19) — env: bridge_ttgo_wattcycle
  XT_BRIDGE   = TTGO XT369P SPP (usually COM22) — env: bridge_xt369p

  Both bridges TX ESP-NOW → HUB.

  ESPNOW_PEER_MAC        (on BOTH bridges) = HUB MAC
  ESPNOW_BMS_BRIDGE_MAC  (on HUB)          = BMS bridge MAC
  ESPNOW_BRIDGE_MAC      (on HUB)          = XT bridge MAC
  ESPNOW_PMK             (on ALL three)    = same 32-char hex secret

.EXAMPLE
  .\scripts\pair_espnow_link.ps1 -Flash
.EXAMPLE
  .\scripts\pair_espnow_link.ps1 -HubPort COM23 -BmsBridgePort COM19 -XtBridgePort COM22 -Flash
#>
[CmdletBinding()]
param(
  [string]$HubPort = "COM23",
  [string]$BmsBridgePort = "COM19",
  [string]$XtBridgePort = "COM22",
  # Legacy Phase 1 aliases
  [string]$BridgePort = "",
  [switch]$Flash,
  [switch]$RegeneratePmk,
  [switch]$LegacyTwoBoard
)

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
Set-Location $repoRoot

if ($BridgePort) {
  $XtBridgePort = $BridgePort
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

Write-Host ""
Write-Host "ESP-NOW pairing (auto MAC detect)" -ForegroundColor Cyan

if ($LegacyTwoBoard) {
  Write-Host @"

  Legacy two-board mode (hub_ttgo + XT only):
  HUB=$HubPort  XT=$XtBridgePort

"@
  if ($HubPort -eq $XtBridgePort) {
    throw "HubPort and XtBridgePort must differ."
  }
  $hubMac = Read-Esp32StaMac -Port $HubPort -Label "HUB"
  $xtMac = Read-Esp32StaMac -Port $XtBridgePort -Label "XT bridge"
  $pmk = Get-OrCreatePmk -ForceNew:$RegeneratePmk
  Set-UserEnv -Name "ESPNOW_PEER_MAC" -Value $hubMac
  Set-UserEnv -Name "ESPNOW_BRIDGE_MAC" -Value $xtMac
  Set-UserEnv -Name "ESPNOW_PMK" -Value $pmk
  Write-Host "Saved: PEER=$hubMac  BRIDGE(XT)=$xtMac" -ForegroundColor Green
  if ($Flash) {
    & "$PSScriptRoot\flash.ps1" -Target Both -HubPort $HubPort -BridgePort $XtBridgePort
    exit $LASTEXITCODE
  }
  exit 0
}

Write-Host @"

  HUB        = M5Stack Basic     ($HubPort)  [hub_m5_wattcycle]
  BMS_BRIDGE = TTGO Wattcycle    ($BmsBridgePort)  [bridge_ttgo_wattcycle]
  XT_BRIDGE  = TTGO XT369P       ($XtBridgePort)  [bridge_xt369p]

  BMS + XT  ===== ESP-NOW =====>  HUB

"@

$ports = @($HubPort, $BmsBridgePort, $XtBridgePort)
if (($ports | Select-Object -Unique).Count -ne 3) {
  throw "HubPort, BmsBridgePort and XtBridgePort must be three different COM ports."
}

$hubMac = Read-Esp32StaMac -Port $HubPort -Label "HUB (M5)"
$bmsMac = Read-Esp32StaMac -Port $BmsBridgePort -Label "BMS bridge (TTGO)"
$xtMac = Read-Esp32StaMac -Port $XtBridgePort -Label "XT bridge (TTGO)"
$pmk = Get-OrCreatePmk -ForceNew:$RegeneratePmk

Write-Host ""
Write-Host "Discovered:" -ForegroundColor Yellow
Write-Host "  HUB        MAC = $hubMac"
Write-Host "  BMS_BRIDGE MAC = $bmsMac"
Write-Host "  XT_BRIDGE  MAC = $xtMac"
Write-Host ""

# Bridges peer the hub; hub peers both bridges.
Set-UserEnv -Name "ESPNOW_PEER_MAC" -Value $hubMac
Set-UserEnv -Name "ESPNOW_BMS_BRIDGE_MAC" -Value $bmsMac
Set-UserEnv -Name "ESPNOW_BRIDGE_MAC" -Value $xtMac
Set-UserEnv -Name "ESPNOW_PMK" -Value $pmk

Write-Host "Saved Windows User env:" -ForegroundColor Green
Write-Host "  ESPNOW_PEER_MAC       = $hubMac   (both bridges)"
Write-Host "  ESPNOW_BMS_BRIDGE_MAC = $bmsMac   (M5 hub)"
Write-Host "  ESPNOW_BRIDGE_MAC     = $xtMac    (M5 hub, XT peer)"
Write-Host "  ESPNOW_PMK            = (32-char secret, not printed)"
Write-Host ""

if (-not $Flash) {
  Write-Host "Next:" -ForegroundColor Yellow
  Write-Host "  .\scripts\flash.ps1 -Target All -HubPort $HubPort -BmsBridgePort $BmsBridgePort -XtBridgePort $XtBridgePort"
  Write-Host "Or re-run with -Flash." -ForegroundColor DarkGray
  exit 0
}

Write-Host "==> Flashing all three targets" -ForegroundColor Cyan
& "$PSScriptRoot\flash.ps1" -Target All -HubPort $HubPort -BmsBridgePort $BmsBridgePort -XtBridgePort $XtBridgePort
exit $LASTEXITCODE
