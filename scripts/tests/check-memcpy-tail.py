"""8080: локальный memcpy/memmove, все хвосты и сравнение времени."""
from pathlib import Path
import argparse
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--baseline-source', type=Path)
p.add_argument('--out', type=Path, default=ROOT/'docs/bench/memcpy-tail-2026-11-15')
p.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
a = p.parse_args()
a.out.mkdir(parents=True, exist_ok=True)

def assemble(name, source):
    asm = a.out/(name+'.asm')
    asm.write_text(' ORG 09000h\n INCLUDE "'+(ROOT/'source/common/memcpy_bc_hl.inc').as_posix()+'"\n INCLUDE "'+source.resolve().as_posix()+'"\n', encoding='utf-8')
    stem = a.out/name
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow', '--raw='+str(stem)+'.bin', '--sym='+str(stem)+'.sym', '--lst='+str(stem)+'.lst', str(asm)], check=True, capture_output=True)
    sy = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', stem.with_suffix('.sym').read_text(), re.M)}
    return stem.with_suffix('.bin').read_bytes(), sy['moveCoreEnd']-sy['moveCore']

def run(binary, src, dst, count):
    word = lambda n: (n & 65535).to_bytes(2, 'little')
    memory = bytearray((i*37+(i>>8)*13)&255 for i in range(65536))
    memory[0x9000:0x9000+len(binary)] = binary
    stub = bytes.fromhex('31 00 8F 21')+word(src)+b'\x11'+word(src+count)+b'\x01'+word(dst)
    stub += bytes.fromhex('CD 00 90 22 00 82 60 69 22 02 82 EB 22 04 82 C3 00 81')
    memory[0x8000:0x8000+len(stub)] = stub
    expected = bytearray(memory)
    expected[dst:dst+count] = memory[src:src+count]
    inp, out = a.out/'input.bin', a.out/'output.bin'
    inp.write_bytes(memory)
    r = subprocess.run([str(a.ticks), '-m8080', '-pc', '8000', '-end', '8100', '-counter', '4000000', '-output', str(out), str(inp)], check=True, capture_output=True, text=True, timeout=15)
    data = out.read_bytes()
    assert data[65542:65546] == bytes.fromhex('00 81 00 8F'), (src, dst, count, 'PC/SP')
    assert data[0x8200:0x8206] == word(src+count)+word(dst+count)+word(src+count), (src, dst, count, 'HL/BC/DE')
    assert data[0x1000:0x7800] == expected[0x1000:0x7800], (src, dst, count, 'data/guards')
    return int((r.stdout+r.stderr).strip())

new, size = assemble('after', ROOT/'source/common/move8080.inc')
cases = 0
for count in list(range(514))+[767,768,769,1023,1024,1025,4095,4096,4097,8192]:
    for src, dst in [(0x2000,0x5000),(0x5000,0x2000),(0x2000,0x2001),(0x2001,0x2000),(0x2000,0x2000)]:
        run(new, src, dst, count)
        cases += 1
    if count % 128 == 0:
        print('PASS through length', count, flush=True)
rows = []
report = dict(cpu='Intel 8080 model', cpuHz=2000000, cases=cases, codeBytes=size, comparisons=rows)
if a.baseline_source:
    old, old_size = assemble('before', a.baseline_source)
    report['beforeCodeBytes'] = old_size
    for count in [0,2,14,16,20,32,64,128,240,254,255,256,257,270,272,511,512,4096]:
        for src, dst in [(0x3000,0x2000),(0x2000,0x3000)]:
            before, after = run(old, src, dst, count), run(new, src, dst, count)
            rows.append(dict(count=count, backward=dst>src, before=before, after=after))
(a.out/'result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
print('PASS', cases, 'cases; moveCore bytes', report.get('beforeCodeBytes'), '->', size)
