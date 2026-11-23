[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string] $SourceBase,

    [Parameter(Mandatory = $true, Position = 1)]
    [string] $Extension,

    [Parameter(Mandatory = $true, Position = 2)]
    [string] $Address,

    [Parameter(Position = 3)]
    [string] $OutputBase = $SourceBase,

    [switch] $Force,
    [Parameter(DontShow)]
    [switch] $RunBuild
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

function Get-FullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,

        [Parameter(Mandatory = $true)]
        [string] $BaseDirectory
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $BaseDirectory $Path))
}

$buildDirectory = (Get-Location).ProviderPath
$sourcePath = Get-FullPath -Path ($SourceBase + ".asm") -BaseDirectory $buildDirectory
$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$outputDirectory = Join-Path $repositoryRoot "bin"
$outputPath = Join-Path $outputDirectory ($OutputBase + "." + $Extension)
$listingPath = Join-Path $outputDirectory ($OutputBase + ".lst")
$symbolPath = Join-Path $outputDirectory ($OutputBase + '.sym')
$rawOutputPath = $outputPath
if ($Extension -eq 'cpu') { $rawOutputPath = Join-Path $outputDirectory ($OutputBase + '.i80') }
$assemblerPath = Join-Path $repositoryRoot "tasm\sjasmplus.exe"

$editor = $SourceBase -eq 'EDIT' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\EDIT')
$commander = $SourceBase -eq 'NC' -and $OutputBase -eq 'NC' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\nc')
$dos = $SourceBase -eq 'DOS' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\dos')
$flashDriver = $SourceBase -in @('Flash', 'FlashFst', 'Rom', 'Rom64') -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\flash')
$floppyDriver = $SourceBase -eq 'FLOPPY' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\floppy')
$ramDriver = $SourceBase -eq 'RAMDISK' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\ramdisk')
$monitor2 = $SourceBase -eq 'Monitor2' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\monitor2')
$monitorWin = $SourceBase -eq 'MONWIN' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\monitor_win')

if (-not $RunBuild) {
    $cacheArgs = @((Join-Path $PSScriptRoot 'build_cache.py'), $OutputBase)
    $inputs = @()
    $outputs = @()
    if ($editor -or $commander -or $dos -or $flashDriver -or $ramDriver -or $floppyDriver) {
        $cacheArgs += @('--project-inputs', '--scan-output', $outputDirectory)
        if ($dos) {
            $outputs += @('bin/fonts/Compact.pf', 'source/include/syscall-ids.inc', 'source/bios/syscall-tables.inc',
                'sdk/include/mxos_syscall_ids.h', 'sdk/src/mxos_syscall_ids.inc')
        } else {
            $inputs += 'bin/banked/KERNEL.sym'
        }
        if ($flashDriver) { $inputs += 'bin/BIOS.sym' }
    } else {
        $cacheArgs += @('--asm', $sourcePath, '--output', $listingPath, '--output', $symbolPath, '--output', $outputPath)
        if ($Extension -eq 'cpu') {
            $cacheArgs += @('--output', $rawOutputPath)
            $inputs += 'scripts/images/cpu.ps1'
        }
        if ($SourceBase -eq 'UI_GFX') {
            $inputs += @('scripts/images/executable.py', 'scripts/images/cpu.py')
            $outputs += 'bin/UI_GFX.dll'
        }
        if ($monitor2) {
            $inputs += 'source/monitor2/originals/Monitor2.mon'
            $cacheArgs += @('--hash', (Join-Path $repositoryRoot 'source/monitor2/originals/Monitor2.mon'), '--hash', $outputPath)
        }
        if ($monitorWin) {
            $inputs += @('source/monitor_win/originals/monitor_win.rom', 'scripts/tests/check-monitor-win-build.py')
            $cacheArgs += @('--hash', (Join-Path $repositoryRoot 'source/monitor_win/originals/monitor_win.rom'), '--hash', $outputPath)
        }
    }
    foreach ($path in $inputs) { $cacheArgs += @('--input', (Join-Path $repositoryRoot $path)) }
    foreach ($path in $outputs) { $cacheArgs += @('--output', (Join-Path $repositoryRoot $path)) }
    if ($Force) { $cacheArgs += '--force' }
    $cacheArgs += @('--', $PSCommandPath, $SourceBase, $Extension, $Address, $OutputBase, '-RunBuild')
    & python @cacheArgs
    exit $LASTEXITCODE
}

. (Join-Path $repositoryRoot "scripts/images/cpu.ps1")
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

