[CmdletBinding()]
param(
    [string] $ReferenceSource = $env:SPECIALIST_EMU80_SOURCE_DIR,
    [string] $QtDirectory = $env:SPECIALIST_EMU80_QT_DIR,
    [switch] $MonitorSpeaker
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
if (-not $ReferenceSource) { $ReferenceSource = [Environment]::GetEnvironmentVariable('SPECIALIST_EMU80_SOURCE_DIR','User') }
if (-not $QtDirectory) { $QtDirectory = [Environment]::GetEnvironmentVariable('SPECIALIST_EMU80_QT_DIR','User') }
$root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
$emuScripts = Join-Path $root 'scripts/emu80'
$model = if ($MonitorSpeaker) { 'monitor-speaker' } else { 'lafan-fdc' }
$local = Join-Path $root ('.local/' + $model)
$checkout = Join-Path $local 'source'
$build = Join-Path $local 'build'
$template = Join-Path $local 'template'
$patchName = if ($MonitorSpeaker) { 'monitor-speaker.patch' } else { 'lafan-fdc-search.patch' }
$patch = Join-Path $root ('source-emulators/emu80/' + $patchName)
$modelSources = if ($MonitorSpeaker) { 'SoundMixer.cpp', 'SoundMixer.h' } else { 'Fdc1793.cpp', 'Fdc1793.h' }
$pinned = (Get-Content -LiteralPath (Join-Path $emuScripts 'upstream.json') -Raw | ConvertFrom-Json).referenceCommit

if (-not (Test-Path -LiteralPath (Join-Path $checkout '.git'))) {
    if (-not $ReferenceSource) { throw 'Укажите -ReferenceSource или SPECIALIST_EMU80_SOURCE_DIR.' }
    New-Item -Path $local -ItemType Directory -Force | Out-Null
    & git clone --quiet --shared --no-hardlinks --no-checkout -- $ReferenceSource $checkout
    if ($LASTEXITCODE -ne 0) { throw 'Не удалось создать отдельную копию Emu80.' }
}
if (-not (Test-Path -LiteralPath (Join-Path $checkout 'src/Emu80qt.pro'))) {
    & git -C $checkout checkout --quiet --detach $pinned
    if ($LASTEXITCODE -ne 0) { throw 'Не удалось получить фиксированную ревизию Emu80.' }
}
$actual = & git -C $checkout rev-parse HEAD
if ($LASTEXITCODE -ne 0 -or $actual -ne $pinned) { throw 'Ревизия отдельной копии Emu80 не совпадает с реестром.' }
$priorErrorPreference = $ErrorActionPreference
try {
    $ErrorActionPreference = 'Continue'
    & git -C $checkout apply --reverse --check $patch 2>$null
    $alreadyApplied = $LASTEXITCODE -eq 0
} finally {
    $ErrorActionPreference = $priorErrorPreference
}
if (-not $alreadyApplied) {
    & git -C $checkout apply $patch
    if ($LASTEXITCODE -ne 0) { throw "Не удалось применить $patchName к отдельной копии." }
}

& (Join-Path $emuScripts 'build.ps1') -SourceDirectory $checkout -QtDirectory $QtDirectory -BuildDirectory $build -Mcp
& (Join-Path $emuScripts 'deploy-build.ps1') -SourceDirectory $checkout -QtDirectory $QtDirectory -BuildDirectory $build -DestinationDirectory $template -Mcp -Update
$record = [ordered]@{
    upstream = $pinned
    property = if ($MonitorSpeaker) { 'beepSoundSource.mcpCaptureSpeaker = yes' } else { 'fdc.multiReadSectorSearch = yes' }
    patchSha256 = (Get-FileHash -LiteralPath $patch).Hash.ToLowerInvariant()
    executableSha256 = (Get-FileHash -LiteralPath (Join-Path $template 'Emu80qt-mcp.exe')).Hash.ToLowerInvariant()
    sourceSha256 = [ordered]@{}
}
foreach ($name in $modelSources) {
    $record.sourceSha256[$name] = (Get-FileHash -LiteralPath (Join-Path $checkout ('src/' + $name))).Hash.ToLowerInvariant()
}
$record | ConvertTo-Json -Depth 5 | Set-Content -LiteralPath (Join-Path $template ($model + '-build.json')) -Encoding utf8
