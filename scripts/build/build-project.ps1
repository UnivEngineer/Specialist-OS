[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("OS", "All")]
    [string] $Target = "OS"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$buildScript = Join-Path $PSScriptRoot "build-if-needed.ps1"
& python (Join-Path $PSScriptRoot 'build-compact-font.py')
if ($LASTEXITCODE -ne 0) { throw 'Compact font generation failed.' }
& python (Join-Path $repositoryRoot 'source-apps/copybench/build.py')
if ($LASTEXITCODE -ne 0) { throw 'Copy benchmark generation failed.' }
& python (Join-Path $repositoryRoot 'source-apps/fontbench/build-standalone.py')
if ($LASTEXITCODE -ne 0) { throw 'Font benchmark generation failed.' }
Push-Location (Join-Path $repositoryRoot 'source-apps/fontbench')
try {
    & (Join-Path $repositoryRoot 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow -DMODE=1 -DBATCH=0 -DSHIFT_ORIGIN=0x2000 --raw=../../bin/FONTBEN.renderer.bin SHIFT.asm
    if ($LASTEXITCODE -ne 0) { throw 'Font benchmark renderer assembly failed.' }
} finally { Pop-Location }
Push-Location (Join-Path $repositoryRoot 'source-apps/textbench')
try {
    foreach ($kind in @('TEXTBEN','FONTBEN')) {
        $fontTest = if ($kind -eq 'FONTBEN') { 1 } else { 0 }
        & (Join-Path $repositoryRoot 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow "-DFONT_TEST=$fontTest" "--raw=../../bin/$kind.bank.bin" bank-loop.asm
        if ($LASTEXITCODE -ne 0) { throw "$kind bank loop assembly failed." }
    }
} finally { Pop-Location }
$components = @(
    @{ Directory = "source-apps\copybench"; Source = "COPYBEN"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source-apps\\chkdsk"; Source = "CHKDSK"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\dos";    Source = "DOS"; Extension = "sys"; Address = "0xC000" },
    @{ Directory = "source\\tape";   Source = "tape";    Extension = "cpu"; Address = "SYSTEM_PROGRAM_ADDR" },
    @{ Directory = "source\\flash";  Source = "Flash"; Extension = "cpu"; Address = "DOS_BANK_DRIVER_ADDR" },
    @{ Directory = "source\\flash";  Source = "FlashFst"; Extension = "cpu"; Address = "DOS_BANK_DRIVER_ADDR" },
    @{ Directory = "source\\flash";  Source = "FlashSys"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\ramd";   Source = "RAMD"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\flash";  Source = "Rom";     Extension = "cpu"; Address = "DOS_BANK_DRIVER_ADDR" },
    @{ Directory = "source\\debug";   Source = "Debug"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\monitor2"; Source = "Monitor2"; Extension = "mon"; Address = "0xC000" },
    @{ Directory = "source\\run"; Source = "RUN";  Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\ramfos"; Source = "ramfos"; Output = "RAMFOS"; Extension = "mon"; Address = "0xC000" },
    @{ Directory = "source\\format"; Source = "FORMAT";  Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\E";      Source = "E";       Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\cli";     Source = "CLI"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\nc";     Source = "NC";      Extension = "cpu"; Address = "0" }
)

if ($Target -eq "All") {
    $components += @{
        Directory = "source\\test-scr"
        Source = "test-scr"
        Extension = "cpu"
        Address = "0"
    }
    $components += @{
        Directory = "source\\ramfos"; Source = "test-low"; Output = "RFTEST"; Extension = "cpu"; Address = "0"
    }
    $components += @{
        Directory = "source\\ramfos"; Source = "test-high"; Output = "RFTESTHI"; Extension = "cpu"; Address = "0xE800"
    }
}

$components += @(
    @{ Directory = 'source-apps/textbench'; Source = 'STANDALONE'; Output = 'TEXTCOM'; Extension = 'cpu'; Address = '0' },
    @{ Directory = 'source-apps/fontbench'; Source = 'STANDALONE'; Output = 'FONTCOM'; Extension = 'cpu'; Address = '0' }
)
foreach ($component in $components) {
    Push-Location (Join-Path $repositoryRoot $component.Directory)
    try {
        if ($component.ContainsKey("Output")) {
            & $buildScript $component.Source $component.Extension $component.Address $component.Output
        }
        else {
            & $buildScript $component.Source $component.Extension $component.Address
        }
        if ($LASTEXITCODE -ne 0) { throw "Component assembly failed: $($component.Source)" }
    }
    finally {
        Pop-Location
    }
}

& (Join-Path $repositoryRoot 'source-apps/diskbench/build.ps1')
if ($LASTEXITCODE -ne 0) { throw 'DISKBEN assembly failed.' }
