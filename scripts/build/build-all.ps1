[CmdletBinding()]
param([switch] $Force)

$ErrorActionPreference = 'Stop'
& (Join-Path $PSScriptRoot 'build-almaz.ps1') -Force:$Force
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path $PSScriptRoot 'build-almaz2.ps1') All -PublishMxTimer -Force:$Force
if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
& (Join-Path $PSScriptRoot 'build-project.ps1') All -Force:$Force
exit $LASTEXITCODE
