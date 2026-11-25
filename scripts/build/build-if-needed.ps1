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
    [switch] $Describe,
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
$flashDriver = $SourceBase -in @('Flash', 'FlashFst', 'Rom', 'Rom64', 'SysRom') -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\flash')
$floppyDriver = $SourceBase -eq 'FLOPPY' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\floppy')
$ramDriver = $SourceBase -eq 'RAMDISK' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\ramdisk')
$monitor2 = $SourceBase -eq 'Monitor2' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\monitor2')
$monitorWin = $SourceBase -eq 'MONWIN' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\monitor_win')

if (-not $RunBuild) {
    $cacheArgs = @($OutputBase, '--input-content', '--asm', $sourcePath)
    $inputs = @()
    $outputs = @()
    if ($dos) {
        # The linker selects some INCLUDEs from the layout manifests at build time.
        $inputs += @('source/bios/layout.json', 'source/dos/layout.json', 'source/bios/syscalls.json',
            'assets/system-rom/fonts/6x8/KOI8.pf', 'scripts/build/build-compact-font.py',
            'scripts/build/font-format.py', 'scripts/build/build-syscalls.py',
            'scripts/build/build-banked-transport.py', 'scripts/build/build-banked-dos.py',
            'scripts/build/build-bios-dos.py', 'scripts/build/pack-dos.py')
        foreach ($folder in 'bios', 'dos', 'io', 'mem', 'common', 'bootstrap/tests') {
            $cacheArgs += @('--asm', (Join-Path $repositoryRoot "source/$folder"))
        }
        foreach ($path in 'bin', 'source/include/syscall-ids.inc', 'source/bios/syscall-tables.inc') {
            $cacheArgs += @('--exclude', (Join-Path $repositoryRoot $path))
        }
        $cacheArgs += @('--scan-output', $outputDirectory)
        $outputs += @('bin/BIOS.sys', 'bin/BIOS.sym', 'bin/DOS.sys', 'bin/IO.sys', 'bin/MEM.sys', 'bin/MEM.sym',
            'bin/System.layout.json', 'bin/banked/KERNEL.sym', 'bin/banked/TRANSPORT.sym',
            'bin/fonts/Compact.pf', 'source/include/syscall-ids.inc', 'source/bios/syscall-tables.inc',
            'sdk/include/mxos_syscall_ids.h', 'sdk/src/mxos_syscall_ids.inc')
    } elseif ($editor -or $commander) {
        $inputs += @('bin/banked/KERNEL.sym', 'scripts/build/build-driver.py', 'scripts/build/pack-dos.py',
            'scripts/images/executable.py', 'scripts/images/cpu.py')
        if ($editor) {
            $inputs += 'scripts/build/build-editor.py'
            $cacheArgs += @('--asm', (Join-Path $buildDirectory 'EDIT_DOS.asm'))
            $stems = @('EDIT', 'EDITCORE', 'EDIT_DOS')
            $outputs += @('bin/EDIT_MN.com', 'bin/EDIT.com', 'bin/EDITCORE.dll')
            foreach ($stem in 'EDITHEX', 'EDITTEXT') {
                foreach ($ext in 'cpu', 'i80', 'dll') { $outputs += "bin/$stem.$ext" }
            }
        } else {
            $inputs += 'scripts/build/build-nc.py'
            $cacheArgs += @('--asm', (Join-Path $buildDirectory 'NC_DOS.asm'))
            $stems = @('NC', 'NC_CORE', 'NC_DOS')
            $outputs += @('bin/NC_MN.com', 'bin/NC.com')
        }
        foreach ($stem in $stems) {
            foreach ($ext in 'cpu', 'i80', 'sym', 'lst') { $outputs += "bin/$stem.$ext" }
        }
    } else {
        $cacheArgs += @('--output', $listingPath, '--output', $symbolPath, '--output', $outputPath)
        if ($Extension -eq 'cpu') {
            $cacheArgs += @('--output', $rawOutputPath)
            $inputs += 'scripts/images/cpu.ps1'
        }
        if ($flashDriver -or $ramDriver -or $floppyDriver) {
            $inputs += @('bin/banked/KERNEL.sym', 'scripts/build/build-driver.py', 'scripts/build/pack-dos.py',
                'scripts/images/cpu.py', 'scripts/images/executable.py')
        }
        if ($floppyDriver) {
            $cacheArgs += @('--asm', (Join-Path $buildDirectory 'service.asm'),
                '--exclude', (Join-Path $outputDirectory 'FLOPPYSVC.bin'))
            $outputs += 'bin/FLOPPYSVC.bin'
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
    if ($Describe) {
        return @{ cwd = $buildDirectory; args = $cacheArgs }
    }
    if ($flashDriver) {
        Push-Location (Join-Path $repositoryRoot 'source/dos')
        try {
            $coreJob = & $PSCommandPath DOS sys 0x0100 -Describe
            $coreArgs = $coreJob.args
            & python (Join-Path $PSScriptRoot 'build_cache.py') @coreArgs
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        } finally { Pop-Location }
    }
    & python (Join-Path $PSScriptRoot 'build_cache.py') @cacheArgs
    if ($dos -and $LASTEXITCODE -eq 0) {
        $overlayArgs = @((Join-Path $PSScriptRoot 'build-overlays.py'))
        if ($Force) { $overlayArgs += '--force' }
        & python @overlayArgs
    }
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
    $fields = Get-CpuFieldsFromSymbols $symbolText
    $entry = $numericAddress
    if ($fields.Contains('RELOC_KIND')) {
        $loadMatch = [regex]::Match($symbolText, '(?m)^IMAGE_LOAD_OFFSET: EQU 0x([0-9A-Fa-f]+)')
        $entryMatch = [regex]::Match($symbolText, '(?m)^IMAGE_ENTRY_OFFSET: EQU 0x([0-9A-Fa-f]+)')
        $numericAddress = if ($loadMatch.Success) { [Convert]::ToInt32($loadMatch.Groups[1].Value,16) } else { 0 }
        $entry = if ($entryMatch.Success) { [Convert]::ToInt32($entryMatch.Groups[1].Value,16) } else { $numericAddress }
    }
    $cpuText = New-CpuDescriptor -Load $numericAddress -Entry $entry -Flags $imageFlags -Fields $fields
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
