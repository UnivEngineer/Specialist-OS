"""Проверка FAT12 в банковой DOS на процессоре 8080."""
import argparse
from pathlib import Path
import re
import struct
import subprocess
from tempfile import TemporaryDirectory

from dos_state import initialize_dos_state


ROOT = Path(__file__).resolve().parents[2]
WORD = struct.Struct('<H')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ticks', type=Path, default=Path(
        'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    args = parser.parse_args()
    with TemporaryDirectory() as directory:
        check(Path(directory), args.ticks)


def check(temp, ticks):
    bank = ROOT / 'bin/banked'
    sy = {name: int(value, 16) for name, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (bank / 'KERNEL.sym').read_text(encoding='utf-8'), re.M)}
    mock = temp / 'driver.asm'
    mock.write_text('''    ORG 09D00h
    ld a,b
    cp 3
    jp z,info
    cp 4
    ret z
    ld a,(09F07h)
    cp e
    jp z,failed
    push hl
    push bc
    push de
    push hl
    ld a,e
    cp 8
    jp c,span
    ld a,7
span:
    ld h,a
    ld l,0
    add hl,hl
    ld de,0A000h
    add hl,de
    ex de,hl
    pop hl
    ld a,b
    cp 1
    jp z,copy
    ex de,hl
copy:
    ld bc,512
loop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,loop
    pop de
    pop bc
    pop hl
    ret
info:
    ld de,1440
    ld bc,0
    xor a
    ret
failed:
    scf
    ret
''', encoding='utf-8')
    driver = mock.with_suffix('.bin')
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080',
                    f'--raw={driver}', str(mock)], check=True, capture_output=True)

    memory = bytearray(65536)
    for address, path in [(sy['DOS_CODE_START'], bank / 'KERNEL.bin'),
                          (0x9D00, driver)]:
        image = path.read_bytes()
        memory[address:address + len(image)] = image
    initialize_dos_state(memory, sy)
    floppy = sy['FLOPPY_DRIVE']
    memory[sy['kernelFloppyDrive']] = floppy
    memory[sy['v_drive']] = floppy
    for drive in (sy['RAM_DRIVE'], floppy):
        memory[sy['v_blockShifts'] + drive] = 9
        WORD.pack_into(memory, sy['v_drives'] + drive * 2, 0x9D00)
    WORD.pack_into(memory, sy['j_diskDriver'] + 1, 0x9D00)
    memory[0x9F07] = 255

    def call(label, *, hl=0, de=0, a=0, error=False):
        code = (b'\x31' + WORD.pack(sy['DOS_BANK_STACK_TOP']) + b'\x21' + WORD.pack(hl) +
                b'\x11' + WORD.pack(de) + bytes((0x3E, a, 0xCD)) + WORD.pack(sy[label]) +
                bytes.fromhex('22 00 9F EB 22 02 9F F5 E1 22 04 9F C3 FF B1'))
        memory[0xB100:0xB100 + len(code)] = code
        input_path, output_path = temp / 'in.bin', temp / 'out.bin'
        input_path.write_bytes(memory)
        subprocess.run([str(ticks), '-m8080', '-pc', 'b100', '-end', 'b1ff',
                        '-counter', '5000000', '-output', str(output_path), str(input_path)],
                       capture_output=True, check=True)
        result = output_path.read_bytes()
        assert WORD.unpack_from(result, 65542)[0] == 0xB1FF, label
        memory[:] = result[:65536]
        assert bool(memory[0x9F04] & 1) == error, (label, memory[0x9F05])
        return WORD.unpack_from(memory, 0x9F00)[0], WORD.unpack_from(memory, 0x9F02)[0]

    boot = memoryview(memory)[0xA000:0xA200]
    struct.pack_into('<HBHBHHBH', boot, 11, 512, 2, 1, 2, 112, 1440, 0xF9, 3)
    WORD.pack_into(boot, 510, 0xAA55)
    fat = bytearray(1536)
    fat[:3] = b'\xF9\xFF\xFF'

    def set_entry(cluster, value):
        address, shift = cluster + cluster // 2, 4 * (cluster & 1)
        before = WORD.unpack_from(fat, address)[0]
        WORD.pack_into(fat, address, (before & ~(0xFFF << shift)) | value << shift)

    for cluster, value in [(340, 333), (341, 342), (342, 0xFFF),
                           (682, 0xFF7), (683, 684)]:
        set_entry(cluster, value)
    memory[0xA200:0xAE00] = fat * 2
    call('resetAllSectorCaches')
    WORD.pack_into(boot, 22, 2)
    call('mountVolume', error=True)
    assert memory[sy['v_dosError']] == sy['DOS_ERR_UNSUPPORTED']
    WORD.pack_into(boot, 22, 3)
    memory[0xA201] = 0
    call('mountVolume', error=True)
    assert memory[sy['v_dosError']] == sy['DOS_ERR_CORRUPT']
    memory[0xA201] = 255
    call('mountVolume')
    for field, expected in [('dataClusters', 713), ('fatNumCells', 1024),
                            ('dirStartSector', 7), ('dataStartSector', 14)]:
        assert WORD.unpack_from(memory, sy[f'v_diskInfo.{field}'])[0] == expected
    assert call('getDriveFreeSpace')[1] == 708

    # Чтение второй половины при ошибке не меняет ни данные, ни dirty первой.
    for address in (sy['v_directory'], sy['v_directories'] + 2 * floppy):
        WORD.pack_into(memory, address, 350)
    call('mountVolume')
    for address in (sy['v_directory'], sy['v_directories'] + 2 * floppy):
        assert WORD.unpack_from(memory, address)[0] == 0
    memory[0x9F07] = 2
    call('fatWriteCluster', hl=341, de=0xFFFF, error=True)
    slot = WORD.unpack_from(memory, sy['v_cacheLastSlot'])[0]
    assert memory[slot + sy['FAT_CACHE_DATA.reference']] == 1
    assert memory[slot + sy['FAT_CACHE_DATA.isModified']] == 0
    cache_address = WORD.unpack_from(memory, slot + sy['FAT_CACHE_DATA.address'])[0]
    assert memory[cache_address + 511] == fat[511]
    memory[0x9F07] = 255

    # Заполнить CLOCK так, чтобы первая половина стала первой жертвой.
    for sector in range(7, 22):
        call('getSectorAddressInCache', de=sector)
    memory[sy['v_cacheClock']] = (slot - sy['cacheData']) // sy['FAT_CACHE_DATA']
    assert call('fatReadCluster', de=341)[1] == 342
    assert call('fatReadCluster', de=342)[1] == 0xFFFF
    assert call('fatReadCluster', de=682)[1] == 0xFFF7
    assert call('fatWriteCluster', hl=341, de=0xFFFF)[0] == 0xFFFF
    assert call('fatFreeCluster', de=682)[1] == 0xFFF7
    assert call('fatReadCluster', de=340)[1] == 333
    assert call('fatReadCluster', de=683)[1] == 684
    assert call('getDriveFreeSpace')[1] == 709
    call('flushVolume')
    set_entry(341, 0xFFF)
    set_entry(682, 0)
    assert memory[0xA200:0xAE00] == fat * 2

    # При повторном выборе A: не использовать чистые строки прежней дискеты.
    call('fileSelectDrive', a=1)
    set_entry(341, 343)
    memory[0xA200:0xAE00] = fat * 2
    call('fileSelectDrive', a=floppy)
    assert call('fatReadCluster', de=341)[1] == 343
    print('FAT12: BPB, count, packed entries, CLOCK, I/O failure, mirrored flush and media swap verified')


if __name__ == '__main__':
    main()
