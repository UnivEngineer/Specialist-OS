[CmdletBinding()]
param(
    [ValidateSet("All", "Base", "MX", "MXOptimized", "Ramfos", "RamfosOptimized", "Reference")]
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
    @{ Name = "Base";        Output = "Almaz2-optimized";   Color = 0; Pit = 0; Ramfos = 0; Hud = 1 },
    @{ Name = "MX";          Output = "Almaz2-mx";           Color = 1; Pit = 0; Ramfos = 0; Hud = 1 },
    @{ Name = "MXOptimized"; Output = "Almaz2-mx-optimized"; Color = 1; Pit = 1; Ramfos = 0; Hud = 1 },
    @{ Name = "Ramfos";      Output = "Almaz2-ramfos";       Color = 1; Pit = 0; Ramfos = 1; Hud = 1 },
    @{ Name = "RamfosOptimized"; Output = "Almaz2-ramfos-optimized"; Color = 1; Pit = 1; Ramfos = 1; Hud = 1 },
    @{ Name = "Reference";   Output = "Almaz2";              Color = 0; Pit = 0; Ramfos = 0; Hud = 0 }
)
$selected = @($modes | Where-Object { $Mode -eq "All" -or $_.Name -eq $Mode })
if ($PublishMxTimer -and -not ($selected | Where-Object { $_.Name -eq "MXOptimized" })) {
    throw "PublishMxTimer requires Mode=All or MXOptimized"
}

function Write-AlmazRks([string] $Executable, [string] $Destination) {
    # Начало 0000h и включительный конец — слова little-endian.
    # Контрольная сумма Монитора: перенос в старший байт, кроме последнего.
    $payload = [System.IO.File]::ReadAllBytes($Executable)
    if ($payload.Length -lt 1 -or $payload.Length -ge 0x7F00) {
        throw "Unexpected Almaz2 payload size: $($payload.Length)"
    }
    $endAddress = $payload.Length - 1
    $low = 0
    $high = 0
    for ($i = 0; $i -lt $payload.Length; $i++) {
        $sum = $low + [int]$payload[$i]
        $low = $sum -band 0xFF
        if ($i -lt $payload.Length - 1) {
            $high = ($high + [int]$payload[$i] + ($sum -shr 8)) -band 0xFF
        }
    }
    [byte[]]$rks = @([byte]0, [byte]0, [byte]($endAddress -band 0xFF),
        [byte]($endAddress -shr 8)) + $payload + @([byte]$low, [byte]$high)
    [System.IO.File]::WriteAllBytes($Destination, $rks)
    Write-Host "[app] $Destination ($($rks.Length) bytes)"
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
    Write-AlmazRks "$prefix.exe" "$prefix.rks"
}

if ($PublishMxTimer) {
    $archive = Join-Path $repositoryRoot "assets\games\Almaz-2"
    # Публикация всех выбранных вариантов в обоих форматах.
    foreach ($variant in $selected) {
        $destinationDirectory = $archive
        foreach ($extension in @("exe", "rks")) {
            $fileName = "$($variant.Output).$extension"
            Copy-Item -LiteralPath (Join-Path $outputDirectory $fileName) `
                -Destination (Join-Path $destinationDirectory $fileName) -Force
        }
    }
    Write-Host ("[app] Published {0}" -f $archive)
}
