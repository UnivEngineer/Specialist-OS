[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [ValidateSet('19781', '19782', '19783', '19784')]
    [string] $Port,
    [switch] $TestFlashSeed
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$localAppData = [Environment]::GetFolderPath('LocalApplicationData')
$runtime = Join-Path $localAppData "Specialist-OS/emu80/$Port"
$executable = Join-Path $runtime 'Emu80qt-mcp.exe'
if (-not (Test-Path -LiteralPath $executable -PathType Leaf)) {
    throw "Install the Emu80 MCP build for port $Port in $runtime. See scripts/emu80/README.md."
}
if (-not (Test-Path -LiteralPath (Join-Path $runtime '_settings') -PathType Container)) {
    throw "Create the _settings directory in $runtime to keep this slot's settings separate."
}

$mutex = [Threading.Mutex]::new($false, "Global\SpecialistOS.Emu80.Mcp.$Port")
$locked = $false
$process = $null
$ready = $false
try {
    try { $locked = $mutex.WaitOne(0) }
    catch [Threading.AbandonedMutexException] { $locked = $true }
    if (-not $locked) { throw "Emu80 MCP slot $Port is being prepared by another process." }

    $listeners = @(Get-NetTCPConnection -LocalPort ([int]$Port) -State Listen -ErrorAction SilentlyContinue)
    if ($listeners.Count) {
        $owners = ($listeners.OwningProcess | Sort-Object -Unique) -join ', '
        throw "Emu80 MCP port $Port is already in use (PID $owners). Choose another slot."
    }

    $process = & (Join-Path $PSScriptRoot 'run.ps1') -EmulatorDirectory $runtime -Flash -Mcp -UpdateSystemFlash -TestFlashSeed:$TestFlashSeed -PassThru
    $startup = [Diagnostics.Stopwatch]::StartNew()
    do {
        if ($process.HasExited) { throw "Emu80 for slot $Port exited during startup." }
        $listeners = @(Get-NetTCPConnection -LocalPort ([int]$Port) -State Listen -ErrorAction SilentlyContinue)
        if ($listeners.Count) {
            if ($listeners | Where-Object { $_.OwningProcess -ne $process.Id -or $_.LocalAddress -notin '127.0.0.1', '::1' }) {
                throw "Unexpected listener on MCP port $Port. Confirm the build and slot assignment."
            }
            $ready = $true
            Write-Host "Emu80 MCP http://127.0.0.1:$Port/mcp (PID $($process.Id))"
            break
        }
        Start-Sleep -Milliseconds 250
    } while ($startup.Elapsed.TotalSeconds -lt 10)
    if (-not $ready) { throw "Emu80 PID $($process.Id) did not open MCP port $Port." }
} finally {
    if ($process -and -not $ready -and -not $process.HasExited) { Stop-Process -InputObject $process }
    if ($locked) { $mutex.ReleaseMutex() }
    $mutex.Dispose()
}
