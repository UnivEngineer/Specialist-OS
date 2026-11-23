[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet("SystemEmu", "SystemEmuFlash", "SystemEmuWritable", "SystemRamfos", "SystemHardware", "Games64k", "Games2M", "Floppy720", "MxFloppy", "MxRom32", "MxRom48", "MxRamfos32")]
    [string] $Target,

    [Parameter(Position = 1)]
    [ValidateSet(128, 256, 512)]
    [int] $HardwareSizeKB = 128,
    [string] $ManifestFile,
    [string] $BootFont,
    [switch] $Prebuilt,
    [switch] $MxUseRomBundle,
    [switch] $BootstrapProbe,
    [switch] $Force,
    [Parameter(DontShow)]
    [switch] $FontsReady,
    [Parameter(DontShow)]
    [switch] $RunBuild
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0
if ($BootFont -and $Target -notin @('SystemEmuWritable','SystemRamfos','SystemHardware')) { throw '-BootFont requires SystemEmuWritable, SystemRamfos or SystemHardware.' }
if ($ManifestFile -and $Target -ne 'Games2M') { throw '-ManifestFile is currently supported only for Games2M.' }

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
if ($Target -eq 'MxRamfos32') {
    $ramfosArgs = @('--mx-rom32')
    if ($Prebuilt) { $ramfosArgs += '--prebuilt' }
    if ($Force) { $ramfosArgs += '--force' }
    & python -X utf8 (Join-Path $PSScriptRoot 'build-ramfos.py') @ramfosArgs
    exit $LASTEXITCODE
}
$assetDirectory = Join-Path $repositoryRoot "assets"
$imageDirectory = Join-Path $repositoryRoot "images"
$sectorSize = 256
$sectorsPerCluster = 1
$mxFloppy = $Target -eq 'MxFloppy'
if ($MxUseRomBundle -and -not $mxFloppy) { throw '-MxUseRomBundle requires MxFloppy.' }
$nestedTarget = if ($MxUseRomBundle) { 'MxFloppyRomBundle' } else { $Target }
$mxBootPayload = if ($MxUseRomBundle) { 'bin/mx/reserved-rom.bin' } else { 'bin/mx/reserved.bin' }
$mxFlashStem = if ($MxUseRomBundle) { 'A-Flash-1M-MX-ROM-Bundle' } else { 'A-Flash-1M-MX-System' }
$floppy = $Target -eq 'Floppy720' -or $mxFloppy
$mxRom = $Target -in @('MxRom32','MxRom48')
$fatCopies = 1
$mediaByte = 0xF8

function Get-ImageHash([byte[]] $Bytes, [int] $Offset = 0) {
    $hasher = [Security.Cryptography.SHA256]::Create()
    try { return [BitConverter]::ToString($hasher.ComputeHash($Bytes, $Offset, $Bytes.Length - $Offset)).Replace('-', '') }
    finally { $hasher.Dispose() }
}

function Join-Bytes {
    param([Parameter(ValueFromRemainingArguments = $true)][byte[][]] $Arrays)

    $length = 0
    foreach ($array in $Arrays) {
        $length += $array.Length
    }

    $result = New-Object byte[] $length
    $position = 0
    foreach ($array in $Arrays) {
        [Array]::Copy($array, 0, $result, $position, $array.Length)
        $position += $array.Length
    }
    return ,$result
}

function Expand-Bytes {
    param(
        [byte[]] $Data,
        [int] $Length,
        [byte] $Fill = 0xFF
    )

    if ($Data.Length -gt $Length) {
        throw "Image data does not fit: $($Data.Length) bytes for $Length-byte output"
    }

    $result = New-Object byte[] $Length
    if ($Fill -ne 0 -and $Length -gt 0) {
        # Удвоение заполненного участка вместо побайтового цикла PowerShell.
        $result[0] = $Fill
        $filled = 1
        while ($filled -lt $Length) {
            $count = [Math]::Min($filled, $Length - $filled)
            [Array]::Copy($result, 0, $result, $filled, $count)
            $filled += $count
        }
    }
    [Array]::Copy($Data, $result, $Data.Length)
    return ,$result
}

function Get-LoadAddress {
    param([string] $Text)

    if ([string]::IsNullOrEmpty($Text)) {
        return 0
    }
    if ($Text.StartsWith("0x", [StringComparison]::OrdinalIgnoreCase)) {
        return [Convert]::ToInt32($Text.Substring(2), 16)
    }
    return [int]$Text
}

