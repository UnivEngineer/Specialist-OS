[CmdletBinding()]
param([Parameter(Mandatory = $true)][string] $SourceDirectory)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$destination = Join-Path $root 'hardware'
$designs = @(
    @('specialist-m', 'Specialist-M'),
    @('crossboard', 'Final-crossboard-rev1'),
    @('mapper', 'Final-mapper-rev0'),
    @('ram-slots', 'Final-RAM-slots-rev1'),
    @('ram-module', 'Final-RAM-module-rev0'),
    @('flash-disk', 'Final-flash-disk-rev0')
)
$files = @()
foreach ($design in $designs) {
    foreach ($extension in @('dch', 'dip', 'dchxml', 'pdf')) {
        $sourceName = $design[1] + '.' + $extension
        $relativeSource = if ($extension -in @('dchxml', 'pdf')) { 'AI/' + $sourceName } else { $sourceName }
        $files += [pscustomobject]@{
            Source = $relativeSource
            Destination = 'boards/' + $design[0] + '/' + $design[0] + '.' + $extension
        }
    }
}
foreach ($panel in @(1, 2)) {
    $files += [pscustomobject]@{ Source = "Final-panel-$panel.dip"; Destination = "panels/panel-$panel.dip" }
}
$files += [pscustomobject]@{ Source = 'AI/README.md'; Destination = 'import-notes.md' }

# Сначала проверяем весь набор, чтобы не получить частичный импорт.
foreach ($file in $files) {
    $source = Join-Path $SourceDirectory $file.Source
    if (-not (Test-Path -LiteralPath $source -PathType Leaf)) { throw "Missing source: $source" }
    $target = Join-Path $destination $file.Destination
    if (Test-Path -LiteralPath $target) { throw "Destination already exists: $target" }
}
$manifest = foreach ($file in $files) {
    $source = Join-Path $SourceDirectory $file.Source
    $target = Join-Path $destination $file.Destination
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath $source -Destination $target
    $sourceHash = (Get-FileHash -LiteralPath $source -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($sourceHash -ne (Get-FileHash -LiteralPath $target -Algorithm SHA256).Hash.ToLowerInvariant()) {
        throw "Copy checksum mismatch: $target"
    }
    [ordered]@{ source = $file.Source; path = $file.Destination; bytes = (Get-Item -LiteralPath $source).Length; sha256 = $sourceHash }
}
$record = [ordered]@{ schema = 1; importedOn = (Get-Date).ToString('yyyy-MM-dd'); files = @($manifest) }
[IO.File]::WriteAllText((Join-Path $destination 'import-manifest.json'), ($record | ConvertTo-Json -Depth 5) + "`n", (New-Object Text.UTF8Encoding($false)))
Write-Host "Imported and verified $($files.Count) files. Source directory was not modified."
