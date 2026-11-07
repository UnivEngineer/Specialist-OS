[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet("SystemEmu", "SystemEmuFlash", "SystemEmuWritable", "SystemRamfos", "SystemHardware", "Games64k", "Games2M")]
    [string] $Target,

    [Parameter(Position = 1)]
    [ValidateSet(128, 256, 512)]
    [int] $HardwareSizeKB = 128,
    [string] $ManifestFile,
    [switch] $BootstrapProbe
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0
if ($ManifestFile -and $Target -ne 'Games2M') { throw '-ManifestFile is currently supported only for Games2M.' }

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$assetDirectory = Join-Path $repositoryRoot "assets"
$imageDirectory = Join-Path $repositoryRoot "images"
$sectorSize = 256
$sectorsPerCluster = 1
. (Join-Path $PSScriptRoot 'executable.ps1')

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
    $entries = foreach ($line in [IO.File]::ReadAllLines($Path)) {
        $lineNumber++
        $entry = $line.Trim()
        if ($entry.Length -eq 0 -or $entry.StartsWith("#")) {
            continue
        }
        if ($entry.StartsWith("@")) {
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
        [pscustomobject]@{ ImageName = $fields[0]; Source = $source; LoadAddress = $address; Format = $format; Attributes = $attributes }
    }
    if ($null -eq $volumeLabel) {
        throw "Missing @label in manifest: $Path"
    }
    return [pscustomobject]@{ Entries = @($entries); VolumeLabel = $volumeLabel }
}

switch ($Target) {
    "SystemRamfos" {
        $manifestPath = Join-Path $PSScriptRoot "manifests\A-System.txt"
        $manifest = @((Get-Manifest $manifestPath "emu").Entries |
            Where-Object { $_.ImageName -notin @("Test-Kb.COM", "Test-Fif.COM", "Test-MX.COM") })
        $ramfosManifestPath = Join-Path $PSScriptRoot "manifests\A-RAMFOS.txt"
        $manifestData = Get-Manifest $ramfosManifestPath "emu"
        $manifest += @($manifestData.Entries)
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-system.bin"
        $volumeSize = 64KB
        $chipSize = 64KB
        $maxFiles = 64
        $imageFormat = "Mx2"
        $outputPath = Join-Path $imageDirectory "SystemRom_ramfos_emulator.bin"
        $makeBootDisk = $true
    }
    { $_ -in @('SystemEmu', 'SystemEmuFlash') } {
        $manifestPath = Join-Path $PSScriptRoot "manifests\A-System.txt"
        $autoexecVariant = if ($Target -eq 'SystemEmuFlash') { 'emu-flash' } else { 'emu' }
        $manifestData = Get-Manifest $manifestPath $autoexecVariant
        $manifest = @($manifestData.Entries)
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-system.bin"
        $volumeSize = 64KB
        $chipSize = 64KB
        $maxFiles = 64
        $imageFormat = "Mx2"
        $outputName = if ($Target -eq 'SystemEmuFlash') { 'SystemRom_emu80_flash.bin' } else { 'SystemRom_emulator.bin' }
        $outputPath = Join-Path $imageDirectory $outputName
        $makeBootDisk = $true
    }
    "SystemEmuWritable" {
        $manifestPath = Join-Path $PSScriptRoot "manifests\A-System.txt"
        $manifestData = Get-Manifest $manifestPath "emu-writable"
        $allEntries = @($manifestData.Entries)
        # Стабильный порядок служб; RAMD при необходимости запускает FORMAT.
        # Физические позиции BOOT/BIOS/DOS задаются отдельными проверками ниже.
        $early = @('BIOS.SYS','IO.SYS','DOS.SYS','FONTKOI.FNT','RAMDISK.COM','FORMAT.COM','AUTOEXEC.BAT','FLASHSYS.COM','FLASH.COM','NC.COM','ROM.COM')
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
        $outputPath = Join-Path $imageDirectory "SystemRom_emu80_writable.bin"
        $makeBootDisk = $true
    }
    "SystemHardware" {
        $manifestPath = Join-Path $PSScriptRoot "manifests\A-System.txt"
        $manifestData = Get-Manifest $manifestPath "hardware"
        $allEntries = @($manifestData.Entries)
        # Стабильный порядок загрузочных файлов; первая страница обязательна лишь для DOS.
        $early = @('BIOS.SYS','IO.SYS','DOS.SYS','FONTKOI.FNT','RAMDISK.COM','FORMAT.COM','AUTOEXEC.BAT','FLASHSYS.COM','FLASH.COM','NC.COM','ROM.COM')
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
            128 { "SystemRom_128K.bin" }
            256 { "SystemRom_256K.bin" }
            512 { "SystemRom_512K.bin" }
        }
        $outputPath = Join-Path $imageDirectory $outputName
        $makeBootDisk = $true
    }
    "Games64k" {
        $manifestPath = Join-Path $PSScriptRoot "manifests\H-ROM64k.txt"
        $manifestData = Get-Manifest $manifestPath
        $manifest = @($manifestData.Entries)
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-flash.bin"
        $volumeSize = 64KB
        $chipSize = 64KB
        $maxFiles = 64
        $imageFormat = "Split"
        $outputPath = Join-Path $imageDirectory "FlashDisk_emulator.bin"
        $partNamePattern = "FlashDisk_emulator.bin"
        $makeBootDisk = $false
    }
    "Games2M" {
        $manifestPath = Join-Path $PSScriptRoot "manifests\H-Games2M.txt"
        if ($ManifestFile) { $manifestPath = (Resolve-Path -LiteralPath $ManifestFile).Path }
        $manifestData = Get-Manifest $manifestPath
        $manifest = @($manifestData.Entries)
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-flash.bin"
        $volumeSize = 2MB
        $chipSize = 512KB
        $maxFiles = 192
        $imageFormat = "Split"
        $outputPath = Join-Path $imageDirectory "FlashDisk_512K.bin"
        $partNamePattern = "FlashDisk_chip{0}_512K.bin"
        $makeBootDisk = $false
    }
}

