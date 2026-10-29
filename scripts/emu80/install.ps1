[CmdletBinding()]
param([Parameter(Mandatory = $true)][string] $DestinationDirectory)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$lock = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'upstream.json') -Raw | ConvertFrom-Json
$destination = [IO.Path]::GetFullPath($DestinationDirectory)
if (Test-Path -LiteralPath $destination) { throw "Destination already exists; choose a new directory: $destination" }
$archive = Join-Path ([IO.Path]::GetTempPath()) ($lock.archiveDirectory + '.zip')
if (-not (Test-Path -LiteralPath $archive)) {
    Invoke-WebRequest -UseBasicParsing -Uri $lock.archiveUrl -OutFile $archive
}
if ((Get-FileHash -LiteralPath $archive -Algorithm SHA256).Hash.ToLowerInvariant() -ne $lock.archiveSha256) {
    throw "Distribution checksum mismatch: $archive"
}
Expand-Archive -LiteralPath $archive -DestinationPath $destination
$runtime = Join-Path $destination $lock.archiveDirectory
if (-not (Test-Path -LiteralPath (Join-Path $runtime 'Emu80qt.exe'))) { throw 'Archive does not contain Emu80qt.exe' }
Write-Host "Installed: $runtime"
Write-Host "Set SPECIALIST_EMU80_DIR to this directory, or pass -EmulatorDirectory to run.ps1."
