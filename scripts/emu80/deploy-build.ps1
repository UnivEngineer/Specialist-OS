[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)][string] $SourceDirectory,
    [Parameter(Mandatory = $true)][string] $QtDirectory,
    [Parameter(Mandatory = $true)][string] $BuildDirectory,
    [Parameter(Mandatory = $true)][string] $DestinationDirectory,
    [switch] $Mcp,
    [switch] $Update
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$destination = [IO.Path]::GetFullPath($DestinationDirectory)
$exists = Test-Path -LiteralPath $destination
if ($exists) {
    if (-not $Update) { throw "Choose a new runtime directory: $destination" }
    $owned = Get-Content -LiteralPath (Join-Path $destination 'specialist-build.json') -Raw | ConvertFrom-Json
    if ($owned.model -ne 'SpecialistFlashDisk') { throw 'Refusing to update an unrelated runtime.' }
    $running = @(Get-Process -Name Emu80qt,Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object { $_.Path -and (Split-Path -Parent $_.Path) -eq $destination })
    if ($running.Count) { throw 'Close the target runtime before updating it.' }
}
$exeName = if ($Mcp) { 'Emu80qt-mcp.exe' } else { 'Emu80qt.exe' }
$builtExe = Join-Path $BuildDirectory ('release/' + $exeName)
if (-not (Test-Path -LiteralPath $builtExe)) { throw "Missing executable: $builtExe" }
$buildInfo = Get-Content -LiteralPath (Join-Path $BuildDirectory 'build-specialist.json') -Raw | ConvertFrom-Json
if ((Get-FileHash -LiteralPath $builtExe).Hash.ToLowerInvariant() -ne $buildInfo.executableSha256) { throw 'Executable does not match the recorded build.' }
$lock = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'upstream.json') -Raw | ConvertFrom-Json
$sources = foreach ($name in @('At29Flash.cpp','At29Flash.h','SpecialistFlashDisk.cpp','SpecialistFlashDisk.h','SpecialistMx2VideoRam.cpp','SpecialistMx2VideoRam.h','SpecialistSystemFlash.cpp','SpecialistSystemFlash.h')) {
    $hash = (Get-FileHash -LiteralPath (Join-Path $root ('source-emulators/emu80/' + $name))).Hash.ToLowerInvariant()
    $builtHash = @($buildInfo.sources | Where-Object { $_.path -eq $name })
    if ($builtHash.Count -ne 1 -or $builtHash[0].sha256 -ne $hash) { throw "Source changed since build: $name" }
    [ordered]@{ path = $name; sha256 = $hash }
}
New-Item -ItemType Directory -Path $destination -Force | Out-Null
New-Item -ItemType Directory -Path (Join-Path $destination '_settings') -Force | Out-Null
if (-not $exists) { Copy-Item -Path (Join-Path $SourceDirectory 'dist/*') -Destination $destination -Recurse }
Copy-Item -LiteralPath $builtExe -Destination (Join-Path $destination $exeName)
Copy-Item -LiteralPath (Join-Path $SourceDirectory 'COPYING.txt') -Destination $destination
$qt = [IO.Path]::GetFullPath($QtDirectory)
foreach ($dll in @('Qt5Core.dll','Qt5Gui.dll','Qt5Widgets.dll','Qt5Multimedia.dll','Qt5Network.dll')) {
    Copy-Item -LiteralPath (Join-Path $qt ('bin/' + $dll)) -Destination $destination
}
foreach ($plugin in @('platforms/qwindows.dll','audio/qtaudio_windows.dll')) {
    $target = Join-Path $destination ('_qtplugins/' + $plugin)
    New-Item -ItemType Directory -Path (Split-Path -Parent $target) -Force | Out-Null
    Copy-Item -LiteralPath (Join-Path $qt ('plugins/' + $plugin)) -Destination $target
}
[IO.File]::WriteAllText((Join-Path $destination 'qt.conf'), "[Paths]`nPlugins=_qtplugins`n")
$record = [ordered]@{ model = 'SpecialistFlashDisk'; commit = $lock.referenceCommit; mcp = [bool]$Mcp; sources = @($sources) }
[IO.File]::WriteAllText((Join-Path $destination 'specialist-build.json'), ($record | ConvertTo-Json -Depth 5) + "`n", (New-Object Text.UTF8Encoding($false)))
Write-Host "Deployed custom Emu80: $destination"
