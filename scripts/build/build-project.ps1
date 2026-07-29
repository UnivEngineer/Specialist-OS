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
$components = @(
    @{ Directory = "source\\dos";    Source = "DOSChecked"; Output = "DOS"; Extension = "sys"; Address = "0xC000" },
    @{ Directory = "source\\tape";   Source = "tape";    Extension = "com"; Address = "0xE800" },
    @{ Directory = "source\\flash";  Source = "FlashChecked"; Output = "Flash"; Extension = "com"; Address = "0xDE00" },
    @{ Directory = "source\\flash";  Source = "FlashFst"; Extension = "com"; Address = "0xDE00" },
    @{ Directory = "source\\flash";  Source = "FlashSys"; Extension = "com"; Address = "0xDC00" },
    @{ Directory = "source\\flash";  Source = "Rom";     Extension = "com"; Address = "0xDE00" },
    @{ Directory = "source\\mon2";   Source = "mon2";    Extension = "com"; Address = "0xF100" },
    @{ Directory = "source\\launch"; Source = "LAUNCH";  Extension = "com"; Address = "0xF800" },
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
    }
    finally {
        Pop-Location
    }
}
