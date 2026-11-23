[CmdletBinding()]
param(
    [switch] $Force,
    [Parameter(DontShow)]
    [switch] $RunBuild
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$imageDirectory = Join-Path $root 'images'
$parts = @(1..4 | ForEach-Object { Join-Path $imageDirectory "H-Flash-2M-Games-chip$($_).bin" })
$target = Join-Path $imageDirectory 'H-Flash-2M-Games.bin'
if (-not $RunBuild) {
    $cacheArgs = @((Join-Path $root 'scripts/build/build_cache.py'), 'Emu80 flash seed', '--output', $target)
    foreach ($path in $parts) { $cacheArgs += @('--input', $path) }
    if ($Force) { $cacheArgs += '--force' }
    $cacheArgs += @('--', $PSCommandPath, '-RunBuild')
    & python @cacheArgs
    exit $LASTEXITCODE
}
foreach ($path in $parts) {
    if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -ne 524288) {
        throw "Build Games2M first; missing or invalid chip image: $path"
    }
}
$image = New-Object byte[] 2097152
for ($chip = 0; $chip -lt 4; $chip++) {
    [Array]::Copy([IO.File]::ReadAllBytes($parts[$chip]), 0, $image, $chip * 524288, 524288)
}
[IO.File]::WriteAllBytes($target, $image)
Write-Host "[seed] $target"
