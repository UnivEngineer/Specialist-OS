[CmdletBinding()]
param(
    [ValidateSet(2000000,2500000)][int] $PitHz = 2000000,
    [switch] $Force,
    [Parameter(DontShow)]
    [switch] $RunBuild
)
$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if (-not $RunBuild) {
    $cacheArgs = @((Join-Path $root 'scripts/build/build_cache.py'), "DISKBEN ($PitHz Hz)", '--input-content',
        '--asm', (Join-Path $PSScriptRoot 'DISKBEN.asm'), '--asm', (Join-Path $PSScriptRoot 'worker.asm'),
        '--input', (Join-Path $root 'scripts/images/cpu.ps1'), '--scan-output', (Join-Path $PSScriptRoot 'build'))
    foreach ($name in 'DISKBEN', 'DISKBANK', 'DISKDOS', 'DISKMAIN') {
        foreach ($extension in 'i80', 'cpu') { $cacheArgs += @('--output', (Join-Path $root "bin/$name.$extension")) }
    }
    if ($Force) { $cacheArgs += '--force' }
    $cacheArgs += @('--', $PSCommandPath, '-PitHz', $PitHz, '-RunBuild')
    & python @cacheArgs
    exit $LASTEXITCODE
}
. (Join-Path $root "scripts/images/cpu.ps1")
New-Item -ItemType Directory -Force (Join-Path $PSScriptRoot 'build') | Out-Null
Push-Location $PSScriptRoot
try {
    & (Join-Path $root 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow "-DPIT_HZ=$PitHz" --lst=build/DISKBEN.lst --sym=build/DISKBEN.sym --raw=build/DISKBEN.i80 DISKBEN.asm
    if ($LASTEXITCODE -ne 0) { throw 'DISKBEN assembly failed.' }
    New-Item -ItemType Directory -Force (Join-Path $root 'bin') | Out-Null
    Copy-Item -LiteralPath build/DISKBEN.i80 -Destination (Join-Path $root 'bin/DISKBEN.i80') -Force
    $symbols = Get-Content -LiteralPath build/DISKBEN.sym -Raw
    if ($symbols -notmatch '(?m)^IMAGE_FLAGS: EQU 0x([0-9A-Fa-f]+)') { throw 'Missing DISKBEN IMAGE_FLAGS.' }
    $imageFlags = [Convert]::ToUInt16($Matches[1], 16)
    [IO.File]::WriteAllText((Join-Path $root 'bin/DISKBEN.cpu'), (New-CpuDescriptor -Load 0 -Flags $imageFlags -Fields (Get-CpuFieldsFromSymbols $symbols)), [Text.Encoding]::ASCII)
    foreach ($worker in @(
        @{ Name='DISKBANK'; Define='-DWORKER_BANK=1'; Flags='02' },
        @{ Name='DISKDOS'; Define='-DWORKER_DOS=1'; Flags='01' },
        @{ Name='DISKMAIN'; Define='-DWORKER_MAIN=1'; Flags='04' }
    )) {
        $name = $worker.Name
        & (Join-Path $root 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow $worker.Define "--lst=build/$name.lst" "--sym=build/$name.sym" "--raw=build/$name.i80" worker.asm
        if ($LASTEXITCODE -ne 0) { throw "$name assembly failed." }
        Copy-Item -LiteralPath "build/$name.i80" -Destination (Join-Path $root "bin/$name.i80") -Force
        $workerSymbols = Get-Content -LiteralPath "build/$name.sym" -Raw
        if ($workerSymbols -notmatch '(?m)^entry: EQU 0x([0-9A-Fa-f]+)') { throw "Missing $name entry." }
        $loadAddress = '{0:X4}' -f [Convert]::ToUInt16($Matches[1], 16)
        [IO.File]::WriteAllText((Join-Path $root "bin/$name.cpu"), (New-CpuDescriptor -Load ([Convert]::ToInt32($loadAddress,16)) -Flags ([Convert]::ToInt32($worker.Flags,16)) -Fields (Get-CpuFieldsFromSymbols $workerSymbols)), [Text.Encoding]::ASCII)
    }
} finally { Pop-Location }
