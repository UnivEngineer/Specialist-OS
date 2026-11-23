[CmdletBinding()]
param(
    [string] $EmulatorDirectory,
    [ValidateSet('C-ROM-64k-MMX-System-Emu80.bin', 'C-Flash-512k-MMX-RAMFOS-Tests.bin', 'C-Flash-64k-MMX-System-Emu80.bin', 'C-Flash-512k-MMX-System-Emu80.bin', 'C-Flash-128k-MMX-System.bin', 'C-Flash-256k-MMX-System.bin', 'C-Flash-512k-MMX-System.bin')]
    [string] $SystemImage = 'C-ROM-64k-MMX-System-Emu80.bin',
    [switch] $PrepareOnly,
    [switch] $Hidden,
    [switch] $Flash,
    [switch] $ResetFlash,
    [switch] $ResetFloppy,
    [switch] $FreshFloppy,
    [switch] $TestFlashSeed,
    [switch] $UpdateSystemFlash,
    [switch] $SyncProfileImages,
    [switch] $ProtectSystemFlash,
    [switch] $Mcp,
    [switch] $Mute,
    [switch] $PassThru
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
if ($TestFlashSeed -and -not ($Flash -and $Mcp)) { throw '-TestFlashSeed requires a flash MCP test slot.' }
if ($FreshFloppy -and $ResetFloppy) { throw 'Choose either -FreshFloppy or -ResetFloppy.' }
if ($ResetFlash -and -not $Flash) { throw '-ResetFlash requires -Flash.' }
if ($UpdateSystemFlash -and -not $Flash) { throw '-UpdateSystemFlash requires -Flash.' }
if ($SyncProfileImages -and -not $Flash) { throw '-SyncProfileImages requires -Flash.' }
if ($ProtectSystemFlash -and -not $Flash) { throw '-ProtectSystemFlash requires -Flash.' }
. (Join-Path $PSScriptRoot 'runtime.ps1')
$runtime = Resolve-Emu80Runtime -Directory $EmulatorDirectory -Flash:$Flash
$exeName = if ($Mcp) { 'Emu80qt-mcp.exe' } else { 'Emu80qt.exe' }
$executable = Join-Path $runtime $exeName
if ($Flash -and -not $PSBoundParameters.ContainsKey('SystemImage')) { $SystemImage = 'C-Flash-512k-MMX-System-Emu80.bin' }
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$systemSource = Join-Path $root ('images/' + $SystemImage)
$diskSource = Join-Path $root $(if ($Flash) { 'images/H-Flash-2M-Games.bin' } else { 'images/H-ROM-64k-Games.bin' })
$configSource = Join-Path $PSScriptRoot 'specialist-mx2.conf'
$persistentImage = Join-Path $runtime 'specialist-os/flash-disk.bin'
$persistentSystem = Join-Path $runtime 'specialist-os/system-flash.bin'
$deployment = Join-Path $runtime 'specialist-os'
$deploymentFile = Join-Path $deployment 'deployment.json'
$floppySource = Join-Path $root 'images/A-Floppy-720k-Programs.img'
$floppyName = if ($FreshFloppy) { 'floppy720-test.img' } else { 'floppy720.img' }
$mountedFloppy = Join-Path $deployment $floppyName
$hasFloppySeed = Test-Path -LiteralPath $floppySource -PathType Leaf
$hasFloppy = Test-Path -LiteralPath $mountedFloppy -PathType Leaf
$seedHashes = @{ systemSeedSha256 = $null; diskSeedSha256 = $null; floppySeedSha256 = $null }
if ($Flash -and (Test-Path -LiteralPath $deploymentFile -PathType Leaf)) {
    $previousDeployment = Get-Content -LiteralPath $deploymentFile -Raw | ConvertFrom-Json
    foreach ($key in @($seedHashes.Keys)) {
        if ($previousDeployment.PSObject.Properties[$key]) {
            $seedHashes[$key] = $previousDeployment.$key
        }
    }
}

function Get-SeedHash([string] $Path) {
    if (-not (Test-Path -LiteralPath $Path -PathType Leaf)) { throw "Required image not found: $Path" }
    (Get-FileHash -LiteralPath $Path -Algorithm SHA256).Hash.ToLowerInvariant()
}

$floppySourceHash = if ($SyncProfileImages -or $ResetFloppy -or ($hasFloppySeed -and -not $hasFloppy)) { Get-SeedHash $floppySource }
$seedFloppy = $FreshFloppy -or $ResetFloppy -or ($hasFloppySeed -and -not $hasFloppy) -or
    ($SyncProfileImages -and $floppySourceHash -ne $seedHashes.floppySeedSha256)
if ($seedFloppy -and -not $hasFloppySeed) { throw "Required floppy seed not found: $floppySource" }
if ($seedFloppy -and (Get-Item -LiteralPath $floppySource).Length -ne 720KB) { throw "Unexpected floppy seed size: $floppySource" }
if ($hasFloppy -and -not $seedFloppy -and (Get-Item -LiteralPath $mountedFloppy).Length -ne 720KB) {
    throw "Unexpected working floppy size: $mountedFloppy"
}
$diskMissing = -not (Test-Path -LiteralPath $persistentImage -PathType Leaf)
$needsDiskSource = $TestFlashSeed -or -not $Flash -or $ResetFlash -or $SyncProfileImages -or $diskMissing
$needsSystemSource = -not $Flash -or $ResetFlash -or $UpdateSystemFlash -or $SyncProfileImages -or -not (Test-Path -LiteralPath $persistentSystem -PathType Leaf)
foreach ($path in @($executable, (Join-Path $runtime 'emu80.conf'), $configSource)) {
    if (-not (Test-Path -LiteralPath $path -PathType Leaf)) { throw "Required file not found: $path" }
}
$diskSourceHash = if ($needsDiskSource) { Get-SeedHash $diskSource }
$systemSourceHash = if ($needsSystemSource) { Get-SeedHash $systemSource }
$refreshDisk = $ResetFlash -or $diskMissing -or ($SyncProfileImages -and $diskSourceHash -ne $seedHashes.diskSeedSha256)
$refreshSystem = $ResetFlash -or -not (Test-Path -LiteralPath $persistentSystem -PathType Leaf) -or
    (($UpdateSystemFlash -or $SyncProfileImages) -and $systemSourceHash -ne $seedHashes.systemSeedSha256)
$version = (Get-Item -LiteralPath $executable).VersionInfo
if ($version.FileMajorPart -lt 4 -or ($version.FileMajorPart -eq 4 -and $version.FileMinorPart -eq 0 -and $version.FileBuildPart -lt 571)) {
    throw "Emu80 4.0.571 or newer is required; found $($version.FileVersion)."
}
$systemSize = if ($Flash) {
    $sizedImage = if ($refreshSystem) { $systemSource } else { $persistentSystem }
    $capacity = (Get-Item -LiteralPath $sizedImage).Length
    if ($capacity -notin @(131072, 262144, 524288)) { throw "System ROM/flash must be 128, 256 or 512 KiB: $sizedImage" }
    $capacity
} else { 65536 }
if ((-not $Flash -or $refreshSystem) -and (Get-Item -LiteralPath $systemSource).Length -ne $systemSize) { throw "Unexpected system image size: $systemSource" }
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
if ($hasFloppy) {
    try {
        $probe = [IO.File]::Open($mountedFloppy, 'Open', 'Read', 'None')
        $probe.Dispose()
    } catch [IO.IOException] {
        throw "Floppy image is in use: $mountedFloppy. Unmount it in ImDisk before launching Emu80."
    }
}
New-Item -ItemType Directory -Path $deployment -Force | Out-Null
if ($seedFloppy) {
    Copy-Item -LiteralPath $floppySource -Destination $mountedFloppy -Force
    $hasFloppy = $true
    if ($Flash -and -not $FreshFloppy) { $seedHashes.floppySeedSha256 = $floppySourceHash }
    Write-Host "[deploy] A: $floppySource -> $mountedFloppy"
}
if ($Flash) {
    $protectionFile = $persistentImage + '.sdp'
    $systemProtection = $persistentSystem + '.sdp'
    if ($refreshDisk) {
        Copy-Item -LiteralPath $diskSource -Destination $persistentImage -Force
        $seedHashes.diskSeedSha256 = $diskSourceHash
        if (Test-Path -LiteralPath $protectionFile -PathType Leaf) { Remove-Item -LiteralPath $protectionFile }
        Write-Host "[deploy] H: $diskSource -> $persistentImage"
    }
    if ($refreshSystem) {
        Copy-Item -LiteralPath $systemSource -Destination $persistentSystem -Force
        $seedHashes.systemSeedSha256 = $systemSourceHash
        Write-Host "[deploy] C: $systemSource -> $persistentSystem"
    } elseif ($UpdateSystemFlash) {
        Write-Host "[keep] C: $persistentSystem"
    }
    if ((Get-Item -LiteralPath $persistentImage).Length -ne $diskSize) { throw 'Persistent flash image has an invalid size.' }
    if ((Get-Item -LiteralPath $persistentSystem).Length -ne $systemSize) { throw 'Persistent system flash image has an invalid size.' }
    # Системный образ подготовлен к эксплуатации. Штатный драйвер пишет с SDP.
    if ($refreshSystem -or -not (Test-Path -LiteralPath $systemProtection -PathType Leaf)) {
        [IO.File]::WriteAllBytes($systemProtection, [byte[]]@(1))
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'specialist-mx2-flash.conf') -Destination (Join-Path $deployment 'specialist-mx2-flash.conf') -Force
} else {
    Copy-Item -LiteralPath $systemSource -Destination (Join-Path $deployment 'system.bin') -Force
    Copy-Item -LiteralPath $diskSource -Destination (Join-Path $deployment 'rom-disk.bin') -Force
}
Copy-Item -LiteralPath $configSource -Destination (Join-Path $deployment 'specialist-mx2.conf') -Force
$launchConfig = $(if ($mx2Models) { "define MX2_MODELS`n" }) + $(if ($Flash) { "define FLASH_DISK`ndefine SYSTEM_FLASH`n" }) + $(if ($ProtectSystemFlash) { "define SYSTEM_FLASH_LOCKED`n" })
if ($hasFloppy) { $launchConfig += "define FLOPPY_720`n" }
if ($FreshFloppy) { $launchConfig += "define FRESH_FLOPPY`n" }
$launchConfig += "include specialist-os/specialist-mx2.conf`n"
if ($TestFlashSeed) {
    # Тест читает инструменты со свежего H:, сохраняя рабочий flash-диск слота.
    Copy-Item -LiteralPath $diskSource -Destination (Join-Path $deployment 'test-flash.bin') -Force
    $launchConfig += "flashDisk.image = `"specialist-os/test-flash.bin`"`n"
}
$launchConfig += "kbdLayout.layout = smart`n"
if ($Mcp -or $Mute) {
    $launchConfig += "pitSoundSource.muted = yes`ntapeMonitor.muted = yes`n"
}
[IO.File]::WriteAllText((Join-Path $deployment 'launch.conf'), $launchConfig, (New-Object Text.UTF8Encoding($false)))
$diskName = if ($Flash) { 'flash-disk.bin' } else { 'rom-disk.bin' }
$systemName = if ($Flash) { 'system-flash.bin' } else { 'system.bin' }
$deployedFiles = @($systemName, $diskName, 'specialist-mx2.conf')
if ($hasFloppy) { $deployedFiles += $floppyName }
$record = [ordered]@{
    systemImage = $SystemImage
    mutableDisk = [bool]$Flash
    systemSeedSha256 = $seedHashes.systemSeedSha256
    diskSeedSha256 = $seedHashes.diskSeedSha256
    floppySeedSha256 = $seedHashes.floppySeedSha256
    files = @(
    foreach ($name in $deployedFiles) {
        [ordered]@{ path = $name; sha256 = (Get-FileHash -LiteralPath (Join-Path $deployment $name) -Algorithm SHA256).Hash.ToLowerInvariant() }
    }
) }
[IO.File]::WriteAllText($deploymentFile, ($record | ConvertTo-Json -Depth 4) + "`n", (New-Object Text.UTF8Encoding($false)))
Write-Host "Prepared MXOS profile: $deployment"
if (-not $PrepareOnly) {
    # Эмулятор — интерактивное приложение; скрытый запуск нужен только для диагностики.
    $windowStyle = if ($Hidden) { 'Hidden' } else { 'Normal' }
    $configName = 'specialist-os/launch.conf'
    Start-Process -FilePath $executable -ArgumentList '--conf-file', $configName, '--platform', 'specialist-os' -WorkingDirectory $runtime -WindowStyle $windowStyle -PassThru:$PassThru
}
