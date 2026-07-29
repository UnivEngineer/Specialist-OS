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
$assemblerPath = Join-Path $repositoryRoot "tasm\sjasmplus.exe"

New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null

$dependencies = @(Get-SourceDependencies -SourcePath $sourcePath -BuildDirectory $buildDirectory)
$buildInputs = @($dependencies + $assemblerPath + $PSCommandPath)
$newestInput = $buildInputs |
    Get-Item -ErrorAction Stop |
    Sort-Object LastWriteTimeUtc -Descending |
    Select-Object -First 1

$needsCompile = -not (Test-Path -LiteralPath $outputPath -PathType Leaf) -or
    -not (Test-Path -LiteralPath $listingPath -PathType Leaf)

if (-not $needsCompile) {
    $output = Get-Item -LiteralPath $outputPath
    $listing = Get-Item -LiteralPath $listingPath
    $needsCompile = $output.LastWriteTimeUtc -lt $newestInput.LastWriteTimeUtc -or
        $listing.LastWriteTimeUtc -lt $newestInput.LastWriteTimeUtc
}

if ($needsCompile) {
    Write-Host ("[build] {0} (newest input: {1})" -f $sourcePath, $newestInput.FullName)
    & $assemblerPath `
        "--i8080" `
        "-Wno-rdlow" `
        ("--lst={0}" -f $listingPath) `
        ("--raw={0}" -f $outputPath) `
        $sourcePath

    if ($LASTEXITCODE -ne 0) {
        exit $LASTEXITCODE
    }
}
else {
    Write-Host ("[up-to-date] {0}" -f $outputPath)
}
