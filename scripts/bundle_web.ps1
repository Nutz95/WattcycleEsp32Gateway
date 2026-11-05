<#
.SYNOPSIS
  Bundles modular web/ sources into compact LittleFS assets under data/.
#>
[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
$repoRoot = Split-Path -Parent $PSScriptRoot
$webRoot = Join-Path $repoRoot "web"
$dataRoot = Join-Path $repoRoot "data"
$utf8 = New-Object System.Text.UTF8Encoding $false

New-Item -ItemType Directory -Force -Path $dataRoot | Out-Null

function Read-Utf8Text([string]$path) {
  return [System.IO.File]::ReadAllText($path, $utf8)
}

function Write-Utf8NoBom([string]$path, [string]$text) {
  [System.IO.File]::WriteAllText($path, $text, $utf8)
}

function Minify-Css([string]$text) {
  $text = [regex]::Replace($text, '/\*[\s\S]*?\*/', '')
  $text = [regex]::Replace($text, '\s+', ' ')
  $text = [regex]::Replace($text, '\s*([{};:,>~+])\s*', '$1')
  return $text.Trim()
}

function Minify-Js([string]$text) {
  $lines = $text -split "`r?`n" | ForEach-Object {
    $line = $_
    if ($line -match '^\s*//') { return $null }
    return $line
  } | Where-Object { $_ -ne $null }
  $joined = ($lines -join "`n")
  $joined = [regex]::Replace($joined, '(?m)^\s+', '')
  $joined = [regex]::Replace($joined, '\n{2,}', "`n")
  return $joined.Trim()
}

# HTML: inject view fragments (UTF-8 in, UTF-8 out — never Windows ANSI)
$html = Read-Utf8Text (Join-Path $webRoot "index.html")
$views = @("overview", "dashboard", "solar", "wattcycle", "ecoflow", "history", "reports", "cells", "temperatures", "warnings", "gateway", "esp", "plugs", "smoke", "camera", "account")
foreach ($view in $views) {
  $fragment = Read-Utf8Text (Join-Path $webRoot "views\$view.html")
  $html = $html.Replace("<!--VIEW:$view-->", $fragment.Trim())
}
$html = [regex]::Replace($html, '>\s+<', '><')
$html = $html.Trim()
Write-Utf8NoBom (Join-Path $dataRoot "index.html") $html

# CSS
$css = (Minify-Css (Read-Utf8Text (Join-Path $webRoot "css\app.css"))) +
  (Minify-Css (Read-Utf8Text (Join-Path $webRoot "css\reports.css")))
Write-Utf8NoBom (Join-Path $dataRoot "app.css") $css

# JS bundle order matters
$jsParts = @(
  "js\binaryDecoder.js",
  "js\dateFormat.js",
  "js\historyCsvParse.js",
  "js\historyStore.js",
  "js\historyDayCache.js",
  "js\draftStore.js",
  "js\charts.js",
  "js\auth.js",
  "js\reports\dayAggregate.js",
  "js\reports\plugEnergy.js",
  "js\reports\periodRollup.js",
  "js\reports\tariffValidate.js",
  "js\reports\tariffStore.js",
  "js\reports\tariffEditor.js",
  "js\reports\costing.js",
  "js\reports\barChart.js",
  "js\reports\pieChart.js",
  "js\views\systemFlow.js",
  "js\views\dashboard.js",
  "js\views\solar.js",
  "js\views\wattcycle.js",
  "js\views\ecoflow.js",
  "js\views\historyPlugs.js",
  "js\views\history.js",
  "js\views\reports.js",
  "js\views\cells.js",
  "js\views\temperatures.js",
  "js\views\warnings.js",
  "js\views\gateway.js",
  "js\views\esp.js",
  "js\shellyDeviceAdmin.js",
  "js\views\plugs.js",
  "js\views\smoke.js",
  "js\views\camera.js",
  "js\guestTabs.js",
  "js\views\account.js",
  "js\app.js"
) | ForEach-Object {
  Minify-Js (Read-Utf8Text (Join-Path $webRoot $_))
}
$bundle = ($jsParts -join "`n")
Write-Utf8NoBom (Join-Path $dataRoot "app.js") $bundle
$stamp = (Get-FileHash (Join-Path $dataRoot "index.html")).Hash.Substring(0, 16) +
    (Get-FileHash (Join-Path $dataRoot "app.css")).Hash.Substring(0, 16) +
    (Get-FileHash (Join-Path $dataRoot "app.js")).Hash.Substring(0, 16)
Set-Content -Path (Join-Path $dataRoot "bundle.txt") -Value $stamp -NoNewline -Encoding ascii

# Remove legacy assets if present
@(
  (Join-Path $dataRoot "style.css"),
  (Join-Path $dataRoot "app.js.bak")
) | ForEach-Object {
  if (Test-Path $_) { Remove-Item $_ -Force }
}

Write-Host "Bundled web assets -> data/ (index.html, app.css, app.js)" -ForegroundColor Green
exit 0
