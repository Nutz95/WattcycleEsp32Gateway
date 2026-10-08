<#
.SYNOPSIS
  Generate a cryptographically strong ESP-NOW PMK for the XT369P solar bridge.

.DESCRIPTION
  Low-level PMK generator only. Prefer:

    .\scripts\pair_espnow_link.ps1 -HubPort COM19 -BridgePort COM22 -Flash
#>
[CmdletBinding()]
param(
  [ValidateRange(16, 32)]
  [int]$Bytes = 16
)

$ErrorActionPreference = "Stop"

$buffer = New-Object byte[] $Bytes
$rng = [System.Security.Cryptography.RandomNumberGenerator]::Create()
try {
  $rng.GetBytes($buffer)
} finally {
  $rng.Dispose()
}

$pmk = -join ($buffer | ForEach-Object { $_.ToString("x2") })

Write-Host ""
Write-Host "ESP-NOW PMK (keep secret - never commit to git)" -ForegroundColor Cyan
Write-Host $pmk -ForegroundColor Green
Write-Host ""
Write-Host "Prefer full pairing instead:" -ForegroundColor Yellow
Write-Host "  .\scripts\pair_espnow_link.ps1 -HubPort COM19 -BridgePort COM22 -Flash"
Write-Host ""
