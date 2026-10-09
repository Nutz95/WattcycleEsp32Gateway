<#
.SYNOPSIS
  Auto-detect STA MACs on Phase 2 boards, write User env, optionally flash All.

.DESCRIPTION
  HUB             = M5Stack Basic (usually COM23) — env: hub_m5_wattcycle
  BMS_BRIDGE      = TTGO Wattcycle BLE (usually COM19) — env: bridge_ttgo_wattcycle
  XT_BRIDGE       = TTGO XT369P SPP (usually COM22) — env: bridge_xt369p
  ECOFLOW_BRIDGE  = ESP32-S3 EcoFlow DELTA 3 (usually COM8) — env: bridge_ecoflow_delta3

  All bridges TX ESP-NOW → HUB.

  ESPNOW_PEER_MAC             (on ALL bridges) = HUB MAC
  ESPNOW_BMS_BRIDGE_MAC       (on HUB)         = BMS bridge MAC
  ESPNOW_BRIDGE_MAC           (on HUB)         = XT bridge MAC
  ESPNOW_ECOFLOW_BRIDGE_MAC   (on HUB)         = EcoFlow S3 bridge MAC
  ESPNOW_PMK                  (on ALL)         = same 32-char hex secret

.EXAMPLE
  .\scripts\pair_espnow_link.ps1 -Flash
.EXAMPLE
  .\scripts\pair_espnow_link.ps1 -HubPort COM23 -BmsBridgePort COM19 -XtBridgePort COM22 -EcoflowBridgePort COM8 -Flash
#>
[CmdletBinding()]
param(
  [string]$HubPort = "COM23",
  [string]$BmsBridgePort = "COM19",
  [string]$XtBridgePort = "COM22",
  [string]$EcoflowBridgePort = "COM8",
  # Legacy Phase 1 aliases
  [string]$BridgePort = "",
  [switch]$Flash,
  [switch]$RegeneratePmk,
  [switch]$LegacyTwoBoard,
  [switch]$SkipEcoflow
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

function Read-EspStaMac {
  param(
    [Parameter(Mandatory)][string]$Port,
    [Parameter(Mandatory)][string]$Label,
    [ValidateSet("esp32", "esp32s3")]
    [string]$Chip = "esp32"
  )
  Write-Host "Reading $Label STA MAC on $Port ($Chip) ..." -ForegroundColor Cyan
  $esptool = Get-EsptoolPath
  $raw = & python $esptool --chip $Chip --port $Port read_mac 2>&1 | Out-String
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
  $hubMac = Read-EspStaMac -Port $HubPort -Label "HUB" -Chip esp32
  $xtMac = Read-EspStaMac -Port $XtBridgePort -Label "XT bridge" -Chip esp32
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

$includeEcoflow = -not $SkipEcoflow -and -not [string]::IsNullOrWhiteSpace($EcoflowBridgePort)

Write-Host @"

  HUB            = M5Stack Basic     ($HubPort)  [hub_m5_wattcycle]
  BMS_BRIDGE     = TTGO Wattcycle    ($BmsBridgePort)  [bridge_ttgo_wattcycle]
  XT_BRIDGE      = TTGO XT369P       ($XtBridgePort)  [bridge_xt369p]
$(if ($includeEcoflow) { "  ECOFLOW_BRIDGE = ESP32-S3 DELTA 3 ($EcoflowBridgePort)  [bridge_ecoflow_delta3]" } else { "  ECOFLOW_BRIDGE = (skipped)" })

  bridges  ===== ESP-NOW =====>  HUB

"@

$ports = @($HubPort, $BmsBridgePort, $XtBridgePort)
if ($includeEcoflow) { $ports += $EcoflowBridgePort }
if (($ports | Select-Object -Unique).Count -ne $ports.Count) {
  throw "All COM ports must be distinct (hub / BMS / XT$(if ($includeEcoflow) { ' / EcoFlow' }))."
}

$hubMac = Read-EspStaMac -Port $HubPort -Label "HUB (M5)" -Chip esp32
$bmsMac = Read-EspStaMac -Port $BmsBridgePort -Label "BMS bridge (TTGO)" -Chip esp32
$xtMac = Read-EspStaMac -Port $XtBridgePort -Label "XT bridge (TTGO)" -Chip esp32
$ecoMac = $null
if ($includeEcoflow) {
  $ecoMac = Read-EspStaMac -Port $EcoflowBridgePort -Label "EcoFlow bridge (S3)" -Chip esp32s3
}
$pmk = Get-OrCreatePmk -ForceNew:$RegeneratePmk

Write-Host ""
Write-Host "Discovered:" -ForegroundColor Yellow
Write-Host "  HUB            MAC = $hubMac"
Write-Host "  BMS_BRIDGE     MAC = $bmsMac"
Write-Host "  XT_BRIDGE      MAC = $xtMac"
if ($includeEcoflow) {
  Write-Host "  ECOFLOW_BRIDGE MAC = $ecoMac"
}
Write-Host ""

# Bridges peer the hub; hub peers every bridge.
Set-UserEnv -Name "ESPNOW_PEER_MAC" -Value $hubMac
Set-UserEnv -Name "ESPNOW_BMS_BRIDGE_MAC" -Value $bmsMac
Set-UserEnv -Name "ESPNOW_BRIDGE_MAC" -Value $xtMac
Set-UserEnv -Name "ESPNOW_PMK" -Value $pmk
if ($includeEcoflow) {
  Set-UserEnv -Name "ESPNOW_ECOFLOW_BRIDGE_MAC" -Value $ecoMac
}

Write-Host "Saved Windows User env:" -ForegroundColor Green
Write-Host "  ESPNOW_PEER_MAC             = $hubMac   (all bridges)"
Write-Host "  ESPNOW_BMS_BRIDGE_MAC       = $bmsMac   (M5 hub)"
Write-Host "  ESPNOW_BRIDGE_MAC           = $xtMac    (M5 hub, XT peer)"
if ($includeEcoflow) {
  Write-Host "  ESPNOW_ECOFLOW_BRIDGE_MAC   = $ecoMac   (M5 hub, EcoFlow peer)"
}
Write-Host "  ESPNOW_PMK                  = (32-char secret, not printed)"
Write-Host ""

if (-not $Flash) {
  Write-Host "Next:" -ForegroundColor Yellow
  if ($includeEcoflow) {
    Write-Host "  .\scripts\flash.ps1 -Target All -HubPort $HubPort -BmsBridgePort $BmsBridgePort -XtBridgePort $XtBridgePort -EcoflowBridgePort $EcoflowBridgePort"
  } else {
    Write-Host "  .\scripts\flash.ps1 -Target All -HubPort $HubPort -BmsBridgePort $BmsBridgePort -XtBridgePort $XtBridgePort"
  }
  Write-Host "Or re-run with -Flash." -ForegroundColor DarkGray
  exit 0
}

Write-Host "==> Flashing hub + bridges" -ForegroundColor Cyan
if ($includeEcoflow) {
  & "$PSScriptRoot\flash.ps1" -Target All -HubPort $HubPort -BmsBridgePort $BmsBridgePort `
    -XtBridgePort $XtBridgePort -EcoflowBridgePort $EcoflowBridgePort
} else {
  & "$PSScriptRoot\flash.ps1" -Target All -HubPort $HubPort -BmsBridgePort $BmsBridgePort `
    -XtBridgePort $XtBridgePort
}
exit $LASTEXITCODE
