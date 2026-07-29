[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet("SystemEmu", "SystemHardware", "Games64k", "Games2M")]
    [string] $Target,

    [Parameter(Position = 1)]
    [ValidateSet(128, 256, 512)]
    [int] $HardwareSizeKB = 128
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
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
    param([string] $Path)

    $entries = foreach ($line in [IO.File]::ReadAllLines($Path)) {
        $entry = $line.Trim()
        if ($entry.Length -eq 0 -or $entry.StartsWith("#")) {
            continue
        }
        $entry
    }
    return @($entries)
}

function Get-SystemManifest {
    param([string] $Path, [string] $AutoexecVariant)

    $entries = @(Import-Csv -LiteralPath $Path)
    foreach ($entry in $entries) {
        if ($entry.Source -eq "{AUTOEXEC}") {
            $entry.Source = "assets/system-rom/autoexec.$AutoexecVariant.bat"
        }
        if ([string]::IsNullOrWhiteSpace($entry.Source) -or
            [string]::IsNullOrWhiteSpace($entry.ImageName) -or
            [string]::IsNullOrWhiteSpace($entry.Format)) {
            throw "Invalid system manifest entry in $Path"
        }
    }
    return $entries
}

switch ($Target) {
    "SystemEmu" {
        $manifestPath = Join-Path $PSScriptRoot "manifests\system.csv"
        $manifest = Get-SystemManifest $manifestPath "emu"
        $manifestHasMetadata = $true
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-system.bin"
        $volumeSize = 64KB
        $chipSize = 64KB
        $maxFiles = 64
        $volumeLabel = "SYSTEM ROM "
        $imageFormat = "Mx2"
        $outputPath = Join-Path $imageDirectory "SystemRom_emulator.bin"
        $makeBootDisk = $true
    }
    "SystemHardware" {
        $manifestPath = Join-Path $PSScriptRoot "manifests\system.csv"
        $manifest = Get-SystemManifest $manifestPath "hardware"
        $manifestHasMetadata = $true
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-system.bin"
        $volumeSize = $HardwareSizeKB * 1KB
        $chipSize = 64KB
        $maxFiles = 64
        $volumeLabel = "SYSTEM ROM "
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
        $inputDirectory = Join-Path $repositoryRoot "assets\games"
        $manifestPath = Join-Path $PSScriptRoot "manifests\FlashDisk-emulator.txt"
        $manifest = Get-Manifest $manifestPath
        $manifestHasMetadata = $false
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-flash.bin"
        $volumeSize = 64KB
        $chipSize = 64KB
        $maxFiles = 64
        $volumeLabel = "ROM DISK 64"
        $imageFormat = "Split"
        $outputPath = Join-Path $imageDirectory "FlashDisk_emulator.bin"
        $partNamePattern = "FlashDisk_emulator.bin"
        $makeBootDisk = $false
    }
    "Games2M" {
        $inputDirectory = Join-Path $repositoryRoot "assets\games"
        $manifestPath = Join-Path $PSScriptRoot "manifests\flash-games.txt"
        $manifest = Get-Manifest $manifestPath
        $manifestHasMetadata = $false
        $bootPath = Join-Path $repositoryRoot "assets\system-rom\boot-flash.bin"
        $volumeSize = 2MB
        $chipSize = 512KB
        $maxFiles = 192
        $volumeLabel = "FLASH DISK "
        $imageFormat = "Split"
        $outputPath = Join-Path $imageDirectory "FlashDisk_512K.bin"
        $partNamePattern = "FlashDisk_chip{0}_512K.bin"
        $makeBootDisk = $false
    }
}

New-Item -ItemType Directory -Path $imageDirectory -Force | Out-Null

$filesByName = @{}
if (-not $manifestHasMetadata) {
    foreach ($file in Get-ChildItem -LiteralPath $inputDirectory -File) {
        $key = $file.Name.ToUpperInvariant()
        if ($filesByName.ContainsKey($key)) {
            throw "Input names differ only by case: $($file.Name)"
        }
        $filesByName[$key] = $file
    }
}

$volumeSectors = [int]($volumeSize / $sectorSize)
$fatSectors = [int]($volumeSectors * 2 / $sectorSize)
$directorySectors = [int]($maxFiles * 32 / $sectorSize)
$dataStartSector = 1 + $fatSectors + $directorySectors
$dataSectors = $volumeSectors - $fatSectors - $directorySectors - 1
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

$nextCluster = 2
$fileCount = 0
$dosCluster = 0
$dosAddress = 0
$dosSectors = 0
$imageNames = @{}

try {
    foreach ($manifestEntry in $manifest) {
        if ($manifestHasMetadata) {
            $sourcePath = Join-Path $repositoryRoot $manifestEntry.Source
            if (-not (Test-Path -LiteralPath $sourcePath -PathType Leaf)) {
                throw "Manifest input not found: $($manifestEntry.Source)"
            }
            $sourceFile = Get-Item -LiteralPath $sourcePath
            $sourceName = $manifestEntry.ImageName
            $entryFormat = $manifestEntry.Format
        }
        else {
            $key = $manifestEntry.ToUpperInvariant()
            if (-not $filesByName.ContainsKey($key)) {
                throw "Manifest input not found: $manifestEntry in $inputDirectory"
            }
            $sourceFile = $filesByName[$key]
            $sourceName = $sourceFile.Name
            $entryFormat = if ([IO.Path]::GetExtension($sourceName).Equals(
                ".RKS", [StringComparison]::OrdinalIgnoreCase)) { "rks" } else { "raw" }
        }
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
            $baseName = [IO.Path]::GetFileNameWithoutExtension($sourceName)
        }
        else {
            $baseName = [IO.Path]::GetFileNameWithoutExtension($sourceName)
            if ($manifestHasMetadata) {
                $loadAddress = Get-LoadAddress $manifestEntry.LoadAddress
            }
            else {
                $loadAddress = 0
            }
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
            if ($nextCluster -ge $dataClusters) {
                throw "Image data area is full while adding $sourceName"
            }

            $cluster = $nextCluster
            $nextCluster++
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
            $fileData.Write($block, 0, $block.Length)
        }

        $entry = New-Object byte[] 32
        $nameBytes = [Text.Encoding]::ASCII.GetBytes(($baseName + "        ").Substring(0, 8))
        $extensionBytes = [Text.Encoding]::ASCII.GetBytes(($extension + "   ").Substring(0, 3))
        [Array]::Copy($nameBytes, 0, $entry, 0, 8)
        [Array]::Copy($extensionBytes, 0, $entry, 8, 3)
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
            $upperLength = $dataPageOffset - 4
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
