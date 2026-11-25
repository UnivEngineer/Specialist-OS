"""Verify the proposed 6x8 Fast loop against the actual font and an oracle."""
import argparse
import json
from pathlib import Path
import random
import re
import runpy
import subprocess
import tempfile

report = Path(__file__).resolve().parent
parser = argparse.ArgumentParser()
parser.add_argument('--checkout', type=Path, default=report.parents[2])
root = parser.parse_args().checkout.resolve()
checker = (root/'source-apps/fontbench/check-engines.py').read_text(encoding='utf-8')
ticks = re.search(r"^TICKS='([^']+)'", checker, re.M)[1]
prerender = runpy.run_path(root/'scripts/build/build-font-prerender.py')['build']
font = (root/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
word = lambda value: value.to_bytes(2,'little')
pages = (0x50,0x68,0x70,0x58,0x60,0x40)
variants = (
    ('direct',None,'DE',False,225),
    ('directStream','directLimit','DE',False,252),
    ('directInverse','directInverseLimit','DE',True,264),
    ('stackStream','stackLimit','SP input',False,224),
    ('stackInverse','stackInverseLimit','SP input',True,236),
    ('parkStream','parkLimit','SP screen',False,258),
    ('parkInverse','parkInverseLimit','SP screen',True,270),
)


def expected_screen(text, glyph_row, screen_offset, inverse):
    screen = bytearray([0xA5])*0x3000
    for first in range(0,len(text),4):
        pixels = 0
        for code in text[first:first+4]:
            pixels = (pixels << 6) | (font[code*8+glyph_row] & 63)
        for column,byte in enumerate(pixels.to_bytes(3,'big')):
            screen[(first//4*3+column)*256+screen_offset] = byte ^ (0xFF if inverse else 0)
    return screen


def run(mem, work, start, stop):
    source = work/'input.bin'
    result = work/'output.bin'
    source.write_bytes(mem)
    cmd = [ticks,'-m8080','-pc',f'{start:x}','-end',f'{stop:x}',
           '-counter','50000','-output',str(result),str(source)]
    elapsed = int(subprocess.check_output(cmd, timeout=10))
    data = result.read_bytes()
    assert data[65542:65544] == word(stop)
    return data[:65536], data[65536:], elapsed


results = {}
checks = 0
with tempfile.TemporaryDirectory(prefix='font6-screen-merge-') as directory:
    work = Path(directory)
    binary = work/'probe.i80'
    symbols = work/'probe.sym'
    subprocess.run([str(root/'tasm/sjasmplus.exe'),'--i8080',f'--raw={binary}',f'--sym={symbols}',
                    str(report/'probe.asm')], cwd=work, check=True)
    code = binary.read_bytes()
    labels = {name:int(value,16) for name,value in re.findall(
        r'^([a-zA-Z0-9_.]+): EQU 0x([0-9a-fA-F]+)', symbols.read_text(), re.M)}
    rng = random.Random(0x2168)
    samples = [bytes(rng.randrange(256) for _ in range(count)) for count in (4,16,64)]
    for entry,limit,pointers,inverse,group_time in variants:
        start, body_end, stop = (labels[entry+suffix] for suffix in ('','BodyEnd','End'))
        uses_sp_for_text = pointers == 'SP input'
        parks_screen = pointers == 'SP screen'
        for glyph_row in range(8):
            for text in samples if limit else samples[:1]:
                mem = bytearray(65536)
                mem[0x3000:0x3000+len(code)] = code
                mem[0x4000:0x4800] = bytes(font[c*8+y]&63 for y in range(8) for c in range(256))
                mem[0x5000:0x7800] = prerender(font,'full')
                group = mem[start:body_end]
                for page in pages:
                    insn = bytes([0x26 if parks_screen else 0x06, page])
                    assert group.count(insn)==1, (entry,insn)
                    mem[start+group.index(insn)+1] += glyph_row
                text_ptr = 0x86FD
                mem[text_ptr:text_ptr+len(text)] = text
                screen_offset = 0x48 + glyph_row
                mem[0x9000:0xC000] = bytes([0xA5])*0x3000
                expected_column = 0x90+len(text)//4*3
                if limit:
                    mem[labels[limit]+1] = expected_column
                init = (b'\x31'+word(text_ptr if uses_sp_for_text else 0x8F00)
                        +b'\x11'+word(text_ptr)+b'\x21'+word(0x9000+screen_offset)+b'\xC3'+word(start))
                mem[0x8000:0x8000+len(init)] = init
                state, registers, elapsed = run(mem,work,0x8000,stop)
                assert elapsed-40 == group_time*(len(text)//4), (entry,elapsed)
                assert state[0x9000:0xC000] == expected_screen(text,glyph_row,screen_offset,inverse), entry
                assert state[text_ptr:text_ptr+len(text)] == text
                assert registers[4:6] == bytes([screen_offset,expected_column])
                if uses_sp_for_text:
                    assert registers[8:10] == word(text_ptr+len(text))
                else:
                    assert registers[12:14] == word(text_ptr+len(text)-(0 if limit else 1))
                if parks_screen:
                    assert registers[8:10] == word(0x9000+screen_offset+256*(len(text)//4*3-3))
                checks += 1
        results[entry] = dict(cycles_per_group=group_time, body_bytes=body_end-start,
                              stream_bytes=stop-start, screen_reads=0 if parks_screen else 3,
                              screen_writes=3 if parks_screen else 6)
print(f'{checks} pixel, pointer and timing checks passed')
for name, result in results.items():
    print(name, result)
(report/'results.json').write_text(json.dumps({'checks':checks,'variants':results},indent=2)+'\n')
