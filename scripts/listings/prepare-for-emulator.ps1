[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string] $RepositoryRoot
)

$ErrorActionPreference = "Stop"
Set-StrictMode -Version 2.0

if ([string]::IsNullOrWhiteSpace($RepositoryRoot)) {
    $RepositoryRoot = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
}
$RepositoryRoot = [System.IO.Path]::GetFullPath($RepositoryRoot)
$binDirectory = Join-Path $RepositoryRoot "bin"

if (-not (Test-Path -LiteralPath $binDirectory -PathType Container)) {
    throw "Build output directory not found: $binDirectory"
}

$fixedListings = Get-ChildItem -LiteralPath $binDirectory -File |
    Where-Object { $_.Name.EndsWith(".fix.lst", [System.StringComparison]::OrdinalIgnoreCase) }
foreach ($fixedListing in $fixedListings) {
    Remove-Item -LiteralPath $fixedListing.FullName -Force
}

$listings = Get-ChildItem -LiteralPath $binDirectory -File -Filter "*.lst"
foreach ($listing in $listings) {
    $outputPath = $listing.FullName + ".fix.lst"
    Write-Host ("[listing] {0}" -f $listing.FullName)

    $inputBytes = [System.IO.File]::ReadAllBytes($listing.FullName)
    $output = New-Object System.IO.MemoryStream
    try {
        $lineStart = 0
        for ($position = 0; $position -lt $inputBytes.Length; $position++) {
            if ($inputBytes[$position] -ne 0x0A) {
                continue
            }

            $lineLength = $position - $lineStart + 1
            if ($lineLength -gt 0 -and $inputBytes[$lineStart] -ne 0x23) {
                $output.Write($inputBytes, $lineStart, $lineLength)
            }
            $lineStart = $position + 1
        }

        if ($lineStart -lt $inputBytes.Length -and $inputBytes[$lineStart] -ne 0x23) {
            $output.Write($inputBytes, $lineStart, $inputBytes.Length - $lineStart)
        }

        [System.IO.File]::WriteAllBytes($outputPath, $output.ToArray())
    }
    finally {
        $output.Dispose()
    }
}
