[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('Flash2M', 'Floppy720', 'CPM', 'RAMFOS')]
    [string] $Profile,
    [string] $EmulatorDirectory,
    [switch] $PrepareOnly,
    [switch] $Mute
)

$ErrorActionPreference = 'Stop'
$scripts = Split-Path -Parent $PSScriptRoot
$launch = @{ PrepareOnly = $PrepareOnly; Mute = $Mute }
if ($EmulatorDirectory) { $launch.EmulatorDirectory = $EmulatorDirectory }

switch ($Profile) {
    'Flash2M' {
        & (Join-Path $PSScriptRoot 'run-profile.ps1') 'MMX Flash + floppy' -Build @launch
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    'Floppy720' {
        & (Join-Path $scripts 'images/build-image.ps1') Floppy720
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & (Join-Path $PSScriptRoot 'run.ps1') -Flash -FreshFloppy @launch
    }
    'CPM' {
        & python -X utf8 (Join-Path $scripts 'images/build-cpm.py')
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & (Join-Path $PSScriptRoot 'run-cpm.ps1') @launch
    }
    'RAMFOS' {
        & python -X utf8 (Join-Path $scripts 'images/build-ramfos.py')
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & (Join-Path $PSScriptRoot 'run-ramfos.ps1') @launch
    }
}
