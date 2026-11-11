[CmdletBinding()]
param(
    [string] $EmulatorDirectory,
    [switch] $PrepareOnly,
    [switch] $ResetDisk,
    [switch] $Mcp,
    [switch] $Mute
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
. (Join-Path $PSScriptRoot 'runtime.ps1')
$runtime = Resolve-Emu80Runtime -Directory $EmulatorDirectory
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$exeName = if ($Mcp) { 'Emu80qt-mcp.exe' } else { 'Emu80qt.exe' }
$executable = Join-Path $runtime $exeName
if (-not (Test-Path -LiteralPath $executable)) { throw "Emu80 not found: $executable" }
$version = (Get-Item -LiteralPath $executable).VersionInfo
if ($version.FileMajorPart -lt 4 -or ($version.FileMajorPart -eq 4 -and $version.FileMinorPart -eq 0 -and $version.FileBuildPart -lt 571)) { throw 'Emu80 4.0.571 or newer is required.' }
$running = @(Get-Process -Name Emu80qt,Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object { $_.Path -and (Split-Path -Parent $_.Path) -eq $runtime })
if ($running.Count) { throw 'Close this Emu80 instance before preparing CP/M.' }
if ($Mcp -and (Get-NetTCPConnection -LocalPort 19781 -State Listen -ErrorAction SilentlyContinue)) { throw 'MCP port 19781 is already in use.' }
$manifest = Get-Content -LiteralPath (Join-Path $root 'images/CPM-images.json') -Raw | ConvertFrom-Json
foreach ($name in @('CPM-system.bin', 'CPM-seed.dsk')) {
    $path = Join-Path $root ('images/' + $name)
    $entry = $manifest.files.PSObject.Properties[$name].Value
    if ((Get-Item -LiteralPath $path).Length -ne $entry.bytes -or (Get-FileHash -LiteralPath $path -Algorithm SHA256).Hash.ToLowerInvariant() -ne $entry.sha256) { throw "Invalid image: $name" }
}
$deployment = Join-Path $runtime 'specialist-cpm'
$sd = Join-Path $deployment 'sdcard/CPM'
New-Item -ItemType Directory -Path $sd -Force | Out-Null
$disk = Join-Path $sd 'CPM.DSK'
if ($ResetDisk -and (Test-Path -LiteralPath $disk)) {
    $backup = $disk + '.' + (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0,8) + '.bak'
    Copy-Item -LiteralPath $disk -Destination $backup
    Write-Host "Previous disk backed up: $backup"
}
if ($ResetDisk -or -not (Test-Path -LiteralPath $disk)) { Copy-Item -LiteralPath (Join-Path $root 'images/CPM-seed.dsk') -Destination $disk -Force }
if ((Get-Item -LiteralPath $disk).Length -ne 8388608) { throw 'Working CPM.DSK must be 8 MiB.' }
Copy-Item -LiteralPath (Join-Path $root 'images/CPM-system.bin') -Destination (Join-Path $deployment 'system.bin') -Force
$config = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'specialist-mx2.conf')).Replace('specialist-os/', 'specialist-cpm/').Replace('(MXOS / Emu80)', '(CP/M Syabr / Emu80)')
# Вместо внешнего ROM-диска подключаем протокол SD Сябра на ВВ55-2.
$old = '  RkRomDisk romDisk = "specialist-cpm/rom-disk.bin"' + "`n" + '  ppi2.circuit = &romDisk'
$normalized = $config.Replace("`r`n", "`n")
if (-not $normalized.Contains($old)) { throw 'The base profile ROM disk block has changed.' }
$config = $normalized.Replace($old, "  RkSdController sdCard = `"specialist-cpm/sdcard`"`n  sdCard.model = spec`n  connect sdCard.data -> ppi2.portC`n  connect ppi2.portC -> sdCard.data`n  connect ppi2.portB -> sdCard.cmd")
# Таблица клавиш CP/M рассчитана на физическую ЙЦУКЕН Специалиста.
# QWERTY Emu80 меняет расположение виртуальных клавиш и даёт asd -> fcl.
$config += "`nkbdLayout.layout = jcuken`n"
[IO.File]::WriteAllText((Join-Path $deployment 'specialist-mx2.conf'), $config, (New-Object Text.UTF8Encoding($false)))
$defines = ''
$marker = Join-Path $runtime 'specialist-build.json'
if (Test-Path -LiteralPath $marker) {
    $info = Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
    if (@($info.sources | Where-Object { $_.path -eq 'SpecialistMx2VideoRam.cpp' }).Count) { $defines = "define MX2_MODELS`n" }
}
$launch = $defines + "include specialist-cpm/specialist-mx2.conf`n"
if ($Mcp -or $Mute) { $launch += "pitSoundSource.muted = yes`ntapeMonitor.muted = yes`n" }
[IO.File]::WriteAllText((Join-Path $deployment 'launch.conf'), $launch, (New-Object Text.UTF8Encoding($false)))
Write-Host "Prepared CP/M: $deployment"
if (-not $PrepareOnly) {
    Start-Process -FilePath $executable -ArgumentList '--conf-file', 'specialist-cpm/launch.conf', '--platform', 'specialist-cpm' -WorkingDirectory $runtime -WindowStyle Normal
}