$volumeLabel = $manifestData.VolumeLabel

# Линейный bootstrap: фиксированные адреса не зависят от длины соседних файлов.
$stagedBoot = $makeBootDisk -and $imageFormat -eq 'Raw'
if ($makeBootDisk -and -not $stagedBoot) { throw 'Split BIOS/DOS requires a linear system image: use SystemEmuWritable or SystemHardware (128/256/512 KB).' }
$fixedBootFiles = @{ 'BOOT.SYS' = 0x2000; 'BIOS.SYS' = 0x3000; 'FONTKOI.FNT' = 0x5800; 'IO.SYS' = 0x6000; 'MEM.SYS' = 0x7000; 'DOS.SYS' = 0x8000 }
if ($BootstrapProbe -and -not $stagedBoot) { throw '-BootstrapProbe requires a linear system image.' }
if ($stagedBoot) {
    $bootstrapArgs = @()
    if ($BootstrapProbe) { $bootstrapArgs += '--probe' }
    & python (Join-Path $repositoryRoot 'scripts/build/build-bootstrap.py') @bootstrapArgs
    if ($LASTEXITCODE -ne 0) { throw 'Bootstrap build failed.' }
    $bootPath = Join-Path $repositoryRoot 'bin/bootstrap/sector.bin'
    foreach ($name in @('BIOS.SYS','IO.SYS','DOS.SYS','FONTKOI.FNT')) {
        if (@($manifest | Where-Object { $_.ImageName -eq $name }).Count -ne 1) { throw "Bootstrap requires exactly one $name" }
    }
    $bootEntry = [pscustomobject]@{ ImageName='BOOT.SYS'; Source='../bin/bootstrap/BOOT.bin'; LoadAddress='0x2000'; Attributes=0x05 }
    $tailEntry = [pscustomobject]@{ ImageName='MEM.SYS'; Source='../bin/MEM.sys'; LoadAddress='0x0000'; Attributes=0x05 }
    $manifest = @($bootEntry) + @($manifest | Where-Object { $_.ImageName -eq 'BIOS.SYS' }) + @($manifest | Where-Object { $_.ImageName -eq 'IO.SYS' }) + @($tailEntry) + @($manifest | Where-Object { $_.ImageName -eq 'DOS.SYS' }) + @($manifest | Where-Object { $_.ImageName -notin @('BIOS.SYS','IO.SYS','DOS.SYS') })
    if ($BootstrapProbe) { $outputPath = $outputPath -replace '\.bin$', '-bootprobe.bin' }
}

