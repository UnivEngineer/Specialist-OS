# SPMX CPU: load, entry, monitor; additional KEY=VALUE lines.
function Read-CpuDescriptor {
    param([byte[]]$Data)
    if (@($Data | Where-Object { $_ -gt 127 }).Count) { throw 'CPU descriptor must be ASCII' }
    $lines=[Text.Encoding]::ASCII.GetString($Data) -split "\r\n|\n|\r"
    if ($lines.Count -lt 2 -or $lines[0].Trim() -notmatch '^[0-9A-Fa-f]{1,4}$' -or $lines[1].Trim() -notmatch '^[0-9A-Fa-f]{1,4}$') { throw 'CPU requires separate load and entry address lines' }
    $monitor=if($lines.Count -gt 2){$lines[2].Trim()}else{''}
    if ($monitor -and ($monitor -match '\s|=' -or $monitor -match '^[0-9A-Fa-f]{1,4}$')) { throw 'CPU third line must be a monitor filename' }
    $fields=@{}
    for($i=3;$i -lt $lines.Count;$i++) {
        $line=$lines[$i].Trim()
        if(-not $line){continue}
        if(-not $monitor){throw 'CPU monitor is required before extra fields'}
        if($line -notmatch '^([A-Za-z][A-Za-z0-9_]*)=(.*)$'){throw 'CPU extra fields require KEY=VALUE'}
        $key=$Matches[1].ToUpperInvariant();$value=$Matches[2].Trim()
        if($fields.ContainsKey($key)){throw "Duplicate CPU field: $key"}
        $fields[$key]=$value
    }
    $flags=if($monitor -ieq 'mxos2.mon'){4}else{0}
    if($fields.ContainsKey('FLAGS')){$flags=[Convert]::ToInt32($fields['FLAGS'],16)}
    if($flags -notin @(0,1,2,4)){throw 'Unknown CPU flags'}
    [pscustomobject]@{Load=[Convert]::ToInt32($lines[0].Trim(),16);Entry=[Convert]::ToInt32($lines[1].Trim(),16);Monitor=$monitor;Flags=$flags;Fields=$fields}
}
function New-CpuDescriptor {
    param([int]$Load,[int]$Entry=-1,[int]$Flags=0,[string]$Monitor='',[System.Collections.IDictionary]$Fields=@{})
    if($Entry -lt 0){$Entry=$Load}
    if(-not $Monitor){$Monitor=if($Flags){'mxos2.mon'}else{'mon2.mon'}}
    $text="{0:X4}`r`n{1:X4}`r`n{2}`r`nFLAGS={3:X2}`r`n" -f $Load,$Entry,$Monitor,$Flags
    foreach($key in $Fields.Keys) { $text += "{0}={1}`r`n" -f $key,$Fields[$key] }
    $null=Read-CpuDescriptor ([Text.Encoding]::ASCII.GetBytes($text))
    return $text
}
function Get-CpuFieldsFromSymbols {
    param([string]$Symbols)
    $names=[ordered]@{IMAGE_BSS_SIZE='BSS';IMAGE_MEM_BEGIN='MEM_BEGIN';IMAGE_MEM_END='MEM_END';IMAGE_MIN_HEAP_SIZE='MIN_HEAP';IMAGE_STACK_SIZE='STACK';IMAGE_MEMORY_POLICY='POLICY';IMAGE_API='API';IMAGE_COMPAT_MODE='MODE';IMAGE_CACHEABLE='CACHEABLE'}
    $fields=[ordered]@{}
    foreach($key in $names.Keys) {
        $match=[regex]::Match($Symbols,'(?m)^'+$key+': EQU 0x([0-9A-Fa-f]+)')
        if($match.Success) { $fields[$names[$key]]=$match.Groups[1].Value.TrimStart('0');if(-not $fields[$names[$key]]){$fields[$names[$key]]='0'} }
    }
    return $fields
}