# DOS требует предварительного измерения блоков и размещения между входами ABI.
if ($editor) {
    & python (Join-Path $PSScriptRoot 'build-editor.py')
    exit $LASTEXITCODE
}
if ($commander) {
    & python (Join-Path $PSScriptRoot 'build-nc.py')
    exit $LASTEXITCODE
}
if ($dos) {
    & python (Join-Path $PSScriptRoot 'build-bios-dos.py')
    exit $LASTEXITCODE
}

# Одноразовый установщик включает переносимую службу и её адреса патчей.
if ($flashDriver) {
    & python (Join-Path $PSScriptRoot 'build-banked-transport.py')
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

# Fixed-address installers are linked from their measured size, against arenaEnd.
if ($flashDriver -or $ramDriver -or $floppyDriver) {
    & python (Join-Path $PSScriptRoot 'build-driver.py') $sourcePath
    exit $LASTEXITCODE
}

Write-Host "[build] $sourcePath"
& $assemblerPath `
    "--i8080" `
    "-Wno-rdlow" `
    ("--lst={0}" -f $listingPath) `
    ("--sym={0}" -f $symbolPath) `
    ("--raw={0}" -f $rawOutputPath) `
    ($sourcePath.Replace('\', '/'))
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

# CPU — текстовые адреса, машинный образ находится в соседнем I80.
if ($Extension -eq 'cpu') {
    $numericAddress = if ($Address -match '^0x') {
        [Convert]::ToInt32($Address.Substring(2), 16)
    } elseif ($Address -match '^\d+$') {
        [int]$Address
    } else {
        # Адреса карты памяти берутся из символов именно собранного компонента.
        $symbolText = [IO.File]::ReadAllText((Join-Path $outputDirectory ($OutputBase + '.sym')))
        $symbolMatch = [regex]::Match($symbolText, '(?m)^' + [regex]::Escape($Address) + ': EQU 0x([0-9A-Fa-f]+)')
        if (-not $symbolMatch.Success) { throw "Unknown load-address symbol: $Address" }
        [Convert]::ToInt32($symbolMatch.Groups[1].Value, 16)
    }
    $symbolText = [IO.File]::ReadAllText($symbolPath)
    $flagsMatch = [regex]::Match($symbolText, '(?m)^IMAGE_FLAGS: EQU 0x([0-9A-Fa-f]+)')
    $imageFlags = if ($flagsMatch.Success) { [Convert]::ToInt32($flagsMatch.Groups[1].Value, 16) } else { 0 }
    $cpuText = New-CpuDescriptor -Load $numericAddress -Flags $imageFlags -Fields (Get-CpuFieldsFromSymbols $symbolText)
    [IO.File]::WriteAllText($outputPath, $cpuText, [Text.Encoding]::ASCII)
    if ($SourceBase -eq 'UI_GFX') {
        & python (Join-Path $repositoryRoot 'scripts/images/executable.py') $outputPath (Join-Path $outputDirectory 'UI_GFX.dll') --format dll
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
}

# Monitor-2 должен точно воспроизводить сохранённое историческое ядро.
if ($monitor2) {
    $referencePath = Join-Path $buildDirectory 'originals\Monitor2.mon'
    $referenceHash = 'E8EABDEE87CBBB679F0D6E7C30BCDB2E48A793FC0A7938D8D9411F8BCAA9555A'
    $referenceBytes = [IO.File]::ReadAllBytes($referencePath)
    $hasher = [Security.Cryptography.SHA256]::Create()
    try {
        $actualReferenceHash = [BitConverter]::ToString($hasher.ComputeHash($referenceBytes)).Replace('-', '')
    }
    finally { $hasher.Dispose() }
    if ($actualReferenceHash -ne $referenceHash) {
        throw "Monitor-2 reference has changed: $referencePath"
    }
    $compiledBytes = [IO.File]::ReadAllBytes($outputPath)
    if ($compiledBytes.Length -ne $referenceBytes.Length) {
        throw "Monitor-2 size mismatch: compiled $($compiledBytes.Length), original $($referenceBytes.Length)"
    }
    for ($index = 0; $index -lt $referenceBytes.Length; $index++) {
        if ($compiledBytes[$index] -ne $referenceBytes[$index]) {
            throw ("Monitor-2 mismatch at offset {0:X4}h (address {1:X4}h): compiled {2:X2}, original {3:X2}" -f `
                $index, (0xC000 + $index), $compiledBytes[$index], $referenceBytes[$index])
        }
    }
    Write-Host ("[verified] Monitor-2: {0} bytes identical to original" -f $compiledBytes.Length)
}
if ($monitorWin) {
    & python (Join-Path $repositoryRoot 'scripts/tests/check-monitor-win-build.py')
    if ($LASTEXITCODE -ne 0) { throw 'Window monitor differs from its historical reference.' }
}
exit 0
