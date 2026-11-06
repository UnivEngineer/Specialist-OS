[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
# Прежний экспериментальный профиль стал штатной парной сборкой.
& python (Join-Path $PSScriptRoot 'build-bios-dos.py')
if ($LASTEXITCODE -ne 0) { throw 'BIOS/DOS assembly failed.' }
