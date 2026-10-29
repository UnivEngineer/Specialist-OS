[CmdletBinding()]
param(
    [string] $SourceDirectory = $env:SPECIALIST_EMU80_SOURCE_DIR,
    [string] $QtDirectory = $env:SPECIALIST_EMU80_QT_DIR,
    [string] $BuildDirectory,
    [switch] $Mcp,
    [switch] $Deploy
)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
if (-not $SourceDirectory) { $SourceDirectory = [Environment]::GetEnvironmentVariable('SPECIALIST_EMU80_SOURCE_DIR', 'User') }
if (-not $QtDirectory) { $QtDirectory = [Environment]::GetEnvironmentVariable('SPECIALIST_EMU80_QT_DIR', 'User') }
if (-not $SourceDirectory -or -not $QtDirectory) { throw 'Set SPECIALIST_EMU80_SOURCE_DIR and SPECIALIST_EMU80_QT_DIR or pass their parameters.' }
if (-not $BuildDirectory) {
    $variant = if ($Mcp) { 'flash-mcp' } else { 'flash-gui' }
    $BuildDirectory = Join-Path (Split-Path -Parent $SourceDirectory) ('Emu80-build/' + $variant)
}
$source = [IO.Path]::GetFullPath($SourceDirectory)
$qt = [IO.Path]::GetFullPath($QtDirectory)
$build = [IO.Path]::GetFullPath($BuildDirectory)
$vcvars = Join-Path ${env:ProgramFiles} 'Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat'
foreach ($file in @($vcvars, (Join-Path $qt 'bin/qmake.exe'), (Join-Path $source 'src/Emu80qt.pro'))) {
    if (-not (Test-Path -LiteralPath $file)) { throw "Required build tool/source missing: $file" }
}
& python (Join-Path $PSScriptRoot 'apply-upstream.py') $source
if ($LASTEXITCODE -ne 0) { throw 'Upstream integration failed' }
# Загружаем окружение компилятора только в текущий процесс.
New-Item -ItemType Directory -Path $build -Force | Out-Null
$environmentScript = Join-Path $build 'msvc-environment.cmd'
[IO.File]::WriteAllText($environmentScript, "@echo off`r`ncall `"$vcvars`" >nul`r`nif errorlevel 1 exit /b 1`r`nset`r`n", [Text.Encoding]::ASCII)
$compilerEnvironment = & $env:ComSpec /d /c $environmentScript
if ($LASTEXITCODE -ne 0) { throw 'MSVC environment setup failed' }
foreach ($line in $compilerEnvironment) {
    if ($line -match '^([^=]+)=(.*)$') { [Environment]::SetEnvironmentVariable($matches[1], $matches[2], 'Process') }
}
New-Item -ItemType Directory -Path $build -Force | Out-Null
Push-Location $build
try {
    $qmakeArgs = @('-spec', 'win32-msvc', '-o', 'Makefile', (Join-Path $source 'src/Emu80qt.pro'), 'CONFIG+=release', 'CONFIG-=debug', 'QMAKE_CXXFLAGS+=/utf-8', 'QMAKE_CXXFLAGS+=/MP')
    if ($Mcp) { $qmakeArgs += 'MCP_SERVER=1' }
    & (Join-Path $qt 'bin/qmake.exe') @qmakeArgs
    if ($LASTEXITCODE -ne 0) { throw 'qmake failed' }
    & nmake /nologo /f Makefile.Release
    if ($LASTEXITCODE -ne 0) { throw 'Emu80 compilation failed' }
} finally { Pop-Location }
$exeName = if ($Mcp) { 'Emu80qt-mcp.exe' } else { 'Emu80qt.exe' }
$files = foreach ($name in @('At29Flash.cpp','At29Flash.h','SpecialistFlashDisk.cpp','SpecialistFlashDisk.h','SpecialistMx2VideoRam.cpp','SpecialistMx2VideoRam.h','SpecialistSystemFlash.cpp','SpecialistSystemFlash.h')) {
    [ordered]@{ path = $name; sha256 = (Get-FileHash -LiteralPath (Join-Path $source ('src/' + $name))).Hash.ToLowerInvariant() }
}
$record = [ordered]@{ executableSha256 = (Get-FileHash -LiteralPath (Join-Path $build ('release/' + $exeName))).Hash.ToLowerInvariant(); sources = @($files) }
[IO.File]::WriteAllText((Join-Path $build 'build-specialist.json'), ($record | ConvertTo-Json -Depth 5) + "`n", (New-Object Text.UTF8Encoding($false)))
if ($Deploy) {
    . (Join-Path $PSScriptRoot 'runtime.ps1')
    $runtime = Resolve-Emu80Runtime -Flash
    & (Join-Path $PSScriptRoot 'deploy-build.ps1') -SourceDirectory $source -QtDirectory $qt -BuildDirectory $build -DestinationDirectory $runtime -Mcp:$Mcp -Update
}
