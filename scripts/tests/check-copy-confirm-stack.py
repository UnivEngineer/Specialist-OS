"""Actual NC/CLI confirmation adapters: stack snapshot, errors and A/CF result."""
from pathlib import Path
import re, subprocess, tempfile

root = Path(__file__).resolve().parents[2]
word = lambda n: n.to_bytes(2, 'little')
symbols = lambda p: {k: int(v, 16) for k, v in re.findall(
    r'^([\w.]+): EQU 0x([\da-fA-F]+)', p.read_text(encoding='utf-8'), re.M)}

with tempfile.TemporaryDirectory(prefix='mxos-confirm-') as directory:
    out = Path(directory)
    for app, entry, body in [('NC', 'ncOverwritePrompt', 'ncOverwriteBody'),
                             ('CLI', 'cliOverwritePrompt', 'cliOverwriteBody')]:
        sy = symbols(root / f'bin/{app}.sym')
        for answer, carry, failure in [(0, 0, False), (1, 0, False),
                                        (2, 0, False), (3, 0, False),
                                        (0x31, 1, False), (0xE2, 1, True)]:
            asm = f''' ORG 09000h
body:
 ld de,0A000h
 ld b,32
loop:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec b
 jp nz,loop
 ld a,{answer}
 {'scf' if carry else 'or a'}
 ret
 DS 09100h-$,0
copy:
 push bc
 push de
 push hl
 inc hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 inc hl
 inc hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 ex de,hl
 ld d,b
 ld e,c
 ld b,32
copyLoop:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec b
 jp nz,copyLoop
 pop hl
 pop de
 pop bc
 xor a
 ret
'''
            (out / 'stub.asm').write_text(asm, encoding='utf-8')
            subprocess.run([str(root/'tasm/sjasmplus.exe'), '--i8080',
                            '--raw='+str(out/'stub.bin'), str(out/'stub.asm')],
                           check=True, capture_output=True)
            memory = bytearray(65536)
            data = (root/f'bin/{app}.i80').read_bytes()
            memory[:len(data)] = data
            data = (out/'stub.bin').read_bytes()
            memory[0x9000:0x9000+len(data)] = data
            memory[sy[body]:sy[body]+3] = b'\xc3'+word(0x9000)
            memory[0xF812:0xF816] = bytes.fromhex('3E 03 B7 C9')
            memory[0xF80F:0xF812] = b'\xc3'+word(0x9100)
            if failure:
                memory[0x9100:0x9104] = bytes.fromhex('3E E2 37 C9')
            payload = bytes(range(32))
            memory[0xD000:0xD020] = payload
            memory[0xA000:0xA020] = bytes([0xA5])*32
            code = (b'\x31'+word(0xB000)+b'\x3e\x01\x21'+word(0xD000)+
                    b'\xcd'+word(sy[entry])+b'\xf5\xe1\x22'+word(0xA100)+
                    b'\x21\x00\x00\x39\x22'+word(0xA102)+b'\xc3'+word(0x82FF))
            memory[0x8200:0x8200+len(code)] = code
            (out/'in.bin').write_bytes(memory)
            subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                            '-m8080', '-pc', '8200', '-end', '82ff', '-counter', '200000',
                            '-output', str(out/'out.bin'), str(out/'in.bin')],
                           check=True, capture_output=True)
            result = (out/'out.bin').read_bytes()
            assert int.from_bytes(result[65542:65544], 'little') == 0x82FF
            assert result[0xA101] == answer and result[0xA100] & 1 == carry
            assert result[0xA102:0xA104] == word(0xB000)
            assert result[0xA000:0xA020] == (bytes([0xA5])*32 if failure else payload), (app, answer, result[0xA000:0xA020].hex(),result[0xAFD0:0xB000].hex())
print('COPY CONFIRM STACK PASS: 12 NC/CLI answer, cancel, copy error and SP cases')
