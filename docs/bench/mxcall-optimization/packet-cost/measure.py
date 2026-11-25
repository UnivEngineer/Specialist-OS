"""Сравнить доставку восьми байт MX_REGS в плоской модели 8080."""
import json
import re
import struct
import subprocess
from pathlib import Path

out = Path(__file__).resolve().parent
root = out.parents[3]
word = struct.Struct('<H').pack
ticks = Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')
symbols = (root/'bin/banked/TRANSPORT.sym').read_text()
quads = int(re.search(r'^bankCopyQuads: EQU 0x(\w+)', symbols, re.M)[1], 16)
dest = 0xffc8
variants = {
    'bankCopyQuads': f'push bc\nex de,hl\nld de,{dest}\nld b,2\ncall {quads}\npop bc',
    'eight_bytes': '\n'.join(['ex de,hl'] + [f'ld a,(hl)\nld ({dest+i}),a'+ ('\ninc hl' if i<7 else '') for i in range(8)]),
    'four_words': ('ld hl,0\nadd hl,sp\nld (restore+1),hl\nex de,hl\nld sp,hl\n'
                   + '\n'.join(f'pop hl\nld ({dest+2*i}),hl' for i in range(4)) + '\nrestore: ld sp,0'),
}
rows = []
for name, variant in variants.items():
    # Общая часть задаёт SP/DE/BC и возвращает DE на начало пакета.
    asm = ('    org 08000h\n    ld sp,08f00h\n    ld de,08501h\n    ld bc,01234h\n'
           + ''.join(('' if ':' in line else '    ')+line+'\n' for line in variant.splitlines()) + f'    ld de,{dest}\nendMeasure:\n'
           + '    push bc\n    push de\n    nop\n')
    (out/'test.asm').write_text(asm)
    subprocess.run([str(root/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow','--raw=test.bin','test.asm'], cwd=out, check=True, capture_output=True)
    code = (out/'test.bin').read_bytes()
    mem = bytearray(65536)
    local = (root/'bin/banked/LOCALBLOCK.bin').read_bytes()
    mem[0xf1c0:0xf1c0+len(local)] = local
    mem[0x8000:0x8000+len(code)] = code
    payload = bytes.fromhex('3412452351349371')
    mem[0x8501:0x8509] = payload
    mem[dest-1:dest+9] = bytes([0xa5])*10
    (out/'memory.bin').write_bytes(mem)
    def run(stop):
        p = subprocess.run([str(ticks),'-m8080','-pc','8000','-end',f'{stop:x}','-counter','100000','-output','out.bin','memory.bin'],cwd=out,capture_output=True,text=True,check=True)
        data = (out/'out.bin').read_bytes()
        assert data[65542:65544] == word(stop)
        return int((p.stdout+p.stderr).strip()), data
    stop = 0x8000+len(code)-3
    total, result = run(stop)
    assert result[dest-1:dest+9] == bytes([0xa5])+payload+bytes([0xa5])
    assert result[0x8501:0x8509] == payload and result[65544:65546] == word(0x8f00)
    _, saved = run(stop+2)
    assert saved[0x8efc:0x8f00] == word(dest)+word(0x1234)
    rows.append(dict(variant=name,cycles=total-30,inlineBytes=len(code)-12))
(out/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
print(rows)
