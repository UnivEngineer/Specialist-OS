. (Join-Path $PSScriptRoot "cpu.ps1")
# Контейнеры COM/EXE/DLL/FNT MXOS2, RKS и CPU/I80. Без внешних процессов.

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
    # Срез нужен только для заголовка (14 байт v1 или 30 байт v2); нагрузка передаётся целиком.
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
        [string]$Path,
        [ValidateSet('com','exe','dll','fnt','rks')][string]$Format,
        [int]$LoadAddress=0,
        [ValidateSet('AUTO','MX','STD','MON','RF','NATIVE')][string]$GameMode
    )
    try {
        $data=[IO.File]::ReadAllBytes($Path);$kind=[IO.Path]::GetExtension($Path).ToLowerInvariant()
        $load=$LoadAddress;$entry=$load;$flags=0;$offset=0;$size=$data.Length;$expected=$null
        $fields=@{};$nativeHeader=$false;$oldFont=$false
        if($kind -in @('.com','.exe','.dll','.fnt') -and $data.Length -ge 4 -and $data[2] -eq 0x4D -and $data[3] -eq 0x58) {
            if($data.Length -lt 16 -or $data[0] -ne 0xCD -or $data[1] -ne 0x20 -or $data[4] -notin @(1,2)){throw 'Expected MXOS2 container header'}
            $offset=if($data[4] -eq 1){16}else{32}
            if($data.Length -lt $offset){throw 'Short image header'}
            $flags=$data[5];$load=[BitConverter]::ToUInt16($data,6);$entry=[BitConverter]::ToUInt16($data,8);$size=[BitConverter]::ToUInt16($data,10)
            $expected=[BitConverter]::ToUInt16($data,12)
            if($data.Length -ne $offset+$size -or (Get-ExecutableChecksum $data ($offset-2)) -ne [BitConverter]::ToUInt16($data,$offset-2)){throw 'Invalid image size or checksum'}
            $oldFont=$offset -eq 16 -and $Format -eq 'fnt'
            if($offset -eq 32) {
                if($data[27] -ne 32 -or $data[29]){throw 'Invalid image header size/reserve'}
                foreach($pair in @(@('BSS',14),@('MEM_BEGIN',16),@('MEM_END',18),@('MIN_HEAP',20),@('STACK',24))){$fields[$pair[0]]='{0:X}' -f [BitConverter]::ToUInt16($data,$pair[1])}
                $fields.POLICY='{0:X}' -f $data[22];$fields.API='{0:X}' -f $data[23];$fields.MODE='{0:X}' -f $data[26];$fields.CACHEABLE='{0:X}' -f $data[28]
            }
        } elseif($kind -eq '.rks') {
            if($size -lt 6){throw 'Short RKS'}
            $load=[BitConverter]::ToUInt16($data,0);$end=[BitConverter]::ToUInt16($data,2);$entry=$load;$size=$end-$load+1;$offset=4
            if($end -lt $load -or $data.Length -ne $size+6){throw 'Invalid RKS size'}
            $expected=[BitConverter]::ToUInt16($data,$data.Length-2)
        } elseif($kind -eq '.cpu') {
            $i80=[IO.Path]::ChangeExtension($Path,'.i80')
            if([IO.File]::Exists($i80)) {
                $descriptor=Read-CpuDescriptor $data;$load=$descriptor.Load;$entry=$descriptor.Entry;$flags=$descriptor.Flags;$fields=$descriptor.Fields
                $data=[IO.File]::ReadAllBytes($i80);$size=$data.Length
            } else {$load=0;$entry=0}
        }
        if($flags -notin @(0,1,2,4)){throw 'Unknown COM/EXE flags'}
        if(-not $size -or $load -lt 0 -or $load -ge 65536 -or $load+$size -gt 65536){throw 'Invalid image address or size'}
        if($entry -lt $load -or $entry -ge $load+$size){throw 'Entry is outside the image'}
        $payload=New-Object byte[] $size;[Array]::Copy($data,$offset,$payload,0,$size)
        $checksum=Get-ExecutableChecksum $payload
        if($null -ne $expected -and $checksum -ne $expected){throw 'Invalid image checksum'}
        function Read-ImageNumber($Key,$Default) {
            if($fields.ContainsKey($Key)){return [Convert]::ToInt32($fields[$Key],16)}
            return $Default
        }
        $nativeDefaults=$flags -or $Format -in @('dll','fnt')
        $bss=Read-ImageNumber 'BSS' $(if($nativeDefaults){0}else{65535})
        $begin=Read-ImageNumber 'MEM_BEGIN' $load
        $end=Read-ImageNumber 'MEM_END' ($load+$size+$(if($bss -eq 65535){0}else{$bss}))
        $heap=Read-ImageNumber 'MIN_HEAP' 0;$stack=Read-ImageNumber 'STACK' 0
        $policy=if($fields.ContainsKey('POLICY') -and $fields.POLICY -in @('EXCLUSIVE','PRESERVE','MANAGED')){@{EXCLUSIVE=0;PRESERVE=1;MANAGED=2}[$fields.POLICY]}else{Read-ImageNumber 'POLICY' $(if($nativeDefaults){2}else{0})}
        $api=Read-ImageNumber 'API' $(if($nativeDefaults){2}else{0})
        $modes = @{AUTO=0;MX=1;STD=2;MON=3;RF=4}
        $mode=if($fields.ContainsKey('MODE') -and $modes.ContainsKey($fields.MODE)){$modes[$fields.MODE]}else{Read-ImageNumber 'MODE' 0}
        if ($GameMode) {
            if ($Format -ne 'exe') { throw 'GameMode requires EXE output' }
            if ($GameMode -eq 'NATIVE') {
                if ($flags -ne 4) { throw 'NATIVE requires a source with IMAGE_FLAG_MAIN' }
            } else {
                if ($flags) { throw 'RUN modes require a source with flags=0' }
                $mode = $modes[$GameMode]
            }
        }
        $options=Read-ImageNumber 'CACHEABLE' 0
        if($Format -ne 'rks') {
            if($bss -lt 0 -or $bss -gt 65535 -or $begin -lt 0 -or $begin -gt $load -or $end -lt $load+$size -or $end -gt 65535 -or ($bss -ne 65535 -and $load+$size+$bss -gt $end)){throw 'Invalid image memory interval'}
            if($policy -notin @(0,1,2) -or ($bss -eq 65535 -and $policy -ne 0)){throw 'Invalid image memory policy'}
            if($heap -lt 0 -or $heap -gt 65535 -or $stack -lt 0 -or $stack -gt 65535 -or $api -notin @(0,2) -or $mode -notin @(0,1,2,3,4) -or $options -notin @(0,1,3)){throw 'Invalid image requirements'}
        }
        if($Format -eq 'rks') {
            if($flags){throw 'RKS cannot represent executable flags'}
            if($entry -ne $load){throw 'RKS cannot represent a separate entry address'}
            $result=New-Object byte[] ($size+6);Set-Word $result 0 $load;Set-Word $result 2 ($load+$size-1)
            [Array]::Copy($payload,0,$result,4,$size);Set-Word $result ($size+4) $checksum
        } else {
            if($size -gt 65535){throw 'Image payload exceeds 65535 bytes'}
            $header=32;$version=2
            $result=New-Object byte[] ($size+$header)
            $result[0]=0xCD;$result[1]=0x20;$result[2]=0x4D;$result[3]=0x58;$result[4]=$version;$result[5]=$flags
            Set-Word $result 6 $load;Set-Word $result 8 $entry;Set-Word $result 10 $size;Set-Word $result 12 $checksum
            if($header -eq 32) {
                Set-Word $result 14 $bss;Set-Word $result 16 $begin;Set-Word $result 18 $end;Set-Word $result 20 $heap
                $result[22]=$policy;$result[23]=$api;Set-Word $result 24 $stack;$result[26]=$mode;$result[27]=32;$result[28]=$options
            }
            Set-Word $result ($header-2) (Get-ExecutableChecksum $result ($header-2))
            [Array]::Copy($payload,0,$result,$header,$size)
        }
        return ,$result
    } catch {throw "Executable conversion failed for '${Path}': $_"}
}
