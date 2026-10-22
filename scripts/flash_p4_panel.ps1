#Requires -Version 5.1
<#
.SYNOPSIS
  Stage the JC 4.3 panel sketch and compile or flash it with arduino-cli.
.PARAMETER VendorDemo
  Path to the vendor lvgl_demo_v8 folder (LCD and touch sources). Not stored in the repo.
.PARAMETER DenyUsbSerial
  Optional USB parent id to refuse, so a bridge dongle is not flashed by mistake.
  Empty by default. Can also be set with P4_DENY_USB_SERIAL.
#>
param(
    [Parameter(Mandatory = $true)]
    [string]$VendorDemo,
    [string]$Port = "",
    [string]$DenyUsbSerial = "",
    [switch]$CompileOnly
)

$ErrorActionPreference = "Stop"
$Repo = Split-Path $PSScriptRoot -Parent
$Stage = Join-Path $Repo ".pio\hub_p4_panel"
$Build = Join-Path $Stage "build"
$Cli = Join-Path $env:LOCALAPPDATA "Programs\Arduino IDE\resources\app\lib\backend\resources\arduino-cli.exe"
# ROM on this panel prints esp32p4-eco2. A v3 bootloader executes an illegal instruction and watchdog-resets.
$Fqbn = "esp32:esp32:esp32p4:ChipVariant=prev3,PSRAM=enabled,FlashSize=16M,USBMode=hwcdc,PartitionScheme=app3M_fat9M_16MB"

if ($DenyUsbSerial -eq "" -and $env:P4_DENY_USB_SERIAL) {
    $DenyUsbSerial = $env:P4_DENY_USB_SERIAL
}

function Test-DeniedPort([string]$ComPort, [string]$SerialId) {
    if ($SerialId -eq "") {
        return $false
    }
    $match = Get-PnpDevice -Class Ports -PresentOnly -ErrorAction SilentlyContinue |
        Where-Object { $_.FriendlyName -match "\($([regex]::Escape($ComPort))\)" }
    if (-not $match) {
        return $false
    }
    $parent = Get-PnpDeviceProperty -InstanceId $match.InstanceId -KeyName "DEVPKEY_Device_Parent" -ErrorAction SilentlyContinue
    return [string]$parent.Data -match [regex]::Escape($SerialId)
}

if (-not (Test-Path $Cli)) {
    throw "arduino-cli not found at $Cli"
}
if (-not (Test-Path (Join-Path $VendorDemo "src\lcd\st7701_lcd.cpp"))) {
    throw "Vendor panel demo not found at $VendorDemo"
}

Write-Host "Staging panel sketch"
if (Test-Path $Stage) {
    Remove-Item $Stage -Recurse -Force
}
New-Item -ItemType Directory -Path (Join-Path $Stage "src\Display") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Stage "src\lcd") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Stage "src\touch") -Force | Out-Null
Copy-Item (Join-Path $VendorDemo "pins_config.h") $Stage
Copy-Item (Join-Path $VendorDemo "src\lcd\*") (Join-Path $Stage "src\lcd")
Copy-Item (Join-Path $VendorDemo "src\touch\*") (Join-Path $Stage "src\touch")
Copy-Item (Join-Path $Repo "src\hub_p4_wattcycle\panel\hub_p4_panel.ino") $Stage
Copy-Item (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4PanelApp.cpp") (Join-Path $Stage "src")
Copy-Item (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4PanelApp.h") $Stage
Copy-Item (Join-Path $Repo "src\hub_p4_wattcycle\Display\P4OverviewView.cpp") (Join-Path $Stage "src")
Copy-Item (Join-Path $Repo "src\hub_p4_wattcycle\Display\P4OverviewView.h") (Join-Path $Stage "src\Display")
Copy-Item (Join-Path $Repo "src\hub_p4_wattcycle\Display\P4OverviewFlow.h") (Join-Path $Stage "src\Display")
Copy-Item (Join-Path $Repo "src\hub_p4_wattcycle\panel\lv_conf.h") (Join-Path $Stage "src")

Write-Host "Compiling"
$Include = (($Stage -replace '\\', '/') + "/src")
$Flags = "-I$Include -DLV_CONF_INCLUDE_SIMPLE"
& $Cli compile --fqbn $Fqbn --build-path $Build `
    --build-property "compiler.c.extra_flags=$Flags" `
    --build-property "compiler.cpp.extra_flags=$Flags" `
    $Stage
if ($LASTEXITCODE -ne 0) {
    throw "Panel compile failed ($LASTEXITCODE)"
}
if ($CompileOnly) {
    Write-Host "Compile only. Sketch: $Stage"
    return
}

if ($Port -eq "") {
    throw "Pass -Port COMx. The panel USB port was not selected."
}
if (Test-DeniedPort $Port $DenyUsbSerial) {
    throw "Refusing ${Port}: it matches the denied USB id."
}

Write-Host "Flashing ${Port}"
& $Cli upload --fqbn $Fqbn --input-dir $Build --port $Port
if ($LASTEXITCODE -ne 0) {
    throw "Panel upload failed ($LASTEXITCODE)"
}
Write-Host "Panel flashed on ${Port}"
