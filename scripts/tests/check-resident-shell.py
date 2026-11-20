"""8080 shell resume dispatch, module AF propagation, release and epochs."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/resident-shell-check'
W = lambda value: int(value).to_bytes(2, 'little')


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    s = {key: int(value, 16) for key, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (ROOT / 'bin/banked/KERNEL.sym').read_text('utf-8-sig'), re.M)}
    seed = bytearray(65536)
    binary = (ROOT / 'bin/DOS.sys').read_bytes()
    seed[0x100:0x100+len(binary)] = binary
    for label in ('kdFinish', 'kdError'):
        seed[s[label]] = 0xc9
    # Nested MODULE_CALL/UNLOAD transport stub; the kernel wrapper is real.
    src = f'''
    ORG 0E500h
stub:
    ex (sp),hl
    inc hl
    inc hl
    ex (sp),hl
    push hl
    ld hl,0E600h
    inc (hl)
    pop hl
    ld a,(0E600h)
    cp 1
    jp nz,unload
    ld (0E602h),hl
    push de
    pop hl
    ld bc,6
    add hl,bc
    ld a,(hl)
    ld (0E608h),a
    inc hl
    ld a,(hl)
    ld (0E609h),a
    dec hl
    ld a,(0E604h)
    ld (hl),a
    inc hl
    ld a,(0E605h)
    ld (hl),a
    xor a
    ret
unload:
    ld (0E606h),hl
    xor a
    ret
    END
'''
    (OUT / 'stub.asm').write_text(src)
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080',
                    f'--raw={OUT / "stub.bin"}', str(OUT / 'stub.asm')],
                   check=True, capture_output=True)
    data = (OUT / 'stub.bin').read_bytes()
    seed[0xe500:0xe500+len(data)] = data
    seed[s['mx_callRegs']:s['mx_callRegs']+3] = b'\xc3'+W(0xe500)

    def call(memory, entry):
        memory = bytearray(memory)
        code = b'\x31'+W(0xef70)+b'\xcd'+W(s[entry])+b'\xc3'+W(0xe400)
        memory[0xe000:0xe000+len(code)] = code
        (OUT / 'input.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', 'e000', '-end', 'e400', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        result = (OUT / 'output.bin').read_bytes()
        assert result[65542:65544] == W(0xe400), (entry, 'PC')
        assert result[65544:65546] == W(0xef70), (entry, 'SP')
        return result

    for key in (s['NC_BANK_KEY'], 0):
        memory = bytearray(seed)
        memory[s['residentShellKey']:s['residentShellKey']+2] = W(key)
        absent = call(memory, 'residentShellResume')
        assert absent[65536]&1 and absent[65537] == 0
        assert absent[s['kdRegs']:s['kdRegs']+2] == W(key)
        assert absent[0xe600] == 0
    for slot in (0, 3, 7):
        for module_af in (0, 0x0701):
            memory = bytearray(seed)
            memory[s['residentShellKey']:s['residentShellKey']+2] = W(0x434e)
            memory[s['residentKeys']+slot*2:s['residentKeys']+slot*2+2] = W(0x434e)
            memory[s['MODULE_TABLE']+slot*16:s['MODULE_TABLE']+slot*16+2] = W(123)
            memory[s['kdPage']] = 9
            memory[0xe604:0xe606] = W(module_af)
            result = call(memory, 'residentShellResume')
            assert result[0xe608:0xe60a] == W(0x0100), 'RESTORE input'
            assert result[0xe600] == 2, (slot, 'call/unload count')
            assert result[0xe602:0xe604] == result[0xe606:0xe608] == W(123)
            assert result[s['kdPage']] == 9, 'caller bank'
            assert bool(result[65536]&1) == bool(module_af&1), 'module CF'
            assert result[65537] == module_af>>8, 'module A'
    memory = bytearray(seed)
    memory[s['moduleActor']] = s['MODULE_OWNER_BASE']+2
    memory[s['residentKeys']:s['residentKeys']+6] = W(1)+W(2)+W(3)
    result = call(memory, 'residentRelease')
    assert result[s['residentKeys']:s['residentKeys']+6] == W(1)+W(2)+W(0)
    assert result[s['MODULE_TABLE']+2*16+8] == s['DOS_IO_PAGE']
    for before in (0, 255, 65535, 0xffffff, 0xffffffff):
        memory = bytearray(seed)
        memory[s['cliChangeHook']:s['cliChangeHook']+2] = W(0)
        memory[s['cliChangeEpoch']:s['cliChangeEpoch']+4] = before.to_bytes(4, 'little')
        result = call(memory, 'cliChanged')
        actual = int.from_bytes(result[s['cliChangeEpoch']:s['cliChangeEpoch']+4], 'little')
        assert actual == (before+1)&0xffffffff
    print('Resident shell PASS: absent, all table positions, module A/CF, cleanup, caller bank, 32-bit epochs')


if __name__ == '__main__':
    main()
