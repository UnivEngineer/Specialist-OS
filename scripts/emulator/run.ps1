[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string] $EmulatorDirectory = $env:SPECIALIST_EMU_DIR
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

if ([string]::IsNullOrWhiteSpace($EmulatorDirectory)) {
    throw "Set SPECIALIST_EMU_DIR to the directory containing EMU.exe"
}

$emulatorDirectoryPath = [System.IO.Path]::GetFullPath($EmulatorDirectory)
$emulatorPath = Join-Path $emulatorDirectoryPath "EMU.exe"
if (-not (Test-Path -LiteralPath $emulatorPath -PathType Leaf)) {
    throw "EMU.exe not found: $emulatorPath"
}

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$systemRomSource = Join-Path $repositoryRoot "ROM\MXOS_MY.bin"
$flashRomSource = Join-Path $repositoryRoot "FlashDrive\FLASH64k0.BIN"
$configSource = Join-Path $PSScriptRoot "SpecialistMX2_My_MXOS.cfg"

foreach ($sourcePath in @($systemRomSource, $flashRomSource, $configSource)) {
    if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
        throw "Required emulator file not found: $sourcePath"
    }
}

$specialistDirectory = Join-Path $emulatorDirectoryPath "Specialist"
$configDirectory = Join-Path $emulatorDirectoryPath "config"
New-Item -ItemType Directory -Path $specialistDirectory -Force | Out-Null
New-Item -ItemType Directory -Path $configDirectory -Force | Out-Null

Copy-Item -LiteralPath $systemRomSource `
    -Destination (Join-Path $specialistDirectory "MXOS_MY.bin") `
    -Force
Copy-Item -LiteralPath $flashRomSource `
    -Destination (Join-Path $specialistDirectory "FLASH64k.rom") `
    -Force
Copy-Item -LiteralPath $configSource `
    -Destination (Join-Path $configDirectory "SpecialistMX2_My_MXOS.cfg") `
    -Force

Push-Location $emulatorDirectoryPath
try {
    & $emulatorPath "/c" "SpecialistMX2_My_MXOS"
    exit $LASTEXITCODE
}
finally {
    Pop-Location
}
