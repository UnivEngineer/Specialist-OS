[CmdletBinding()]
param(
    [string] $EmulatorDirectory,
    [ValidateSet('SystemRom_emulator.bin', 'SystemRom_ramfos_emulator.bin', 'SystemRom_emu80_flash.bin', 'SystemRom_emu80_writable.bin')]
    [string] $SystemImage = 'SystemRom_emulator.bin',
    [switch] $PrepareOnly,
    [switch] $Hidden,
    [switch] $Flash,
    [switch] $ResetFlash,
    [switch] $ProtectSystemFlash,
    [switch] $Mcp
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
if ($ResetFlash -and -not $Flash) { throw '-ResetFlash requires -Flash.' }
if ($ProtectSystemFlash -and -not $Flash) { throw '-ProtectSystemFlash requires -Flash.' }
. (Join-Path $PSScriptRoot 'runtime.ps1')
$runtime = Resolve-Emu80Runtime -Directory $EmulatorDirectory -Flash:$Flash
$exeName = if ($Mcp) { 'Emu80qt-mcp.exe' } else { 'Emu80qt.exe' }
$executable = Join-Path $runtime $exeName
if ($Flash -and -not $PSBoundParameters.ContainsKey('SystemImage')) { $SystemImage = 'SystemRom_emu80_writable.bin' }
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$systemSource = Join-Path $root ('images/' + $SystemImage)
$diskSource = Join-Path $root $(if ($Flash) { 'images/FlashDisk_emu80_seed.bin' } else { 'images/FlashDisk_emulator.bin' })
$configSource = Join-Path $PSScriptRoot 'specialist-mx2.conf'
$persistentImage = Join-Path $runtime 'specialist-os/flash-disk.bin'
$persistentSystem = Join-Path $runtime 'specialist-os/system-flash.bin'
$needsDiskSource = -not $Flash -or $ResetFlash -or -not (Test-Path -LiteralPath $persistentImage -PathType Leaf)
$needsSystemSource = -not $Flash -or $ResetFlash -or -not (Test-Path -LiteralPath $persistentSystem -PathType Leaf)
foreach ($path in @($executable, (Join-Path $runtime 'emu80.conf'), $configSource)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
}
if ($needsDiskSource -and -not (Test-Path -LiteralPath $diskSource -PathType Leaf)) { throw "Required disk seed not found: $diskSource" }
if ($needsSystemSource -and -not (Test-Path -LiteralPath $systemSource -PathType Leaf)) { throw "Required system seed not found: $systemSource" }
$version = (Get-Item -LiteralPath $executable).VersionInfo
if ($version.FileMajorPart -lt 4 -or ($version.FileMajorPart -eq 4 -and $version.FileMinorPart -eq 0 -and $version.FileBuildPart -lt 571)) {
    throw "Emu80 4.0.571 or newer is required; found $($version.FileVersion)."
}
$systemSize = if ($Flash) { 524288 } else { 65536 }
if ($needsSystemSource -and (Get-Item -LiteralPath $systemSource).Length -ne $systemSize) { throw "Unexpected system image size: $systemSource" }
$diskSize = if ($Flash) { 2097152 } else { 65536 }
if ($needsDiskSource -and (Get-Item -LiteralPath $diskSource).Length -ne $diskSize) { throw "Unexpected disk image size: $diskSource" }
if ($Flash) {
    $buildInfo = Get-Content -LiteralPath (Join-Path $runtime 'specialist-build.json') -Raw | ConvertFrom-Json
    if ($buildInfo.model -ne 'SpecialistFlashDisk') { throw 'This runtime does not provide the flash model.' }
    if (@($buildInfo.sources | Where-Object { $_.path -eq 'SpecialistSystemFlash.cpp' }).Count -ne 1) { throw 'Rebuild and deploy Emu80 with SpecialistSystemFlash before starting this profile.' }
}
# Модели проекта подключаем и для ROM-профиля собственной сборки.
$mx2Models = $false
$marker = Join-Path $runtime 'specialist-build.json'
if (Test-Path -LiteralPath $marker) {
    $customBuild = Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
    $mx2Models = @($customBuild.sources | Where-Object { $_.path -eq 'SpecialistMx2VideoRam.cpp' }).Count -ne 0
}
# Не заменяем образы и профиль работающего экземпляра этой установки.
$running = @(Get-Process -Name Emu80qt,Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object { $_.Path -and (Split-Path -Parent $_.Path) -eq $runtime })
if ($running.Count -ne 0) { throw 'Close this Emu80 instance before preparing or launching the profile.' }
$deployment = Join-Path $runtime 'specialist-os'
New-Item -ItemType Directory -Path $deployment -Force | Out-Null
if ($Flash) {
    $protectionFile = $persistentImage + '.sdp'
    $systemProtection = $persistentSystem + '.sdp'
    if ($ResetFlash) {
        # Перед явным сбросом сохраняем оба диска вместе с состоянием SDP.
        $oldFiles = @($persistentImage, $protectionFile, $persistentSystem, $systemProtection)
        if (@($oldFiles | Where-Object { Test-Path -LiteralPath $_ -PathType Leaf }).Count) {
            $backupName = (Get-Date -Format 'yyyyMMdd-HHmmss') + '-' + [guid]::NewGuid().ToString('N').Substring(0, 8)
            $backupDirectory = Join-Path $deployment ('flash-backups/' + $backupName)
            New-Item -ItemType Directory -Path $backupDirectory | Out-Null
            foreach ($oldFile in $oldFiles) {
                if (Test-Path -LiteralPath $oldFile -PathType Leaf) { Copy-Item -LiteralPath $oldFile -Destination $backupDirectory }
            }
            Write-Host "Previous flash backed up: $backupDirectory"
        }
        Copy-Item -LiteralPath $diskSource -Destination $persistentImage -Force
        if (Test-Path -LiteralPath $protectionFile -PathType Leaf) { Remove-Item -LiteralPath $protectionFile }
        Copy-Item -LiteralPath $systemSource -Destination $persistentSystem -Force
        if (Test-Path -LiteralPath $systemProtection -PathType Leaf) { Remove-Item -LiteralPath $systemProtection }
    } elseif (-not (Test-Path -LiteralPath $persistentImage)) {
        Copy-Item -LiteralPath $diskSource -Destination $persistentImage
    }
    if (-not (Test-Path -LiteralPath $persistentSystem)) { Copy-Item -LiteralPath $systemSource -Destination $persistentSystem }
    if ((Get-Item -LiteralPath $persistentImage).Length -ne $diskSize) { throw 'Persistent flash image has an invalid size.' }
    if ((Get-Item -LiteralPath $persistentSystem).Length -ne $systemSize) { throw 'Persistent system flash image has an invalid size.' }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'specialist-mx2-flash.conf') -Destination (Join-Path $deployment 'specialist-mx2-flash.conf') -Force
} else {
    Copy-Item -LiteralPath $systemSource -Destination (Join-Path $deployment 'system.bin') -Force
    Copy-Item -LiteralPath $diskSource -Destination (Join-Path $deployment 'rom-disk.bin') -Force
}
Copy-Item -LiteralPath $configSource -Destination (Join-Path $deployment 'specialist-mx2.conf') -Force
$launchConfig = $(if ($mx2Models) { "define MX2_MODELS`n" }) + $(if ($Flash) { "define FLASH_DISK`ndefine SYSTEM_FLASH`n" }) + $(if ($ProtectSystemFlash) { "define SYSTEM_FLASH_LOCKED`n" }) + "include specialist-os/specialist-mx2.conf`n"
[IO.File]::WriteAllText((Join-Path $deployment 'launch.conf'), $launchConfig, (New-Object Text.UTF8Encoding($false)))
$diskName = if ($Flash) { 'flash-disk.bin' } else { 'rom-disk.bin' }
$systemName = if ($Flash) { 'system-flash.bin' } else { 'system.bin' }
$record = [ordered]@{ systemImage = $SystemImage; mutableDisk = [bool]$Flash; files = @(
    foreach ($name in @($systemName, $diskName, 'specialist-mx2.conf')) {
        [ordered]@{ path = $name; sha256 = (Get-FileHash -LiteralPath (Join-Path $deployment $name) -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
) }
[IO.File]::WriteAllText((Join-Path $deployment 'deployment.json'), ($record | ConvertTo-Json -Depth 4) + "`n", (New-Object Text.UTF8Encoding($false)))
Write-Host "Prepared MXOS profile: $deployment"
if (-not $PrepareOnly) {
    # Эмулятор — интерактивное приложение; скрытый запуск нужен только для диагностики.
    $windowStyle = if ($Hidden) { 'Hidden' } else { 'Normal' }
    $configName = 'specialist-os/launch.conf'
    Start-Process -FilePath $executable -ArgumentList '--conf-file', $configName, '--platform', 'specialist-os' -WorkingDirectory $runtime -WindowStyle $windowStyle
}
