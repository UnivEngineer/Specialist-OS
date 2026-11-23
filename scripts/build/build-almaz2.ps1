[CmdletBinding()]
param(
    [ValidateSet("All", "Base", "Color8", "Color8Optimized", "MX", "MXOptimized", "Ramfos", "RamfosOptimized", "Reference")]
    [string] $Mode = "All",
    [ValidateSet(0, 2000000, 2500000)]
    [int] $PitClockHz = 0,
    [int] $StepHz = 0,
    [ValidateRange(0, 345)]
    [int] $InputCellStride = 0,
    [switch] $NoOptimizations,
    [switch] $PublishMxTimer,
    [switch] $Force,
    [Parameter(DontShow)]
    [switch] $Describe,
    [Parameter(DontShow)]
    [switch] $RunBuild
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
$archive = Join-Path $repositoryRoot 'assets\games\families\almaz-2\builds'

function Get-FileSignature([string] $Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

$modes = @(
    @{ Name = "Base";        Output = "Almaz2-optimized";   Color = 0; Pit = 0; Ramfos = 0; Hud = 1; Format = 'rks' },
    @{ Name = "Color8";      Output = "Almaz2-8color";       Color = 2; Pit = 0; Ramfos = 0; Hud = 0; Format = 'rks' },
    @{ Name = "Color8Optimized"; Output = "Almaz2-8color-optimized"; Color = 2; Pit = 0; Ramfos = 0; Hud = 1; Format = 'rks' },
    @{ Name = "MX";          Output = "Almaz2-mx";           Color = 1; Pit = 0; Ramfos = 0; Hud = 0; Format = 'exe' },
    @{ Name = "MXOptimized"; Output = "Almaz2-mx-optimized"; Color = 1; Pit = 1; Ramfos = 0; Hud = 1; Format = 'exe' },
    @{ Name = "Ramfos";      Output = "Almaz2-ramfos";       Color = 1; Pit = 0; Ramfos = 1; Hud = 0; Format = 'exe' },
    @{ Name = "RamfosOptimized"; Output = "Almaz2-ramfos-optimized"; Color = 1; Pit = 1; Ramfos = 1; Hud = 1; Format = 'exe' },
    @{ Name = "Reference";   Output = "Almaz2";              Color = 0; Pit = 0; Ramfos = 0; Hud = 0; Format = 'rks' }
)
$selected = @($modes | Where-Object { $Mode -eq "All" -or $_.Name -eq $Mode })
if (-not $RunBuild) {
    $cacheArgs = @("Almaz-2 ($Mode)", '--content', '--asm', $sourcePath)
    $cacheArgs += @('--input', (Join-Path $repositoryRoot 'scripts/images/executable.ps1'),
        '--input', (Join-Path $repositoryRoot 'scripts/images/cpu.ps1'))
    foreach ($variant in $selected) {
        $stem = $variant.Output
        foreach ($extension in 'cpu', 'i80', $variant.Format, 'lst', 'sym') {
            $cacheArgs += @('--output', (Join-Path $outputDirectory "$stem.$extension"))
        }
        if ($PublishMxTimer) {
            $cacheArgs += @('--output', (Join-Path $archive "$stem.$($variant.Format)"))
        }
    }
    if ($Force) { $cacheArgs += '--force' }
    $cacheArgs += @('--', $PSCommandPath, '-Mode', $Mode, '-PitClockHz', $PitClockHz, '-StepHz', $StepHz,
        '-InputCellStride', $InputCellStride, '-RunBuild')
    if ($NoOptimizations) { $cacheArgs += '-NoOptimizations' }
    if ($PublishMxTimer) { $cacheArgs += '-PublishMxTimer' }
    if ($Describe) { return @{ cwd = (Get-Location).ProviderPath; args = [string[]]$cacheArgs } }
    & python (Join-Path $PSScriptRoot 'build_cache.py') @cacheArgs
    exit $LASTEXITCODE
}

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
if ($PublishMxTimer) { New-Item -ItemType Directory -Path $archive -Force | Out-Null }
. (Join-Path $repositoryRoot 'scripts/images/executable.ps1')

foreach ($variant in $selected) {
    $prefix = Join-Path $outputDirectory $variant.Output
    $extraDefines = @()
    $optimize = if ($variant.Name -in @("Reference", "Color8", "MX", "Ramfos") -or $NoOptimizations) { 0 } else { 1 }
    $extraDefines += "-DALMAZ_OPTIMIZE=$optimize"
    $monitorTarget = [int]($variant.Format -eq 'rks')
    $extraDefines += "-DALMAZ_MONITOR_TARGET=$monitorTarget"
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
        ("--raw={0}.i80" -f $prefix) ("--lst={0}.lst" -f $prefix) `
        ("--sym={0}.sym" -f $prefix) ($sourcePath.Replace('\', '/'))
    if ($LASTEXITCODE -ne 0) { throw "Assembly failed: $($variant.Name)" }
    $payloadSize = (Get-Item -LiteralPath "$prefix.i80").Length
    if ($payloadSize -lt 1 -or $payloadSize -ge 0x7F00) {
        throw "Unexpected Almaz2 payload size: $payloadSize"
    }
    $descriptor = switch ($variant.Name) {
        "MXOptimized" {
            # Игра использует собственный стек и рабочую карту до 81FFh.
            New-CpuDescriptor -Load 0 -Flags 4 -Fields ([ordered]@{POLICY='EXCLUSIVE'; MEM_END='8200'})
        }
        "MX" { New-CpuDescriptor -Load 0 -Monitor 'mxos2.mon' -Fields @{MODE='MX'} }
        { $variant.Ramfos } { New-CpuDescriptor -Load 0 -Monitor 'ramfos.mon' -Fields @{MODE='RF'} }
        default { New-CpuDescriptor -Load 0 }
    }
    [IO.File]::WriteAllText("$prefix.cpu", $descriptor, [Text.Encoding]::ASCII)
    $packed = Convert-ExecutableFile "$prefix.cpu" $variant.Format
    $fileName = "$($variant.Output).$($variant.Format)"
    $source = Join-Path $outputDirectory $fileName
    [IO.File]::WriteAllBytes($source, $packed)
    if ($PublishMxTimer) {
        $destination = Join-Path $archive $fileName
        if ((Get-FileSignature $source) -ne (Get-FileSignature $destination)) {
            Copy-Item -LiteralPath $source -Destination $destination -Force
        }
    }
}
if ($PublishMxTimer) {
    Write-Host ("[app] Published {0}" -f $archive)
}
