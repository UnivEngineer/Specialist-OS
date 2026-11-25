"""CPU 8080: владельцы MAIN, тёплый сброс, FILL и статистика арены."""
from pathlib import Path
from dos_state import initialize_dos_state
import re
import runpy
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def symbols(path):
    return {n: int(v, 16) for n, v in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)', path.read_text('utf-8'), re.M)}


def main():
    bios = symbols(ROOT / 'bin/BIOS.sym')
    kernel = symbols(ROOT / 'bin/banked/KERNEL.sym')
    names = ('memoryInit', 'moduleInit', 'moduleWarm', 'dosMemoryService', 'kdPage',
             'appEnter', 'appLeave', 'appOwner', 'memoryMainStart',
             'moduleActor', 'residentKeys', 'residentActorCheck',
             'memoryRamOwner',
             'memoryRamPages', 'memoryRamFresh', 'diActive')
    imports = '\n'.join(f'{n} = 0{kernel[n]:04X}h' for n in names)
    body = '''
    call memoryInit
    call moduleInit
    xor a
    ld (kdPage),a
    call appEnter
    ; Flat CPU model: MAIN arena must not overwrite the banked DOS image.
    ld hl,08400h
    ld de,00800h
    ld b,0
    ld a,MEM_OP_ARENA
    call dosMemoryService
    jp c,fail
    ld hl,32
    ld b,0
    ld a,MEM_OP_ALLOC
    call dosMemoryService
    jp c,fail
    ld (08510h),hl
    call appEnter
    ld hl,08400h
    ld b,0
    ld a,MEM_OP_FREE
    call dosMemoryService
    jp nc,fail
    cp MEM_ERROR_OWNER
    jp nz,fail
    call appLeave
    ld hl,(memoryMainStart)
    ld (08512h),hl
    ld hl,08400h
    ld de,32
    ld bc,000A5h
    ld a,MEM_OP_FILL
    call dosMemoryService
    jp c,fail
    ld b,0
    ld a,MEM_OP_QUERY
    call dosMemoryService
    jp c,fail
    ld (08514h),hl
    ex de,hl
    ld (08516h),hl
    ld a,c
    ld (08518h),a
    ; Неверный конец не должен записывать даже первый байт.
    ld hl,MEM_DATA_LIMIT-1
    ld de,2
    ld bc,00099h
    ld a,MEM_OP_FILL
    call dosMemoryService
    jp nc,fail
    cp MEM_ERROR_RANGE
    jp nz,fail
    call appLeave
    call appEnter
    ld hl,08400h
    ld de,00800h
    ld b,0
    ld a,MEM_OP_ARENA
    call dosMemoryService
    jp c,fail
    ld hl,32
    ld b,0
    ld a,MEM_OP_ALLOC
    call dosMemoryService
    jp c,fail
    call moduleWarm
    ld hl,08400h
    ld de,00800h
    ld b,0
    ld a,MEM_OP_ARENA
    call dosMemoryService
    jp c,fail
    ; Потомок резидента не должен получать MAIN-арену.
    ld hl,04452h
    ld (residentKeys),hl
    ld a,MODULE_OWNER_BASE
    ld (MODULE_TABLE+MODULE_RECORD_SIZE+8),a
    inc a
    ld (moduleActor),a
    call residentActorCheck
    jp z,fail
    ld hl,08400h
    ld de,00800h
    ld b,0
    ld a,MEM_OP_ARENA
    call dosMemoryService
    jp c,fail
    ; Владение пулом DOS не требует модуля и сохраняется после очистки.
    ld a,4
    ld (memoryRamPages),a
    ld a,DOS_BANK_PAGE
    ld (memoryRamOwner),a
    ld a,1
    ld (memoryRamFresh),a
    xor a
    ld (diActive),a
    ld c,MEM_RAMDISK_STATUS
    ld a,MEM_OP_RAMDISK
    call dosMemoryService
    jp c,fail
    ld a,b
    cp 4
    jp nz,fail
    ld a,c
    cp 1
    jp nz,fail
    ld a,d
    dec a
    jp nz,fail
    ld a,e
    cp RAM_DRIVE
    jp nz,fail
    ld c,MEM_RAMDISK_COMMIT
    ld a,MEM_OP_RAMDISK
    call dosMemoryService
    jp nc,fail
    cp MEM_ERROR_OWNER
    jp nz,fail
    call moduleWarm
    ld a,(memoryRamOwner)
    cp DOS_BANK_PAGE
    jp nz,fail
    ld a,(memoryRamFresh)
    dec a
    jp nz,fail
    ; Reset сохраняет резидентный DOS-слот и освобождает временную арену.
    call moduleInit
    ld hl,dosRecords
    ld de,MODULE_TABLE
    ld b,2*MODULE_RECORD_SIZE
copyDosRecords:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,copyDosRecords
    ld hl,04452h
    ld (residentKeys),hl
    call moduleWarm
    ld hl,(MODULE_TABLE)
    ld a,h
    or l
    jp z,fail
    ld hl,(MODULE_TABLE+MODULE_RECORD_SIZE)
    ld a,h
    or l
    jp nz,fail
    ld hl,0
    ld (residentKeys),hl
    call moduleWarm
    ld hl,(MODULE_TABLE)
    ld a,h
    or l
    jp nz,fail
    ld hl,055AAh
    ld (08500h),hl
    jp 08400h
fail:
    push af
    pop hl
    ld (08500h),hl
    jp 08400h
dosRecords:
    DW 1
    DB 0,DOS_BANK_PAGE
    DW DOS_ARENA_START,0
    DB 0,MODULE_OWNER_BASE
    DW DOS_ARENA_START,DOS_ARENA_START+1
    DW 0
    DW 2
    DB 0,DOS_BANK_PAGE
    DW DOS_BANK_DRIVER_ADDR,0
    DB 0,MODULE_OWNER_BASE+1
    DW DOS_BANK_DRIVER_ADDR,DOS_BANK_DRIVER_ADDR+1
    DW 0
'''
    branches = []
    def mark(match):
        branches.append(match.group(0))
        return match.group(0).replace('fail', 'fail_' + str(len(branches)))
    body = re.sub(r'jp (?:n?[zc]),fail', mark, body)
    marks = ''.join(f'fail_{index}:\n ld hl,{index}\n ld (0851Ch),hl\n jp fail\n'
                    for index in range(1, len(branches)+1))
    body += marks
    with tempfile.TemporaryDirectory(prefix='mxos-memory-') as folder:
        out = Path(folder)
        src = imports + f'\n INCLUDE "{(ROOT / "source/include/mxos.inc").as_posix()}"\n ORG 0B800h\n ld sp,08FDFh\n' + body + '\n END\n'
        (out / 'case.asm').write_text(src, encoding='utf-8')
        subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', f'--raw={out / "case.bin"}', str(out / 'case.asm')], check=True, capture_output=True)
        memory = bytearray(65536)
        runpy.run_path(str(ROOT / 'scripts/tests/mem-image.py'))['install'](memory, bios)
        data = (ROOT / 'bin/DOS.sys').read_bytes()
        memory[0x100:0x100 + len(data)] = data
        initialize_dos_state(memory, kernel)
        code = (out / 'case.bin').read_bytes()
        memory[0xb800:0xb800 + len(code)] = code
        (out / 'in.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe', '-m8080', '-pc', 'b800', '-end', '8400', '-counter', '10000000', '-output', str(out / 'out.bin'), str(out / 'in.bin')], check=True, capture_output=True, timeout=20)
        result = (out / 'out.bin').read_bytes()
        assert result[0x8500:0x8502] == bytes.fromhex('aa55'), (result[0x8500:0x8502].hex(), int.from_bytes(result[0x851c:0x851e], 'little'))
        assert result[0x8400:0x8420] == bytes([0xA5]) * 32
        boundary = kernel['MEM_DATA_LIMIT']
        assert result[boundary-1:boundary+1] == memory[boundary-1:boundary+1]
        assert int.from_bytes(result[0x8514:0x8516], 'little') == 2048 - 32
        assert result[0x8514:0x8516] == result[0x8516:0x8518]
        assert result[0x8518] == 31
        print('PASS: MAIN owners, nested arena, cleanup, warm reset, DOS slots, FILL, QUERY')


if __name__ == '__main__':
    main()
