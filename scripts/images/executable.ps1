# Контейнеры COM/EXE/DLL MXOS2, RKS и CPU/I80. Без внешних процессов.

function Set-Word {
    param([byte[]] $Data, [int] $Offset, [int] $Value)
    $Data[$Offset] = $Value -band 0xFF
    $Data[$Offset + 1] = ($Value -shr 8) -band 0xFF
}

function Get-ExecutableChecksum {
    param([byte[]] $Data, [int] $Count = -1)
    if ($Count -lt 0) { $Count = $Data.Length }
    if ($Count -eq 0) { return 0 }
    # Штатная сумма bios_calcCS через сумму байтов: без цикла PowerShell.
    # Срез нужен только для первых 14 байт заголовка; нагрузка передаётся целиком.
    if ($Count -eq $Data.Length) {
        $sum = [Linq.Enumerable]::Sum([int[]]$Data)
    } else {
        $sum = [Linq.Enumerable]::Sum([int[]]$Data[0..($Count - 1)])
    }
    $prefix = $sum - $Data[$Count - 1]
    return ((($prefix + ($prefix -shr 8)) -band 0xFF) -shl 8) -bor ($sum -band 0xFF)
}

function Convert-ExecutableFile {
    param(
        [string] $Path,
        [ValidateSet('com', 'exe', 'dll', 'rks')][string] $Format,
        [int] $LoadAddress = 0
    )
    try {
        $data = [IO.File]::ReadAllBytes($Path)
        $kind = [IO.Path]::GetExtension($Path).ToLowerInvariant()
        $load = $LoadAddress
        $entry = $load
        $flags = 0
        $offset = 0
        $size = $data.Length
        $expectedChecksum = $null
        $magic = [byte[]](0xCD, 0x20, 0x4D, 0x58, 1, 0)

        if ($kind -in @('.com', '.exe', '.dll') -and $data.Length -ge 4 -and
            $data[2] -eq 0x4D -and $data[3] -eq 0x58) {
            if ($data.Length -lt 16) { throw 'Short COM/EXE header' }
            for ($i = 0; $i -lt 5; $i++) {
                if ($data[$i] -ne $magic[$i]) { throw 'Expected MXOS2 COM/EXE header' }
            }
            $flags = $data[5]
            if ($flags -notin @(0, 1, 2, 4)) { throw 'Unknown COM/EXE flags' }
            $load = [BitConverter]::ToUInt16($data, 6)
            $entry = [BitConverter]::ToUInt16($data, 8)
            $size = [BitConverter]::ToUInt16($data, 10)
            if ($data.Length -ne 16 + $size -or
                (Get-ExecutableChecksum $data 14) -ne [BitConverter]::ToUInt16($data, 14)) {
                throw 'Invalid COM/EXE size or header checksum'
            }
            $offset = 16
            $expectedChecksum = [BitConverter]::ToUInt16($data, 12)
        } elseif ($kind -eq '.rks') {
            if ($data.Length -lt 6) { throw 'Short RKS' }
            $load = [BitConverter]::ToUInt16($data, 0)
            $end = [BitConverter]::ToUInt16($data, 2)
            $entry = $load
            $size = $end - $load + 1
            if ($end -lt $load -or $data.Length -ne $size + 6) { throw 'Invalid RKS size' }
            $offset = 4
            $expectedChecksum = [BitConverter]::ToUInt16($data, $data.Length - 2)
        } elseif ($kind -eq '.cpu') {
            $i80 = [IO.Path]::ChangeExtension($Path, '.i80')
            if ([IO.File]::Exists($i80)) {
                foreach ($value in $data) {
                    if ($value -gt 127) { throw 'CPU addresses must be ASCII' }
                }
                $fields = [Text.Encoding]::ASCII.GetString($data).Trim() -split '\s+'
                if ($fields.Count -notin @(2, 3)) { throw 'CPU requires load, entry and optional flags' }
                if ($fields.Count -eq 3) { $flags = [Convert]::ToInt32($fields[2], 16) }
                if ($flags -notin @(0, 1, 2, 4)) { throw 'Unknown CPU flags' }
                $load = [Convert]::ToInt32($fields[0], 16)
                $entry = [Convert]::ToInt32($fields[1], 16)
                $data = [IO.File]::ReadAllBytes($i80)
                $size = $data.Length
            } else {
                $load = 0
                $entry = 0
            }
        }
        if ($size -eq 0 -or $load -lt 0 -or $load -ge 65536 -or $load + $size -gt 65536) {
            throw 'Invalid image address or size'
        }
        if ($entry -lt $load -or $entry -ge $load + $size) { throw 'Entry is outside the image' }
        if ($offset -eq 0) {
            $payload = $data
        } else {
            $payload = New-Object byte[] $size
            [Array]::Copy($data, $offset, $payload, 0, $size)
        }
        $checksum = Get-ExecutableChecksum $payload
        if ($null -ne $expectedChecksum -and $checksum -ne $expectedChecksum) {
            throw 'Invalid image checksum'
        }
        # Проверенный контейнер того же формата уже содержит нужные байты.
        # COM, EXE и DLL различаются расширением, их заголовок одинаков.
        if (($kind -eq '.rks' -and $Format -eq 'rks') -or
            ($offset -eq 16 -and $Format -ne 'rks')) {
            return ,$data
        }

        if ($Format -eq 'rks') {
            if ($flags) { throw 'RKS cannot represent executable flags' }
            if ($entry -ne $load) { throw 'RKS cannot represent a separate entry address' }
            $result = New-Object byte[] ($size + 6)
            Set-Word $result 0 $load
            Set-Word $result 2 ($load + $size - 1)
            [Array]::Copy($payload, 0, $result, 4, $size)
            Set-Word $result ($size + 4) $checksum
        } else {
            if ($size -gt 65535) { throw 'COM payload exceeds 65535 bytes' }
            $result = New-Object byte[] ($size + 16)
            [Array]::Copy($magic, $result, $magic.Length)
            $result[5] = $flags
            Set-Word $result 6 $load
            Set-Word $result 8 $entry
            Set-Word $result 10 $size
            Set-Word $result 12 $checksum
            Set-Word $result 14 (Get-ExecutableChecksum $result 14)
            [Array]::Copy($payload, 0, $result, 16, $size)
        }
        return ,$result
    }
    catch {
        throw "Executable conversion failed for '${Path}': $_"
    }
}
