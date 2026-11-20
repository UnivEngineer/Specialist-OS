[CmdletBinding()]
param([switch] $Force)
$ErrorActionPreference = 'Stop'
# Прежний экспериментальный профиль стал штатной парной сборкой.
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Push-Location (Join-Path $root 'source/dos')
try {
    & (Join-Path $PSScriptRoot 'build-if-needed.ps1') DOS sys '0x0100' DOS -Force:$Force
    exit $LASTEXITCODE
} finally { Pop-Location }
