[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('Flash2M', 'CPM', 'RAMFOS')]
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
        & (Join-Path $scripts 'build/build-project.ps1') OS
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        $buildImage = Join-Path $scripts 'images/build-image.ps1'
        foreach ($target in 'SystemEmuWritable', 'Games2M') {
            & $buildImage $target
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
        & (Join-Path $PSScriptRoot 'seed-flash.ps1')
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        & (Join-Path $PSScriptRoot 'run.ps1') -Flash -ResetFlash @launch
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
