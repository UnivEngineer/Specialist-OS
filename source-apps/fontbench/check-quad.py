"""Независимый 8080: четверки, прямой вывод и защита трех живых записей."""
from pathlib import Path
import argparse
import json
import subprocess
import re
import struct

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/font-matrix-001/cpu')
    parser.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    sym = lambda name: {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (ROOT / 'bin' / f'{name}.sym').read_text(), re.M)}
    s, io = sym('FONTCOM'), sym('IO')
    font = (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
    packed = (ROOT / 'bin/FONTKOI.packed.bin').read_bytes()
    packed = bytearray(packed)
    for c in range(256):
        struct.pack_into('<H', packed, 32 + c * 2, struct.unpack_from('<H', packed, 32 + c * 2)[0] + 0xdb90)
    base = bytearray((i * 37 + (i >> 8) * 13) & 255 for i in range(65536))
    for name, address in [('FONTCOM.i80', 0), ('BIOS.sys', 0xc000), ('IO.sys', 0xf000)]:
        data = (ROOT / 'bin' / name).read_bytes()
        base[address:address + len(data)] = data
    word = lambda value: value.to_bytes(2, 'little')
    def put(m, name, value):
        m[s[name]:s[name] + 2] = word(value)
    base[0xf806:0xf80d] = b'\xe1\x23\x23\xe5\xc3' + word(io['glyphCacheService'])
    put(base, 'GLYPH_CACHE_START', 0xdb90)
    put(base, 'GLYPH_CACHE_ENTRIES', 0)
    put(base, 'bios_vars.charGen', 0x2800 // 8)
    base[s['bios_vars.escMode']] = 255
    base[s['bios_vars.koi7']] = 0
    base[s['bios_vars.inverse']] = 0
    base[s['GLYPH_CACHE_INVALID']] = 1
    # Развёрнутое ядро: источники через DE/HL, экран только STAX B.
    # Разрешённые инструкции исключают чтение экрана, маски, CALL и стек.
    kernel = base[s['quadPixels']:s['quadPixelsEnd']]
    offset = 0
    stores = 0
    while offset < len(kernel):
        opcode = kernel[offset]
        assert opcode in (0x2a, 0xeb, 0x1a, 0xb6, 0x02, 0x1c, 0x2c, 0x0c, 0x04, 0x79, 0xd6, 0x4f), hex(opcode)
        stores += opcode == 0x02
        offset += 3 if opcode == 0x2a else 2 if opcode == 0xd6 else 1
    assert stores == 24 and offset == len(kernel)
    # Новые ядра читают четыре указателя POP HL; экран только STAX D.
    for begin, end in [('quadDirectPixels', 'quadDirectPixelsEnd'), ('quadHalfPixels', 'quadHalfPixelsEnd')]:
        code = base[s[begin]:s[end]]
        offset = stores = pops = 0
        while offset < len(code):
            opcode = code[offset]
            assert opcode in (0x31, 0xe1, 0x7e, 0x2c, 0x22, 0x87, 0x47, 0x0f, 0x4f, 0xe6, 0xb0, 0x12, 0x14, 0x79, 0x15, 0x1c), hex(opcode)
            stores += opcode == 0x12
            pops += opcode == 0xe1
            offset += 3 if opcode in (0x22, 0x31) else 2 if opcode == 0xe6 else 1
        assert stores == 12 and pops == 16 and offset == len(code)
    checks = 0

    def run(events, slots=168, full=False, direct=None, prepare=False, half_missing=False, serial=None, optimized=False, inverse=False):
        nonlocal checks
        m = bytearray(base)
        limit = 0xe110 + slots * 16 if slots else 0xdb90
        put(m, 'GLYPH_CACHE_LIMIT', limit)
        if direct == 'half' and not prepare and not half_missing:
            put(m, 'quadHalfFont', 0xdb90 // 8)
            m[0xdb90:0xe390] = bytes((v >> 4) | ((v << 4) & 255) for v in font)
        if direct == 'packed':
            put(m, 'packedBase', 0 if half_missing else 0xdb90)
            m[s['packedLookupBase'] + 1:s['packedLookupBase'] + 3] = word(0xdb90 + 32)
            if not prepare:
                m[0xdb90:0xdb90 + len(packed)] = packed
        expected = bytearray(m[0x9000:0xc000])
        code = bytearray(b'\x31\x00\x8b')
        if optimized:
            code += b'\xcd' + word(s['installOptimized'])
        m[s['bios_vars.inverse']] = 255 if inverse else 0
        if prepare:
            code += b'\xcd' + word(s['quadPreparePacked' if direct == 'packed' else 'quadPrepareHalf'])
        source = 0x6000
        records = []
        for glyphs, x, baseline in events:
            count = len(glyphs)
            m[source:source + count] = bytes(glyphs)
            half_ready = not half_missing and (not prepare or limit - 0xdb90 >= 2048)
            packed_ready = not half_missing and (not prepare or limit - 0xdb90 >= 4072)
            valid = count == 0 or (count <= 48 and (serial or count % 4 == 0 and x % 8 == 0) and baseline >= 7 and x + count * 6 <= 384 and (serial or full or direct or slots >= 3) and (direct != 'half' or half_ready) and (direct != 'packed' or packed_ready))
            code += b'\x21' + bytes((baseline, x // 2)) + b'\x22' + word(s['bios_vars.cursorY'])
            code += bytes.fromhex('21 97 A5 E5 F1')
            code += b'\x01' + bytes((count, 0x12)) + bytes.fromhex('11 78 56 21') + word(source)
            entry = 'quadDynamicRender' if direct == 'dynamic' else 'quadHalfRender' if direct == 'half' else 'quadFullRender' if full else 'quadRender'
            if direct == 'packed':
                entry = 'quadPackedRender'
            if serial:
                entry = {'bios': 'serialBIOS', 'full': 'serialFull', 'cache': 'serialCache'}[serial]
            code += b'\xcd' + word(s[entry])
            # Снимок всех регистров и флага отказа после каждого вызова.
            record = 0x8000 + len(records) * 8
            code += b'\x22' + word(record) + bytes.fromhex('60 69 22') + word(record + 2)
            code += bytes.fromhex('EB 22') + word(record + 4) + bytes.fromhex('F5 E1 22') + word(record + 6)
            records.append((record, source, count, valid, x, baseline))
            if valid:
                for col, point in enumerate(glyphs):
                    bitmap = font[point * 8:point * 8 + 8]
                    if direct == 'packed':
                        address = struct.unpack_from('<H', packed, 32 + point * 2)[0] - 0xdb90
                        bitmap = bytes(v >> 2 for v in packed[address:address + 8])
                    for row, value in enumerate(bitmap):
                        if inverse:
                            value ^= 63
                        for bit in range(6):
                            xx = x + col * 6 + bit
                            address = xx // 8 * 256 + baseline - 7 + row
                            mask = 1 << (7 - xx % 8)
                            expected[address] = (expected[address] & ~mask) | (mask if value & (1 << (5 - bit)) else 0)
            source += count
        code += b'\xc3\x80\x8b'
        assert source < 0x6500 and len(code) < 0x1b00
        m[0x6500:0x6500 + len(code)] = code
        before = bytes(m)
        (args.out / 'input.bin').write_bytes(m)
        subprocess.run([str(args.ticks), '-m8080', '-pc', '6500', '-end', '8b80', '-counter', '20000000', '-output', str(args.out / 'output.bin'), str(args.out / 'input.bin')], capture_output=True, check=True, timeout=20)
        data = (args.out / 'output.bin').read_bytes()
        assert data[0x9000:0xc000] == expected, ('pixels', events, slots, full)
        for record, src, count, valid, x, y in records:
            assert data[record:record + 6] == word(src) + bytes((count, 0x12, 0x78, 0x56)), ('registers', events)
            assert data[record + 7] == 0xa5
            assert bool(data[record + 6] & 1) == (not valid), ('carry', events, slots)
        _, _, count, valid, x, y = records[-1]
        assert data[s['bios_vars.cursorY']:s['bios_vars.cursorY'] + 2] == bytes((y, x // 2 + (count * 3 if valid else 0)))
        assert data[0xdb80:0xdb90] == before[0xdb80:0xdb90]
        assert data[limit:0xeba0] == before[limit:0xeba0]
        assert data[0x2800:0x3800] == before[0x2800:0x3800]
        assert data[0x4000:0x6000] == before[0x4000:0x6000]
        assert data[s['cacheVictimGuard'] + 1:s['cacheVictimGuard'] + 3] == word(s['cacheVictimReady'])
        if serial:
            assert data[s['CACHE_RENDERERRETURN']:s['CACHE_RENDERERRETURN'] + 3] == bytes.fromhex('E1 D1 C1')
            assert data[s['cacheFallbackExit'] + 1:s['cacheFallbackExit'] + 3] == word(s['cacheFallbackScalar'])
        if direct:
            expected_arena = bytearray(before[0xdb90:0xeb90])
            if prepare and direct == 'packed' and packed_ready:
                expected_arena[:len(packed)] = packed
            elif prepare and direct == 'half' and half_ready:
                expected_arena[:2048] = bytes((v >> 4) | ((v << 4) & 255) for v in font)
                assert data[s['quadHalfFont']:s['quadHalfFont'] + 2] == word(0xdb90 // 8)
            if direct == 'half' and not half_ready:
                assert data[s['quadHalfFont']:s['quadHalfFont'] + 2] == word(0)
            assert data[0xdb90:0xeb90] == expected_arena, 'immutable arena'
            assert data[s['cacheMisses']:s['cacheMisses'] + 2] == before[s['cacheMisses']:s['cacheMisses'] + 2]
            assert data[s['GLYPH_CACHE_INVALID']] == before[s['GLYPH_CACHE_INVALID']]
        checks += 1
        return data

    for full in (False, True):
        for c in range(256):
            # Все байты, включая управляющие, здесь являются кодами глифов.
            run([(bytes((c, (c + 1) % 256, (c + 127) % 256, (255 - c))), (c % 46) * 8, 255)], full=full)
        run([(bytes(range(48)), 0, 7), (bytes(range(48, 96)), 96, 15)], full=full)
        for count, x, y in [(0, 0, 7), (1, 0, 7), (2, 0, 7), (3, 0, 7), (5, 0, 7), (52, 0, 7), (4, 2, 7), (4, 4, 7), (4, 6, 7), (4, 368, 7), (4, 0, 6)]:
            run([(bytes(range(count)), x, y)], full=full)
    for slots in (0, 1, 2):
        run([(b'ABCD', 0, 7)], slots=slots)
    # При трех слотах первый hit может оказаться точно на FIFO-жертве.
    # Следующие два miss обязаны обойти его и сохранить оба новых указателя.
    for c in range(16):
        run([(b'ABCD', 0, 7), (bytes((65, c, c + 32, 90)), 24, 15), (b'ABCD', 48, 23)], slots=3)
    run([(bytes(range(i, i + 48)), 0, 7 + i) for i in (0, 48, 96, 144)], slots=3)
    for direct in ('dynamic', 'half'):
        for c in range(256):
            run([(bytes((c, (c + 1) % 256, (c + 127) % 256, 255 - c)), (c % 46) * 8, 255)], slots=0, direct=direct)
        run([(bytes(range(48)), 0, 7), (bytes(range(48, 96)), 96, 15)], slots=0, direct=direct)
        for count, x, y in [(0, 0, 7), (1, 0, 7), (2, 0, 7), (3, 0, 7), (5, 0, 7), (52, 0, 7), (4, 2, 7), (4, 4, 7), (4, 6, 7), (4, 368, 7), (4, 0, 6)]:
            run([(bytes(range(count)), x, y)], slots=0, direct=direct)
    run([(bytes(range(48)), 0, 7)], direct='half', prepare=True)
    run([(b'ABCD', 0, 7)], direct='half', half_missing=True)
    run([(b'ABCD', 0, 7)], direct='half', prepare=True, slots=0)
    run([(b'ABCD', 0, 7)], direct='half', prepare=True, slots=1)
    for c in range(256):
        run([(bytes((c, (c + 1) % 256, (c + 127) % 256, 255 - c)), (c % 46) * 8, 255)], direct='packed')
    run([(bytes(range(48)), 0, 7), (bytes(range(48, 96)), 96, 15)], direct='packed', prepare=True)
    for count, x, y in [(0, 0, 7), (1, 0, 7), (3, 0, 7), (52, 0, 7), (4, 2, 7), (4, 368, 7), (4, 0, 6)]:
        run([(bytes(range(count)), x, y)], direct='packed')
    run([(b'ABCD', 0, 7)], direct='packed', half_missing=True)
    run([(b'ABCD', 0, 7)], direct='packed', prepare=True, slots=0)
    run([(b'ABCD', 0, 7)], direct='packed', prepare=True, slots=166)
    run([(b'ABCD', 0, 7)], direct='packed', prepare=True, slots=167)
    for serial in ('bios', 'full', 'cache'):
        for point in range(32, 256):
            run([(bytes([point]) * 4, 0, 7)], serial=serial)
        for count in (0, 1, 2, 3, 48, 49):
            run([(b'A' * count, 2, 15)], serial=serial)
        run([(b'ABCD', 0, 7)], serial=serial, slots=0)
    for serial in ('full', 'cache'):
        for point in range(32):
            run([(bytes([point]) * 4, 0, 7)], serial=serial)
    for inverse in (False, True):
        for point in range(32, 256):
            run([(bytes([point]) * 4, 0, 7)], serial='bios', optimized=True, inverse=inverse)
    report = dict(checks=checks, bitmap='PASS', registers='PASS', bounds='PASS', minimumCacheSlots=3, directCacheSlots=0, leaseAcquisitions=2, kernelBytes=len(kernel), screenReads=0, screenWrites=24, quadBytes=s['quadUseFull'] + 1 - s['quadRender'], directBytes=s['quadHalfFont'] + 2 - s['quadDirectGroup'])
    (args.out / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print('QUAD CPU PASS', checks)


if __name__ == '__main__':
    main()
