<#
.SYNOPSIS
  Compatibility wrapper: flash the Wattcycle hub (COM19 by default).
  Prefer: .\scripts\flash.ps1 -Target HubTtgoWattcycle
#>
[CmdletBinding()]
param(
  [string]$Port = "COM19"
)

& "$PSScriptRoot\flash.ps1" -Target HubTtgoWattcycle -Port $Port
exit $LASTEXITCODE
