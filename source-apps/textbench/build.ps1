$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
Push-Location $PSScriptRoot
try {
    foreach ($name in @('TEXTBEN', 'GATE', 'TEXTBUF', 'BUFFERGATE', 'DISPATCH')) {
        & (Join-Path $root 'tasm/sjasmplus.exe') --i8080 -Wno-rdlow "--raw=$root/bin/$name.i80" "--lst=$root/bin/$name.lst" "--sym=$root/bin/$name.sym" "$name.asm"
        if ($LASTEXITCODE -ne 0) { throw "$name assembly failed" }
    }
} finally { Pop-Location }
