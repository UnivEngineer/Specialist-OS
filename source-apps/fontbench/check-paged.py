"""8080: сравнение адресации линейного и построчного полного шрифта."""
from pathlib import Path
import json
import re
import subprocess

R = Path(__file__).resolve().parents[2]
O = R / 'docs/bench/paged-font'
O.mkdir(parents=True, exist_ok=True)
source = O / 'fixture.asm'
source.write_text('PAGED_FONT_BASE = 05000h\nPAGED_PRE_BASE = 05800h\n ORG 01000h\n INCLUDE "' + (R/'source-apps/fontbench/engines/paged.inc').as_posix() + '"\n', encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow', '--raw='+str(O/'fixture.bin'), '--sym='+str(O/'fixture.sym'), str(source)], check=True)
s = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (O/'fixture.sym').read_text(), re.M)}
old = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (R/'bin/FONTLAB.sym').read_text(), re.M)}
font = (R/'assets/system-rom/FONT.FNT').read_bytes()
corpus = (R/'assets/system-rom/README.TXT').read_bytes().replace(b'\r\n',b' ').replace(b'\r',b' ').replace(b'\n',b' ')
corpus = bytes(c if c >= 32 else 32 for c in corpus)[:1152].ljust(1152,b' ')
word = lambda n:n.to_bytes(2,'little')
base = bytearray(65536)
binary = (O/'fixture.bin').read_bytes()
assert 0x1000+len(binary) < 0x3000
base[0x1000:0x1000+len(binary)] = binary
base[0x3000:0x3480] = corpus
base[0x5000:0x5800] = bytes(font[c*8+y] for y in range(8) for c in range(256))
transforms = [lambda b:b<<2, lambda b:b>>2, lambda b:(b<<6)&255, lambda b:b>>4, lambda b:(b<<4)&255]
for phase, transform in enumerate(transforms):
    base[0x5800+phase*2048:0x6000+phase*2048] = bytes(transform(font[c*8+y]&63) for y in range(8) for c in range(256))
linear = bytearray(65536)
program = (R/'bin/FONTLAB.i80').read_bytes()
linear[:len(program)] = program
linear[0x3000:0x6000] = (R/'bin/FONTBEN.shifts.bin').read_bytes()
linear[0x8600:0x8A80] = corpus
linear[old['renderFont8']:old['renderFont8']+2] = word(0x2800//8)
entries = dict(raw6='quadDynamicRender',pre6='quadFullRender',raw8='aligned8Batch')

def run(memory, calls):
    mem = bytearray(memory)
    code = b'\x31'+word(0x8F00)+b'\x21'+word(0xA597)+b'\xe5\xf1'
    for text, length, target, screen in calls:
        if target.startswith('linear-'):
            code += b'\x21'+word((screen&255)+7)+b'\x22'+word(old['bios_vars.cursorY'])
            entry = old[entries[target[7:]]]
        else:
            entry = s['paged.'+target]
        code += b'\x21'+word(text)+b'\x01'+word(0x1200+length)+b'\x11'+word(screen)+b'\xcd'+word(entry)
    code += b'\xc3'+word(0x8B80)
    assert len(code)<0x800
    mem[0x8000:0x8000+len(code)] = code
    (O/'input.bin').write_bytes(mem)
    result = subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8b80','-counter','20000000','-output',str(O/'output.bin'),str(O/'input.bin')], capture_output=True,check=True,timeout=20)
    out = (O/'output.bin').read_bytes()
    assert out[65544:65546] == word(0x8F00), 'SP'
    assert out[65538:65542] == word(0x1200+length)+word(text), 'BC/HL'
    assert out[65548:65550] == word(screen), 'DE'
    if not target.startswith('linear-'):
        assert out[65536:65538] == word(0xA597), 'AF'
    return out[:65536], int(result.stdout.strip())

checks = 0
timings = {}
for target, width in [('raw6',6),('pre6',6),('raw8',8)]:
    for chars in [corpus, bytes(range(256))*4+bytes(range(128))]:
        mem = bytearray(base)
        mem[0x3000:0x3480] = chars
        mem[0x9000:0xC000] = bytes((i*37)&255 for i in range(12288))
        expected = bytearray(mem[0x9000:0xC000])
        calls = []
        for row in range(24):
            calls.append((0x3000+row*48,48,target,0x9000+row*8))
            for x,c in enumerate(chars[row*48:row*48+48]):
                for y,b in enumerate(font[c*8:c*8+8]):
                    for bit in range(width):
                        at = ((x*width+bit)//8)*256+row*8+y
                        mask = 128>>((x*width+bit)%8)
                        if b & (1<<(width-1-bit)):
                            expected[at] |= mask
                        else:
                            expected[at] &= 255^mask
        actual,cycles = run(mem,calls)
        assert actual[0x9000:0xC000] == expected, target
        assert actual[0x5000:0x8000] == mem[0x5000:0x8000], 'font/prerender unchanged'
        checks += 1
        if chars == corpus:
            timings[target] = dict(cycles=cycles,ms=cycles/2000)
            oldmem = bytearray(linear)
            oldmem[0x9000:0xC000] = mem[0x9000:0xC000]
            oldcalls = [(0x8600+row*48,48,'linear-'+target,0x9000+row*8) for row in range(24)]
            before,oldcycles = run(oldmem,oldcalls)
            assert before[0x9000:0xC000] == expected, ('linear',target)
            timings['linear-'+target] = dict(cycles=oldcycles,ms=oldcycles/2000)
            checks += 1
result = dict(checks=checks,bitmap='PASS',timings=timings,code_bytes=len(binary),measurement='8080 CPU model, 2 MHz; kernels only, full 256-glyph font; preparation excluded')
(O/'results.json').write_text(json.dumps(result,indent=2)+'\n')
print(json.dumps(result,indent=2))
