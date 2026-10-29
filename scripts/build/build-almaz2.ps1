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
    [switch] $Force
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

# Учитываем все исходники игры, включая условные INCLUDE, общий ABI, сборщик
# и ассемблер. Хеш содержимого не зависит от времени файла и смены ветки Git.
$inputPaths = @(
    Get-ChildItem -LiteralPath (Split-Path -Parent $sourcePath) -File |
        Where-Object { $_.Extension -in @('.asm', '.inc') } |
        ForEach-Object { $_.FullName }
)
$inputPaths += @((Join-Path $repositoryRoot 'source\include\mxos.inc'), $assembler, $PSCommandPath)
$inputSignature = (($inputPaths | Sort-Object | ForEach-Object {
    '{0}|{1}' -f $_, (Get-FileHash -LiteralPath $_ -Algorithm SHA256).Hash
}) -join "`n")

function Get-FileSignature([string] $Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { return $null }
    return (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash
}

$modes = @(
    @{ Name = "Base";        Output = "Almaz2-optimized";   Color = 0; Pit = 0; Ramfos = 0; Hud = 1 },
    @{ Name = "Color8";      Output = "Almaz2-8color";       Color = 2; Pit = 0; Ramfos = 0; Hud = 0 },
    @{ Name = "Color8Optimized"; Output = "Almaz2-8color-optimized"; Color = 2; Pit = 0; Ramfos = 0; Hud = 1 },
    @{ Name = "MX";          Output = "Almaz2-mx";           Color = 1; Pit = 0; Ramfos = 0; Hud = 0 },
    @{ Name = "MXOptimized"; Output = "Almaz2-mx-optimized"; Color = 1; Pit = 1; Ramfos = 0; Hud = 1 },
    @{ Name = "Ramfos";      Output = "Almaz2-ramfos";       Color = 1; Pit = 0; Ramfos = 1; Hud = 0 },
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
    $optimize = if ($variant.Name -in @("Reference", "Color8", "MX", "Ramfos") -or $NoOptimizations) { 0 } else { 1 }
    $extraDefines += "-DALMAZ_OPTIMIZE=$optimize"
    $monitorTarget = if ($variant.Name -in @("Reference", "Base", "Color8", "Color8Optimized")) { 1 } else { 0 }
    $extraDefines += "-DALMAZ_MONITOR_TARGET=$monitorTarget"
    if ($StepHz -gt 0) { $extraDefines += "-DALMAZ_STEP_HZ=$StepHz" }
    if ($InputCellStride -gt 0) { $extraDefines += "-DINPUT_POLL_CELL_STRIDE=$InputCellStride" }
    if ($PitClockHz -gt 0) { $extraDefines += "-DPIT_CLOCK_HZ=$PitClockHz" }
    $stampPath = "$prefix.build.json"
    $variantSignature = @($inputSignature, $variant.Name, $variant.Color, $variant.Pit,
        $variant.Ramfos, $variant.Hud, ($extraDefines -join ' ')) -join "`n"
    $outputs = @("$prefix.exe", "$prefix.rks", "$prefix.lst", "$prefix.sym")
    $needsBuild = $true
    if (-not $Force -and (Test-Path -LiteralPath $stampPath -PathType Leaf)) {
        try {
            $stamp = Get-Content -LiteralPath $stampPath -Raw | ConvertFrom-Json
            $needsBuild = $stamp.signature -ne $variantSignature
            foreach ($output in $outputs) {
                $currentHash = Get-FileSignature $output
                $name = [IO.Path]::GetExtension($output).TrimStart('.')
                if (-not $currentHash -or $currentHash -ne $stamp.outputs.$name) { $needsBuild = $true }
            }
        }
        catch { $needsBuild = $true }
    }
    if (-not $needsBuild) {
        Write-Host ("[up-to-date] {0}" -f $variant.Name)
        continue
    }
    # При ошибке сборки старый штамп не должен подтверждать частичные результаты.
    Remove-Item -LiteralPath $stampPath -Force -ErrorAction SilentlyContinue
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
    $outputHashes = @{}
    foreach ($output in $outputs) {
        $name = [IO.Path]::GetExtension($output).TrimStart('.')
        $outputHashes[$name] = Get-FileSignature $output
        if (-not $outputHashes[$name]) { throw "Build output missing: $output" }
    }
    @{ signature = $variantSignature; outputs = $outputHashes } | ConvertTo-Json |
        Set-Content -LiteralPath $stampPath -Encoding UTF8
}

if ($PublishMxTimer) {
    $archive = Join-Path $repositoryRoot "assets\games\Almaz-2"
    # Публикация всех выбранных вариантов в обоих форматах.
    foreach ($variant in $selected) {
        $destinationDirectory = $archive
        foreach ($extension in @("exe", "rks")) {
            $fileName = "$($variant.Output).$extension"
            $source = Join-Path $outputDirectory $fileName
            $destination = Join-Path $destinationDirectory $fileName
            if ((Get-FileSignature $source) -ne (Get-FileSignature $destination)) {
                Copy-Item -LiteralPath $source -Destination $destination -Force
            }
        }
    }
    Write-Host ("[app] Published {0}" -f $archive)
}
