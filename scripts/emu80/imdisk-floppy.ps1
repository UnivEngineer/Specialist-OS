[CmdletBinding()]
param(
    [Parameter(Mandatory = $true, Position = 0)]
    [ValidateSet('Mount', 'Unmount')][string] $Action,
    [string] $EmulatorDirectory,
    [ValidatePattern('^[A-Za-z]$')][string] $DriveLetter = 'A'
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0
. (Join-Path $PSScriptRoot 'runtime.ps1')
$runtime = Resolve-Emu80Runtime -Directory $EmulatorDirectory
$deployment = Join-Path $runtime 'specialist-os'
$image = Join-Path $deployment 'floppy720.img'
$mountPoint = $DriveLetter.ToUpperInvariant() + ':'
$imdisk = Get-Command imdisk.exe -CommandType Application -ErrorAction SilentlyContinue
if (-not $imdisk) { throw 'Install ImDisk before using this task.' }

# ImDisk lists the backing file using a native Windows path.
$imagePath = if ($image.StartsWith('\\')) { '\??\UNC\' + $image.Substring(2) } else { '\??\' + $image }
try {
    # An unused letter is reported as a native stderr error by ImDisk.
    $ErrorActionPreference = 'Continue'
    $details = @(& $imdisk.Source -l -m $mountPoint 2>$null)
    $mounted = $LASTEXITCODE -eq 0
} finally {
    $ErrorActionPreference = 'Stop'
}
if ($mounted -and ($details.Trim() -notcontains $imagePath)) {
    throw "Windows drive $mountPoint belongs to a different ImDisk image."
}

if ($Action -eq 'Unmount') {
    if (-not $mounted) {
        Write-Host "No ImDisk mount for $image on $mountPoint"
        return
    }
    foreach ($attempt in 1..3) {
        & $imdisk.Source -d -m $mountPoint
        if ($LASTEXITCODE -eq 0) { return }
        if ($attempt -lt 3) { Start-Sleep -Milliseconds 700 }
    }
    throw "Could not unmount $mountPoint. Close any Explorer windows and files on that drive, then run the task again."
}

if ($mounted) {
    if ($details -match '\bReadOnly\b') {
        throw "$mountPoint is mounted read-only. Unmount it before mounting for writing."
    }
    Write-Host "$mountPoint already holds $image"
    return
}
if ([IO.DriveInfo]::GetDrives().Name -contains ($mountPoint + '\')) {
    throw "Windows drive $mountPoint is already in use."
}
$running = @(Get-Process -Name Emu80qt,Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object {
    $_.Path -and (Split-Path -Parent $_.Path) -eq $runtime
})
if ($running.Count) { throw 'Close this Emu80 instance before mounting the floppy for writing.' }

if (-not (Test-Path -LiteralPath $image -PathType Leaf)) {
    $root = Split-Path -Parent (Split-Path -Parent $PSScriptRoot)
    $seed = Join-Path $root 'images/Floppy720.img'
    if (-not (Test-Path -LiteralPath $seed -PathType Leaf) -or (Get-Item -LiteralPath $seed).Length -ne 720KB) {
        throw "Build the Floppy720 image first: expected a 720 KiB image at $seed."
    }
    [IO.Directory]::CreateDirectory($deployment) | Out-Null
    [IO.File]::Copy($seed, $image, $false)
}
if ((Get-Item -LiteralPath $image).Length -ne 720KB) {
    throw "Expected a 720 KiB floppy image: $image"
}

& $imdisk.Source -a -t file -f $image -m $mountPoint -o fd,rw -S 512 -x 9 -y 2
if ($LASTEXITCODE -ne 0) { throw "ImDisk could not mount $image on $mountPoint." }
