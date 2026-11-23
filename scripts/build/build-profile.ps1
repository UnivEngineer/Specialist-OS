[CmdletBinding()]
param(
    [Parameter(Mandatory, Position = 0)]
    [ValidateSet('MMX Flash + floppy', 'MX ROM + floppy', 'RAMFOS')]
    [string] $Profile,
    [switch] $Force,
    [switch] $ComponentsReady
)

Write-Host "[build] Profile '$Profile' started"
$timer = [Diagnostics.Stopwatch]::StartNew()
try {
    $ErrorActionPreference = 'Stop'
    $buildImage = Join-Path $PSScriptRoot '../images/build-image.ps1'

    switch ($Profile) {
        'MMX Flash + floppy' {
            if (-not $ComponentsReady) {
                & (Join-Path $PSScriptRoot 'build-project.ps1') OS -Force:$Force
                if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            }
            & $buildImage SystemHardware 512 -FontsReady -Force:$Force
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            foreach ($target in 'SystemEmuWritable', 'Games2M', 'Floppy720') {
                & $buildImage $target -FontsReady -Force:$Force
                if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            }
        }
        'MX ROM + floppy' {
            & (Join-Path $PSScriptRoot 'build-project.ps1') MX -ComponentsReady:$ComponentsReady -Force:$Force
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
            & $buildImage Games64k -Force:$Force
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
        'RAMFOS' {
            & $buildImage MxRamfos32 -Force:$Force
            if ($LASTEXITCODE -ne 0) { exit $LASTEXITCODE }
        }
    }
    exit 0
} finally {
    Write-Host ('[time] Profile {0}: {1:F3} s' -f $Profile, $timer.Elapsed.TotalSeconds)
}
