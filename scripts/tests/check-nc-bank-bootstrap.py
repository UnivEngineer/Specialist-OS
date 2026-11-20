"""8080: MAIN bootstrap registers/selects CORE and returns before UI startup."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/nc-bank-bootstrap'
W = lambda value: int(value).to_bytes(2, 'little')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    s = {key: int(value, 16) for key, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (ROOT / 'bin/NC_CORE.sym').read_text('utf-8-sig'), re.M)}
    source = '''
    ORG 0E500h
callStub:
    ex (sp),hl
    inc hl
    inc hl
    ex (sp),hl
    ld (0E601h),a
    push bc
    push hl
    ld hl,0E600h
    ld c,(hl)
    inc (hl)
    ld b,0
    ld hl,0E610h
    add hl,bc
    ld a,(0E601h)
    ld (hl),a
    pop hl
    pop bc
    xor a
    ret
    DS 0E550h-$,0
infoStub:
    ld a,2
    or a
    ret
    END
'''
    (OUT / 'stub.asm').write_text(source)
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080',
                    f'--raw={OUT / "stub.bin"}', str(OUT / 'stub.asm')],
                   check=True, capture_output=True)
    memory = bytearray(65536)
    data = (ROOT / 'bin/NC_CORE.i80').read_bytes()
    memory[0x100:0x100+len(data)] = data
    data = (OUT / 'stub.bin').read_bytes()
    memory[0xe500:0xe500+len(data)] = data
    memory[s['mx_callRegs']:s['mx_callRegs']+3] = b'\xc3'+W(0xe500)
    memory[s['mx_memInfo']:s['mx_memInfo']+3] = b'\xc3'+W(0xe550)
    for a, bc, wanted in [(ord('H'), 0x100, [0, 3, 0]),
                           (ord('A'), 0x100, [3, 0]), (2, 0, [5, 3])]:
        memory[0xe600:0xe620] = bytes(32)
        code = b'\x31'+W(0xef70)+b'\x3e'+bytes([a])+b'\x01'+W(bc)
        code += b'\xcd'+W(0x100)+b'\xc3'+W(0xe400)
        memory[0xe000:0xe000+len(code)] = code
        (OUT / 'input.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', 'e000', '-end', 'e400', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        result = (OUT / 'output.bin').read_bytes()
        assert result[65542:65544] == W(0xe400), 'entry did not return'
        assert result[65544:65546] == W(0xef70), 'entry stack'
        assert not result[65536]&1, 'entry CF'
        assert result[0xe600] == len(wanted), list(result[0xe610:0xe620])
        assert list(result[0xe610:0xe610+len(wanted)]) == wanted
        assert result[s['ncBankRegistered']] == 1
        assert result[s['ncBankNumber']] == 2
        assert result[s['ncBankInitialized']] == 0, 'UI entered before MAIN launcher returned'
        assert result[s['ncGfxPath']] == ord('H'), 'resume changed the launch drive'
        memory = bytearray(result[:65536])
    # The transport stub leaves the prepared epoch reply in the CORE buffer.
    # A CLI history write on B: must preserve cached roots A: and H:.
    catalogs = b''.join(bytes([1, drive])+W(0)+bytes([3, 2, age, 0])
                        for age, drive in enumerate((0, 7, 1)))
    memory[s['ncCatalogs']:s['ncCatalogs']+24] = catalogs
    for label, value in [('state.panelA_drive', 0), ('state.panelB_drive', 7),
                         ('vars.cacheSlotA', 0), ('vars.cacheSlotB', 1),
                         ('vars.cacheValidA', 1), ('vars.cacheValidB', 1),
                         ('vars.pathValidA', 1), ('vars.pathValidB', 1)]:
        memory[s[label]] = value
    memory[s['ncBankEpoch']:s['ncBankEpoch']+32] = bytes(32)
    memory[s['ncBankNewEpoch']:s['ncBankNewEpoch']+32] = bytes(32)
    for drive in (1, 7):
        memory[s['ncBankNewEpoch']+drive*4] = 1
        code = b'\x31'+W(0xef70)+b'\xcd'+W(s['ncBankCheckEpoch'])+b'\xc3'+W(0xe400)
        memory[0xe000:0xe000+len(code)] = code
        (OUT / 'input.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', 'e000', '-end', 'e400', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        result = (OUT / 'output.bin').read_bytes()
        assert result[65542:65544] == W(0xe400), 'epoch PC'
        assert result[s['ncCatalogs']] == result[s['vars.cacheValidA']] == 1
        assert result[s['vars.cacheValidB']] == (1 if drive == 1 else 0)
        assert result[s['ncCatalogs']+16] == 0, 'B cache not invalidated'
        memory = bytearray(result[:65536])
    print('NC bank bootstrap PASS: registration, CLOSE, drive/stack, per-drive cache invalidation')


if __name__ == '__main__':
    main()
