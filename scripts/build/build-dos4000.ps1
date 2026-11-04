[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
& python (Join-Path $PSScriptRoot 'pack-dos4000.py')
if ($LASTEXITCODE -ne 0) { throw 'DOS4000 assembly failed.' }
