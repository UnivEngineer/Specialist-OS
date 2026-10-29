# Явный параметр имеет приоритет. Для flash пропускаем обычную сборку из
# устаревшего окружения IDE и проверяем актуальное окружение пользователя.
function Resolve-Emu80Runtime {
    param([string] $Directory, [switch] $Flash)
    $candidates = if (-not [string]::IsNullOrWhiteSpace($Directory)) {
        @($Directory)
    } else {
        @($env:SPECIALIST_EMU80_DIR, [Environment]::GetEnvironmentVariable('SPECIALIST_EMU80_DIR', 'User'))
    }
    foreach ($candidate in $candidates) {
        if ([string]::IsNullOrWhiteSpace($candidate)) { continue }
        $path = [IO.Path]::GetFullPath($candidate)
        if ($Flash) {
            $marker = Join-Path $path 'specialist-build.json'
            if (-not (Test-Path -LiteralPath $marker -PathType Leaf)) { continue }
            $info = Get-Content -LiteralPath $marker -Raw | ConvertFrom-Json
            if ($info.model -ne 'SpecialistFlashDisk') { continue }
        }
        return $path
    }
    if ($Flash) {
        throw 'Flash requires a custom SpecialistFlashDisk build. Set SPECIALIST_EMU80_DIR to that runtime or pass -EmulatorDirectory explicitly; the selected installation has no matching specialist-build.json.'
    }
    throw 'Set SPECIALIST_EMU80_DIR or pass -EmulatorDirectory (Emu80 4.0.571 or newer).'
}
