<#
.SYNOPSIS
  Enforces maintainability / DI guardrails for this monorepo via shared
  tools/esp_cpp_guardrails (CursorCodingSkills).
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$srcRoot = Join-Path $repoRoot "src"

$invokerCandidates = @(
  (Join-Path $repoRoot "tools\esp_cpp_guardrails\Invoke-EspCppGuardrails.ps1"),
  (Join-Path (Split-Path -Parent $repoRoot) "CursorCodingSkills\esp_cpp_guardrails\Invoke-EspCppGuardrails.ps1")
)
$invoker = $invokerCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
if (-not $invoker) {
  throw @"
ESP/C++ guardrails not found.
Deploy CursorCodingSkills into this repo:
  cd E:\AI\CursorCodingSkills
  .\deploy.ps1 -ProjectPath '$repoRoot' -SkipPythonGuardrails
"@
}

$concreteAdapterHeaders = @(
  "EspWifiConnector.h",
  "ArduinoOtaUpdater.h",
  "WattcycleBleClient.h",
  "EspWebGateway.h",
  "TtgoStatusDisplay.h",
  "DongleStatusDisplay.h",
  "NullStatusDisplay.h",
  "DongleButtonSource.h",
  "NullButtonSource.h",
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
  "boards/",
  "common/",
  "hub_common/"
)

$domainLeakRules = @(
  @{
    Root = (Join-Path $srcRoot "bridge_xt369p")
    ForbiddenPattern = '#include\s+"(Auth|Web|Ota|HttpApi)/'
  },
  @{
    Root = (Join-Path $srcRoot "bridge_ttgo_wattcycle")
    ForbiddenPattern = '#include\s+"(Auth|Web|Ota|HttpApi)/'
  },
  @{
    Root = (Join-Path $srcRoot "bridge_ecoflow_delta3")
    ForbiddenPattern = '#include\s+"(Auth|Web|Ota|HttpApi)/'
  }
)

& $invoker `
  -SourceRoot $srcRoot `
  -MaxLines 400 `
  -MaxMethodsPerClass 30 `
  -MaxFunctionBodyLines 60 `
  -FunctionBodyPathRegex '[\\/]bridge_ecoflow_delta3[\\/]' `
  -OneTypePerHeaderRoots @(
    (Join-Path $srcRoot "bridge_ecoflow_delta3"),
    (Join-Path $srcRoot "boards")
  ) `
  -ConcreteAdapterHeaders $concreteAdapterHeaders `
  -TargetPrefixes $targetPrefixes `
  -CompositionRootNamePatterns @("main.cpp", "main_tdongle.cpp", "main_headless.cpp", "CompositionRoot*") `
  -ForbidUnitTestInHeaders $true `
  -ForbidUnitTestInCpp $true `
  -PublicCommentRoots @(
    (Join-Path $srcRoot "bridge_ecoflow_delta3"),
    (Join-Path $srcRoot "bridge_ttgo_wattcycle"),
    (Join-Path $srcRoot "boards"),
    (Join-Path $srcRoot "common"),
    (Join-Path $srcRoot "hub_common")
  ) `
  -CommentInterfaceNameRegex '^I[A-Z][A-Za-z0-9]*\.h$' `
  -CommentAllHeadersInRoots $false `
  -CommentAllHeadersPathRegex '[\\/]bridge_ecoflow_delta3[\\/]' `
  -DomainLeakRules $domainLeakRules

exit $LASTEXITCODE
