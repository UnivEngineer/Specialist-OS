"""Независимый 8080: пиксели, lookup 256 кодов, FIFO, границы и инвалидирование."""
from pathlib import Path
import argparse
import json
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def symbols(name):
    return {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (ROOT / 'bin' / f'{name}.sym').read_text(), re.M)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/phase-cache-001/cpu')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    s = symbols('FONTCOM')
    io = symbols('IO')
    font = (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
    base = bytearray((i * 37 + (i >> 8) * 13) & 255 for i in range(65536))
    for name, address in [('FONTCOM.i80', 0), ('BIOS.sys', 0xc000), ('IO.sys', 0xf000)]:
        data = (ROOT / 'bin' / name).read_bytes()
        base[address:address + len(data)] = data

    def word(m, at, value):
        m[at:at + 2] = value.to_bytes(2, 'little')

    # Только стенд MXCALL Query заменён переходником к реальной службе IO.
    # Пропускаются два байта номера операции после CALL; транспорта здесь нет.
    base[0xf806:0xf806 + 7] = b'\xe1\x23\x23\xe5\xc3' + io['glyphCacheService'].to_bytes(2, 'little')
    arena_start, arena_end = 0xdb90, 0xeb90
    word(base, s['GLYPH_CACHE_START'], arena_start)
    word(base, s['GLYPH_CACHE_LIMIT'], arena_end)
    word(base, s['GLYPH_CACHE_ENTRIES'], 0)
    base[s['GLYPH_CACHE_INVALID']] = 1
    word(base, s['bios_vars.charGen'], 0x2800 // 8)
    base[s['bios_vars.koi7']] = 0
    base[s['bios_vars.inverse']] = 0
    base[s['bios_vars.escMode']] = 255
    checks = 0
    samples = []

    def run(events, limit=arena_end, initial=None):
        nonlocal checks
        m = bytearray(base if initial is None else initial[:65536])
        current_start = int.from_bytes(m[s['GLYPH_CACHE_START']:s['GLYPH_CACHE_START'] + 2], 'little')
        word(m, s['GLYPH_CACHE_LIMIT'], limit)
        expected = bytearray(m[0x9000:0xc000])
        code = bytearray(b'\x31\x00\x8b')
        call = lambda address: b'\xcd' + address.to_bytes(2, 'little')
        glyphs = []
        active_font = font
        for event in events:
            if event[0] == 'invalidate':
                code += b'\x3e\x01\x32' + s['GLYPH_CACHE_INVALID'].to_bytes(2, 'little')
                continue
            if event[0] == 'resize':
                code += b'\x21' + event[1].to_bytes(2, 'little') + b'\x3e\x01' + call(io['glyphCacheService'])
                continue
            if event[0] == 'font':
                alternate = bytes(v ^ 0x3f for v in font)
                active_font = alternate
                m[0x4000:0x4800] = alternate
                code += b'\x21\x00\x08\x22' + s['bios_vars.charGen'].to_bytes(2, 'little')
                code += b'\x3e\x01\x32' + s['GLYPH_CACHE_INVALID'].to_bytes(2, 'little')
                continue
            _, point, x, y = event
            glyphs.append((point, x, y))
            code += b'\x21' + bytes((y + 7, x // 2)) + b'\x22' + s['bios_vars.cursorY'].to_bytes(2, 'little')
            code += b'\x21\x97\xa5\xe5\xf1\x01' + bytes((point, 0x12)) + b'\x11\x78\x56\x21\xbc\x9a' + call(s['cacheRender'])
            for row, value in enumerate(active_font[point * 8:point * 8 + 8]):
                for bit in range(6):
                    xx = x + bit
                    address = (xx // 8) * 256 + y + row
                    mask = 1 << (7 - xx % 8)
                    expected[address] = (expected[address] & ~mask) | (mask if value & (1 << (5 - bit)) else 0)
        # Сохраняем регистры после последнего render до изменения флагов.
        code += bytes.fromhex('22 00 84 60 69 22 02 84 EB 22 04 84 F5 E1 22 06 84 C3 80 8B')
        assert len(code) < 0x1e00
        m[0x6500:0x6500 + len(code)] = code
        before = bytes(m)
        (args.out / 'input.bin').write_bytes(m)
        result = subprocess.run([str(args.ticks), '-m8080', '-pc', '6500', '-end', '8b80', '-counter', '20000000', '-output', str(args.out / 'output.bin'), str(args.out / 'input.bin')], capture_output=True, text=True, check=True, timeout=20)
        data = (args.out / 'output.bin').read_bytes()
        assert data[0x9000:0xc000] == expected, ('pixels', events[-5:])
        if glyphs:
            point, x, y = glyphs[-1]
            assert data[0x8400:0x8408] == bytes.fromhex('BC 9A') + bytes((point, 0x12)) + bytes.fromhex('78 56 97 A5'), ('registers', events[-1])
            assert data[s['bios_vars.cursorY']:s['bios_vars.cursorY'] + 2] == bytes((y + 7, x // 2 + 3))
        # Нельзя писать за границу полученной арены, в шрифт или стек BIOS.
        assert data[current_start - 16:current_start] == before[current_start - 16:current_start]
        assert data[limit:arena_end + 16] == before[limit:arena_end + 16]
        checks += 1
        return data, int((result.stdout + result.stderr).strip())

    for point in range(256):
        for phase in range(4):
            data, ticks = run([('glyph', point, phase * 2, 0), ('glyph', point, phase * 2, 248)])
            assert int.from_bytes(data[s['cacheMisses']:s['cacheMisses'] + 2], 'little') == (0 if phase == 1 else 1)
            if phase == 1:
                # Сырая фаза вообще не запрашивает и не инициализирует кеш.
                assert data[s['GLYPH_CACHE_INVALID']] == 1
                assert data[arena_start:arena_end] == base[arena_start:arena_end]
            else:
                index = int.from_bytes(data[s['cacheIndex']:s['cacheIndex'] + 2], 'little')
                occupied = {(page, c) for page in range(3) for c in range(256) if data[index + page * 256 + c]}
                assert occupied == {(phase - (phase > 1), point)}, ('unrequested phase', point, phase, occupied)
            if phase == 0:
                pool = int.from_bytes(data[s['cachePool']:s['cachePool'] + 2], 'little')
                # Однобайтный сдвиг записывает ровно 8 байт, а не весь слот.
                assert data[pool + 24:pool + 32] == base[pool + 24:pool + 32]
            if point == 65:
                samples.append(dict(phase=phase, coldAndWarmTicks=ticks))
    data, _ = run([('glyph', i, 0, (i % 32) * 8) for i in range(256)])
    capacity = data[s['cacheSlots']]
    assert capacity == 168
    assert int.from_bytes(data[s['cacheEvictions']:s['cacheEvictions'] + 2], 'little') == 256 - capacity
    index = int.from_bytes(data[s['cacheIndex']:s['cacheIndex'] + 2], 'little')
    assert {i for i in range(256) if data[index + i]} == set(range(256 - capacity, 256))
    assert not any(data[index + 256:index + 768])
    # Один и тот же символ в разных фазах — разные ключи, сырая фаза не занимает слот.
    data, _ = run([('glyph', 65, x, 0) for x in (0, 2, 4, 6, 0, 2, 4, 6)])
    assert int.from_bytes(data[s['cacheMisses']:s['cacheMisses'] + 2], 'little') == 3
    # После вытеснения фазы 4/6 не должны оставаться ложные указатели на слот фазы 0.
    data, _ = run([('glyph', 65, 4, 0), ('glyph', 65, 6, 0)] + [('glyph', i, 0, 8) for i in range(256)] + [('glyph', 65, 4, 16), ('glyph', 65, 6, 16)])
    data, _ = run([('glyph', 65, 0, 0), ('invalidate',), ('glyph', 65, 0, 8)])
    assert int.from_bytes(data[s['cacheMisses']:s['cacheMisses'] + 2], 'little') == 2
    data, _ = run([('glyph', 65, 0, 0), ('font',), ('glyph', 65, 0, 8)])
    assert int.from_bytes(data[s['cacheMisses']:s['cacheMisses'] + 2], 'little') == 2
    # Один слот, нулевая ёмкость, конец ровно/не ровно по границе слота.
    for limit, slots in [(0xe120, 1), (0xe11f, 0), (0xe110, 0), (0xdb90, 0), (0xe181, 7)]:
        data, _ = run([('glyph', 65, 0, 0), ('glyph', 66, 2, 8), ('glyph', 65, 4, 16)], limit=limit)
        assert data[s['cacheSlots']] == slots, (hex(limit), data[s['cacheSlots']])
    data, _ = run([('glyph', 65, x, i * 8) for i, x in enumerate((0, 4, 6, 0, 4, 2))], limit=0xe120)
    assert int.from_bytes(data[s['cacheMisses']:s['cacheMisses'] + 2], 'little') == 5
    # Исходная фаза не обращается к отсутствующему кешу даже для кодов управления.
    for point in (0, 7, 10, 13, 27, 255):
        data, _ = run([('glyph', point, 2, 0)], limit=arena_start)
        assert int.from_bytes(data[s['cacheMisses']:s['cacheMisses'] + 2], 'little') == 0
    run([('glyph', 65, 0, 0), ('resize', 0xe120), ('glyph', 65, 2, 8), ('glyph', 66, 4, 16)])
    populated, _ = run([('glyph', i, (i % 48) * 6, (i // 48) * 8) for i in range(64)])
    # После сужения хвост прежнего кеша становится чужим: даже очистка запрещена.
    populated = bytearray(populated[:65536])
    populated[s['GLYPH_CACHE_INVALID']] = 1
    run([('glyph', i, (i % 48) * 6, 32) for i in range(32)], limit=0xe120, initial=populated)
    # Изменяемое начало арены и все допустимые однобайтные номера слотов.
    for begin, slots in [(0xdca1, 152), (0xd660, 248)]:
        initial = bytearray(base)
        word(initial, s['GLYPH_CACHE_START'], begin)
        data, _ = run([('glyph', 65, 0, 0)], initial=initial)
        assert data[s['cacheSlots']] == slots
        assert data[begin:(begin + 255) & ~255] == initial[begin:(begin + 255) & ~255]
    report = dict(checks=checks, lookupBytes=768, ownerBytes=512, slotBytes=16, defaultSlots=capacity, rawPhase=2, samples=samples)
    (args.out / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print('CACHE CPU PASS', checks)


if __name__ == '__main__':
    main()
