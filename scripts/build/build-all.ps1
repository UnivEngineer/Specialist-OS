[CmdletBinding()]
param([switch] $Force)

Write-Host '[build] Build all started'
$timer = [Diagnostics.Stopwatch]::StartNew()
try {
    $ErrorActionPreference = 'Stop'
    & (Join-Path $PSScriptRoot 'build-project.ps1') All -IncludeGames -Force:$Force
    if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    foreach ($profile in 'MMX Flash + floppy', 'MX ROM + floppy', 'RAMFOS') {
        & (Join-Path $PSScriptRoot 'build-profile.ps1') $profile -ComponentsReady -Force:$Force
        if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
    }
    Write-Host 'Done.'
    exit 0
} finally {
    Write-Host ('[time] Build all: {0:F3} s' -f $timer.Elapsed.TotalSeconds)
}
