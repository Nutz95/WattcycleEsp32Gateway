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
$maxFunctionBodyLines = 60
# One primary type (class / struct / enum / enum class) per public header.
$oneTypePerHeaderRoots = @(
  (Join-Path $srcRoot "bridge_ecoflow_delta3")
)

$sourceFiles = Get-ChildItem -Path $srcRoot -Recurse -Include *.cpp, *.h |
  Where-Object { $_.Name -notmatch '\.gen\.(cpp|h)$' }
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

  # Function body length — EcoFlow bridge only (bring-up surface; rest of monorepo is gradual).
  $isEcoflowCpp = $file.Extension -eq ".cpp" -and (
    $file.FullName -match '[\\/]bridge_ecoflow_delta3[\\/]'
  )
  if ($isEcoflowCpp) {
    $depth = 0
    $bodyStack = New-Object System.Collections.Generic.List[object]
    for ($li = 0; $li -lt $lines.Count; $li++) {
      $line = $lines[$li]
      $opens = ([regex]::Matches($line, '\{')).Count
      $closes = ([regex]::Matches($line, '\}')).Count
      if ($opens -gt 0 -and $line -match '\)\s*(?:const\s*)?(?:override\s*)?\{') {
        # Body is live while depth >= depth after this function's opening brace.
        $bodyStack.Add([pscustomobject]@{ Start = $li; Depth = ($depth + 1) })
      }
      $depth += $opens - $closes
      if ($depth -lt 0) { $depth = 0 }
      while ($bodyStack.Count -gt 0 -and $depth -lt $bodyStack[$bodyStack.Count - 1].Depth) {
        $finished = $bodyStack[$bodyStack.Count - 1]
        $bodyStack.RemoveAt($bodyStack.Count - 1)
        $bodyLen = $li - $finished.Start + 1
        if ($bodyLen -gt $maxFunctionBodyLines) {
          $failures += ("LONG FUNCTION: {0}:{1} ~{2} lines (max {3})" -f `
            $file.FullName, ($finished.Start + 1), $bodyLen, $maxFunctionBodyLines)
        }
      }
    }
  }
}

foreach ($root in $oneTypePerHeaderRoots) {
  if (-not (Test-Path $root)) { continue }
  $headers = Get-ChildItem -Path $root -Recurse -Filter *.h |
    Where-Object { $_.Name -notmatch '\.gen\.h$' }
  foreach ($header in $headers) {
    $content = Get-Content -Path $header.FullName -Raw
    # Top-level type introductions only (start of line, optional template).
    $typeMatches = [regex]::Matches(
      $content,
      '(?m)^(template\s*<[^>]+>\s*)?(enum\s+class|enum|struct|class)\s+\w+'
    )
    if ($typeMatches.Count -gt 1) {
      $failures += ("ONE TYPE PER HEADER: {0} defines {1} types" -f `
        $header.FullName, $typeMatches.Count)
    }
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

# Public API methods in headers need a brief intent comment (/// or //).
# Scoped to interfaces (I*.h) and the EcoFlow bridge (bring-up surface).
$publicCommentRoots = @(
  (Join-Path $srcRoot "bridge_ecoflow_delta3"),
  (Join-Path $srcRoot "common"),
  (Join-Path $srcRoot "hub_common")
)
foreach ($root in $publicCommentRoots) {
  if (-not (Test-Path $root)) { continue }
  $headers = Get-ChildItem -Path $root -Recurse -Filter *.h |
    Where-Object { $_.Name -notmatch '\.gen\.h$' }
  foreach ($header in $headers) {
    # ITelemetryStore.h yes; InMemoryTelemetryStore.h no (leading "In").
    # Use -cmatch: PowerShell -match is case-insensitive by default.
    $isInterface = $header.Name -cmatch '^I[A-Z][A-Za-z0-9]*\.h$'
    $isEcoflow = $header.FullName -match '[\\/]bridge_ecoflow_delta3[\\/]'
    if (-not $isInterface -and -not $isEcoflow) { continue }

    $lines = @(Get-Content -Path $header.FullName)
    $inPublic = $isInterface  # interfaces are entirely public API
    for ($i = 0; $i -lt $lines.Count; $i++) {
      $line = $lines[$i]
      if ($line -match '^\s*public\s*:') { $inPublic = $true; continue }
      if ($line -match '^\s*private\s*:') { $inPublic = $false; continue }
      if ($line -match '^\s*protected\s*:') { $inPublic = $false; continue }
      if (-not $inPublic) { continue }

      # Method / ctor declaration ending with ; { or override;
      if ($line -notmatch '^\s+(?:virtual\s+)?(?:static\s+)?[\w:<>,\s\*&~]+?\s+\w+\s*\([^;{]*\)\s*(?:const\s*)?(?:override\s*)?(?:noexcept\s*)?[;{]') {
        continue
      }
      if ($line -match '^\s+(?:using|friend|enum|struct|class)\b') { continue }

      $hasComment = $false
      for ($back = $i - 1; $back -ge 0 -and ($i - $back) -le 4; $back--) {
        $prev = $lines[$back].Trim()
        if ($prev -eq '') { continue }
        if ($prev -match '^///' -or $prev -match '^//[^/]' -or $prev -match '^/\*' -or $prev -match '^\*') {
          $hasComment = $true
        }
        break
      }
      if (-not $hasComment) {
        $failures += ("PUBLIC API COMMENT: {0}:{1} - add /// intent comment" -f $header.FullName, ($i + 1))
      }
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