function Get-Manifest {
    param([string] $Path, [string] $AutoexecVariant = "emu")

    $lineNumber = 0
    $volumeLabel = $null
    $bootFontName = $null
    $entries = foreach ($line in [IO.File]::ReadAllLines($Path)) {
        $lineNumber++
        $entry = $line.Trim()
        if ($entry.Length -eq 0 -or $entry.StartsWith("#")) {
            continue
        }
        if ($entry.StartsWith("@")) {
            if ($entry -match '^@bootfont\s+(\S+)$') {
                if ($null -ne $bootFontName) { throw "Duplicate @bootfont at ${Path}:${lineNumber}" }
                $bootFontName = $Matches[1].Replace('\', '/')
                continue
            }
            if ($entry -notmatch '^@label\s+(.+)$') {
                throw "Invalid manifest directive at ${Path}:${lineNumber}: expected @label Volume label"
            }
            if ($null -ne $volumeLabel) {
                throw "Duplicate @label at ${Path}:${lineNumber}"
            }
            $label = $Matches[1].Trim()
            if ($label.Length -eq 0 -or $label.Length -gt 11 -or
                $label -match '[^\x20-\x7E]' -or $label -match '["*+,./:;<=>?\[\]\\|]') {
                throw "Invalid volume label at ${Path}:${lineNumber}: use 1-11 ASCII characters without FAT-reserved characters"
            }
            $volumeLabel = $label.PadRight(11)
            continue
        }
        $fields = @($entry -split '\s+')
        if ($fields.Count -lt 2 -or $fields.Count -gt 4) {
            throw "Invalid manifest entry at ${Path}:${lineNumber}: expected ImageName Source [LoadAddress [Attributes]]"
        }
        $source = $fields[1]
        if ($source -eq "{AUTOEXEC}") {
            $source = "system-rom/autoexec.$AutoexecVariant.bat"
        }
        $format = if ([IO.Path]::GetExtension($source).Equals(
            ".rks", [StringComparison]::OrdinalIgnoreCase)) { "rks" } else { "raw" }
        $address = if ($fields.Count -ge 3) { $fields[2] } else { "0" }
        if ($format -eq "rks" -and $fields.Count -ge 3) {
            throw "RKS load address comes from its header at ${Path}:${lineNumber}"
        }
        $attributes = 0
        if ($fields.Count -eq 4) {
            switch ($fields[3]) {
                "R"  { $attributes = 0x01 }
                "S"  { $attributes = 0x04 }
                "RS" { $attributes = 0x05 }
                default { throw "Invalid attributes at ${Path}:${lineNumber}: use R, S or RS" }
            }
        }
        [pscustomobject]@{ ImageName = $fields[0]; Source = $source; LoadAddress = $address; Format = $format; Attributes = $attributes; Location = "${Path}:${lineNumber}" }
    }
    if ($null -eq $volumeLabel) {
        throw "Missing @label in manifest: $Path"
    }
    return [pscustomobject]@{ Entries = @($entries); VolumeLabel = $volumeLabel; BootFont = $bootFontName }
}

$ramfosManifestPath = $null
switch ($Target) {
    { $_ -in @('SystemEmu', 'SystemEmuFlash') } {
        $manifestPath = Join-Path $assetDirectory "C-Flash-512k-MMX-System.txt"
        $autoexecVariant = if ($Target -eq 'SystemEmuFlash') { 'emu-flash' } else { 'emu' }
        $manifestData = Get-Manifest $manifestPath $autoexecVariant
        $manifest = @($manifestData.Entries)
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-system.bin"
        $volumeSize = 64KB
        $chipSize = 64KB
        $maxFiles = 64
        $imageFormat = "Mx2"
        $outputName = if ($Target -eq 'SystemEmuFlash') { 'C-Flash-64k-MMX-System-Emu80.bin' } else { 'C-ROM-64k-MMX-System-Emu80.bin' }
        $outputPath = Join-Path $imageDirectory $outputName
        $makeBootDisk = $true
    }
    { $_ -in @('SystemEmuWritable', 'SystemRamfos') } {
        $manifestPath = Join-Path $assetDirectory "C-Flash-512k-MMX-System.txt"
        $manifestData = Get-Manifest $manifestPath "emu-writable"
        $allEntries = @($manifestData.Entries)
        if ($Target -eq 'SystemRamfos') {
            $ramfosManifestPath = Join-Path $assetDirectory 'C-Flash-512k-MMX-RAMFOS-Tests.txt'
            $ramfosManifest = Get-Manifest $ramfosManifestPath 'emu-writable'
            $allEntries += @($ramfosManifest.Entries)
            $manifestData.VolumeLabel = $ramfosManifest.VolumeLabel
        }
        # Стабильный порядок служб; RAMD при необходимости запускает FORMAT.
        # Физические позиции BOOT/BIOS/DOS задаются отдельными проверками ниже.
        if ($BootFont) { $manifestData.BootFont = $BootFont.Replace('\', '/') }
        if (-not $manifestData.BootFont) { throw 'System manifest requires @bootfont path/to/engine.fnt' }
        $early = @('DOS/BOOT.SYS','DOS/BIOS.SYS','DOS/IO.SYS','DOS/MEM.SYS','DOS/DOS.SYS',$manifestData.BootFont,'DOS/FORMAT.COM','AUTOEXEC.BAT','DOS/NC.COM')
        foreach ($name in $early) {
            if (@($allEntries | Where-Object { $_.ImageName -eq $name }).Count -ne 1) { throw "System boot requires exactly one $name" }
        }
        $manifest = @($early | ForEach-Object { $name = $_; $allEntries | Where-Object { $_.ImageName -eq $name } })
        $manifest += @($allEntries | Where-Object { $_.ImageName -notin $early })
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-system.bin"
        $volumeSize = 512KB
        $chipSize = 64KB
        $maxFiles = 64
        $imageFormat = "Raw"
        $outputName = if ($Target -eq 'SystemRamfos') { 'C-Flash-512k-MMX-RAMFOS-Tests.bin' } else { 'C-Flash-512k-MMX-System-Emu80.bin' }
        $outputPath = Join-Path $imageDirectory $outputName
        $makeBootDisk = $true
    }
    "SystemHardware" {
        $manifestPath = Join-Path $assetDirectory "C-Flash-512k-MMX-System.txt"
        $manifestData = Get-Manifest $manifestPath "hardware"
        $allEntries = @($manifestData.Entries)
        # Стабильный порядок загрузочных файлов; первая страница обязательна лишь для DOS.
        if ($BootFont) { $manifestData.BootFont = $BootFont.Replace('\', '/') }
        if (-not $manifestData.BootFont) { throw 'System manifest requires @bootfont path/to/engine.fnt' }
        $early = @('DOS/BOOT.SYS','DOS/BIOS.SYS','DOS/IO.SYS','DOS/MEM.SYS','DOS/DOS.SYS',$manifestData.BootFont,'DOS/FORMAT.COM','AUTOEXEC.BAT','DOS/NC.COM')
        foreach ($name in $early) {
            if (@($allEntries | Where-Object { $_.ImageName -eq $name }).Count -ne 1) { throw "System boot requires exactly one $name" }
        }
        $manifest = @($early | ForEach-Object { $name = $_; $allEntries | Where-Object { $_.ImageName -eq $name } })
        $manifest += @($allEntries | Where-Object { $_.ImageName -notin $early })
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-system.bin"
        $volumeSize = $HardwareSizeKB * 1KB
        $chipSize = 64KB
        $maxFiles = 64
        $imageFormat = "Raw"
        $outputName = switch ($HardwareSizeKB) {
            128 { "C-Flash-128k-MMX-System.bin" }
            256 { "C-Flash-256k-MMX-System.bin" }
            512 { "C-Flash-512k-MMX-System.bin" }
        }
        $outputPath = Join-Path $imageDirectory $outputName
        $makeBootDisk = $true
    }
    "Games64k" {
        $manifestPath = Join-Path $assetDirectory "H-ROM-64k-Games.txt"
        $manifestData = Get-Manifest $manifestPath
        $manifest = @($manifestData.Entries)
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-flash.bin"
        $volumeSize = 64KB
        $chipSize = 64KB
        $maxFiles = 64
        $imageFormat = "Split"
        $outputPath = Join-Path $imageDirectory "H-ROM-64k-Games.bin"
        $partNamePattern = [IO.Path]::ChangeExtension([IO.Path]::GetFileName($manifestPath), ".bin")
        $makeBootDisk = $false
    }
    "Games2M" {
        $manifestPath = Join-Path $assetDirectory "H-Flash-2M-Games.txt"
        if ($ManifestFile) { $manifestPath = (Resolve-Path -LiteralPath $ManifestFile).Path }
        $manifestData = Get-Manifest $manifestPath
        $manifest = @($manifestData.Entries)
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-flash.bin"
        $volumeSize = 2MB
        $chipSize = 512KB
        # Size the fixed FAT root in whole sectors. Only top-level names and
        # the volume label need slots here; subdirectories use data clusters.
        $rootNames = @($manifest | ForEach-Object { ($_.ImageName -split '[/\\]', 2)[0] } | Sort-Object -Unique)
        $entriesPerSector = [int]($sectorSize / 32)
        $maxFiles = [int]([Math]::Ceiling(($rootNames.Count + 1) / $entriesPerSector) * $entriesPerSector)
        if ($maxFiles -gt 256) { throw "Games2M root exceeds the MXOS limit of 256 entries" }
        $imageFormat = "Split"
        $stem = [IO.Path]::GetFileNameWithoutExtension($manifestPath)
        $outputPath = Join-Path $imageDirectory "$stem.bin"
        $partNamePattern = "$stem-chip{0}.bin"
        $makeBootDisk = $false
    }
    { $mxRom } {
        $manifestPath = Join-Path $assetDirectory 'C-ROM-32k-MX-System.txt'
        $manifestData = Get-Manifest $manifestPath
        $manifest = @($manifestData.Entries)
        $volumeSize = if ($Target -eq 'MxRom48') { 48KB } else { 32KB }
        $maxFiles = 16
        $imageFormat = 'Raw'
        $bootPath = Join-Path $repositoryRoot 'bin/mx/rom-sector.bin'
        $romName = [IO.Path]::GetFileNameWithoutExtension($manifestPath).Replace('-32k-', ('-{0}k-' -f ($volumeSize / 1KB)))
        $outputPath = Join-Path $imageDirectory "$romName.bin"
        $makeBootDisk = $false
    }
    { $floppy } {
        $manifestName = if ($mxFloppy) { "A-Floppy-720k-MX-System.txt" } else { "A-Floppy-720k-Programs.txt" }
        $manifestPath = Join-Path $assetDirectory $manifestName
        $autoexecVariant = if ($mxFloppy) { 'mx' } else { 'emu' }
        $manifestData = Get-Manifest $manifestPath $autoexecVariant
        $manifest = @($manifestData.Entries)
        $bootPath = $null
        $sectorSize = 512
        $sectorsPerCluster = 2
        $fatCopies = 2
        $mediaByte = 0xF9
        $volumeSize = 720KB
        $maxFiles = 112
        $imageFormat = "Raw"
        $diskName = if ($MxUseRomBundle) { 'A-Floppy-720k-MX-ROM-Bundle.img' } else { [IO.Path]::ChangeExtension($manifestName, ".img") }
        $outputPath = Join-Path $imageDirectory $diskName
        $makeBootDisk = $false
    }
}

$volumeLabel = $manifestData.VolumeLabel
$gameExeEntries = @($manifest | Where-Object {
    $_.Source -match '^games[/\\]' -and $_.ImageName -match '\.exe$'
})
$gameCatalogPath = Join-Path $assetDirectory 'games/catalog.csv'

# Линейный bootstrap: фиксированные адреса не зависят от длины соседних файлов.
$stagedBoot = $makeBootDisk -and $imageFormat -eq 'Raw'
if ($makeBootDisk -and -not $stagedBoot) { throw 'Split BIOS/DOS requires a linear system image: use SystemEmuWritable, SystemRamfos or SystemHardware (128/256/512 KB).' }
$fixedBootFiles = @{ 'DOS/BOOT.SYS' = 0x2000; 'DOS/BIOS.SYS' = 0x3000; 'DOS/IO.SYS' = 0x6000; 'DOS/MEM.SYS' = 0x7000; 'DOS/DOS.SYS' = 0x8000 }
if ($BootstrapProbe -and -not $stagedBoot) { throw '-BootstrapProbe requires a linear system image.' }
if ($stagedBoot) {
    $bootFontName = $manifestData.BootFont
    $fontEntries = @($manifest | Where-Object { $_.ImageName -eq $bootFontName })
    if ($fontEntries.Count -ne 1) { throw "Bootstrap requires exactly one $bootFontName" }
    $fontSource = Join-Path $assetDirectory $fontEntries[0].Source
    $fixedBootFiles[$bootFontName] = 0x4800
    $bootstrapArgs = @('--font-body', $fontSource, '--font-name', [IO.Path]::GetFileNameWithoutExtension($bootFontName))
    if ($BootstrapProbe) { $bootstrapArgs += '--probe' }
    if (-not $RunBuild -and -not $Prebuilt) {
        if ($FontsReady) { $bootstrapArgs += '--fonts-ready' }
        if ($Force) { $bootstrapArgs += '--force' }
        & python (Join-Path $repositoryRoot 'scripts/build/build-bootstrap.py') @bootstrapArgs
        if ($LASTEXITCODE -ne 0) { throw 'Bootstrap build failed.' }
    }
    $bootPath = Join-Path $repositoryRoot 'bin/bootstrap/sector.bin'
    $bootSetPath = Join-Path $repositoryRoot 'bin/bootstrap/boot-set.json'
    $bootSet = Get-Content -LiteralPath $bootSetPath -Raw | ConvertFrom-Json
    if ($bootSet.probe -ne [bool]$BootstrapProbe -or $bootSet.fontName -ne [IO.Path]::GetFileNameWithoutExtension($bootFontName)) {
        throw "Boot sector set does not match the selected boot font or probe mode: $bootSetPath"
    }
    if ((Get-ImageHash ([IO.File]::ReadAllBytes($bootPath))) -ne $bootSet.files.sector) { throw "Boot sector does not match $bootSetPath" }
    foreach ($name in @('DOS/BOOT.SYS','DOS/BIOS.SYS','DOS/IO.SYS','DOS/MEM.SYS','DOS/DOS.SYS',$bootFontName)) {
        if (@($manifest | Where-Object { $_.ImageName -eq $name }).Count -ne 1) { throw "Bootstrap requires exactly one $name" }
    }
    if ($BootstrapProbe) { $outputPath = $outputPath -replace '\.bin$', '-bootprobe.bin' }
}

$hasNestedFiles = @($manifest | Where-Object { $_.ImageName -match '[/\\]' }).Count -gt 0
$errorBuilder = Join-Path $repositoryRoot 'scripts/build/build-error-strings.py'
$errorTable = Join-Path $repositoryRoot 'bin/ERRORS.DAT'
if (-not $RunBuild) {
    $inputs = @($manifestPath, (Join-Path $PSScriptRoot 'executable.ps1'), (Join-Path $PSScriptRoot 'cpu.ps1'))
    if ($mxFloppy) { $inputs += Join-Path $repositoryRoot $mxBootPayload }
    if ($mxRom) { $inputs += Join-Path $repositoryRoot 'bin/mx/rom-recovery.bin' }
    if ($ramfosManifestPath) { $inputs += $ramfosManifestPath }
    if ($bootPath) { $inputs += $bootPath }
    if ($gameExeEntries.Count) { $inputs += $gameCatalogPath }
    if ($stagedBoot) { $inputs += $bootSetPath }
    if ($stagedBoot -and -not $Prebuilt) { $inputs += @($errorBuilder, (Join-Path $repositoryRoot 'source/include/mxos.inc')) }
    if ($hasNestedFiles) { $inputs += (Join-Path $PSScriptRoot 'nest-fat-image.py') }
    $inputs += @(foreach ($entry in $manifest) {
        $path = [IO.Path]::GetFullPath([IO.Path]::Combine($assetDirectory, $entry.Source))
        if ($Prebuilt -or -not $stagedBoot -or $path -ne $errorTable) { $path }
        if ([IO.Path]::GetExtension($path) -eq '.cpu' -and $entry.ImageName -match '\.(com|exe|dll|fnt|rks)$') {
            [IO.Path]::ChangeExtension($path, '.i80')
        }
    })

    $cacheArgs = @((Join-Path $repositoryRoot 'scripts/build/build_cache.py'), "image: $nestedTarget", '--inputs-from-stdin')
    if ($stagedBoot) { $cacheArgs += '--input-content' }
    $outputs = switch ($Target) {
        'MxFloppy' {
            $outputPath
            Join-Path $imageDirectory "$mxFlashStem.bin"
            1..2 | ForEach-Object { Join-Path $imageDirectory "$mxFlashStem-chip$_.bin" }
        }
        'Games2M' { $outputPath; 1..4 | ForEach-Object { Join-Path $imageDirectory ($partNamePattern -f $_) } }
        default { $outputPath }
    }
    if ($stagedBoot -and -not $Prebuilt) { $outputs = @($outputs) + $errorTable }
    if ($hasNestedFiles) {
        $outputs = @($outputs) + (Join-Path $repositoryRoot "bin/nested-$nestedTarget.entries") + (Join-Path $repositoryRoot "bin/nested-$nestedTarget.bin")
    }
    foreach ($path in $outputs) { $cacheArgs += @('--output', $path) }
    if ($Force) { $cacheArgs += '--force' }
    $cacheArgs += @('--', $PSCommandPath, $Target, '-HardwareSizeKB', $HardwareSizeKB, '-RunBuild')
    if ($ManifestFile) { $cacheArgs += @('-ManifestFile', $manifestPath) }
    if ($BootFont) { $cacheArgs += @('-BootFont', $BootFont) }
    if ($BootstrapProbe) { $cacheArgs += '-BootstrapProbe' }
    if ($Prebuilt) { $cacheArgs += '-Prebuilt' }
    if ($MxUseRomBundle) { $cacheArgs += '-MxUseRomBundle' }
    $OutputEncoding = [Text.UTF8Encoding]::new($false)
    ConvertTo-Json -InputObject $inputs -Compress | & python -X utf8 @cacheArgs
    exit $LASTEXITCODE
}

. (Join-Path $PSScriptRoot 'executable.ps1')
$gameModes = @{}
if ($gameExeEntries.Count) {
    foreach ($game in Import-Csv -LiteralPath $gameCatalogPath -Encoding UTF8) {
        $key = 'games/' + $game.File.Replace('\', '/')
        if ($gameModes.ContainsKey($key)) { throw "Duplicate game in catalog: $key" }
        $gameModes[$key] = $game.RunMode
    }
}
if ($stagedBoot -and -not $Prebuilt) {
    & python $errorBuilder $errorTable
    if ($LASTEXITCODE -ne 0) { throw 'Cannot build ERRORS.DAT' }
}
New-Item -ItemType Directory -Path $imageDirectory -Force | Out-Null

$volumeSectors = [int]($volumeSize / $sectorSize)
$fatSectors = if ($floppy) { 3 } elseif ($mxRom) { 2 } else { [int]($volumeSectors * 2 / $sectorSize) }
$directorySectors = [int]($maxFiles * 32 / $sectorSize)
$reservedPayload = [byte[]]@()
if ($mxFloppy) {
    $reservedPayload = [IO.File]::ReadAllBytes((Join-Path $repositoryRoot $mxBootPayload))
    if ($reservedPayload.Length % 512) { throw 'Unaligned MX bootstrap payload' }
} elseif ($mxRom) {
    $reservedPayload = [IO.File]::ReadAllBytes((Join-Path $repositoryRoot 'bin/mx/rom-recovery.bin'))
    if ($reservedPayload.Length -ne $sectorSize) { throw 'Invalid MX recovery sector size' }
}
$reservedSectors = 1 + [int]($reservedPayload.Length / $sectorSize)
$dataStartSector = $reservedSectors + $fatCopies * $fatSectors + $directorySectors
$dataSectors = $volumeSectors - $dataStartSector
if ($imageFormat -eq 'Mx2') {
    # STD-вход занимает первые четыре байта физической половины ROM.
    # Последний логический сектор доступен не целиком; файлы в него не кладём.
    $dataSectors--
}
$dataClusters = [int][Math]::Floor($dataSectors / $sectorsPerCluster)
$clusterSize = $sectorSize * $sectorsPerCluster

$boot = if ($floppy) { ,([byte[]]::new($sectorSize)) } else { ,([IO.File]::ReadAllBytes($bootPath)) }
if ($boot.Length -ne $sectorSize) {
    throw "boot.bin must be exactly $sectorSize bytes"
}
if ($mxRom) {
    if ($boot[0] -ne 0xC3 -or [BitConverter]::ToUInt16($boot, 1) -ne $dataStartSector * $sectorSize -or
        $manifest[0].ImageName -ne 'BOOT.SYS' -or $manifest[0].Source -ne '../bin/mx/loader.bin') {
        throw 'MX ROM reset must point to BOOT.SYS in the first data cluster'
    }
}
$fatEntryCount = if ($floppy) { [int]($fatSectors * $sectorSize * 2 / 3) } else { [int]($fatSectors * $sectorSize / 2) }
$fat = [uint16[]]::new($fatEntryCount)
$fat[0] = 0xFF00 + $mediaByte
$fat[1] = 0xFFFF
$directory = New-Object IO.MemoryStream
$fileData = New-Object IO.MemoryStream

# Физическая нумерация остаётся обычной. В 2-МБ образе чередуем только
# кластеры данных через FAT; загрузочный сектор, FAT и корень остаются на чипе 0.
$interleaveFiles = $Target -eq 'Games2M'
$usedClusters = New-Object bool[] ($dataClusters + 2)
if ($interleaveFiles) {
    $chipSectors = [int]($chipSize / $sectorSize)
    $emptyData = Expand-Bytes ([byte[]]@()) ($dataSectors * $sectorSize)
    $fileData.Write($emptyData, 0, $emptyData.Length)
}
# Запас таблицы не относится к области данных и не должен считаться свободным.
if (-not $floppy) {
    for ($index = $dataClusters + 2; $index -lt $fat.Length; $index++) {
        $fat[$index] = 0xFFFF
    }
}
if ($imageFormat -eq 'Mx2') {
    # Частично недоступный сектор внутри объявленного тома — bad cluster,
    # а не занятый кластер без владельца (его CHKDSK счёл бы потерянным).
    $fat[$dataClusters + 2] = 0xFFF7
}
$nextCluster = 2
$fileCount = 1
# Метка в BPB совпадает с единственной меткой в корне.
$labelEntry = New-Object byte[] 32
[Array]::Copy([Text.Encoding]::ASCII.GetBytes($volumeLabel), 0, $labelEntry, 0, 11)
$labelEntry[11] = 0x08
$directory.Write($labelEntry, 0, $labelEntry.Length)
$dosCluster = 0
$dosAddress = 0
$dosSectors = 0
$imageNames = @{}
$mxRomBundleOffsets = @{ "BIOS.SYS" = 0x24; "ROTATE.FNT" = 0x26; "IO.SYS" = 0x28; "MEM.SYS" = 0x2A }
$mxRomBundleFiles = 0
$clusterPadding = Expand-Bytes ([byte[]]@()) $clusterSize

try {
    foreach ($manifestEntry in $manifest) {
        $sourcePath = [IO.Path]::Combine($assetDirectory, $manifestEntry.Source)
        if (-not [IO.File]::Exists($sourcePath)) {
            throw "Manifest input not found at $($manifestEntry.Location): $sourcePath"
        }
        $sourceName = if ($floppy) { $manifestEntry.ImageName.ToUpperInvariant() } else { $manifestEntry.ImageName }
        $extension = [IO.Path]::GetExtension($sourceName).TrimStart(".")

        $baseName = [IO.Path]::GetFileNameWithoutExtension($sourceName)
        $loadAddress = Get-LoadAddress $manifestEntry.LoadAddress
        $gameMode = $gameModes[$manifestEntry.Source.Replace('\', '/')]
        if ($extension -eq 'exe' -and $manifestEntry.Source -match '^games[/\\]') {
            if ($gameMode -notin @('AUTO','MX','STD','MON','RF','NATIVE')) {
                throw "Missing or invalid RunMode in the game catalog for $($manifestEntry.Source) at $($manifestEntry.Location)"
            }
            $storedBytes = Convert-ExecutableFile $sourcePath $extension $loadAddress -GameMode $gameMode
        } elseif ($extension -in @('com', 'exe', 'dll', 'fnt', 'rks')) {
            $storedBytes = Convert-ExecutableFile $sourcePath $extension $loadAddress
        } else {
            $storedBytes = [IO.File]::ReadAllBytes($sourcePath)
        }
        $originalSize = $storedBytes.Length

        if ($baseName.Length -gt 8 -or $extension.Length -gt 3) {
            throw "Image name '$sourceName' does not fit FAT 8.3. Specify a separate short image name."
        }
        $imageNameKey = $sourceName.Replace('\', '/').ToUpperInvariant()
        if ($imageNames.ContainsKey($imageNameKey)) {
            throw "FAT 8.3 name conflict '$baseName.$extension': '$($imageNames[$imageNameKey])' and '$sourceName'."
        }
        $imageNames[$imageNameKey] = $sourceName

        if ($fileCount -ge $maxFiles -and -not $hasNestedFiles) {
            throw "$($Target): root directory is full while adding $sourceName ($fileCount of $maxFiles files) at $($manifestEntry.Location)"
        }
        $fileCount++

        if ($stagedBoot -and $fixedBootFiles.ContainsKey($sourceName)) {
            if ($sourceName -eq $bootFontName) {
                if ((Get-ImageHash $storedBytes 32) -ne $bootSet.fontHash) { throw "Boot font does not match $bootSetPath" }
            } elseif ((Get-ImageHash $storedBytes) -ne $bootSet.files.$sourceName) { throw "$sourceName does not match $bootSetPath" }
            $fixedAddress = $fixedBootFiles[$sourceName]
            $limit = switch ($sourceName) { 'DOS/BOOT.SYS' { 0x3000 }; 'DOS/BIOS.SYS' { 0x4800 }; 'DOS/IO.SYS' { 0x7000 }; 'DOS/MEM.SYS' { 0x8000 }; 'DOS/DOS.SYS' { 0x18000 } }
            if ($sourceName -eq $bootFontName) { $limit = 0x6000 }
            if ($fixedAddress -lt $dataStartSector * $sectorSize -or $fixedAddress + $originalSize -gt $limit) { throw "Fixed bootstrap slot overflow: $sourceName" }
            if ($sourceName -eq $bootFontName -and $loadAddress -ne 0xD800) { throw 'Boot font must load at D800h' }
            if ($sourceName -eq 'DOS/IO.SYS' -and $loadAddress -ne 0xF000) { throw 'Invalid IO load range' }
            if ($sourceName -eq 'DOS/BIOS.SYS' -and ($loadAddress -ne 0xC000 -or $loadAddress+$originalSize -gt 0xF000)) { throw 'Invalid BIOS load range' }
            if ($sourceName -eq 'DOS/DOS.SYS') {
                $dosLimit = $bootSet.dosLimit
                if ($loadAddress -ne 0x0100 -or $loadAddress+$originalSize -gt $dosLimit) { throw 'Invalid DOS load range' }
            }
            $nextCluster = [int]($fixedAddress / $sectorSize) - $dataStartSector + 2
        }

        $firstCluster = 0
        $previousCluster = 0
        for ($offset = 0; $offset -lt $storedBytes.Length; $offset += $clusterSize) {
            $searched = 0
            while ($true) {
                if ($nextCluster -ge $dataClusters + 2) { $nextCluster = 2 }
                if (-not $usedClusters[$nextCluster]) { break }
                $nextCluster++
                $searched++
                if ($searched -ge $dataClusters) {
                    $required = [int][Math]::Ceiling($storedBytes.Length / $clusterSize)
                    $available = [int]($offset / $clusterSize)
                    throw "$($Target): image data area is full while adding $sourceName at $($manifestEntry.Location). Needs $required clusters, $available were free ($clusterSize bytes per cluster)."
                }
            }
            $cluster = $nextCluster
            $usedClusters[$cluster] = $true
            if ($interleaveFiles) {
                $sector = $dataStartSector + $cluster - 2 + $chipSectors
                if ($sector -ge $volumeSectors) { $sector = $sector - $volumeSectors + 1 }
                $nextCluster = [Math]::Max(2, $sector - $dataStartSector + 2)
            } else {
                $nextCluster++
            }
            if ($firstCluster -eq 0) {
                $firstCluster = $cluster
            }
            if ($previousCluster -ne 0) {
                $fat[$previousCluster] = $cluster
            }
            $fat[$cluster] = 0xFFFF
            $previousCluster = $cluster

            $count = [Math]::Min($clusterSize, $storedBytes.Length - $offset)
            $fileData.Position = ($cluster - 2) * $clusterSize
            $fileData.Write($storedBytes, $offset, $count)
            if ($count -lt $clusterSize) {
                $fileData.Write($clusterPadding, 0, $clusterSize - $count)
            }
        }

        if ($stagedBoot -and $sourceName -eq 'DOS/BIOS.SYS' -and
            ($firstCluster -eq 0 -or ($dataStartSector + $nextCluster - 2) * $sectorSize -gt 32768)) {
            throw "Boot file must fit in the first 32-KB page: $sourceName"
        }

        if ($mxRom -and $mxRomBundleOffsets.ContainsKey($sourceName)) {
            $fileOffset = ($dataStartSector + $firstCluster - 2) * $sectorSize
            if ($sourceName -eq 'ROTATE.FNT') { $fileOffset += 32 }
            Set-Word $reservedPayload $mxRomBundleOffsets[$sourceName] $fileOffset
            $mxRomBundleFiles++
        }
        $entry = New-Object byte[] 32
        $nameBytes = [Text.Encoding]::ASCII.GetBytes(($baseName + "        ").Substring(0, 8))
        $extensionBytes = [Text.Encoding]::ASCII.GetBytes(($extension + "   ").Substring(0, 3))
        [Array]::Copy($nameBytes, 0, $entry, 0, 8)
        [Array]::Copy($extensionBytes, 0, $entry, 8, 3)
        $entry[11] = $manifestEntry.Attributes
        Set-Word $entry 26 $firstCluster
        Set-Word $entry 28 ($originalSize -band 0xFFFF)
        Set-Word $entry 30 ($originalSize -shr 16)
        $directory.Write($entry, 0, $entry.Length)

        if ($makeBootDisk -and $sourceName.Equals("DOS/BIOS.SYS", [StringComparison]::OrdinalIgnoreCase)) {
            $dosCluster = $firstCluster
            $dosAddress = $loadAddress
            $dosSectors = [int][Math]::Ceiling($originalSize / 256.0)
        }
    }

    while ($directory.Length -lt $directorySectors * $sectorSize) {
        $directory.WriteByte(0)
    }

    if ($floppy) {
        [Array]::Copy([byte[]](0xEB, 0x3C, 0x90), $boot, 3)
        Set-Word $boot 0x0E $reservedSectors
        $boot[0x10] = $fatCopies
        $boot[0x15] = $mediaByte
        Set-Word $boot 0x18 9
        Set-Word $boot 0x1A 2
        $boot[0x26] = 0x29
        [Array]::Copy([Text.Encoding]::ASCII.GetBytes("FAT12   "), 0, $boot, 0x36, 8)
        $boot[0x3E] = 0xEB
        $boot[0x3F] = 0xFE
        $boot[510] = 0x55
        $boot[511] = 0xAA
    }
    $oemName = if ($floppy) { "MXOS2   " } else { "MXOS256 " }
    [Array]::Copy([Text.Encoding]::ASCII.GetBytes($oemName), 0, $boot, 3, 8)
    Set-Word $boot 0x0B $sectorSize
    if ($mxRom) { Set-Word $boot 0x0E $reservedSectors }
    $boot[0x0D] = $sectorsPerCluster
    Set-Word $boot 0x11 $maxFiles
    Set-Word $boot 0x13 $volumeSectors
    Set-Word $boot 0x16 $fatSectors
    $labelBytes = [Text.Encoding]::ASCII.GetBytes($volumeLabel)
    if ($labelBytes.Length -ne 11) {
        throw "Volume label must contain exactly 11 characters: '$volumeLabel'"
    }
    [Array]::Copy($labelBytes, 0, $boot, 0x2B, 11)

    if ($makeBootDisk) {
        if ($dosCluster -eq 0) {
            throw "BIOS.SYS is missing from the system manifest"
        }
        $dosRomAddress = (($dosCluster - 2) * $sectorsPerCluster + $dataStartSector) * $sectorSize
        if (-not $stagedBoot -and $imageFormat -eq "Raw" -and
            ($dosCluster -ne 2 -or $dosRomAddress + $dosSectors * $sectorSize -gt 32768)) {
            throw "BIOS.SYS must be first and fit in the first 32-KB hardware ROM page"
        }
        if (-not $stagedBoot) {
            Set-Word $boot 0x3F $dosRomAddress
            Set-Word $boot 0x42 $dosAddress
            $boot[0x4A] = (($dosAddress -shr 8) + $dosSectors) -band 0xFF
        }
    }

    # Keep the complete host entry list out of the fixed-size FAT root.
    # nest-fat-image writes only top-level children into the real root.
    $rootBytes = $directory.ToArray()
    if ($hasNestedFiles) {
        $entryListPath = Join-Path $repositoryRoot "bin/nested-$nestedTarget.entries"
        [IO.File]::WriteAllBytes($entryListPath, $rootBytes)
        $rootBytes = New-Object byte[] ($directorySectors * $sectorSize)
    }
    $fatBytes = [byte[]]::new($fatCopies * $fatSectors * $sectorSize)
    if ($floppy) {
        for ($index = 0; $index -lt $fat.Length; $index += 2) {
            $position = ($index -shr 1) * 3
            $even = $fat[$index]
            $odd = $fat[$index + 1]
            $fatBytes[$position] = $even -band 0xFF
            $fatBytes[$position + 1] = (($even -shr 8) -band 0x0F) -bor (($odd -band 0x0F) -shl 4)
            $fatBytes[$position + 2] = ($odd -shr 4) -band 0xFF
        }
        [Array]::Copy($fatBytes, 0, $fatBytes, $fatSectors * $sectorSize, $fatSectors * $sectorSize)
    } else {
        [Buffer]::BlockCopy($fat, 0, $fatBytes, 0, $fatBytes.Length)
    }
    if ($mxRom -and $mxRomBundleFiles -ne $mxRomBundleOffsets.Count) { throw 'Incomplete MX ROM BIOS bundle' }
    $start = Join-Bytes $boot $reservedPayload $fatBytes $rootBytes
    $data = $fileData.ToArray()
    $nestedPaths = @($manifest | ForEach-Object { $_.ImageName })
    if ($hasNestedFiles) {
        $stagingPath = Join-Path $repositoryRoot "bin/nested-$nestedTarget.bin"
        [IO.File]::WriteAllBytes($stagingPath, (Expand-Bytes (Join-Bytes $start $data) $volumeSize))
        $nestOptions = @('--entries', $entryListPath)
        if ($floppy) {
            $nestOptions += '--fat12'
            $nestedPaths = @($nestedPaths | ForEach-Object { $_.ToUpperInvariant() })
        }
        & python (Join-Path $PSScriptRoot 'nest-fat-image.py') $stagingPath @nestedPaths @nestOptions
        if ($LASTEXITCODE -ne 0) { throw 'Nested FAT image generation failed.' }
        $nestedImage = [IO.File]::ReadAllBytes($stagingPath)
        [Array]::Copy($nestedImage, 0, $start, 0, $start.Length)
        $data = New-Object byte[] ($nestedImage.Length - $start.Length)
        [Array]::Copy($nestedImage, $start.Length, $data, 0, $data.Length)
    }

    switch ($imageFormat) {
        "Mx2" {
            $dataPageOffset = 32768 - $start.Length
            # Boot/FAT/корень уже входят в нижнюю MX-половину. STD-половина
            # вмещает полные 32 КБ минус вход, а не остаток после этих таблиц.
            $upperLength = 32768 - 4
            $upperAvailable = [Math]::Max(0, $data.Length - $dataPageOffset)
            $upperCount = [Math]::Min($upperLength, $upperAvailable)
            $upperData = New-Object byte[] $upperCount
            if ($upperCount -gt 0) {
                [Array]::Copy($data, $dataPageOffset, $upperData, 0, $upperCount)
            }
            $firstPage = Expand-Bytes (Join-Bytes ([byte[]](0x31, 0xFF, 0xF7, 0xC7)) $upperData) 32768

            $lowerCount = [Math]::Min($dataPageOffset, $data.Length)
            $lowerData = New-Object byte[] $lowerCount
            [Array]::Copy($data, 0, $lowerData, 0, $lowerData.Length)
            $secondPage = Expand-Bytes (Join-Bytes $start $lowerData) 32768
            [IO.File]::WriteAllBytes($outputPath, (Join-Bytes $firstPage $secondPage))
            Write-Host ("[image] {0}" -f $outputPath)
        }
        "Raw" {
            $raw = Expand-Bytes (Join-Bytes $start $data) $volumeSize
            [IO.File]::WriteAllBytes($outputPath, $raw)
            if ($mxFloppy) {
                $flash = Expand-Bytes $raw 1MB
                $flash[514] = 0x46  # MXF1, ??????????? ????????? ??????? flash
                [IO.File]::WriteAllBytes((Join-Path $imageDirectory "$mxFlashStem.bin"), $flash)
                $part = [byte[]]::new(512KB)
                foreach ($chip in 1..2) {
                    [Array]::Copy($flash, ($chip - 1) * 512KB, $part, 0, 512KB)
                    [IO.File]::WriteAllBytes((Join-Path $imageDirectory "$mxFlashStem-chip$chip.bin"), $part)
                }
            }
            Write-Host ("[image] {0}" -f $outputPath)
        }
        "Split" {
            $image = Join-Bytes $start $data
            if ($Target -eq 'Games2M') { [IO.File]::WriteAllBytes($outputPath, (Expand-Bytes $image $volumeSize)) }
            $outputDirectory = Split-Path -Parent $outputPath
            $partNumber = 0
            for ($offset = 0; $offset -lt $image.Length; $offset += $chipSize) {
                $count = [Math]::Min($chipSize, $image.Length - $offset)
                $part = New-Object byte[] $count
                [Array]::Copy($image, $offset, $part, 0, $count)
                $part = Expand-Bytes $part $chipSize
                $partPath = Join-Path $outputDirectory ($partNamePattern -f ($partNumber + 1))
                [IO.File]::WriteAllBytes($partPath, $part)
                Write-Host ("[image] {0}" -f $partPath)
                $partNumber++
            }
        }
    }
}
finally {
    $directory.Dispose()
    $fileData.Dispose()
}
