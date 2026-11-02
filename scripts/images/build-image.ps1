[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet("SystemEmu", "SystemEmuFlash", "SystemEmuWritable", "SystemRamfos", "SystemHardware", "Games64k", "Games2M")]
    [string] $Target,

    [Parameter(Position = 1)]
    [ValidateSet(128, 256, 512)]
    [int] $HardwareSizeKB = 128
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$assetDirectory = Join-Path $repositoryRoot "assets"
$imageDirectory = Join-Path $repositoryRoot "images"
$sectorSize = 256
$sectorsPerCluster = 1

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
    return $result
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
    if ($Fill -ne 0) {
        for ($index = 0; $index -lt $result.Length; $index++) {
            $result[$index] = $Fill
        }
    }
    [Array]::Copy($Data, $result, $Data.Length)
    return $result
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

function Set-Word {
    param([byte[]] $Data, [int] $Offset, [int] $Value)

    $Data[$Offset] = $Value -band 0xFF
    $Data[$Offset + 1] = ($Value -shr 8) -band 0xFF
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
        $manifestPath = Join-Path $PSScriptRoot "manifests\SystemRom.txt"
        $manifest = @((Get-Manifest $manifestPath "emu").Entries |
            Where-Object { $_.ImageName -notin @("Test-Kb.COM", "Test-Fif.COM", "Test-MX.COM") })
        $ramfosManifestPath = Join-Path $PSScriptRoot "manifests\Ramfos.txt"
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
        $manifestPath = Join-Path $PSScriptRoot "manifests\SystemRom.txt"
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
        $manifestPath = Join-Path $PSScriptRoot "manifests\SystemRom.txt"
        $manifestData = Get-Manifest $manifestPath "emu-writable"
        $allEntries = @($manifestData.Entries)
        # Стабильный порядок загрузочных файлов; лишь DOS обязан находиться
        # в первой 32-КБ странице для начального загрузчика без стека.
        $early = @('DOS.SYS','FONT.FNT','FORMAT.BAT','FORMAT.COM','AUTOEXEC.BAT','FLASHSYS.COM','FLASH.COM','NC.COM','ROM.COM')
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
        $manifestPath = Join-Path $PSScriptRoot "manifests\SystemRom.txt"
        $manifestData = Get-Manifest $manifestPath "hardware"
        $allEntries = @($manifestData.Entries)
        # Стабильный порядок загрузочных файлов; первая страница обязательна лишь для DOS.
        $early = @('DOS.SYS','FONT.FNT','FORMAT.BAT','FORMAT.COM','AUTOEXEC.BAT','FLASHSYS.COM','FLASH.COM','NC.COM','ROM.COM')
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
        $manifestPath = Join-Path $PSScriptRoot "manifests\FlashDisk-emulator.txt"
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
        $manifestPath = Join-Path $PSScriptRoot "manifests\FlashDisk.txt"
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
$fileCount = 0
$dosCluster = 0
$dosAddress = 0
$dosSectors = 0
$imageNames = @{}

try {
    foreach ($manifestEntry in $manifest) {
        $sourcePath = Join-Path $assetDirectory $manifestEntry.Source
        if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
            throw "Manifest input not found: $($manifestEntry.Source)"
        }
        $sourceFile = Get-Item -LiteralPath $sourcePath
        $sourceName = $manifestEntry.ImageName
        $entryFormat = $manifestEntry.Format
        $sourceBytes = [IO.File]::ReadAllBytes($sourceFile.FullName)
        $storedBytes = $sourceBytes
        $originalSize = $sourceBytes.Length
        $extension = [IO.Path]::GetExtension($sourceName).TrimStart(".")

        if ($entryFormat.Equals("rks", [StringComparison]::OrdinalIgnoreCase)) {
            if ($sourceBytes.Length -lt 4) {
                throw "Invalid RKS header: $sourceName"
            }
            $loadAddress = [int]$sourceBytes[0] + ([int]$sourceBytes[1] -shl 8)
            $endAddress = [int]$sourceBytes[2] + ([int]$sourceBytes[3] -shl 8)
            $payloadLength = $endAddress - $loadAddress + 1
            if ($payloadLength -lt 0) {
                throw "Invalid RKS payload length: $sourceName"
            }
            $payloadLength = [Math]::Min($payloadLength, $sourceBytes.Length - 4)
            $storedBytes = New-Object byte[] $payloadLength
            [Array]::Copy($sourceBytes, 4, $storedBytes, 0, $payloadLength)
            # EXE из RKS — сырой образ нынешнего MXOS, без заголовка и суммы RKS.
            if ($extension.Equals("exe", [StringComparison]::OrdinalIgnoreCase)) {
                $originalSize = $payloadLength
            }
            $baseName = [IO.Path]::GetFileNameWithoutExtension($sourceName)
        }
        else {
            $baseName = [IO.Path]::GetFileNameWithoutExtension($sourceName)
            $loadAddress = Get-LoadAddress $manifestEntry.LoadAddress
        }

        if ($baseName.Length -gt 8 -or $extension.Length -gt 3) {
            throw "Image name '$sourceName' does not fit FAT 8.3. Specify a separate short image name."
        }
        $imageNameKey = "$baseName.$extension".ToUpperInvariant()
        if ($imageNames.ContainsKey($imageNameKey)) {
            throw "FAT 8.3 name conflict '$baseName.$extension': '$($imageNames[$imageNameKey])' and '$sourceName'."
        }
        $imageNames[$imageNameKey] = $sourceName

        if ($fileCount -ge $maxFiles) {
            throw "Root directory is full while adding $sourceName ($fileCount of $maxFiles files)"
        }
        $fileCount++

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
            Set-Word $fat ($cluster * 2) $cluster
            $previousCluster = $cluster

            $block = New-Object byte[] $sectorSize
            for ($index = 0; $index -lt $block.Length; $index++) {
                $block[$index] = 0xFF
            }
            $count = [Math]::Min($sectorSize, $storedBytes.Length - $offset)
            [Array]::Copy($storedBytes, $offset, $block, 0, $count)
            $fileData.Position = ($cluster - 2) * $sectorSize
            $fileData.Write($block, 0, $block.Length)
        }

        if ($Target -in @('SystemEmuWritable', 'SystemHardware') -and $sourceName -eq 'DOS.SYS' -and
            ($firstCluster -eq 0 -or ($dataStartSector + $nextCluster - 2) * $sectorSize -gt 32768)) {
            throw "Boot file must fit in the first 32-KB page: $sourceName"
        }

        $entry = New-Object byte[] 32
        $nameBytes = [Text.Encoding]::ASCII.GetBytes(($baseName + "        ").Substring(0, 8))
        $extensionBytes = [Text.Encoding]::ASCII.GetBytes(($extension + "   ").Substring(0, 3))
        [Array]::Copy($nameBytes, 0, $entry, 0, 8)
        [Array]::Copy($extensionBytes, 0, $entry, 8, 3)
        $entry[11] = $manifestEntry.Attributes
        Set-Word $entry 18 $loadAddress
        Set-Word $entry 26 $firstCluster
        Set-Word $entry 28 ($originalSize - 1)
        $directory.Write($entry, 0, $entry.Length)

        if ($makeBootDisk -and "$baseName.$extension".Equals("DOS.SYS", [StringComparison]::OrdinalIgnoreCase)) {
            $dosCluster = $firstCluster
            $dosAddress = $loadAddress
            $dosSectors = [int](($originalSize + 255) / 256)
        }
    }

    while ($directory.Length -lt $directorySectors * $sectorSize) {
        $directory.WriteByte(0xFF)
    }

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
            throw "DOS.SYS is missing from the system manifest"
        }
        $dosRomAddress = (($dosCluster - 2) * $sectorsPerCluster + $dataStartSector) * $sectorSize
        if ($imageFormat -eq "Raw" -and
            ($dosCluster -ne 2 -or $dosRomAddress + $dosSectors * $sectorSize -gt 32768)) {
            throw "DOS.SYS must be first and fit in the first 32-KB hardware ROM page"
        }
        Set-Word $boot 0x3F $dosRomAddress
        Set-Word $boot 0x42 $dosAddress
        $boot[0x4A] = (($dosAddress -shr 8) + $dosSectors) -band 0xFF
    }

    $start = Join-Bytes $boot $fat $directory.ToArray()
    $data = $fileData.ToArray()

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
