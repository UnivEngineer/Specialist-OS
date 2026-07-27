[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [ValidateSet("OS", "All")]
    [string] $Target = "OS"
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

$repositoryRoot = Split-Path -Parent $PSScriptRoot
$buildScript = Join-Path $PSScriptRoot "build-if-needed.ps1"
$components = @(
    @{ Directory = "dos";    Source = "DOSChecked"; Output = "DOS"; Extension = "sys"; Address = "0xC000" },
    @{ Directory = "tape";   Source = "tape";    Extension = "com"; Address = "0xE800" },
    @{ Directory = "flash";  Source = "FlashChecked"; Output = "Flash"; Extension = "com"; Address = "0xDE00" },
    @{ Directory = "flash";  Source = "FLSHFAST"; Extension = "com"; Address = "0xDE00" },
    @{ Directory = "flash";  Source = "FlashSys"; Extension = "com"; Address = "0xDC00" },
    @{ Directory = "flash";  Source = "Rom";     Extension = "com"; Address = "0xDE00" },
    @{ Directory = "mon2";   Source = "mon2";    Extension = "com"; Address = "0xF100" },
    @{ Directory = "launch"; Source = "LAUNCH";  Extension = "com"; Address = "0xF800" },
    @{ Directory = "format"; Source = "FORMAT";  Extension = "com"; Address = "0xF100" },
    @{ Directory = "E";      Source = "E";       Extension = "com"; Address = "0xE800" },
    @{ Directory = "nc";     Source = "NC";      Extension = "com"; Address = "0xE800" }
)

if ($Target -eq "All") {
    $components += @{
        Directory = "test-scr"
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
