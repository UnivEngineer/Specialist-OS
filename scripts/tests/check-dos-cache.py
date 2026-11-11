"""Кэш DOS на настоящем коде 8080: геометрии, отдельный хвост и отказы I/O."""
import argparse
import json
import re
import struct
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
W = lambda n: struct.pack('<H', n)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, default=ROOT/'docs/diskbench/cache-2026-11-09/cpu')
    args = parser.parse_args()
    out = args.report.resolve()
    out.mkdir(parents=True, exist_ok=True)
    s = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)',
                                           (ROOT/'bin/banked/KERNEL.sym').read_text('utf-8'), re.M)}
    rows = []
    for size, shift in ((128, 7), (256, 8), (512, 9)):
        source = out/'mock.asm'
        source.write_text(f'''    ORG 08000h
    ld a,(08102h)
    or a
    jp nz,failed
    push hl
    push de
    push bc
    ld a,b
    ld (08103h),a
    ld b,h
    ld c,l
    ld h,0
    ld l,e
    {'\n    '.join(['add hl,hl']*shift)}
    ld de,0C000h
    add hl,de
    ld a,(08103h)
    cp 2
    jp nz,write
    ld a,(08100h)
    inc a
    ld (08100h),a
    ld d,b
    ld e,c
    jp copy
write:
    ld a,(08101h)
    inc a
    ld (08101h),a
    ex de,hl
    ld h,b
    ld l,c
copy:
    ld bc,{size}
loop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,loop
    pop bc
    pop de
    pop hl
    ret
failed:
    scf
    ret
''', encoding='utf-8')
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '--raw='+str(out/'mock.bin'), str(source)], check=True, capture_output=True)
        memory = bytearray(65536)
        kernel = (ROOT/'bin/banked/KERNEL.bin').read_bytes()
        memory[0x100:0x100+len(kernel)] = kernel
        memory[0x8000:0x8000+(out/'mock.bin').stat().st_size] = (out/'mock.bin').read_bytes()
        memory[s['j_diskDriver']+1:s['j_diskDriver']+3] = W(0x8000)
        for name in ('sectorSize', 'blockSize'):
            memory[s['v_diskInfo.'+name]:s['v_diskInfo.'+name]+2] = W(size)
        memory[s['v_diskInfo.fatCount']] = 1
        memory[s['v_logicalCapacity']:s['v_logicalCapacity']+2] = W(128)
        memory[s['v_logicalShift']] = 0
        payload = bytes((i*13+(i//size)*7)&255 for i in range((8192//size+8)*size))
        memory[0xC000:0xC000+len(payload)] = payload

        def call(name, sector=0, dirty=0, error=False, slot=None):
            nonlocal memory
            # Результат AF не пересекается со счётчиками mock по 8100h.
            code = b'\x31'+W(0xF480)+(b'\x21'+W(slot) if slot is not None else b'')+b'\x11'+W(sector)+bytes((0x3E, dirty, 0xCD))+W(s[name])+bytes.fromhex('22 06 81 F5 E1 22 04 81 C3 FF 82')
            memory[0x8200:0x8200+len(code)] = code
            (out/'in.bin').write_bytes(memory)
            subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe', '-m8080', '-pc', '8200', '-end', '82ff', '-counter', '5000000', '-output', str(out/'out.bin'), str(out/'in.bin')], check=True, capture_output=True)
            result = (out/'out.bin').read_bytes()
            assert int.from_bytes(result[65542:65544], 'little') == 0x82FF
            memory = bytearray(result[:65536])
            assert bool(memory[0x8104]&1) == error, (name, sector, size)

        call('resetAllSectorCaches')
        memory[s['v_drive']] = 0
        memory[s['v_diskInfo.sectorShift']] = shift
        call('configureSectorCache')
        assert memory[s['v_cacheCount']] == 64
        call('getSectorAddressInCache', 1)
        assert memory[0x9000:0x9000+size] == payload[size:2*size]
        call('getSectorAddressInCache', 1)
        assert memory[0x8100] == 1
        call('cachePartialSector', 2, 1)
        assert memory[s['DOS_BANK_PARTIAL_ADDR']:s['DOS_BANK_PARTIAL_ADDR']+size] == payload[2*size:3*size]
        assert int.from_bytes(memory[s['cacheData']:s['cacheData']+2], 'little') == 1
        memory[s['DOS_BANK_PARTIAL_ADDR']:s['DOS_BANK_PARTIAL_ADDR']+size] = b'X'*size
        call('cachePartialSector', 3, 1)
        assert memory[0xC000+2*size:0xC000+3*size] == b'X'*size
        memory[s['DOS_BANK_PARTIAL_ADDR']:s['DOS_BANK_PARTIAL_ADDR']+size] = b'Y'*size
        memory[0x8102] = 1
        call('cachePartialSector', 4, error=True)
        partial = s['partialCacheData']
        assert memory[partial:partial+2] == W(3) and memory[partial+6] == 1
        memory[0x8102] = 0
        call('saveSectorCache')
        assert memory[0xC000+3*size:0xC000+4*size] == b'Y'*size
        assert memory[partial+6] == 0
        # Полный прямой обмен сначала сохраняет частичный dirty-сектор,
        # затем инвалидирует ровно его описатель (не соседнюю память).
        memory[partial+6] = 1
        memory[0x8102] = 1
        call('streamDirectCached', 3, error=True, slot=partial)
        assert memory[partial:partial+2] == W(3) and memory[partial+6] == 1
        memory[0x8102] = 0
        after = bytes(memory[partial+8:partial+24])
        call('streamDirectCached', 3, slot=partial)
        assert memory[partial:partial+2] == W(65535) and memory[partial+7] == 255
        assert bytes(memory[partial+8:partial+24]) == after
        memory[0x8102] = 1
        call('cachePartialSector', 4, error=True)
        assert memory[partial:partial+2] == W(65535)
        memory[0x8102] = 0
        call('cachePartialSector', 4)
        assert memory[s['DOS_BANK_PARTIAL_ADDR']:s['DOS_BANK_PARTIAL_ADDR']+size] == payload[4*size:5*size]
        call('getSectorAddressInCache', 1)
        assert memory[0x9000:0x9000+size] == payload[size:2*size]
        # Один диск может занять весь пул, а не только прежние 4 КиБ.
        call('resetAllSectorCaches')
        call('configureSectorCache')
        capacity = 8192//size
        for sector in range(capacity):
            call('getSectorAddressInCache', sector)
        reads = memory[0x8100]
        for sector in range(capacity):
            call('getSectorAddressInCache', sector)
        assert memory[0x8100] == reads

        def select(drive, geometry=size):
            call('saveSectorCache')
            memory[s['v_drive']] = drive
            call('selectCacheForDrive')
            memory[s['v_diskInfo.sectorShift']] = geometry.bit_length()-1
            memory[s['v_diskInfo.sectorSize']:s['v_diskInfo.sectorSize']+2] = W(geometry)
            memory[s['v_logicalShift']] = (geometry//size).bit_length()-1
            call('configureSectorCache')

        # Одинаковые LBA разных дисков не должны совпасть в поиске.
        select(1)
        memory[0xC000:0xC000+size] = b'B'*size
        call('getSectorAddressInCache', 0)
        pointer = int.from_bytes(memory[0x8106:0x8108],'little')
        assert memory[pointer:pointer+size] == b'B'*size
        reads = memory[0x8100]
        select(0)
        call('getSectorAddressInCache', 1)
        assert memory[0x8100] == reads
        select(1)
        call('getSectorAddressInCache', 0)
        assert memory[0x8100] == reads
        assert memory[pointer:pointer+size] == b'B'*size
        select(0)
        call('invalidateDriveCache', dirty=1)
        call('getSectorAddressInCache', 1)
        assert memory[0x8100] == reads
        select(1)
        call('getSectorAddressInCache', 0)
        assert memory[0x8100] == (reads+1)&255
        # Три диска также делят пул: отдельного лимита двух контекстов нет.
        select(7)
        call('getSectorAddressInCache', 0)
        select(1)
        reads = memory[0x8100]
        call('getSectorAddressInCache', 0)
        assert memory[0x8100] == reads
        # Грязный сектор при отказе сохраняется у прежнего диска.
        call('getSectorAddressInCache', 0, 1)
        memory[0x8102] = 1
        call('fileSelectDrive', dirty=0, error=True)
        assert memory[s['v_drive']] == 1 and memory[s['v_cacheDrive']] == 1
        memory[0x8102] = 0
        call('saveSectorCache')
        if size == 128:
            # Смесь секторов 128 и 512 байт: вытеснение охватывает целые головы.
            for iteration in range(120):
                select(iteration%2, 512 if iteration%2 else 128)
                call('getSectorAddressInCache', (iteration//2)%24)
                pointer = int.from_bytes(memory[0x8106:0x8108],'little')
                geometry = 512 if iteration%2 else 128
                start = ((iteration//2)%24)*geometry
                assert memory[pointer:pointer+geometry] == memory[0xC000+start:0xC000+start+geometry]
                spans=[]
                for index in range(64):
                    slot=s['cacheData']+index*8
                    tag=memory[slot+7]
                    if tag >= 128: continue
                    address=int.from_bytes(memory[slot+2:slot+4],'little')
                    length=128 << (tag>>3)
                    spans.append((address,address+length))
                spans.sort()
                assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:])),spans
        # Отказ вытеснения грязной головы не должен терять ни одну строку.
        call('resetAllSectorCaches')
        memory[s['v_drive']]=0
        memory[s['v_diskInfo.sectorSize']:s['v_diskInfo.sectorSize']+2]=W(size)
        memory[s['v_diskInfo.sectorShift']]=shift
        memory[s['v_logicalShift']]=0
        call('configureSectorCache')
        for sector in range(capacity):
            call('getSectorAddressInCache',sector,1)
        numbers=bytes(memory[s['cacheData']:s['cacheData']+512])
        memory[0x8102]=1
        call('getSectorAddressInCache',capacity,error=True)
        for index in range(64):
            slot=s['cacheData']+index*8
            assert memory[slot:slot+4] == numbers[index*8:index*8+4]
            assert memory[slot+6:slot+8] == numbers[index*8+6:index*8+8]
        memory[0x8102]=0
        call('saveSectorCache')
        memory[0x8102]=1
        call('getSectorAddressInCache',capacity,error=True)
        memory[0x8102]=0
        call('getSectorAddressInCache',capacity)
        rows.append(dict(sectorBytes=size, slots=8192//size, dirtyEviction=True, failedWriteRetained=True, failedReadInvalidated=True, metadataPreserved=True, twoDisks=True, thirdDiskShared=True, inactiveInvalidation=True, geometryChange=True, failedSwitchRetained=True))
    (out/'result.json').write_text(json.dumps(rows, indent=2)+'\n', encoding='utf-8')
    print('DOS CACHE PASS:', ', '.join(str(r['sectorBytes']) for r in rows))


if __name__ == '__main__':
    main()
