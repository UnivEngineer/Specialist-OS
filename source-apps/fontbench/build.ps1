$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$asm = Join-Path $root 'tasm/sjasmplus.exe'
Push-Location $PSScriptRoot
try {
    & $asm --i8080 -Wno-rdlow "--raw=$root/bin/FONT8.i80" "--sym=$root/bin/FONT8.sym" "--lst=$root/bin/FONT8.lst" FONT8.asm
    if ($LASTEXITCODE -ne 0) { throw 'FONT8 assembly failed' }
    $symbols = Get-Content -LiteralPath (Join-Path $root 'bin/FONT8.sym') -Raw
    $entries = foreach ($pair in @(@('buffer8','FONT8_BUFFER'),@('scroll8','FONT8_SCROLL'))) {
        $match = [regex]::Match($symbols, ('(?m)^'+$pair[0]+': EQU 0x([0-9A-Fa-f]+)'))
        if (-not $match.Success) { throw "Missing symbol $($pair[0])" }
        "$($pair[1]) = 0$($match.Groups[1].Value)h"
    }
    [IO.File]::WriteAllLines((Join-Path $root 'bin/FONT8.entries.inc'), $entries, [Text.UTF8Encoding]::new($false))
    & $asm --i8080 -Wno-rdlow "--raw=$root/bin/F8DISP.i80" "--sym=$root/bin/F8DISP.sym" "--lst=$root/bin/F8DISP.lst" F8DISP.asm
    if ($LASTEXITCODE -ne 0) { throw 'F8DISP assembly failed' }
    foreach ($batch in @(0,1)) {
        $name = if ($batch) { 'F8BUF' } else { 'F8CHAR' }
        & $asm --i8080 -Wno-rdlow "-DBATCH=$batch" "--raw=$root/bin/$name.i80" "--sym=$root/bin/$name.sym" "--lst=$root/bin/$name.lst" FONTBEN.asm
        if ($LASTEXITCODE -ne 0) { throw "$name assembly failed" }
    }
    & $asm --i8080 -Wno-rdlow "--raw=$root/bin/SHIFTDISP.i80" "--sym=$root/bin/SHIFTDISP.sym" "--lst=$root/bin/SHIFTDISP.lst" SHIFTDISP.asm
    if ($LASTEXITCODE -ne 0) { throw 'SHIFTDISP assembly failed' }
    foreach ($batch in @(0,1)) {
        & $asm --i8080 -Wno-rdlow "-DBATCH=$batch" "--raw=$root/bin/SHIFTBEN$batch.i80" "--sym=$root/bin/SHIFTBEN$batch.sym" "--lst=$root/bin/SHIFTBEN$batch.lst" SHIFTBEN.asm
        if ($LASTEXITCODE -ne 0) { throw 'SHIFTBEN assembly failed' }
        foreach ($mode in @(0,1,2,3)) {
            $name = "SHIFT_M${mode}_B${batch}"
            & $asm --i8080 -Wno-rdlow "-DBATCH=$batch" "-DMODE=$mode" "--raw=$root/bin/$name.i80" "--sym=$root/bin/$name.sym" "--lst=$root/bin/$name.lst" SHIFT.asm
            if ($LASTEXITCODE -ne 0) { throw "$name assembly failed" }
        }
    }
    & $asm --i8080 -Wno-rdlow "--raw=$root/bin/OPT_DRAW.i80" "--sym=$root/bin/OPT_DRAW.sym" "--lst=$root/bin/OPT_DRAW.lst" OPT_DRAW.asm
    if ($LASTEXITCODE -ne 0) { throw 'OPT_DRAW assembly failed' }
} finally { Pop-Location }
