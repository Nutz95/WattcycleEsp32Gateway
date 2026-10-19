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
# pio test has no -j; SCons still uses CPU count for compiles inside each test binary.
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

# Environments build one after another; -j parallelizes object compiles within each env.
$jobs = [Environment]::ProcessorCount
if ($jobs -lt 1) { $jobs = 1 }

$envs = @(
  "hub_m5_wattcycle",
  "bridge_ttgo_wattcycle",
  "bridge_wattcycle_tdongle",
  "bridge_xt369p",
  "bridge_ecoflow_delta3",
  "hub_ttgo_wattcycle"
)

function Invoke-PioEnv {
  param(
    [Parameter(Mandatory)][string]$EnvName,
    [Parameter(Mandatory)][int]$JobCount
  )
  $logPath = Join-Path $env:TEMP ("wattcycle-pio-" + $EnvName + ".log")
  & pio run -j $JobCount -e $EnvName *> $logPath
  $code = $LASTEXITCODE
  Get-Content -LiteralPath $logPath | Write-Host
  return @{
    ExitCode = $code
    LogPath  = $logPath
  }
}

foreach ($envName in $envs) {
  Write-Host ("==> Compile " + $envName) -ForegroundColor Cyan
  $result = Invoke-PioEnv -EnvName $envName -JobCount $jobs
  if ($result.ExitCode -eq 0) {
    continue
  }

  $logText = Get-Content -LiteralPath $result.LogPath -Raw
  $depfileRace = $logText -match "opening dependency file"
  if (-not $depfileRace) {
    exit $result.ExitCode
  }

  Write-Warning ("SCons depfile race on " + $envName + " - cleaning build dir and retrying once")
  $buildDir = Join-Path $repoRoot (".pio\build\" + $envName)
  if (Test-Path -LiteralPath $buildDir) {
    Remove-Item -Recurse -Force -LiteralPath $buildDir
  }
  $retry = Invoke-PioEnv -EnvName $envName -JobCount $jobs
  if ($retry.ExitCode -ne 0) {
    exit $retry.ExitCode
  }
}

exit 0
