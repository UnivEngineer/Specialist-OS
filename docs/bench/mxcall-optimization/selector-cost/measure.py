from pathlib import Path
import json
import re
import struct
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[4]
OUT = Path(__file__).resolve().parent
pack = struct.Struct('<H').pack
symbols = {}
for filename in ('bin/MEM.sym', 'bin/banked/LOCALBLOCK.sym'):
    for key, value in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (ROOT/filename).read_text(), re.M):
        symbols[key] = int(value, 16)
s = symbols
image = (ROOT/'bin/MEM.sys').read_bytes()
base = bytearray(65536)
for i in range(image[5]):
    address, count, offset = struct.unpack_from('<HHH', image, 6+6*i)
    base[address:address+count] = image[offset:offset+count]
base[s['bankPageCount']] = 16
base[s['MEM_COMMAND_SHADOW']:s['MEM_COMMAND_SHADOW']+32] = base[0xffc0:0xffe0]
start = s['copyPrepareBody']
end = start + 27
pair = s['bankCopyPair']
for at in (start+3,start+12,start+21):
    assert base[at:at+3] == b'\xcd'+pack(pair)
assert base[end] == 0x7c
key = 0x7450
code = 0x7400
cold = 0x7460
keys = (s['bankCopyRequest'], s['bankCopyRequest']+3, s['bankPage'])
jmp = lambda n: b'\xc3'+pack(n)
lda = lambda n: b'\x3a'+pack(n)
sta = lambda n: b'\x32'+pack(n)
ldhl = lambda n: b'\x2a'+pack(n)
cached = b'\x21'+pack(key)
for ix, address in enumerate(keys):
    cached += lda(address)+b'\xbe\xc2'+pack(cold)
    if ix < 2:
        cached += b'\x23'
# Cache hit rejects mismatched source, destination, or initiator before using prepared pairs.
hit = ldhl(s['bankCopyPairs']+4)+jmp(end)
assert code+len(cached)+len(hit) <= key
miss = bytes(base[start:end])
for ix,address in enumerate(keys):
    miss += lda(address)+sta(key+ix)
miss += hit
am = runpy.run_path(str(ROOT/'scripts/tests/measure-mem-copy-setup.py'))
def run(memory, stop):
    row = am['run'](OUT,memory,stop,37,{})
    return row['cycles'], (OUT/'out.bin').read_bytes()
def writer(buf, offset, chunk):
    buf[offset:offset+len(chunk)] = chunk
def run_variant(case, mode, valid_previous):
    source, dest, caller = case
    memory = bytearray(base)
    memory[0xffdf] = 1
    memory[s['bankPage']] = caller
    writer(memory,0x8500,bytes([source])+pack(0x2001)+bytes([dest])+pack(0x6001)+pack(256))
    writer(memory,0x8800,b'\x31'+pack(0x8f00)+b'\x21'+pack(0x8500)+b'\xcd'+pack(s['bankCopySector']))
    if mode == 'preseed':
        writer(memory,start,hit)
    elif mode == 'guarded':
        writer(memory,start,jmp(code))
        writer(memory,code,cached+hit)
        writer(memory,cold,miss)
    if mode != 'base':
        writer(memory,key,bytes(valid_previous[0]))
        writer(memory,s['bankCopyPairs'],valid_previous[1])
    return run(memory,s['bankCopyTail'])
def inner(case, stop):
    source,dest,caller = case
    memory = bytearray(base)
    memory[s['bankPage']]=caller
    memory[0xffdf] = 1
    writer(memory,0x8500,bytes([source])+pack(0x2001)+bytes([dest])+pack(0x6001)+pack(256))
    writer(memory,0x8800,b'\x31'+pack(0x8f00)+b'\x21'+pack(0x8500)+b'\xcd'+pack(s['bankCopySector']))
    return run(memory,stop)[0]
def observe(memory):
    ranges = [(s['bankCopyPairs'],6), (s['bankCopySourcePair'],6),
              (s['bankCopyRest'],1), (s['BANK_WINDOW'],32)]
    return [memory[address:address+size].hex() for address,size in ranges] + [memory[65536:65546].hex()]
def selector(page):
    if page & 0x80:
        return bytes([0xfe,page & 15])
    return bytes([0xfd,page-1]) if page else b'\xfc\0'
rows = []
cold_base=(2,3,0)
for case in (cold_base,(0,3,0),(2,0,0),(2,3,6),(0x82,3,0)):
    ticks, standard = run_variant(case,'base',None)
    ports = s['bankCopyPairs']
    precalc = standard[ports:ports+6]
    assert precalc==b''.join(map(selector,case))
    ideal, preseed = run_variant(case,'preseed',(case,precalc))
    cached_ticks, hit_state = run_variant(case,'guarded',(case,precalc))
    assert observe(standard)==observe(preseed)==observe(hit_state),(case,observe(standard),observe(preseed),observe(hit_state))
    original_start = inner(case,start)
    original_end = inner(case,end)
    previous = cold_base if case!=cold_base else (5,3,0)
    _, old = run_variant(previous,'base',None)
    cold_ticks, miss_state = run_variant(case,'guarded',(previous,old[ports:ports+6]))
    assert observe(standard)==observe(miss_state)
    assert miss_state[key:key+3]==bytes(case)
    rows.append(dict(source=case[0],destination=case[1],caller=case[2],previous=previous,
                     preparation=ticks,inner=ticks-original_start,three_pairs=original_end-original_start,
                     calls_and_handlers=original_end-original_start-3*(13+16),
                     cold_pair=(case[0]!=previous[0],case[1]!=previous[1],case[2]!=previous[2]),
                     ideal_preseed=ideal, guarded_same=cached_ticks,guarded_change=cold_ticks,
                     observed=observe(standard)))
(OUT/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
print(json.dumps(rows))
