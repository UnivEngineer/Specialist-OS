[CmdletBinding()]
param([ValidateSet(2000000,2500000)][int] $PitHz = 2000000)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
New-Item -ItemType Directory -Force (Join-Path $PSScriptRoot 'build') | Out-Null
Push-Location $PSScriptRoot
try {
    & (Join-Path $root 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow "-DPIT_HZ=$PitHz" --lst=build/DISKBEN.lst --sym=build/DISKBEN.sym --raw=build/DISKBEN.i80 DISKBEN.asm
    if ($LASTEXITCODE -ne 0) { throw 'DISKBEN assembly failed.' }
    New-Item -ItemType Directory -Force (Join-Path $root 'bin') | Out-Null
    Copy-Item -LiteralPath build/DISKBEN.i80 -Destination (Join-Path $root 'bin/DISKBEN.i80') -Force
    [IO.File]::WriteAllText((Join-Path $root 'bin/DISKBEN.cpu'), "0000`r`n0000`r`n", [Text.Encoding]::ASCII)
} finally { Pop-Location }
