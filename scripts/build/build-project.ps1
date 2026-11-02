[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("OS", "All")]
    [string] $Target = "OS",
    [ValidateSet('Configured', 'Legacy', 'Buffered')]
    [string] $RamDiskDriver = 'Configured'
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$buildScript = Join-Path $PSScriptRoot "build-if-needed.ps1"
$components = @(
    @{ Directory = "source-apps\\chkdsk"; Source = "CHKDSK"; Extension = "com"; Address = "0" },
    @{ Directory = "source\\dos";    Source = "DOS"; Extension = "sys"; Address = "0xC000" },
    @{ Directory = "source\\tape";   Source = "tape";    Extension = "com"; Address = "0xE800" },
    @{ Directory = "source\\flash";  Source = "Flash"; Extension = "com"; Address = "0" },
    @{ Directory = "source\\flash";  Source = "FlashFst"; Extension = "com"; Address = "0" },
    @{ Directory = "source\\flash";  Source = "FlashSys"; Extension = "com"; Address = "0" },
    @{ Directory = "source\\ramd";   Source = "RAMD"; Extension = "com"; Address = "0" },
    @{ Directory = "source\\flash";  Source = "Rom";     Extension = "com"; Address = "0xDE00" },
    @{ Directory = "source\\debug";   Source = "Debug"; Extension = "com"; Address = "0xF100" },
    @{ Directory = "source\\launch"; Source = "LAUNCH";  Extension = "com"; Address = "0xF800" },
    @{ Directory = "source\\ramfos"; Source = "ramfos"; Output = "RAMFOS"; Extension = "sys"; Address = "0xC000" },
    @{ Directory = "source\\ramfos"; Source = "RAMRUN";  Extension = "com"; Address = "0xF800" },
    @{ Directory = "source\\format"; Source = "FORMAT";  Extension = "com"; Address = "0xF100" },
    @{ Directory = "source\\E";      Source = "E";       Extension = "com"; Address = "0xE800" },
    @{ Directory = "source\\nc";     Source = "NC";      Extension = "com"; Address = "0xE800" }
)

if ($Target -eq "All") {
    $components += @{
        Directory = "source\\test-scr"
        Source = "test-scr"
        Extension = "exe"
        Address = "0"
    }
    $components += @{
        Directory = "source\\ramfos"; Source = "test-low"; Output = "RFTEST"; Extension = "com"; Address = "0"
    }
    $components += @{
        Directory = "source\\ramfos"; Source = "test-high"; Output = "RFTESTHI"; Extension = "com"; Address = "0xE800"
    }
}

foreach ($component in $components) {
    Push-Location (Join-Path $repositoryRoot $component.Directory)
    try {
        if ($component.ContainsKey("Output")) {
            & $buildScript $component.Source $component.Extension $component.Address $component.Output -RamDiskDriver $RamDiskDriver
        }
        else {
            & $buildScript $component.Source $component.Extension $component.Address -RamDiskDriver $RamDiskDriver
        }
        if ($LASTEXITCODE -ne 0) { throw "Component assembly failed: $($component.Source)" }
    }
    finally {
        Pop-Location
    }
}

& (Join-Path $repositoryRoot 'source-apps/diskbench/build.ps1')
if ($LASTEXITCODE -ne 0) { throw 'DISKBEN assembly failed.' }
