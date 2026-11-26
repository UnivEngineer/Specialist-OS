[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("OS", "All", "MX")]
    [string] $Target = "OS",
    [switch] $Force,
    [switch] $ComponentsReady,
    [Parameter(DontShow)]
    [switch] $IncludeGames,
    [Parameter(DontShow)]
    [switch] $PrepareBenchmarks
)

if (-not $PrepareBenchmarks) { Write-Host "[build] Components ($Target) started" }
$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0
$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$buildScript = Join-Path $PSScriptRoot 'build-if-needed.ps1'
$allExtras = @(
    @{ Directory = 'source/ramfos'; Source = 'test-low'; Output = 'RFTEST'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source/ramfos'; Source = 'test-high'; Output = 'RFTESTHI'; Extension = 'cpu'; Address = '0xE800' }
)

function Get-ComponentJob($component) {
    Push-Location (Join-Path $repositoryRoot $component.Directory)
    try {
        $output = if ($component.ContainsKey('Output')) { $component.Output } else { $component.Source }
        & $buildScript $component.Source $component.Extension $component.Address $output -Describe -Force:$Force
    } finally { Pop-Location }
}

if ($PrepareBenchmarks) {
    . (Join-Path $repositoryRoot "scripts/images/cpu.ps1")
    & python (Join-Path $repositoryRoot 'source-apps/fontbench/build-standalone.py')
    if ($LASTEXITCODE -ne 0) { throw 'Font benchmark generation failed.' }
    Push-Location (Join-Path $repositoryRoot 'source-apps/fontbench')
    try {
        & (Join-Path $repositoryRoot 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow -DSHIFT_ORIGIN=0x2000 --raw=../../bin/FONTBEN.renderer.bin --sym=../../bin/FONTBEN.renderer.sym engines/SHIFT.asm
        if ($LASTEXITCODE -ne 0) { throw 'Font benchmark renderer assembly failed.' }
        & python (Join-Path $repositoryRoot 'source-apps/fontbench/build-standalone.py') --entries
        if ($LASTEXITCODE -ne 0) { throw 'Font benchmark renderer entries failed.' }
    } finally { Pop-Location }
    Push-Location (Join-Path $repositoryRoot 'source-apps/textbench')
    try {
        foreach ($kind in @('TEXTBEN','FONTBEN')) {
            $fontTest = if ($kind -eq 'FONTBEN') { 1 } else { 0 }
            $moduleName = if ($fontTest) { 'FONTBANK' } else { 'TEXTBANK' }
            & (Join-Path $repositoryRoot 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow "-DFONT_TEST=$fontTest" "--raw=../../bin/$moduleName.i80" "--sym=../../bin/$moduleName.sym" bank-loop.asm
            if ($LASTEXITCODE -ne 0) { throw "$kind bank loop assembly failed." }
            Set-Content -LiteralPath (Join-Path $repositoryRoot "bin/$moduleName.cpu") -Value (New-CpuDescriptor -Load 0x4000 -Flags 2 -Fields (Get-CpuFieldsFromSymbols ([IO.File]::ReadAllText((Join-Path $repositoryRoot "bin/$moduleName.sym"))))) -Encoding ASCII
        }
    } finally { Pop-Location }
    exit 0
}
if ($ComponentsReady -and $Target -ne 'MX') { throw '-ComponentsReady is only supported for MX.' }
if ($IncludeGames -and $Target -ne 'All') { throw '-IncludeGames requires All.' }
$forceArgs = @()
if ($Force) { $forceArgs += '--force' }

function Build-MxImages {
    & python (Join-Path $PSScriptRoot 'build-bootstrap.py') --fonts-ready --font-body (Join-Path $repositoryRoot 'bin/fonts/Rotate.cpu') @forceArgs
    if ($LASTEXITCODE) { throw 'Bootstrap configuration failed.' }
    & python (Join-Path $PSScriptRoot 'build-mx.py') @forceArgs
    if ($LASTEXITCODE) { throw 'Original MX bootstrap failed.' }
    & python (Join-Path $PSScriptRoot 'build-error-strings.py') (Join-Path $repositoryRoot 'bin/ERRORS.DAT')
    if ($LASTEXITCODE) { throw 'Error catalog generation failed.' }
    foreach ($image in 'MxFloppy','MxRom32','MxRom48') {
        & (Join-Path $repositoryRoot 'scripts/images/build-image.ps1') $image -Force:$Force
        if ($LASTEXITCODE) { throw "$image generation failed." }
    }
    Copy-Item -LiteralPath (Join-Path $repositoryRoot 'bin/mx/MxRom2.bin') -Destination (Join-Path $repositoryRoot 'images/C-ROM-2k-MX-boot.bin') -Force
}

if ($ComponentsReady) {
    Build-MxImages
    exit 0
}

$components = @(
    @{ Directory = 'source-apps/test-scr'; Source = 'test-scr'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source/exe'; Source = 'EXE'; Extension = 'cpu'; Address = 'EXE_LOAD_ADDR' },
    @{ Directory = 'source/font'; Source = 'FONT'; Extension = 'cpu'; Address = '0' },
    @{ Directory = "source-apps/test-mx"; Source = "TESTMX"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source-apps\copybench"; Source = "COPYBEN"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source-apps\\chkdsk"; Source = "CHKDSK"; Extension = "cpu"; Address = "0" },
    @{ Directory = 'source/floppy'; Source = 'FLOPPY'; Extension = 'cpu'; Address = 'DOS_BANK_DRIVER_ADDR' },
    @{ Directory = "source\\tape";   Source = "tape";    Extension = "cpu"; Address = "SYSTEM_PROGRAM_ADDR" },
    @{ Directory = "source\\flash";  Source = "Flash"; Extension = "cpu"; Address = "DOS_BANK_DRIVER_ADDR" },
    @{ Directory = "source\\flash";  Source = "FlashFst"; Extension = "cpu"; Address = "DOS_BANK_DRIVER_ADDR" },
    @{ Directory = "source\\flash";  Source = "FlashSys"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\ramdisk";   Source = "RAMDISK"; Extension = "cpu"; Address = "DOS_BANK_DRIVER_ADDR" },
    @{ Directory = "source\\flash";  Source = "Rom";     Extension = "cpu"; Address = "DOS_BANK_DRIVER_ADDR" },
    @{ Directory = "source\\flash";  Source = "Rom64";   Extension = "cpu"; Address = "DOS_BANK_DRIVER_ADDR" },
    @{ Directory = "source\\debug";   Source = "Debug"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\monitor2"; Source = "Monitor2"; Extension = "mon"; Address = "0xC000" },
    @{ Directory = "source\\monitor_win"; Source = "MONWIN"; Extension = "mon"; Address = "0xC000" },
    @{ Directory = "source\\run"; Source = "RUN";  Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\ramfos"; Source = "ramfos"; Output = "RAMFOS"; Extension = "mon"; Address = "0xC000" },
    @{ Directory = "source\\format"; Source = "FORMAT";  Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\EDIT";      Source = "EDIT";       Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\cli";     Source = "CLI"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\nc";     Source = "NC";      Extension = "cpu"; Address = "0" }
)

$components += @(
    @{ Directory = 'source-apps/textbench'; Source = 'STANDALONE'; Output = 'TEXTCOM'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source-apps/fontbench'; Source = 'FONTGRID'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ENGINEM'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ENGINEB'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ENGINE6'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ENGINEP'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ENGINEW'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ORIONM1'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ENGINEQ'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ENGINES'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'ENGINE4'; Extension = 'cpu'; Address = '0x2000' },
    @{ Directory = 'source-apps/fontbench'; Source = 'STANDALONE'; Output = 'FONTCOM'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source-apps/fontbench'; Source = 'LEGACY'; Output = 'FONTLAB'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source-apps/fontbench'; Source = 'BLOCK32'; Output = 'FONTBLK'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source-apps/fontbench'; Source = 'REMAP32'; Output = 'FONTRMP'; Extension = 'cpu'; Address = '0' }
)

& python (Join-Path $PSScriptRoot 'build-syscalls.py')
if ($LASTEXITCODE -ne 0) { throw 'System call registry generation failed.' }
if ($Target -eq 'MX') {
    $components = @($components | Where-Object { $_.Source -in @('TESTMX','test-scr','FLOPPY','EDIT','CLI','FORMAT','DEBUG','CHKDSK','FONT','Flash','FlashFst','Rom','Rom64','RAMDISK','NC') })
} else {
    $benchArgs = @('Benchmark support', '--input-content')
    foreach ($source in 'source-apps/fontbench/engines/SHIFT.asm', 'source-apps/textbench/bank-loop.asm') {
        $benchArgs += @('--asm', (Join-Path $repositoryRoot $source))
    }
    foreach ($path in 'bin/BIOS.sym', 'bin/fonts/Compact.pf', 'assets/system-rom/fonts/6x8/KOI8.pf',
            'assets/system-rom/fonts/4x8/Cpm4.pf', 'source-apps/fontbench/build-standalone.py',
            'source-apps/fontbench/analyze-fontkoi.py', 'scripts/build/font-format.py', 'scripts/images/cpu.ps1') {
        $benchArgs += @('--input', (Join-Path $repositoryRoot $path))
    }
    foreach ($name in 'FONTKOI.packed.glyphs.bin', 'FONTKOI.packed.overrides.inc', 'FONTKOI.packed.bin',
            'FONTKOI.packed.index.bin', 'FONTKOI.packed.codes.bin', 'FONTBEN.shifts.bin', 'FONTBEN.font4.bin',
            'FONTBEN.renderer.bin', 'FONTBEN.renderer.sym', 'FONTBEN.renderer.entries.inc', 'FONTBEN.bios.entries.inc',
            'FONTBANK.cpu', 'FONTBANK.i80', 'FONTBANK.sym', 'TEXTBANK.cpu', 'TEXTBANK.i80', 'TEXTBANK.sym') {
        $benchArgs += @('--output', (Join-Path $repositoryRoot "bin/$name"))
    }
    $benchArgs += $forceArgs
    $benchArgs += @('--', $PSCommandPath, '-PrepareBenchmarks')
}
Write-Host '[prepare] Component build plan'
$planTimer = [Diagnostics.Stopwatch]::StartNew()
$jobs = @(
    # Этот этап пишет в разные файлы и каталоги. DOS ждёт его завершения.
    @{ cwd = $repositoryRoot; describe = @((Join-Path $PSScriptRoot 'build-font-engines.py')) + $forceArgs; serial = $false }
    Get-ComponentJob @{ Directory = 'source/ui-gfx'; Source = 'UI_GFX'; Extension = 'cpu'; Address = 'entry' }
    if ($Target -ne 'MX') {
        @{ cwd = (Get-Location).ProviderPath; describe = @((Join-Path $repositoryRoot 'source-apps/copybench/build.py')) + $forceArgs; serial = $false }
    }
    if ($IncludeGames) {
        & (Join-Path $PSScriptRoot 'build-almaz.ps1') -Describe -Force:$Force
        & (Join-Path $PSScriptRoot 'build-almaz2.ps1') All -PublishMxTimer -Describe -Force:$Force
    }
    Get-ComponentJob @{ Directory = 'source/dos'; Source = 'DOS'; Extension = 'sys'; Address = '0x0100' }
    if ($Target -ne 'MX') { @{ cwd = $repositoryRoot; args = $benchArgs; serial = $true } }
    foreach ($component in $components) { Get-ComponentJob $component }
    if ($Target -eq 'All') {
        foreach ($component in $allExtras) { Get-ComponentJob $component }
    }
)
$jobsJson = ConvertTo-Json -InputObject $jobs -Depth 4 -Compress
Write-Host ('[time] Component build plan: {0:F3} s' -f $planTimer.Elapsed.TotalSeconds)
$OutputEncoding = [Text.UTF8Encoding]::new($false)
$jobsJson | & python -X utf8 (Join-Path $PSScriptRoot 'build_cache.py') --batch
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }

& python (Join-Path $PSScriptRoot 'build-overlays.py') @forceArgs
if ($LASTEXITCODE -ne 0) { throw 'DOS overlay assembly failed.' }

if ($Target -eq 'MX') {
    Build-MxImages
    exit 0
}

& (Join-Path $repositoryRoot 'source-apps/diskbench/build.ps1') -Force:$Force
if ($LASTEXITCODE -ne 0) { throw 'DISKBEN assembly failed.' }
& python (Join-Path $repositoryRoot 'source-apps/sortbench/build.py') @forceArgs
if ($LASTEXITCODE -ne 0) { throw 'SORTBEN assembly failed.' }
