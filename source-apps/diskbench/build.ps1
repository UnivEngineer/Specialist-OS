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
    $symbols = Get-Content -LiteralPath build/DISKBEN.sym -Raw
    if ($symbols -notmatch '(?m)^IMAGE_FLAGS: EQU 0x([0-9A-Fa-f]+)') { throw 'Missing DISKBEN IMAGE_FLAGS.' }
    $imageFlags = [Convert]::ToUInt16($Matches[1], 16)
    [IO.File]::WriteAllText((Join-Path $root 'bin/DISKBEN.cpu'), ("0000`r`n0000`r`n{0:X2}`r`n" -f $imageFlags), [Text.Encoding]::ASCII)
    foreach ($worker in @(
        @{ Name='DISKBANK'; Define='-DWORKER_BANK=1'; Load='C000'; Flags='02' },
        @{ Name='DISKDOS'; Define='-DWORKER_DOS=1'; Load='C000'; Flags='01' },
        @{ Name='DISKMAIN'; Define='-DWORKER_MAIN=1'; Load='2000'; Flags='04' }
    )) {
        $name = $worker.Name
        & (Join-Path $root 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow $worker.Define "--lst=build/$name.lst" "--sym=build/$name.sym" "--raw=build/$name.i80" worker.asm
        if ($LASTEXITCODE -ne 0) { throw "$name assembly failed." }
        Copy-Item -LiteralPath "build/$name.i80" -Destination (Join-Path $root "bin/$name.i80") -Force
        [IO.File]::WriteAllText((Join-Path $root "bin/$name.cpu"), ($worker.Load+"`r`n"+$worker.Load+"`r`n"+$worker.Flags+"`r`n"), [Text.Encoding]::ASCII)
    }
} finally { Pop-Location }
