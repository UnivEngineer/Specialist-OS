[CmdletBinding()]
param()
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$imageDirectory = Join-Path $root 'images'
$parts = @(1..4 | ForEach-Object { Join-Path $imageDirectory "FlashDisk_chip$($_)_512K.bin" })
foreach ($path in $parts) {
    if (-not (Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).Length -ne 524288) {
        throw "Build Games2M first; missing or invalid chip image: $path"
    }
}
$image = New-Object byte[] 2097152
for ($chip = 0; $chip -lt 4; $chip++) {
    [Array]::Copy([IO.File]::ReadAllBytes($parts[$chip]), 0, $image, $chip * 524288, 524288)
}
$target = Join-Path $imageDirectory 'FlashDisk_emu80_seed.bin'
[IO.File]::WriteAllBytes($target, $image)
Write-Host "[seed] $target"
