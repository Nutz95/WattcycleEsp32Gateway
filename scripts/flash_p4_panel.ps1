#Requires -Version 5.1
<#
.SYNOPSIS
  Stage the JC 4.3 panel sketch and compile or flash it with arduino-cli.
  LCD and touch sources come from third_party/jc4880_panel.
.PARAMETER DenyUsbSerial
  Optional USB parent id to refuse, so a bridge dongle is not flashed by mistake.
  Empty by default. Can also be set with P4_DENY_USB_SERIAL.
#>
param(
    [string]$Port = "",
    [string]$DenyUsbSerial = "",
    [switch]$CompileOnly
)

$ErrorActionPreference = "Stop"
$Repo = Split-Path $PSScriptRoot -Parent
$Panel = Join-Path $Repo "third_party\jc4880_panel"
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
if (-not (Test-Path (Join-Path $Panel "lcd\st7701_lcd.cpp"))) {
    throw "Panel driver missing under third_party/jc4880_panel"
}

function Copy-IfChanged {
    param([Parameter(Mandatory)][string]$Source, [Parameter(Mandatory)][string]$Destination)
    $items = @(Get-Item -Path $Source -ErrorAction SilentlyContinue)
    if ($items.Count -eq 0) {
        throw "Missing staged source: $Source"
    }
    foreach ($item in $items) {
        if ($item.PSIsContainer) {
            Get-ChildItem $item.FullName -File | ForEach-Object { Copy-IfChanged $_.FullName $Destination }
            continue
        }
        $destFile = $Destination
        if ((Test-Path $Destination) -and (Get-Item $Destination).PSIsContainer) {
            $destFile = Join-Path $Destination $item.Name
        }
        if (Test-Path $destFile) {
            $same = (Get-FileHash -Path $item.FullName).Hash -eq (Get-FileHash -Path $destFile).Hash
            if ($same) { continue }
        }
        Microsoft.PowerShell.Management\Copy-Item -Path $item.FullName -Destination $destFile -Force
    }
}

Write-Host "Staging panel sketch (keeping the previous build)"
New-Item -ItemType Directory -Path (Join-Path $Stage "src\Display") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Stage "src\lcd") -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $Stage "src\touch") -Force | Out-Null
Copy-IfChanged (Join-Path $Panel "pins_config.h") $Stage
Copy-IfChanged (Join-Path $Panel "lcd\*") (Join-Path $Stage "src\lcd")
Copy-IfChanged (Join-Path $Panel "touch\*") (Join-Path $Stage "src\touch")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\hub_p4_panel.ino") $Stage
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4PanelApp.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4PanelApp.h") $Stage
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4StaRadio.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4StaRadio.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4HostedNow.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4HostedNow.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4C6Update.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4C6Update.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4AuthTap.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4AuthTap.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4MeterCommands.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4MeterCommands.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4SdHistory.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4SdHistory.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4WebPublish.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4WebPublish.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4ScreenSleep.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4ScreenSleep.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4LiveView.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4LiveView.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4LiveCapture.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4LiveJpeg.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4LiveClaim.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4CameraPage.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4CameraPage.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4Es8311.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4Es8311.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4LiveHttp.cpp") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\P4LiveHttp.h") (Join-Path $Stage "src")
$Cam = Join-Path $Repo "third_party\jc4880_cam"
New-Item -ItemType Directory -Path (Join-Path $Stage "src\ov02c10") -Force | Out-Null
Copy-IfChanged (Join-Path $Cam "ov02c10\ov02c10.c") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Cam "ov02c10\include\*") (Join-Path $Stage "src\ov02c10")
Copy-IfChanged (Join-Path $Cam "ov02c10\private_include\*") (Join-Path $Stage "src\ov02c10")
Copy-IfChanged (Join-Path $Cam "ov02c10_force_config.h") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Cam "ov02c10_ipa_config.c") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Cam "P4CamSensorHooks.c") (Join-Path $Stage "src")
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\partitions.csv") $Stage
$Display = Join-Path $Repo "src\hub_p4_wattcycle\Display"
# Hub-only store adapters stay off the panel sketch. Everything else under Display/ is staged.
$HubOnly = @(
    "P4PageBuilder.cpp", "P4PageBuilder.h", "P4PageModel.h",
    "P4StatusDisplay.cpp", "P4StatusDisplay.h"
)
Get-ChildItem -Path $Display -File | ForEach-Object {
    if ($HubOnly -contains $_.Name) {
        return
    }
    if ($_.Extension -eq ".cpp") {
        Copy-IfChanged $_.FullName (Join-Path $Stage "src")
    } elseif ($_.Extension -in @(".h", ".hpp")) {
        Copy-IfChanged $_.FullName (Join-Path $Stage "src\Display")
    }
}
Copy-IfChanged (Join-Path $Repo "src\hub_p4_wattcycle\panel\lv_conf.h") (Join-Path $Stage "src")
$HubSources = @(
    "src\hub_common\Telemetry\InMemoryTelemetryStore.cpp",
    "src\hub_common\Telemetry\BinaryTelemetryCodec.cpp",
    "src\hub_common\Time\NtpClock.cpp",
    "src\hub_common\HttpApi\EspWebGateway.cpp",
    "src\hub_common\HttpApi\EspWebGatewayHistory.cpp",
    "src\hub_common\Storage\HistoryCatalog.cpp",
    "src\hub_common\Storage\HistoryDayFile.cpp",
    "src\hub_common\HttpApi\JsonField.cpp",
    "src\hub_common\Auth\AuthService.cpp",
    "src\hub_common\Auth\PasswordHasher.cpp",
    "src\hub_common\Auth\CredentialPolicy.cpp",
    "src\hub_common\Auth\SessionStore.cpp",
    "src\hub_common\Auth\NvsCredentialStore.cpp",
    "src\hub_common\Auth\Sha256.cpp",
    "src\common\Util\RandomBytes.cpp",
    "src\hub_common\Esp\EspHealthSampler.cpp",
    "src\hub_m5_wattcycle\EspNow\EspNowPacketApply.cpp"
)
foreach ($rel in $HubSources) {
    Copy-IfChanged (Join-Path $Repo $rel) (Join-Path $Stage "src")
}

