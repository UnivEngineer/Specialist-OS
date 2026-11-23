[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateSet('MMX Flash + floppy', 'MX ROM + floppy', 'RAMFOS')]
    [string] $Profile,
    [ValidateSet(2, 32, 48)]
    [int] $RomKiB = 48,
    [string] $EmulatorDirectory,
    [switch] $Build,
    [switch] $PrepareOnly,
    [switch] $ExternalFlash,
    [switch] $Mute
)

$ErrorActionPreference = 'Stop'
$launch = @{ PrepareOnly = $PrepareOnly }
if ($EmulatorDirectory) { $launch.EmulatorDirectory = $EmulatorDirectory }
if ($Build) {
    if ($Profile -eq 'MX ROM + floppy' -and $ExternalFlash) {
        & (Join-Path $PSScriptRoot '../build/build-project.ps1') MX
    } else {
        & (Join-Path $PSScriptRoot '../build/build-profile.ps1') $Profile
    }
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
}

switch ($Profile) {
    'MMX Flash + floppy' { & (Join-Path $PSScriptRoot 'run.ps1') -Flash -SyncProfileImages -Mute:$Mute @launch }
    'MX ROM + floppy'  { & (Join-Path $PSScriptRoot 'run-mx.ps1') -RomKiB $RomKiB -Flash:$ExternalFlash -Mute:$Mute @launch }
    'RAMFOS'     { & (Join-Path $PSScriptRoot 'run-mx.ps1') -Ramfos @launch }
}
if ($Build) { Write-Host 'Done.' }
