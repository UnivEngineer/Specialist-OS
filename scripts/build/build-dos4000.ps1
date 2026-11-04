[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
& python (Join-Path $PSScriptRoot 'pack-dos4000.py')
if ($LASTEXITCODE -ne 0) { throw 'DOS4000 assembly failed.' }
& python (Join-Path $PSScriptRoot 'build-dos-module.py') DOS4000
if ($LASTEXITCODE -ne 0) { throw 'DOS4000 module assembly failed.' }
