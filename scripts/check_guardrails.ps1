<#
.SYNOPSIS
  Enforces maintainability / DI guardrails for this repository.
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
  "InMemoryTelemetryStore.h",
  "NvsCredentialStore.h",
  "AuthService.h"
)

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

# Concrete adapter headers may only be included from main.cpp, CompositionRoot, or their own domain folder.
foreach ($file in $sourceFiles) {
  $relative = $file.FullName.Substring($srcRoot.Length).TrimStart('\', '/')
  $includes = Select-String -Path $file.FullName -Pattern '#include\s+"([^"]+)"' -AllMatches
  foreach ($match in $includes.Matches) {
    $included = Split-Path -Leaf $match.Groups[1].Value
    if ($concreteAdapterHeaders -notcontains $included) {
      continue
    }

    $allowed = $false
    if ($relative -eq "main.cpp") { $allowed = $true }
    if ($relative -like "CompositionRoot*") { $allowed = $true }
    if ($file.Name -eq $included) { $allowed = $true }
    if ($file.BaseName -eq ([IO.Path]::GetFileNameWithoutExtension($included))) { $allowed = $true }

    # Same domain folder (e.g. Wifi/EspWifiConnector.cpp including its header) is OK
    $includePath = $match.Groups[1].Value -replace '/', '\'
    if ($relative -replace '\\', '/' -match ('^' + ($includePath -replace '\\', '/' -replace '\.h$', '') -replace '/', '\/')) {
      $allowed = $true
    }
    $domain = ($includePath -split '[\\/]')[0]
    if ($relative.StartsWith($domain + '\') -or $relative.StartsWith($domain + '/')) {
      $allowed = $true
    }

    if (-not $allowed) {
      $failures += "DI BOUNDARY: $($file.FullName) includes concrete adapter '$included'"
    }
  }
}

if ($failures.Count -gt 0) {
  Write-Host "Guardrail failures:" -ForegroundColor Red
  $failures | ForEach-Object { Write-Host " - $_" -ForegroundColor Red }
  exit 1
}

Write-Host "Guardrails OK ($($sourceFiles.Count) source files scanned)" -ForegroundColor Green
exit 0
