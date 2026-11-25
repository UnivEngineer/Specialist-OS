"""Сравнить прямой CALL BIOS и три подстановки в автономном стенде 8080."""
import hashlib
import json
from pathlib import Path
import struct
import subprocess
from tempfile import TemporaryDirectory

OUT = Path(__file__).resolve().parent
ROOT = OUT.parents[4]
BIOS = (ROOT/'bin/BIOS.sys').read_bytes()
TICKS = 'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'
DIRECT = {'upper': 0xc875, 'cmp': 0xc427, 'sub': 0xc87e}
INLINE = {
    'upper': '''
        cp 061h
        jp c,.done
        cp 07Bh
        jp c,.latin
        cp 0A3h
        jp c,.middle
        cp 0C0h
        jp c,.done
        cp 0E0h
        jp nc,.done
        add a,020h
        jp .done
.middle: add a,010h
        jp .done
.latin: sub 020h
.done:
''',
    'cmp': '''
        ld a,h
        cp d
        jp nz,.done
        ld a,l
        cp e
.done:
''',
    'sub': '''
        ld a,l
        sub e
        ld l,a
        ld a,h
        sbc a,d
        ld h,a
''',
}


def run(args):
    return subprocess.run(args, capture_output=True, text=True, check=True,
                          creationflags=getattr(subprocess, 'CREATE_NO_WINDOW', 0))


with TemporaryDirectory() as folder:
    work = Path(folder)
    def assemble(body, origin=0x2000):
        src, raw = work/'case.asm', work/'case.bin'
        src.write_text(f'    ORG 0{origin:X}h\nstart:\n'+body+'\n    END\n', encoding='utf8')
        run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', f'--raw={raw}', str(src)])
        return raw.read_bytes()

    image = bytearray(65536)
    image[0xc000:0xc000+len(BIOS)] = BIOS
    def execute(program, stop, expected_sp):
        image[0x2000:0x2000+len(program)] = program
        src, dump = work/'image.bin', work/'dump.bin'
        src.write_bytes(image)
        result = run([TICKS, '-m8080', '-pc', '2000', '-end', f'{stop:x}', '-counter', '2000000',
                      '-output', str(dump), str(src)])
        data = dump.read_bytes()
        assert struct.unpack_from('<H', data, 65542)[0] == stop
        assert struct.unpack_from('<H', data, 65544)[0] == expected_sp
        return int(result.stdout.strip()), data

    codes = {op: assemble(body) for op, body in INLINE.items()}
    samples = []
    verified = {}
    for op, code in codes.items():
        args = ([(a,0x1234,0x2345) for a in range(256)] if op == 'upper' else
                [(0x93,h,d) for h in (0,1,0xff,0x100,0x1234,0x12ff,0x8000,0xffff)
                 for d in (0,1,0xff,0x100,0x1234,0x12ff,0x8000,0xffff)])
        direct = bytes([0xcd])+struct.pack('<H', DIRECT[op])
        traces = {}
        for mode in ('direct', 'inline'):
            # Все входы выполняются последовательно, каждый на одинаковом стеке.
            # В точке 3800h сохраняется независимый снимок восьми байт для каждого входа.
            dump_addr = 0x4000
            step = bytearray()
            for a,h,d in args:
                step += b'\x31'+struct.pack('<H',dump_addr+8)+b'\x01\xcd\xab'
                step += b'\x11'+struct.pack('<H',d)+b'\x21'+struct.pack('<H',h)+bytes([0x3e,a])
                step += b'\xcd\x00\x38'+b'\xf5\xe5\xd5\xc5'
                dump_addr += 8
            # Для подстановки абсолютные JMP ссылаются на адрес её собственной копии.
            body = assemble(INLINE[op]+'    ret\n', 0x3800) if mode == 'inline' else direct+b'\xc9'
            image[0x3800:0x3800+len(body)] = body
            _,data = execute(step,0x2000+len(step),dump_addr-8)
            traces[mode] = data[0x4000:dump_addr]
        assert traces['direct'] == traces['inline'], op
        verified[op] = dict(inputs=len(args), inlineBytes=len(code), callBytes=len(direct),
                            registersSha256=hashlib.sha256(traces['direct']).hexdigest())

        timed = ([('q',0x71,0,0),('A',0x41,0,0),('A0h',0xa0,0,0),('C0h',0xc0,0,0),('E0h',0xe0,0,0)]
                 if op == 'upper' else [('1234h/2345h',0,0x1234,0x2345),('1234h/1235h',0,0x1234,0x1235),
                                        ('1234h/1234h',0,0x1234,0x1234),('2345h/1234h',0,0x2345,0x1234)])
        for label,a,h,d in timed:
            # Инициализация перед измеряемым кодом; её время вычитается отдельно.
            setup = b'\x31\x00\x80\x21'+struct.pack('<H',h)+b'\x11'+struct.pack('<H',d)+bytes([0x3e,a])
            relocated=assemble(INLINE[op],0x2000+len(setup))
            overhead,_=execute(setup,0x2000+len(setup),0x8000)
            row=dict(operation=op,input=label,inlineBytes=len(code),comparedInputs=len(args))
            for mode,body in [('direct',direct),('inline',relocated)]:
                actual,_=execute(setup+body,0x2000+len(setup)+len(body),0x8000)
                row[mode]=actual-overhead
            samples.append(row)
    (OUT/'result.json').write_text(json.dumps(samples,indent=2)+'\n',encoding='utf8')
    (OUT/'verification.json').write_text(json.dumps(dict(biosSha256=hashlib.sha256(BIOS).hexdigest(),
        cpu='8080', validated=['AF','BC','DE','HL','SP','PC'], operations=verified),indent=2)+'\n',encoding='utf8')
    for row in samples:
        print(row)