function ConvertTo-CStringLiteral([string]$Value) {
    if ($null -eq $Value) { return "" }
    return ($Value -replace '\\', '\\' -replace '"', '\"')
}
function Read-UserEnv([string]$Name) {
    $value = [Environment]::GetEnvironmentVariable($Name, "User")
    if ([string]::IsNullOrEmpty($value)) { return "" }
    return $value
}
$ssid = ConvertTo-CStringLiteral (Read-UserEnv "WIFI_SSID")
$pass = ConvertTo-CStringLiteral (Read-UserEnv "WIFI_PASS")
$pmk = ConvertTo-CStringLiteral (Read-UserEnv "ESPNOW_PMK")
$bms = ConvertTo-CStringLiteral (Read-UserEnv "ESPNOW_BMS_BRIDGE_MAC")
$eco = ConvertTo-CStringLiteral (Read-UserEnv "ESPNOW_ECOFLOW_BRIDGE_MAC")
$solar = ConvertTo-CStringLiteral (Read-UserEnv "ESPNOW_BRIDGE_MAC")
$secretHeader = @"
#pragma once
#define P4_WIFI_SSID "$ssid"
#define P4_WIFI_PASS "$pass"
#define P4_ESPNOW_PMK "$pmk"
#define P4_BMS_MAC "$bms"
#define P4_ECO_MAC "$eco"
#define P4_SOLAR_MAC "$solar"
"@
$secretPath = Join-Path $Stage "src\P4BuildSecrets.h"
$secretPending = "$secretPath.new"
Set-Content -Path $secretPending -Value $secretHeader -Encoding ascii
$secretChanged = -not (Test-Path $secretPath) -or ((Get-FileHash $secretPending).Hash -ne (Get-FileHash $secretPath).Hash)
if ($secretChanged) {
    Move-Item -Path $secretPending -Destination $secretPath -Force
} else {
    Remove-Item $secretPending -Force
}
Write-Host ("Radio secrets: ssidLen={0} passLen={1} pmkLen={2} peers={3}" -f $ssid.Length, $pass.Length, $pmk.Length, (@($bms, $eco, $solar) | Where-Object { $_ -ne "" }).Count)

Write-Host "Compiling"
$StageInclude = (($Stage -replace '\\', '/') + "/src")
$CommonInclude = (($Repo -replace '\\', '/') + "/src/common")
$HubInclude = (($Repo -replace '\\', '/') + "/src/hub_common")
$RxInclude = (($Repo -replace '\\', '/') + "/src/hub_m5_wattcycle")
$CamInclude = "$StageInclude/ov02c10"
$Flags = "-I$StageInclude -I$CamInclude -I$CommonInclude -I$HubInclude -I$RxInclude -DLV_CONF_INCLUDE_SIMPLE"
$LinkFlags = "-Wl,--wrap=esp_cam_sensor_detect_get_array -Wl,--wrap=esp_ipa_pipeline_get_config"
& $Cli compile --fqbn $Fqbn --build-path $Build `
    --build-property "compiler.c.extra_flags=$Flags" `
    --build-property "compiler.cpp.extra_flags=$Flags" `
    --build-property "compiler.c.elf.extra_flags=$LinkFlags" `
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

& (Join-Path $PSScriptRoot "bundle_web.ps1")
$LittleFs = Join-Path $Stage "littlefs.bin"
$MkLittle = Get-ChildItem -Path (Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\tools\mklittlefs") -Recurse -Filter "mklittlefs.exe" |
    Select-Object -First 1 -ExpandProperty FullName
$EspTool = Get-ChildItem -Path (Join-Path $env:LOCALAPPDATA "Arduino15\packages\esp32\tools\esptool_py") -Recurse -Filter "esptool.exe" |
    Select-Object -First 1 -ExpandProperty FullName
if (-not $MkLittle -or -not $EspTool) {
    throw "mklittlefs or esptool was not found under the Arduino ESP32 package"
}
& $MkLittle -c (Join-Path $Repo "data") -b 4096 -p 128 -s 1048576 $LittleFs
if ($LASTEXITCODE -ne 0) {
    throw "LittleFS image failed ($LASTEXITCODE)"
}
Write-Host "Writing web filesystem"
Start-Sleep -Seconds 2
& $EspTool --chip esp32p4 --port $Port --baud 921600 write-flash 0x610000 $LittleFs
if ($LASTEXITCODE -ne 0) {
    throw "Web filesystem upload failed ($LASTEXITCODE)"
}
Write-Host "Web filesystem written"
