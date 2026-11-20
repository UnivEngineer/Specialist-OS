[CmdletBinding()]
param()

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$source = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$localAppData = [Environment]::GetFolderPath('LocalApplicationData')
if (-not $localAppData -or [IO.DriveInfo]::new([IO.Path]::GetPathRoot($localAppData)).DriveType -ne 'Fixed') {
    throw 'LocalAppData must be on a local drive.'
}
$parent = Join-Path $localAppData 'Specialist-OS/workspaces'
New-Item -ItemType Directory -Path $parent -Force | Out-Null
$checkout = Join-Path $parent ([Guid]::NewGuid().ToString('N'))

# Use Git transport so the clone has its own object store and index.
& git clone --no-local -- $source $checkout
if ($LASTEXITCODE -ne 0) { throw "Could not clone $source into $checkout." }
Write-Output (Join-Path $checkout 'Specialist-OS.code-workspace')
