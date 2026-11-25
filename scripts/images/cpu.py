"""SPMX CPU descriptor: two addresses, explicit monitor, then KEY=VALUE fields."""
import re

def parse_cpu(data):
    try: lines=data.decode('ascii').splitlines()
    except UnicodeDecodeError as e: raise ValueError('CPU descriptor must be ASCII') from e
    if len(lines)<2 or any(not re.fullmatch(r'[0-9A-Fa-f]{1,4}',v.strip()) for v in lines[:2]):
        raise ValueError('CPU requires separate load and entry address lines')
    monitor=lines[2].strip() if len(lines)>2 else ''
    if monitor and (any(c.isspace() for c in monitor) or '=' in monitor or re.fullmatch(r'[0-9A-Fa-f]{1,4}',monitor)):
        raise ValueError('CPU third line must be a monitor filename')
    fields={}
    for line in lines[3:]:
        if not line.strip(): continue
        if not monitor: raise ValueError('CPU monitor is required before extra fields')
        m=re.fullmatch(r'([A-Za-z][A-Za-z0-9_]*)=(.*)',line.strip())
        if not m: raise ValueError('CPU extra fields require KEY=VALUE')
        key,value=m[1].upper(),m[2].strip()
        if key in fields: raise ValueError('Duplicate CPU field: '+key)
        fields[key]=value
    try: flags=int(fields.get('FLAGS','04' if monitor.lower()=='mxos2.mon' else '00'),16)
    except ValueError as e: raise ValueError('Invalid CPU FLAGS') from e
    if flags not in (0,1,2,4): raise ValueError('Unknown CPU flags')
    return dict(load=int(lines[0],16),entry=int(lines[1],16),monitor=monitor,flags=flags,fields=fields)

def format_cpu(load,entry=None,flags=0,monitor=None,fields=None):
    if entry is None: entry=load
    if monitor is None: monitor='mxos2.mon' if flags else 'mon2.mon'
    extra={'FLAGS':f'{flags:02X}',**(fields or {})}
    text=f'{load:04X}\n{entry:04X}\n{monitor}\n'+''.join(f'{k}={v}\n' for k,v in extra.items())
    parsed=parse_cpu(text.encode('ascii'))
    if parsed['flags']!=flags: raise ValueError('Conflicting CPU FLAGS')
    return text

def fields_from_symbols(symbols):
    names={'IMAGE_BSS_SIZE':'BSS','IMAGE_MEM_BEGIN':'MEM_BEGIN','IMAGE_MEM_END':'MEM_END',
           'IMAGE_MIN_HEAP_SIZE':'MIN_HEAP','IMAGE_STACK_SIZE':'STACK','IMAGE_MEMORY_POLICY':'POLICY',
           'IMAGE_API':'API','IMAGE_COMPAT_MODE':'MODE','IMAGE_CACHEABLE':'CACHEABLE',
           'IMAGE_BODY_SIZE':'BODY_SIZE','IMAGE_RELOC_KIND':'RELOC_KIND','IMAGE_ALIGN_LOG2':'ALIGN_LOG2'}
    values={name:int(value,16) for name,value in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',symbols,re.M)}
    return {field:f'{values[name]:X}' for name,field in names.items() if name in values}

def fields_from_c_map(text,load,body_bytes):
    values={name:int(value,16) for name,value in re.findall(r'^([\w.]+)\s*=\s*\$([0-9A-Fa-f]+)',text,re.M)}
    end=values.get('__BSS_END_tail',load+body_bytes)
    if end<load+body_bytes: raise ValueError('C BSS ends before image body')
    return {'BSS':f'{end-load-body_bytes:X}','MEM_END':f'{end:X}','API':'2','POLICY':'MANAGED'}
