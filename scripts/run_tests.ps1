<#
.SYNOPSIS
  Runs PlatformIO native unit tests and project guardrails.
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

Write-Host "==> Unit tests (native)" -ForegroundColor Cyan
pio test -e native
exit $LASTEXITCODE
