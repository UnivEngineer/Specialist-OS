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
$components = @(
    @{ Directory = "source-apps\copybench"; Source = "COPYBEN"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source-apps\\chkdsk"; Source = "CHKDSK"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\dos";    Source = "DOS"; Extension = "sys"; Address = "0xC000" },
    @{ Directory = "source\\tape";   Source = "tape";    Extension = "cpu"; Address = "SYSTEM_PROGRAM_ADDR" },
    @{ Directory = "source\\flash";  Source = "Flash"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\flash";  Source = "FlashFst"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\flash";  Source = "FlashSys"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\ramd";   Source = "RAMD"; Extension = "cpu"; Address = "0" },
    @{ Directory = "source\\flash";  Source = "Rom";     Extension = "cpu"; Address = "0" },
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
