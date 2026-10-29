[CmdletBinding()]
param([Parameter(Mandatory = $true)][string] $ToolsDirectory)
$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
$tools = [IO.Path]::GetFullPath($ToolsDirectory)
if (Test-Path -LiteralPath $tools) { throw "Choose a new tools directory: $tools" }
New-Item -ItemType Directory -Path $tools -Force | Out-Null
$archives = @(
    @{ Name = 'qtbase-Windows-Windows_10-MSVC2019-Windows-Windows_10-X86_64.7z'; Sha256 = 'e563de40230295841c2eece26e347de709edc4bf035515cc239cae3c994c9af6' },
    @{ Name = 'qtmultimedia-Windows-Windows_10-MSVC2019-Windows-Windows_10-X86_64.7z'; Sha256 = 'f07053981afe77d4bfba8841470a685b1abd4545c18168d14656fbb4db1aeffe' }
)
$sevenZip = Join-Path ${env:ProgramFiles} '7-Zip/7z.exe'
if (-not (Test-Path -LiteralPath $sevenZip)) { throw '7-Zip is required to extract the Qt SDK.' }
$prefix = 'https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/qt5_5152/qt.qt5.5152.win64_msvc2019_64/5.15.2-0-202011130602'
foreach ($entry in $archives) {
    $archive = Join-Path $tools $entry.Name
    Invoke-WebRequest -UseBasicParsing -Uri ($prefix + $entry.Name) -OutFile $archive
    if ((Get-FileHash -LiteralPath $archive).Hash.ToLowerInvariant() -ne $entry.Sha256) { throw 'Qt package checksum mismatch' }
    & $sevenZip x $archive ('-o' + (Join-Path $tools 'qt')) -y -bso0
    if ($LASTEXITCODE -ne 0) { throw 'Qt extraction failed' }
}
$qt = Join-Path $tools 'qt/5.15.2/msvc2019_64'
[IO.File]::WriteAllText((Join-Path $qt 'bin/qt.conf'), "[Paths]`nPrefix=..`n")
# Штатная настройка OpenSource-дистрибутива, как в aqtinstall Updater.set_license.
# Использование Qt и распространение сборки подчиняются его open-source лицензиям.
$configPath = Join-Path $qt 'mkspecs/qconfig.pri'
$config = [IO.File]::ReadAllText($configPath).Replace('QT_EDITION = Enterprise', 'QT_EDITION = OpenSource').Replace('QT_LICHECK = licheck.exe', 'QT_LICHECK =')
[IO.File]::WriteAllText($configPath, $config)
Write-Host "Qt SDK: $qt (MSVC 2022 and Windows SDK must already be installed)"
