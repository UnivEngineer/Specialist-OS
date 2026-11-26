"""Сверка поправок настоящего контейнера с прямой сборкой на заданной базе."""
from pathlib import Path
import re
import runpy
import subprocess
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]
EXE = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))


def load_at(source, stem, origin_symbol, base, kind="dll"):
    descriptor = ROOT / 'bin' / (stem + '.cpu')
    image = EXE['decode'](descriptor.read_bytes(), 'cpu', path=descriptor)
    packed = EXE['encode'](image, kind)
    body = EXE['relocated_body'](EXE['decode'](packed, kind), base)
    with TemporaryDirectory() as temp:
        raw = Path(temp) / 'body.bin'
        sym = Path(temp) / 'body.sym'
        subprocess.run([ROOT / 'tasm/sjasmplus.exe', '--i8080', '-Wno-rdlow',
                        f'-D{origin_symbol}={base}', f'--raw={raw}', f'--sym={sym}',
                        source.name], cwd=source.parent, check=True, capture_output=True)
        assert body == raw.read_bytes()[:len(body)], (source, hex(base))
        symbols = {key: int(value, 16) for key, value in re.findall(
            r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', sym.read_text('utf8'), re.M)}
    return body, symbols


def load_graphics():
    packed = (ROOT / 'bin/UI_GFX.dll').read_bytes()
    image = EXE['decode'](packed, 'dll')
    symbols = {key: int(value, 16) for key, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (ROOT / 'bin/UI_GFX.sym').read_text('utf8'), re.M)}
    assert packed[4] == 2 and image.reloc_kind is None
    assert image.load == image.entry == image.memory_begin == symbols['entry'] == 0x1B00
    assert image.load + len(image.data) == image.memory_end == symbols['ngFileEnd']
    assert image.options == symbols['IMAGE_OPTION_CACHEABLE'] | symbols['IMAGE_OPTION_REUSABLE']
    return image.data, symbols


if __name__ == '__main__':
    load_graphics()
    for origin in (0x3200, 0xA500):
        load_at(ROOT / 'source-apps/fontbench/FONTGRID.asm', 'FONTGRID', 'FONTGRID_ORIGIN', origin)
    print('UI_GFX matches its fixed v2 layout; FONTGRID HIGH8 matches direct assembly at two bases')
