[CmdletBinding()]
param(
    [ValidateSet("All", "Base", "MXSoft", "MXTimer", "RamfosSoft", "RamfosTimer", "Reference")]
    [string] $Mode = "All",
    [ValidateSet(0, 2000000, 2500000)]
    [int] $PitClockHz = 0,
    [int] $StepHz = 0,
    [ValidateRange(0, 345)]
    [int] $InputCellStride = 0,
    [switch] $NoOptimizations,
    [switch] $PublishMxTimer
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0
if ($StepHz -ne 0 -and ($StepHz -lt 1 -or $StepHz -gt 50)) {
    throw "StepHz must be 1..50, or 0 to use config.inc"
}
$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$assembler = Join-Path $repositoryRoot "tasm\sjasmplus.exe"
$sourcePath = Join-Path $repositoryRoot "source-apps\almaz2\Almaz2.asm"
$outputDirectory = Join-Path $repositoryRoot "bin"
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$modes = @(
    @{ Name = "Base";        Output = "Almaz2-base";        Color = 0; Pit = 0; Ramfos = 0; Hud = 1 },
    @{ Name = "MXSoft";      Output = "Almaz2-mx-soft";     Color = 1; Pit = 0; Ramfos = 0; Hud = 1 },
    @{ Name = "MXTimer";     Output = "Almaz2X";            Color = 1; Pit = 1; Ramfos = 0; Hud = 1 },
    @{ Name = "RamfosSoft";  Output = "Almaz2-ramfos-soft"; Color = 1; Pit = 0; Ramfos = 1; Hud = 1 },
    @{ Name = "RamfosTimer"; Output = "Almaz2-ramfos-pit";  Color = 1; Pit = 1; Ramfos = 1; Hud = 1 },
    @{ Name = "Reference";   Output = "Almaz2-reference";   Color = 0; Pit = 0; Ramfos = 0; Hud = 0 }
)
$selected = @($modes | Where-Object { $Mode -eq "All" -or $_.Name -eq $Mode })
if ($PublishMxTimer -and -not ($selected | Where-Object { $_.Name -eq "MXTimer" })) {
    throw "PublishMxTimer requires Mode=All or MXTimer"
}

foreach ($variant in $selected) {
    $prefix = Join-Path $outputDirectory $variant.Output
    $extraDefines = @()
    if ($variant.Name -eq "Reference" -or $NoOptimizations) { $extraDefines += "-DALMAZ_OPTIMIZE=0" }
    if ($StepHz -gt 0) { $extraDefines += "-DALMAZ_STEP_HZ=$StepHz" }
    if ($InputCellStride -gt 0) { $extraDefines += "-DINPUT_POLL_CELL_STRIDE=$InputCellStride" }
    if ($PitClockHz -gt 0) { $extraDefines += "-DPIT_CLOCK_HZ=$PitClockHz" }
    $clockDescription = if ($PitClockHz -gt 0) { "$PitClockHz Hz" } else { "config.inc" }
    Write-Host ("[app] {0}, PIT input {1}" -f $variant.Name, $clockDescription)
    & $assembler "--i8080" "-Wno-rdlow" `
        ("-DALMAZ_COLOR={0}" -f $variant.Color) `
        ("-DALMAZ_PIT={0}" -f $variant.Pit) `
        ("-DALMAZ_RAMFOS={0}" -f $variant.Ramfos) `
        ("-DALMAZ_HUD={0}" -f $variant.Hud) `
        @extraDefines `
        ("--raw={0}.exe" -f $prefix) ("--lst={0}.lst" -f $prefix) `
        ("--sym={0}.sym" -f $prefix) $sourcePath
    if ($LASTEXITCODE -ne 0) { throw "Assembly failed: $($variant.Name)" }
}

if ($PublishMxTimer) {
    # Историческая исправленная версия должна остаться независимым эталоном.
    $legacy = Join-Path $repositoryRoot "assets\games\originals\Almaz2X-mxos-legacy.bin"
    $target = Join-Path $repositoryRoot "assets\games\Almaz2X.exe"
    $legacyHash = "DF73430DD8F6D17A40AAB8FEDFA75BA21663DA13EF0C2F888DBCCB5D4B0A6713"
    if (-not (Test-Path -LiteralPath $legacy)) {
        if ((Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash -ne $legacyHash) {
            throw "Cannot create legacy backup from an unknown Almaz2X.exe"
        }
        Copy-Item -LiteralPath $target -Destination $legacy
    }
    if ((Get-FileHash -LiteralPath $legacy -Algorithm SHA256).Hash -ne $legacyHash) {
        throw "Legacy Almaz2X backup SHA-256 differs"
    }
    Copy-Item -LiteralPath (Join-Path $outputDirectory "Almaz2X.exe") -Destination $target -Force
    Write-Host ("[app] Published {0}" -f $target)
}
