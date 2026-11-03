[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [string] $SourceBase,

    [Parameter(Mandatory = $true, Position = 1)]
    [string] $Extension,

    [Parameter(Mandatory = $true, Position = 2)]
    [string] $Address,

    [Parameter(Position = 3)]
    [string] $OutputBase = $SourceBase
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

function Get-FullPath {
    param(
        [Parameter(Mandatory = $true)]
        [string] $Path,

        [Parameter(Mandatory = $true)]
        [string] $BaseDirectory
    )

    if ([System.IO.Path]::IsPathRooted($Path)) {
        return [System.IO.Path]::GetFullPath($Path)
    }

    return [System.IO.Path]::GetFullPath((Join-Path $BaseDirectory $Path))
}

function Get-SourceDependencies {
    param(
        [Parameter(Mandatory = $true)]
        [string] $SourcePath,

        [Parameter(Mandatory = $true)]
        [string] $BuildDirectory
    )

    $visited = @{}
    $dependencies = New-Object System.Collections.Generic.List[string]
    $directivePattern = '^\s*(?:include|incbin)\s+(?:"([^"]+)"|''([^'']+)''|([^\s;]+))'

    function Visit-Source {
        param([string] $Path)

        $fullPath = [System.IO.Path]::GetFullPath($Path)
        $key = $fullPath.ToUpperInvariant()
        if ($visited.ContainsKey($key)) {
            return
        }

        if (-not (Test-Path -LiteralPath $fullPath -PathType Leaf)) {
            throw "Build dependency not found: $fullPath"
        }

        $visited[$key] = $true
        $dependencies.Add($fullPath)

        foreach ($line in [System.IO.File]::ReadLines($fullPath)) {
            $match = [System.Text.RegularExpressions.Regex]::Match(
                $line,
                $directivePattern,
                [System.Text.RegularExpressions.RegexOptions]::IgnoreCase
            )
            if (-not $match.Success) {
                continue
            }

            $includePath = $match.Groups[1].Value
            if ([string]::IsNullOrEmpty($includePath)) {
                $includePath = $match.Groups[2].Value
            }
            if ([string]::IsNullOrEmpty($includePath)) {
                $includePath = $match.Groups[3].Value
            }

            $resolvedPath = Get-FullPath -Path $includePath -BaseDirectory (Split-Path -Parent $fullPath)
            if (-not (Test-Path -LiteralPath $resolvedPath -PathType Leaf)) {
                $resolvedPath = Get-FullPath -Path $includePath -BaseDirectory $BuildDirectory
            }

            $dependencies.Add($resolvedPath)
            if ($line -match '^\s*include\b') {
                Visit-Source -Path $resolvedPath
            }
        }
    }

    Visit-Source -Path $SourcePath
    return $dependencies | Sort-Object -Unique
}

$buildDirectory = (Get-Location).ProviderPath
$sourcePath = Get-FullPath -Path ($SourceBase + ".asm") -BaseDirectory $buildDirectory
$repositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$outputDirectory = Join-Path $repositoryRoot "bin"
$outputPath = Join-Path $outputDirectory ($OutputBase + "." + $Extension)
$listingPath = Join-Path $outputDirectory ($OutputBase + ".lst")
$symbolPath = Join-Path $outputDirectory ($OutputBase + '.sym')
$configurationPath = $outputPath + '.build-config'
$rawOutputPath = $outputPath
if ($Extension -eq 'cpu') { $rawOutputPath = Join-Path $outputDirectory ($OutputBase + '.i80') }
$configuration = 'Configured'
$assemblerPath = Join-Path $repositoryRoot "tasm\sjasmplus.exe"

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

# DOS требует предварительного измерения блоков и размещения между входами ABI.
if ($SourceBase -eq 'DOS' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\dos')) {
    $packingArguments = @('--output', $outputPath, '--configuration', $configuration)
    & python (Join-Path $PSScriptRoot 'pack-dos.py') @packingArguments
    exit $LASTEXITCODE
}

$dependencies = @(Get-SourceDependencies -SourcePath $sourcePath -BuildDirectory $buildDirectory)
$buildInputs = @($dependencies + $assemblerPath + $PSCommandPath)
$newestInput = $buildInputs |
    Get-Item -ErrorAction Stop |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1

$needsCompile = -not (Test-Path -LiteralPath $rawOutputPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $outputPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $listingPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $symbolPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $configurationPath -PathType Leaf)

if (-not $needsCompile) {
    $needsCompile = [IO.File]::ReadAllText($configurationPath) -ne $configuration
}

if (-not $needsCompile) {
    $output = Get-Item -LiteralPath $outputPath
    $listing = Get-Item -LiteralPath $listingPath
    $needsCompile = $output.LastWriteTimeUtc -lt $newestInput.LastWriteTimeUtc -or
        $listing.LastWriteTimeUtc -lt $newestInput.LastWriteTimeUtc
}

if ($needsCompile) {
    if (Test-Path -LiteralPath $configurationPath) { Remove-Item -LiteralPath $configurationPath }
    Write-Host ("[build] {0} (newest input: {1})" -f $sourcePath, $newestInput.FullName)
    & $assemblerPath `
        "--i8080" `
        "-Wno-rdlow" `
        ("--lst={0}" -f $listingPath) `
        ("--sym={0}" -f $symbolPath) `
        ("--raw={0}" -f $rawOutputPath) `
        $sourcePath

    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
    [IO.File]::WriteAllText($configurationPath, $configuration)
}
else {
    Write-Host ("[up-to-date] {0}" -f $outputPath)
}

# CPU — текстовые адреса, машинный образ находится в соседнем I80.
if ($Extension -eq 'cpu') {
    $numericAddress = if ($Address -match '^0x') {
        [Convert]::ToInt32($Address.Substring(2), 16)
    } elseif ($Address -match '^\d+$') {
        [int]$Address
    } else {
        # Адреса карты памяти берутся из символов именно собранного компонента.
        $symbolText = [IO.File]::ReadAllText((Join-Path $outputDirectory ($OutputBase + '.sym')))
        $symbolMatch = [regex]::Match($symbolText, '(?m)^' + [regex]::Escape($Address) + ': EQU 0x([0-9A-Fa-f]+)')
        if (-not $symbolMatch.Success) { throw "Unknown load-address symbol: $Address" }
        [Convert]::ToInt32($symbolMatch.Groups[1].Value, 16)
    }
    $cpuText = ("{0:X4}`r`n{0:X4}`r`n" -f $numericAddress)
    [IO.File]::WriteAllText($outputPath, $cpuText, [Text.Encoding]::ASCII)
}

# Monitor-2 должен точно воспроизводить сохранённое историческое ядро.
# Проверяем и после пропуска компиляции: подменённый результат недопустим.
if ($SourceBase -eq 'Monitor2' -and $buildDirectory -eq (Join-Path $repositoryRoot 'source\monitor2')) {
    $referencePath = Join-Path $buildDirectory 'originals\Monitor2.mon'
    $referenceHash = 'E8EABDEE87CBBB679F0D6E7C30BCDB2E48A793FC0A7938D8D9411F8BCAA9555A'
    $referenceBytes = [IO.File]::ReadAllBytes($referencePath)
    $hasher = [Security.Cryptography.SHA256]::Create()
    try {
        $actualReferenceHash = [BitConverter]::ToString($hasher.ComputeHash($referenceBytes)).Replace('-', '')
    }
    finally { $hasher.Dispose() }
    if ($actualReferenceHash -ne $referenceHash) {
        throw "Monitor-2 reference has changed: $referencePath"
    }
    $compiledBytes = [IO.File]::ReadAllBytes($outputPath)
    if ($compiledBytes.Length -ne $referenceBytes.Length) {
        throw "Monitor-2 size mismatch: compiled $($compiledBytes.Length), original $($referenceBytes.Length)"
    }
    for ($index = 0; $index -lt $referenceBytes.Length; $index++) {
        if ($compiledBytes[$index] -ne $referenceBytes[$index]) {
            throw ("Monitor-2 mismatch at offset {0:X4}h (address {1:X4}h): compiled {2:X2}, original {3:X2}" -f `
                $index, (0xC000 + $index), $compiledBytes[$index], $referenceBytes[$index])
        }
    }
    Write-Host ("[verified] Monitor-2: {0} bytes identical to original" -f $compiledBytes.Length)
}
exit 0
