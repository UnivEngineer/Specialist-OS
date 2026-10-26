[CmdletBinding()]
param()

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0
$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$assembler = Join-Path $repositoryRoot "tasm\sjasmplus.exe"
$sourcePath = Join-Path $repositoryRoot "source-apps\almaz\Almaz.asm"
$outputDirectory = Join-Path $repositoryRoot "bin"
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
$prefix = Join-Path $outputDirectory "Almaz-original"
& $assembler "--i8080" "-Wno-rdlow" `
    ("--raw={0}.bin" -f $prefix) ("--lst={0}.lst" -f $prefix) `
    ("--sym={0}.sym" -f $prefix) $sourcePath
if ($LASTEXITCODE -ne 0) { throw "Almaz assembly failed" }

# Заголовок RKS: начало и включительный конец, оба слова little-endian.
# Контрольная сумма: перенос суммы младших байтов в старший, кроме последнего.
$payload = [System.IO.File]::ReadAllBytes("$prefix.bin")
if ($payload.Length -ne 0x1203) { throw "Unexpected Almaz payload size" }
$low = 0
$high = 0
for ($i = 0; $i -lt $payload.Length; $i++) {
    $sum = $low + [int]$payload[$i]
    $low = $sum -band 0xFF
    if ($i -lt $payload.Length - 1) {
        $high = ($high + [int]$payload[$i] + ($sum -shr 8)) -band 0xFF
    }
}
[byte[]]$rks = @([byte]0xFD, [byte]0x1E, [byte]0xFF, [byte]0x30) + $payload + @([byte]$low, [byte]$high)
[System.IO.File]::WriteAllBytes("$prefix.rks", $rks)
Write-Host "[app] $prefix.rks ($($rks.Length) bytes)"
