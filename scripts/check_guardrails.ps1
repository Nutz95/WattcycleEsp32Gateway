<#
.SYNOPSIS
  Enforces maintainability / DI guardrails for this monorepo.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$srcRoot = Join-Path $repoRoot "src"
$failures = @()

$maxLines = 400
$maxFunctionsPerClass = 30

$sourceFiles = Get-ChildItem -Path $srcRoot -Recurse -Include *.cpp, *.h
$concreteAdapterHeaders = @(
  "EspWifiConnector.h",
  "ArduinoOtaUpdater.h",
  "WattcycleBleClient.h",
  "EspWebGateway.h",
  "TtgoStatusDisplay.h",
  "M5StatusDisplay.h",
  "InMemoryTelemetryStore.h",
  "NvsCredentialStore.h",
  "AuthService.h",
  "EspNowTelemetryReceiver.h",
  "EspNowTelemetryPublisher.h",
  "AtorchSppClient.h",
  "NtpClock.h",
  "SdDailyHistory.h",
  "NullDailyHistoryStore.h"
)

$targetPrefixes = @(
  "hub_ttgo_wattcycle/",
  "hub_m5_wattcycle/",
  "bridge_ttgo_wattcycle/",
  "bridge_xt369p/",
  "bridge_ecoflow_delta3/",
  "common/",
  "hub_common/"
)

function Get-RelativeInTarget {
  param([string]$RelativeNorm)
  foreach ($prefix in $targetPrefixes) {
    if ($RelativeNorm.StartsWith($prefix)) {
      return $RelativeNorm.Substring($prefix.Length)
    }
  }
  return $RelativeNorm
}

foreach ($file in $sourceFiles) {
  $lines = @(Get-Content -Path $file.FullName)
  if ($lines.Count -gt $maxLines) {
    $failures += "FILE SIZE: $($file.FullName) has $($lines.Count) lines (max $maxLines)"
  }

  $content = Get-Content -Path $file.FullName -Raw
  if ($content -match '(?m)^[ \t]+class\s+\w+') {
    $failures += "NESTED CLASS: $($file.FullName)"
  }

  $functionMatches = [regex]::Matches(
    $content,
    '(?m)^\s+(?:virtual\s+)?(?:static\s+)?[\w:<>,\s\*&]+?\s+\w+\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?(?:;|\{)'
  )
  if ($functionMatches.Count -gt $maxFunctionsPerClass) {
    $failures += "TOO MANY METHODS: $($file.FullName) ~$($functionMatches.Count) (max $maxFunctionsPerClass)"
  }
}

$secretHits = $sourceFiles | Select-String -Pattern 'password\s*=\s*"[^"]{4,}"'
foreach ($hit in $secretHits) {
  $failures += "HARDCODED SECRET: $($hit.Path):$($hit.LineNumber)"
}

foreach ($file in $sourceFiles) {
  $relative = $file.FullName.Substring($srcRoot.Length).TrimStart('\', '/')
  $relativeNorm = $relative -replace '\\', '/'
  $relativeInTarget = Get-RelativeInTarget $relativeNorm

  $includes = Select-String -Path $file.FullName -Pattern '#include\s+"([^"]+)"' -AllMatches
  foreach ($match in $includes.Matches) {
    $included = Split-Path -Leaf $match.Groups[1].Value
    if ($concreteAdapterHeaders -notcontains $included) {
      continue
    }

    $allowed = $false
    if ($relativeInTarget -eq "main.cpp") { $allowed = $true }
    if ($relativeInTarget -like "CompositionRoot*") { $allowed = $true }
    if ($file.Name -eq $included) { $allowed = $true }
    if ($file.BaseName -eq ([IO.Path]::GetFileNameWithoutExtension($included))) { $allowed = $true }

    $includePath = ($match.Groups[1].Value -replace '\\', '/')
    $domain = ($includePath -split '/')[0]
    if ($relativeInTarget.StartsWith($domain + '/')) {
      $allowed = $true
    }

    if (-not $allowed) {
      $failures += "DI BOUNDARY: $($file.FullName) includes concrete adapter '$included'"
    }
  }
}

# Bridges must not pull in hub web/auth/ota stacks
foreach ($bridgeName in @("bridge_xt369p", "bridge_ttgo_wattcycle", "bridge_ecoflow_delta3")) {
  $bridgeRoot = Join-Path $srcRoot $bridgeName
  if (-not (Test-Path $bridgeRoot)) { continue }
  $forbidden = Get-ChildItem -Path $bridgeRoot -Recurse -Include *.cpp, *.h |
    Select-String -Pattern '#include\s+"(Auth|Web|Ota|HttpApi)/'
  foreach ($hit in $forbidden) {
    $failures += "BRIDGE LEAK: $($hit.Path):$($hit.LineNumber) includes hub-only domain"
  }
}

if ($failures.Count -gt 0) {
  Write-Host "Guardrail failures:" -ForegroundColor Red
  $failures | ForEach-Object { Write-Host " - $_" -ForegroundColor Red }
  exit 1
}

Write-Host "Guardrails OK ($($sourceFiles.Count) source files scanned)" -ForegroundColor Green
exit 0
