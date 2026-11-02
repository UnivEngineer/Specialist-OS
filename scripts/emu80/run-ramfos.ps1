[CmdletBinding()]
param(
    [string] $EmulatorDirectory,
    [switch] $PrepareOnly
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
. (Join-Path $PSScriptRoot 'runtime.ps1')
$runtime = Resolve-Emu80Runtime -Directory $EmulatorDirectory
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$source = Join-Path $root 'images/RAMFOS-system.bin'
$seed = Join-Path $root 'images/RAMFOS-games.ramdisk'
$manifest = Join-Path $root 'images/RAMFOS-games.json'
if (-not (Test-Path -LiteralPath $manifest -PathType Leaf)) { throw 'Build RAMFOS ROM first.' }
$catalog = Get-Content -LiteralPath $manifest -Raw | ConvertFrom-Json
$executable = Join-Path $runtime 'Emu80qt.exe'
if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Build RAMFOS ROM first: $source" }
if ((Get-Item -LiteralPath $source).Length -ne 65536) { throw 'Invalid RAMFOS ROM size.' }
if ((Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant() -ne $catalog.system_sha256) { throw 'RAMFOS ROM differs from its build manifest.' }
if (-not (Test-Path -LiteralPath $seed -PathType Leaf)) { throw "RAM disk seed not found: $seed" }
if ((Get-Item -LiteralPath $seed).Length -ne 1048576) { throw 'Invalid RAMFOS RAM disk size.' }
if ((Get-FileHash -LiteralPath $seed -Algorithm SHA256).Hash.ToLowerInvariant() -ne $catalog.ram_sha256) { throw 'RAM disk differs from its build manifest.' }
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Emu80 not found: $executable" }
$version = (Get-Item -LiteralPath $executable).VersionInfo
if ($version.FileMajorPart -lt 4 -or ($version.FileMajorPart -eq 4 -and $version.FileMinorPart -eq 0 -and $version.FileBuildPart -lt 571)) {
    throw 'Emu80 4.0.571 or newer is required.'
}
$running = @(Get-Process -Name Emu80qt,Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object { $_.Path -and (Split-Path -Parent $_.Path) -eq $runtime })
if ($running.Count) { throw 'Close this Emu80 instance before preparing RAMFOS.' }
# Отдельная платформа: настройки и RAM-диски MXOS не заменяются.
$deployment = Join-Path $runtime 'specialist-ramfos'
New-Item -ItemType Directory -Path $deployment -Force | Out-Null
Copy-Item -LiteralPath $source -Destination (Join-Path $deployment 'system.bin') -Force
Copy-Item -LiteralPath $seed -Destination (Join-Path $deployment 'games.ramdisk') -Force
[IO.File]::WriteAllBytes((Join-Path $deployment 'rom-disk.bin'), (New-Object byte[] 65536))
$config = [IO.File]::ReadAllText((Join-Path $PSScriptRoot 'specialist-mx2.conf'))
$config = $config.Replace('specialist-os/', 'specialist-ramfos/').Replace('(MXOS / Emu80)', '(RAMFOS / Emu80)')
# fileName загружает весь образ немедленно, после привязки всех 16 страниц.
# autoSave по умолчанию выключен: изменения сохраняют вручную через Save As.
$config += "`nramDisk.fileName = `"specialist-ramfos/games.ramdisk`"`n"
[IO.File]::WriteAllText((Join-Path $deployment 'specialist-mx2.conf'), $config, (New-Object Text.UTF8Encoding($false)))
$defines = ''
$marker = Join-Path $runtime 'specialist-build.json'
if (Test-Path -LiteralPath $marker) {
    $info = Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
    if (@($info.sources | Where-Object { $_.path -eq 'SpecialistMx2VideoRam.cpp' }).Count) { $defines = "define MX2_MODELS`n" }
}
[IO.File]::WriteAllText((Join-Path $deployment 'launch.conf'), ($defines + "include specialist-ramfos/specialist-mx2.conf`n"), (New-Object Text.UTF8Encoding($false)))
Write-Host "Prepared RAMFOS with preloaded games: $deployment"
if (-not $PrepareOnly) {
    Start-Process -FilePath $executable -ArgumentList '--conf-file', 'specialist-ramfos/launch.conf', '--platform', 'specialist-ramfos' -WorkingDirectory $runtime -WindowStyle Normal
}
