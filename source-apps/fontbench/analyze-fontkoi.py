"""Проверяемая оценка готовых сдвигов FONTKOI без изменения системного шрифта."""
from pathlib import Path
import argparse
import hashlib
import json
import runpy
import struct

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/quad-opt-001/fontkoi')
    parser.add_argument('--build', action='store_true')
    args = parser.parse_args()
    args.out.mkdir(parents=True, exist_ok=True)
    font = (ROOT / 'assets/system-rom/FONTKOI.FNT').read_bytes()
    meta = runpy.run_path(str(ROOT / 'scripts/build/font-format.py'))['parse'](font)
    glyphs = {}
    for page, offset in enumerate(meta['ranges']):
        if offset:
            for i in range(32):
                glyphs[page * 32 + i] = font[offset + i * 8:offset + i * 8 + 8]
    for code, offset in zip((0xa3, 0xb3), meta['yo']):
        if offset:
            glyphs[code] = font[offset:offset + 8]
    unique = list(dict.fromkeys(glyphs.values()))
    assert all(len(g) == 8 and all(v < 64 for v in g) for g in unique)
    # Три готовых поворота по восемь байт. Исходная фаза восстанавливается
    # двумя RRCA из фазы 0, отдельный шрифт при рендере не требуется.
    # Фазы 4/6 содержат обе части в одном байте и требуют двух AND при выводе.
    records = bytearray()
    for g in unique:
        records += bytes(v << 2 for v in g)
        records += bytes((v >> 2) | ((v << 6) & 255) for v in g)
        records += bytes((v >> 4) | ((v << 4) & 255) for v in g)
    pointers = b''.join(struct.pack('<H', 544 + unique.index(glyphs.get(c, glyphs[32])) * 24) for c in range(256))
    header = struct.pack('<4s6H16x', b'QFNT', 1, len(unique), len(glyphs), 32, 544, 544 + len(records))
    packed = header + pointers + records
    if args.build:
        assert len(unique) == 147 and len(packed) == 4072
        raw = (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
        assert all(raw[c * 8:c * 8 + 8] == g for c, g in glyphs.items())
        (ROOT / 'bin/FONTKOI.packed.bin').write_bytes(packed)
        (ROOT / 'bin/FONTKOI.packed.index.bin').write_bytes(bytes(unique.index(glyphs.get(c, glyphs[32])) for c in range(256)))
        (ROOT / 'bin/FONTKOI.packed.codes.bin').write_bytes(bytes(next(c for c, v in glyphs.items() if v == g) for g in unique))
    assert len(header) == 32 and len(packed) <= 4096
    # Проверяется восстановление всех готовых экранных частей без вращений.
    for c in range(256):
        offset = struct.unpack_from('<H', packed, 32 + c * 2)[0]
        for y, v in enumerate(glyphs.get(c, glyphs[32])):
            a, b, d = (packed[offset + phase * 8 + y] for phase in range(3))
            assert a == v << 2
            assert (b & 15, b & 192) == (v >> 2, (v << 6) & 255)
            assert (d & 3, d & 240) == (v >> 4, (v << 4) & 255)
    (args.out / 'FONTKOI.preshift-packed.bin').write_bytes(packed)
    report = dict(fontBytes=len(font), fontSha256=hashlib.sha256(font).hexdigest(), codes=len(glyphs), uniqueGlyphs=len(unique),
                  ranges=[f'{p*32:02X}-{p*32+31:02X}' for p, off in enumerate(meta['ranges']) if off], extraCodes=['A3', 'B3'],
                  expandedShiftBytes=len(glyphs) * 40, deduplicatedExpandedShiftBytes=len(unique) * 40,
                  expandedWithPointerTableAndHeader=len(unique) * 40 + 544,
                  packedShiftBytes=len(records), byteIndexLayout=len(records) + 256,
                  pointerLayout=len(records) + 512, packedWithHeader=len(packed), freeBytes=4096 - len(packed),
                  sourceFontOutsideArena=False, rawPhaseRotateRight=2, requiresMasksForSplitPhases=True, reconstructionChecks=256 * 8)
    (args.out / 'results.json').write_text(json.dumps(report, indent=2) + '\n')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
