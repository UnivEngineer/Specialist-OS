[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("OS", "All")]
    [string] $Target = "OS",
    [switch] $Force,
    [Parameter(DontShow)]
    [switch] $RunBuild
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0
$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$buildScript = Join-Path $PSScriptRoot 'build-if-needed.ps1'
$allExtras = @(
    @{ Directory = 'source/ramfos'; Source = 'test-low'; Output = 'RFTEST'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source/ramfos'; Source = 'test-high'; Output = 'RFTESTHI'; Extension = 'cpu'; Address = '0xE800' }
)

if (-not $RunBuild) {
    $cacheArgs = @((Join-Path $PSScriptRoot 'build_cache.py'), "build: $Target")
    if ($Target -eq 'All') {
        & $PSCommandPath OS -Force:$Force
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $cacheArgs += @('--input', $buildScript, '--input', (Join-Path $repositoryRoot 'scripts/images/cpu.ps1'))
        foreach ($component in $allExtras) {
            $source = "$($component.Directory)/$($component.Source).asm"
            $cacheArgs += @('--asm', (Join-Path $repositoryRoot $source))
            foreach ($extension in 'cpu', 'i80', 'lst', 'sym') {
                $cacheArgs += @('--output', (Join-Path $repositoryRoot "bin/$($component.Output).$extension"))
            }
        }
    } else {
        & python (Join-Path $PSScriptRoot 'build-syscalls.py')
        if ($LASTEXITCODE -ne 0) { throw 'System call registry generation failed.' }
        $fontArgs = @((Join-Path $PSScriptRoot 'build-font-engines.py'))
        if ($Force) { $fontArgs += '--force' }
        & python @fontArgs
        if ($LASTEXITCODE -ne 0) { throw 'Font engine build failed.' }
        $cacheArgs += @('--project-inputs', '--input', (Join-Path $repositoryRoot 'bin/fonts'))
        foreach ($directory in 'bin', 'source-apps/diskbench/build', 'source-apps/sortbench/build') {
            $cacheArgs += @('--scan-output', (Join-Path $repositoryRoot $directory))
        }
        foreach ($path in 'bin/fonts/Compact.pf', 'source/include/syscall-ids.inc', 'source/bios/syscall-tables.inc',
                'sdk/include/mxos_syscall_ids.h', 'sdk/src/mxos_syscall_ids.inc') {
            $cacheArgs += @('--output', (Join-Path $repositoryRoot $path))
        }
        foreach ($path in 'source/monitor2/originals/Monitor2.mon', 'bin/Monitor2.mon',
                'source/monitor_win/originals/monitor_win.rom', 'bin/MONWIN.mon') {
            $cacheArgs += @('--hash', (Join-Path $repositoryRoot $path))
        }
    }
    if ($Force) { $cacheArgs += '--force' }
    $cacheArgs += @('--', $PSCommandPath, $Target, '-RunBuild')
    & python @cacheArgs
    exit $LASTEXITCODE
}

function Build-Component($component) {
    Push-Location (Join-Path $repositoryRoot $component.Directory)
    try {
        $output = if ($component.ContainsKey('Output')) { $component.Output } else { $component.Source }
        if ($output -eq 'FONTCOM') {
            & python (Join-Path $repositoryRoot 'source-apps/fontbench/build-standalone.py') --entries
            if ($LASTEXITCODE -ne 0) { throw 'Font benchmark ABI entries failed.' }
        }
        & $buildScript $component.Source $component.Extension $component.Address $output -RunBuild
        if ($LASTEXITCODE -ne 0) { throw "Component assembly failed: $($component.Source)" }
    } finally { Pop-Location }
}

if ($Target -eq 'All') {
    foreach ($component in $allExtras) { Build-Component $component }
    exit 0
}

. (Join-Path $repositoryRoot "scripts/images/cpu.ps1")
& python (Join-Path $repositoryRoot 'source-apps/copybench/build.py') --run-build
if ($LASTEXITCODE -ne 0) { throw 'Copy benchmark generation failed.' }
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
$components = @(
    @{ Directory = 'source-apps/test-scr'; Source = 'test-scr'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source/ui-gfx'; Source = 'UI_GFX'; Extension = 'cpu'; Address = 'GFX_BASE' },
    @{ Directory = 'source/exe'; Source = 'EXE'; Extension = 'cpu'; Address = 'EXE_LOAD_ADDR' },
    @{ Directory = 'source/font'; Source = 'FONT'; Extension = 'cpu'; Address = '0' },
    @{ Directory = "source-apps/test-mx"; Source = "TESTMX"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source-apps\copybench"; Source = "COPYBEN"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source-apps\\chkdsk"; Source = "CHKDSK"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\dos";    Source = "DOS"; Extension = "sys"; Address = "0x0100" },
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
foreach ($component in $components) {
    Build-Component $component
}

& (Join-Path $repositoryRoot 'source-apps/diskbench/build.ps1') -RunBuild
if ($LASTEXITCODE -ne 0) { throw 'DISKBEN assembly failed.' }
& python (Join-Path $repositoryRoot 'source-apps/sortbench/build.py') --run-build
if ($LASTEXITCODE -ne 0) { throw 'SORTBEN assembly failed.' }
