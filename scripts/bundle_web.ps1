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

New-Item -ItemType Directory -Force -Path $dataRoot | Out-Null

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

# HTML: inject view fragments
$html = Get-Content (Join-Path $webRoot "index.html") -Raw
$views = @("overview", "cells", "temperatures", "warnings", "gateway", "esp", "account")
foreach ($view in $views) {
  $fragment = Get-Content (Join-Path $webRoot "views\$view.html") -Raw
  $html = $html.Replace("<!--VIEW:$view-->", $fragment.Trim())
}
$html = [regex]::Replace($html, '>\s+<', '><')
$html = $html.Trim()
Set-Content -Path (Join-Path $dataRoot "index.html") -Value $html -NoNewline

# CSS
$css = Get-Content (Join-Path $webRoot "css\app.css") -Raw
Set-Content -Path (Join-Path $dataRoot "app.css") -Value (Minify-Css $css) -NoNewline

# JS bundle order matters
$jsParts = @(
  "js\binaryDecoder.js",
  "js\historyStore.js",
  "js\charts.js",
  "js\auth.js",
  "js\views\overview.js",
  "js\views\cells.js",
  "js\views\temperatures.js",
  "js\views\warnings.js",
  "js\views\gateway.js",
  "js\views\esp.js",
  "js\views\account.js",
  "js\app.js"
) | ForEach-Object {
  Minify-Js (Get-Content (Join-Path $webRoot $_) -Raw)
}
$bundle = ($jsParts -join "`n")
Set-Content -Path (Join-Path $dataRoot "app.js") -Value $bundle -NoNewline

# Remove legacy assets if present
@(
  (Join-Path $dataRoot "style.css"),
  (Join-Path $dataRoot "app.js.bak")
) | ForEach-Object {
  if (Test-Path $_) { Remove-Item $_ -Force }
}

Write-Host "Bundled web assets -> data/ (index.html, app.css, app.js)" -ForegroundColor Green
exit 0