New-Item -ItemType Directory -Path $imageDirectory -Force | Out-Null

$volumeSectors = [int]($volumeSize / $sectorSize)
$fatSectors = [int]($volumeSectors * 2 / $sectorSize)
$directorySectors = [int]($maxFiles * 32 / $sectorSize)
$dataStartSector = 1 + $fatSectors + $directorySectors
$dataSectors = $volumeSectors - $fatSectors - $directorySectors - 1
if ($imageFormat -eq 'Mx2') {
    # STD-вход занимает первые четыре байта физической половины ROM.
    # Последний логический сектор доступен не целиком; файлы в него не кладём.
    $dataSectors--
}
$dataClusters = [int]($dataSectors / $sectorsPerCluster)

$boot = [IO.File]::ReadAllBytes($bootPath)
if ($boot.Length -ne $sectorSize) {
    throw "boot.bin must be exactly $sectorSize bytes"
}
$fat = New-Object byte[] ($fatSectors * $sectorSize)
for ($index = 0; $index -lt 4; $index++) {
    $fat[$index] = 0xFF
}
Set-Word $fat 0 0xFFF8
$directory = New-Object IO.MemoryStream
$fileData = New-Object IO.MemoryStream

# Физическая нумерация остаётся обычной. В 2-МБ образе чередуем только
# кластеры данных через FAT; загрузочный сектор, FAT и корень остаются на чипе 0.
$interleaveFiles = $Target -eq 'Games2M'
$chipSectors = [int]($chipSize / $sectorSize)
$usedClusters = New-Object bool[] ($dataClusters + 2)
if ($interleaveFiles) {
    $emptyData = Expand-Bytes ([byte[]]@()) ($dataSectors * $sectorSize)
    $fileData.Write($emptyData, 0, $emptyData.Length)
}
# Запас таблицы не относится к области данных и не должен считаться свободным.
for ($index = $dataClusters + 2; $index -lt $fat.Length / 2; $index++) {
    Set-Word $fat ($index * 2) 0xFFFF
}
if ($imageFormat -eq 'Mx2') {
    # Частично недоступный сектор внутри объявленного тома — bad cluster,
    # а не занятый кластер без владельца (его CHKDSK счёл бы потерянным).
    Set-Word $fat (($dataClusters + 2) * 2) 0xFFF7
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
$sectorPadding = Expand-Bytes ([byte[]]@()) $sectorSize

try {
    foreach ($manifestEntry in $manifest) {
        $sourcePath = Join-Path $assetDirectory $manifestEntry.Source
        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            throw "Manifest input not found: $($manifestEntry.Source)"
        }
        $sourceName = $manifestEntry.ImageName
        $extension = [IO.Path]::GetExtension($sourceName).TrimStart(".")

        $baseName = [IO.Path]::GetFileNameWithoutExtension($sourceName)
        $loadAddress = Get-LoadAddress $manifestEntry.LoadAddress
        if ($extension.ToLowerInvariant() -in @('com', 'exe', 'rks')) {
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

        if ($fileCount -ge $maxFiles) {
            throw "Root directory is full while adding $sourceName ($fileCount of $maxFiles files)"
        }
        $fileCount++

        if ($stagedBoot -and $fixedBootFiles.ContainsKey($sourceName)) {
            $fixedAddress = $fixedBootFiles[$sourceName]
            $limit = switch ($sourceName) { 'BOOT.SYS' { 0x3000 }; 'BIOS.SYS' { 0x5800 }; 'FONTKOI.FNT' { 0x6000 }; 'IO.SYS' { 0x7000 }; 'MEM.SYS' { 0x8000 }; 'DOS.SYS' { 0x18000 } }
            if ($fixedAddress -lt $dataStartSector * $sectorSize -or $fixedAddress + $originalSize -gt $limit) { throw "Fixed bootstrap slot overflow: $sourceName" }
            if ($sourceName -eq 'FONTKOI.FNT' -and ($originalSize -lt 32 -or $originalSize -gt 2080)) { throw 'FONTKOI.FNT: invalid MXFN file size' }
            if ($sourceName -eq 'IO.SYS' -and ($loadAddress -ne 0xF000 -or $loadAddress+$originalSize -gt 0xF740)) { throw 'Invalid IO load range' }
            if ($sourceName -eq 'BIOS.SYS' -and ($loadAddress -ne 0xC000 -or $loadAddress+$originalSize -gt 0xF000)) { throw 'Invalid BIOS load range' }
            if ($sourceName -eq 'DOS.SYS' -and ($loadAddress -ne 0x0100 -or $loadAddress+$originalSize -gt 0x6000)) { throw 'Invalid DOS load range' }
            $nextCluster = [int]($fixedAddress / $sectorSize) - $dataStartSector + 2
        }

        $firstCluster = 0
        $previousCluster = 0
        for ($offset = 0; $offset -lt $storedBytes.Length; $offset += $sectorSize) {
            $searched = 0
            while ($true) {
                if ($nextCluster -ge $dataClusters + 2) { $nextCluster = 2 }
                if (-not $usedClusters[$nextCluster]) { break }
                $nextCluster++
                $searched++
                if ($searched -ge $dataClusters) {
                    throw "Image data area is full while adding $sourceName"
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
                Set-Word $fat ($previousCluster * 2) $cluster
            }
            Set-Word $fat ($cluster * 2) 0xFFFF
            $previousCluster = $cluster

            $count = [Math]::Min($sectorSize, $storedBytes.Length - $offset)
            $fileData.Position = ($cluster - 2) * $sectorSize
            $fileData.Write($storedBytes, $offset, $count)
            if ($count -lt $sectorSize) {
                $fileData.Write($sectorPadding, 0, $sectorSize - $count)
            }
        }

        if ($Target -in @('SystemEmuWritable', 'SystemHardware') -and $sourceName -eq 'BIOS.SYS' -and
            ($firstCluster -eq 0 -or ($dataStartSector + $nextCluster - 2) * $sectorSize -gt 32768)) {
            throw "Boot file must fit in the first 32-KB page: $sourceName"
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

        if ($makeBootDisk -and "$baseName.$extension".Equals("BIOS.SYS", [StringComparison]::OrdinalIgnoreCase)) {
            $dosCluster = $firstCluster
            $dosAddress = $loadAddress
            $dosSectors = [int][Math]::Ceiling($originalSize / 256.0)
        }
    }

    while ($directory.Length -lt $directorySectors * $sectorSize) {
        $directory.WriteByte(0)
    }

    [Array]::Copy([Text.Encoding]::ASCII.GetBytes("MXOS256 "), 0, $boot, 3, 8)
    Set-Word $boot 0x0B $sectorSize
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

    $start = Join-Bytes $boot $fat $directory.ToArray()
    $data = $fileData.ToArray()
    $nestedPaths = @($manifest | ForEach-Object { $_.ImageName })
    if (@($nestedPaths | Where-Object { $_ -match '[/\\]' }).Count -gt 0) {
        $stagingPath = Join-Path $repositoryRoot "bin/nested-$Target.bin"
        [IO.File]::WriteAllBytes($stagingPath, (Expand-Bytes (Join-Bytes $start $data) $volumeSize))
        & python (Join-Path $PSScriptRoot 'nest-fat-image.py') $stagingPath @nestedPaths
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
            [IO.File]::WriteAllBytes($outputPath, (Expand-Bytes (Join-Bytes $start $data) $volumeSize))
            Write-Host ("[image] {0}" -f $outputPath)
        }
        "Split" {
            $image = Join-Bytes $start $data
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
