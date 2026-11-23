[CmdletBinding()]
param(
    [ValidateSet(2, 32, 48)]
    [int] $RomKiB = 48,
    [switch] $Ramfos,
    [switch] $Flash,
    [string] $EmulatorDirectory,
    [switch] $PrepareOnly,
    [switch] $Mute
)
$ErrorActionPreference = 'Stop'
. (Join-Path $PSScriptRoot 'runtime.ps1')
$runtime = Resolve-Emu80Runtime -Directory $EmulatorDirectory -Flash:$Flash
$executable = Join-Path $runtime 'Emu80qt.exe'
$images = Join-Path (Split-Path -Parent (Split-Path -Parent $PSScriptRoot)) 'images'
if ($Ramfos) { $RomKiB = 32 }
$romName = if ($Ramfos) { 'C-ROM-32k-MX-RAMFOS.bin' } elseif ($RomKiB -eq 2) { 'C-ROM-2k-MX-boot.bin' } else { "C-ROM-${RomKiB}k-MX-System.bin" }
$rom = Get-Item -LiteralPath (Join-Path $images $romName)
if ($rom.Length -ne $RomKiB * 1KB) { throw "Invalid ROM size: $romName" }
if (-not $PrepareOnly -and -not (Test-Path -LiteralPath $executable -PathType Leaf)) { throw "Emu80 not found: $executable" }
if (-not (Test-Path -LiteralPath (Join-Path $runtime 'emu80.conf') -PathType Leaf)) { throw "Invalid Emu80 runtime: $runtime" }

# Both ROM variants mount the same writable disk when their seed matches.
$running = Get-CimInstance Win32_Process -Filter "Name='Emu80qt.exe' OR Name='Emu80qt-mcp.exe'" | Where-Object {
    $_.ExecutablePath -and (Split-Path -Parent $_.ExecutablePath) -eq $runtime -and
        $_.CommandLine -match 'specialist-mx[/\\]'
}
if ($running) { throw 'Close the running Specialist-MX profile before starting it again with the same working floppy.' }

$deployment = Join-Path $runtime 'specialist-mx'
# Each system disk build gets a writable copy shared by both ROM sizes.
New-Item -ItemType Directory -Force -Path $deployment | Out-Null
if (-not $Ramfos) {
    $seed = Get-Item -LiteralPath (Join-Path $images 'A-Floppy-720k-MX-System.img')
    if ($seed.Length -ne 720KB) { throw "Invalid floppy size: $seed" }
    if ($Flash) {
        $flashImage = Join-Path $images 'A-Flash-1M-MX-System.bin'
        if ((Get-Item -LiteralPath $flashImage).Length -ne 1MB) { throw 'Invalid MX flash size' }
        Copy-Item -LiteralPath $flashImage -Destination $deployment -Force
    } else {
        $externalRom = Get-Item -LiteralPath (Join-Path $images 'H-ROM-64k-Games.bin')
        if ($externalRom.Length -ne 64KB) { throw "Invalid ROM disk size: $externalRom" }
        Copy-Item -LiteralPath $externalRom.FullName -Destination $deployment -Force
    }
    $hash = (Get-FileHash -LiteralPath $seed.FullName -Algorithm SHA256).Hash.ToLowerInvariant()
    $relativeDisk = "disks/$hash.img"
    $workingDisk = Join-Path $deployment $relativeDisk
    New-Item -ItemType Directory -Force -Path (Split-Path -Parent $workingDisk) | Out-Null
    if (-not (Test-Path -LiteralPath $workingDisk)) { [IO.File]::Copy($seed.FullName, $workingDisk) }
    if ((Get-Item -LiteralPath $workingDisk).Length -ne 720KB) { throw "Invalid working floppy size: $workingDisk" }
}
Copy-Item -LiteralPath $rom.FullName -Destination (Join-Path $deployment $romName) -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'specialist-mx.conf') -Destination $deployment -Force
$launch = @(
    if ($Ramfos) { 'define MX_RAMFOS' }
    else {
        if ($RomKiB -eq 32) { 'define MX_ROM32' }
        if ($RomKiB -eq 2) { 'define MX_ROM2' }
        'define MX_WORK_DISK'
        if ($Flash) { 'define MX_EXTERNAL_FLASH' } else { 'define MX_EXTERNAL_ROM' }
        "@FLOPPY = `"specialist-mx/$relativeDisk`""
    }
    if ($Mute -or $Ramfos) { 'define MX_MUTE' }
    'include specialist-mx/specialist-mx.conf'
)
$config = if ($Ramfos) { 'specialist-mx/launch-ramfos.conf' } else { "specialist-mx/launch$RomKiB.conf" }
[IO.File]::WriteAllLines((Join-Path $runtime $config), [string[]]$launch, [Text.UTF8Encoding]::new($false))
Write-Host "[deploy] $romName"
if (-not $Ramfos) {
    if ($Flash) {
        Write-Host '[deploy] PPI2: external boot flash (read only)'
    } else {
        Write-Host "[deploy] PPI2: $($externalRom.Name) (H: after AUTOEXEC)"
    }
    Write-Host "[deploy] Drive 0: $workingDisk"
}
if (-not $PrepareOnly) {
    Start-Process -FilePath $executable -ArgumentList '--conf-file', $config, '--platform', 'specialist-mx' -WorkingDirectory $runtime -WindowStyle Normal
}
