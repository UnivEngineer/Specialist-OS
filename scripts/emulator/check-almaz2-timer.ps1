[CmdletBinding()]
param([string] $EmulatorDirectory = $env:SPECIALIST_EMU_DIR)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0
if (-not $EmulatorDirectory) {
    $EmulatorDirectory = [Environment]::GetEnvironmentVariable("SPECIALIST_EMU_DIR", "User")
}
if (-not $EmulatorDirectory) { throw "SPECIALIST_EMU_DIR is not configured" }
$configName = "SpecialistMX2_My_MXOS.cfg"
$repositoryConfig = Join-Path $PSScriptRoot $configName
$deployedConfig = Join-Path $EmulatorDirectory ("config\" + $configName)
$executable = Join-Path $EmulatorDirectory "EMU.exe"

if ((Get-FileHash -LiteralPath $repositoryConfig).Hash -ne
    (Get-FileHash -LiteralPath $deployedConfig).Hash) {
    throw "Repository and deployed emulator configurations differ"
}
$config = [IO.File]::ReadAllText($repositoryConfig, [Text.Encoding]::ASCII)
$expected = @(
    'main.CPUClock=2MHz', 'tim : K580wi53 {',
    'clock[0].rate=1/1', 'clock[1].rate=1/1',
    'clock[2]=tim.out[1]', '~gate[0]=tim.out[2]',
    'gate[1].value=1', 'gate[2].value=1',
    'map[0C-0F]=tim.data', 'input[0]=tim.out[0]'
)
foreach ($line in $expected) {
    if (-not [regex]::IsMatch($config, '(?m)^\s*' + [regex]::Escape($line) + '\s*$')) {
        throw "Missing emulator configuration line: $line"
    }
}
$binaryText = [Text.Encoding]::ASCII.GetString([IO.File]::ReadAllBytes($executable))
if ($binaryText.IndexOf("k580wi53", [StringComparison]::OrdinalIgnoreCase) -lt 0) {
    throw "K580wi53 device name is absent from EMU.exe"
}
Write-Host "OK: K580wi53 in EMU.exe; deployed configuration matches repository."
Write-Host "OK: CPU 2 MHz; channels at FFEC-FFEF; OUT1 -> CLK2; OUT2 -> inverted GATE0; OUT0 -> beep."
Write-Host "Static inspection only; emulator was not launched."
