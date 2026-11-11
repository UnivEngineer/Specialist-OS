"""Полная печать NC ограничена окном, независимо от входного регистра B."""
import re
import struct
import subprocess
from pathlib import Path

R = Path(__file__).resolve().parents[2]
O = R/'.local/nc-file-count'
O.mkdir(parents=True, exist_ok=True)
n = {a:int(b,16) for a,b in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)', (R/'bin/NC.sym').read_text('utf-8'), re.M)}
source = O/'record.asm'
source.write_text('''    ORG 07000h
    push hl
    push de
    push bc
    ld b,h
    ld c,l
    ld hl,(08102h)
    ld (hl),c
    inc hl
    ld (hl),b
    inc hl
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (08102h),hl
    ld hl,08100h
    inc (hl)
    pop bc
    pop de
    pop hl
    ret
''', encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'), '--i8080', '--raw='+str(O/'record.bin'), str(source)], check=True, capture_output=True)
for count in (0, 1, 23, 38, 100, 255):
    for initial_b in (0, 1, 37, 255):
        memory = bytearray(65536)
        nc = (R/'bin/NC.i80').read_bytes()
        memory[:len(nc)] = nc
        stub = (O/'record.bin').read_bytes()
        memory[0x7000:0x7000+len(stub)] = stub
        memory[n['printPanelFile']:n['printPanelFile']+3] = bytes.fromhex('C30070')
        for i in range(count):
            memory[0x5000+i] = i
        memory[0x5000+count] = 255
        struct.pack_into('<H', memory, n['vars.fileListPtr'], 0x5000)
        struct.pack_into('<H', memory, 0x8102, 0x8200)
        code = bytes.fromhex('31008F06')+bytes((initial_b,))+b'\xC3'+struct.pack('<H', n['printWholeFileList'])
        memory[0x8000:0x8000+len(code)] = code
        (O/'in.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe', '-m8080', '-pc', '8000', '-end', f"{n['clearPanelTail']:x}", '-counter', '1000000', '-output', str(O/'out.bin'), str(O/'in.bin')], check=True, capture_output=True)
        result = (O/'out.bin').read_bytes()
        expected = min(count, n['FILE_LIST_SIZE'])
        assert result[0x8100] == expected, (count, initial_b, result[0x8100])
        for i in range(expected):
            pointer, pos = struct.unpack_from('<HH', result, 0x8200+i*4)
            col = n['P_FILE_LIST_X1'] if i < n['P_FILE_LIST_H'] else n['P_FILE_LIST_X2']
            assert pointer == 0x5000+i
            assert pos == (col << 8)+n['P_FILE_LIST_Y']+(i % n['P_FILE_LIST_H'])*10
print('NC FILE COUNT PASS: 24 cases, both columns and last short-list entry')

# Курсор после прокрутки: обычная соседняя строка, затем инверсная новая.
text = source.read_text('utf-8').replace('    ld (08102h),hl',
    f'''    ld a,({n['bios_vars.inverse']})
    ld (hl),a
    inc hl
    ld a,({n['bios_vars.inverse']+1})
    ld (hl),a
    inc hl
    ld (08102h),hl''')
source.write_text(text, encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'), '--i8080', '--raw='+str(O/'record.bin'), str(source)], check=True, capture_output=True)
stub = (O/'record.bin').read_bytes()
memory[0x7000:0x7000+len(stub)] = stub
memory[0x8100] = 0
struct.pack_into('<H', memory, 0x8102, 0x8200)
struct.pack_into('<H', memory, n['bios_vars.inverse'], 0)
code = bytes.fromhex('31008F21005011')+struct.pack('<H', (n['P_FILE_LIST_X2'] << 8)+n['P_FILE_LIST_Y_MAX']-10)+b'\xCD'+struct.pack('<H',n['printKeptScrollFile'])+bytes.fromhex('C3FF80')
memory[0x8000:0x8000+len(code)] = code
(O/'in.bin').write_bytes(memory)
subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe', '-m8080', '-pc', '8000', '-end', '80ff', '-counter', '1000000', '-output', str(O/'out.bin'), str(O/'in.bin')], check=True, capture_output=True)
result = (O/'out.bin').read_bytes()
assert result[0x8100] == 2
assert result[0x8204:0x8206] == b'\0\0'
assert result[0x820A:0x820C] == b'\xff\xff'
assert result[n['bios_vars.inverse']:n['bios_vars.inverse']+2] == b'\0\0'
print('NC SCROLL INVERSE PASS: 0000h, FFFFh, restored 0000h')
