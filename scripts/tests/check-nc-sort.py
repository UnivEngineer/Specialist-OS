"""CPU-стенд NC: порядок индексов, неизменность кэша и выбранный FILE_INFO.

--prepare-only готовит эталоны и образы памяти без запуска CPU-стенда.
"""
import argparse
from pathlib import Path
import random
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def record(name, extension='', directory=False):
    return (name.encode('koi8-r').ljust(8, b' ') +
            extension.encode('koi8-r').ljust(3, b' ') +
            bytes([0x10 if directory else 0]) + bytes(4))


def rank(row):
    if not row[11] & 0x10:
        return 3
    if row[:8] == b'\\       ':
        return 0
    if row[:8] == b'..      ':
        return 1
    return 2


def key(row, mode, original):
    name = row[:8].decode('koi8-r').upper().encode('koi8-r')
    extension = row[8:11].decode('koi8-r').upper().encode('koi8-r')
    return (rank(row), name, extension, original) if mode == 1 else (
        rank(row), extension, name, original)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--prepare-only', action='store_true')
    parser.add_argument('--nc', type=Path, default=ROOT/'bin/NC.i80')
    parser.add_argument('--symbols', type=Path, default=ROOT/'bin/NC.sym')
    parser.add_argument('--ticks', type=Path, default=Path(
        'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    parser.add_argument('--output', type=Path, default=ROOT/'.local/nc-sort/cpu')
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    symbols = {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([\dA-Fa-f]+)',
        args.symbols.read_text(encoding='utf-8'), re.M)}
    nc = args.nc.read_bytes()
    bios = (ROOT/'bin/BIOS.sys').read_bytes()
    word = lambda x: x.to_bytes(2, 'little')
    suites = [[], [record('ONLY')], [
        record('SAME', 'TXT'), record('SAME', 'ASM'),
        record('ZDIR', 'TXT', True), record('ADIR', 'TXT', True),
        record('ADIR', 'ASM', True), record('..', directory=True),
        record('\\', directory=True), record('zeta', 'ASM'),
        record('ALPHA'), record('alpha', 'TXT'), record('SAME', 'TXT'),
        record('я', 'TXT'), record('Я', 'ASM'),
        record('A', 'Z'), record('A', 'A'), record('AA', 'A'),
        record('A B', 'TXT'), record('A C', 'TXT'), record('A  D', 'TXT'),
        record('SAME', 'A B'), record('SAME', 'A C')]]
    suites.append([record('A','Z'),record('A','A'),record('AA'),record('A'),
                   record('B','TXT'),record('B',''),record('A','TXT',True),
                   record('A','ASM',True),record('B','Z',True)])
    randomizer = random.Random(8080)
    for count in (37, 38, 39, 40, 254, 255):
        rows = [record(f'F{i:07}', ('TXT', 'ASM', '')[i % 3], i % 7 == 0)
                for i in range(count)]
        randomizer.shuffle(rows)
        suites.append(rows)
    cases = 0
    for panel in (0, 1):
        for rows in suites:
            for mode in (0, 1, 2):
                memory = bytearray(65536)
                memory[:len(nc)] = nc
                memory[0xC000:0xC000+len(bios)] = bios
                cache = symbols['FILE_CACHE_A' if panel == 0 else 'FILE_CACHE_B']
                index = symbols['FILE_INDEX_A' if panel == 0 else 'FILE_INDEX_B']
                other = symbols['FILE_INDEX_B' if panel == 0 else 'FILE_INDEX_A']
                data = b''.join(rows) + b'\xff'
                memory[cache:cache+len(data)] = data
                memory[index:index+256] = b'\xA5'*256
                memory[other:other+256] = b'\x5A'*256
                memory[symbols['vars.cacheSlotA']+panel] = panel
                memory[symbols['state.activePanel']] = panel
                memory[symbols['state.panelA_filesCnt']+panel] = len(rows)
                memory[symbols['state.panelA_sort']+panel] = mode
                expected = list(range(len(rows)))
                if mode:
                    expected.sort(key=lambda i: key(rows[i], mode, i))
                selected = min(19, max(0, len(rows)-1))
                memory[symbols['state.panelA_curFile']+panel] = selected
                # Проверяем штатный путь выбранного файла после перестроения.
                code = (b'\x31'+word(0x8F00) + b'\xCD'+word(symbols['buildFileIndex']) +
                        b'\xCD'+word(symbols['getSelectedFile']) + b'\x22'+word(0x8100) +
                        b'\xC3'+word(0x80FF))
                memory[0x8000:0x8000+len(code)] = code
                stem = args.output/f'{panel}-{len(rows)}-{mode}'
                input_path = stem.with_suffix('.in.bin')
                output_path = stem.with_suffix('.out.bin')
                input_path.write_bytes(memory)
                if args.prepare_only:
                    cases += 1
                    continue
                run = subprocess.run([str(args.ticks), '-m8080', '-pc', '8000',
                    '-end', '80ff', '-counter', '50000000', '-output',
                    str(output_path), str(input_path)], check=True,
                    capture_output=True, text=True, timeout=30)
                result = output_path.read_bytes()
                assert int.from_bytes(result[65542:65544], 'little') == 0x80FF, run.stdout
                assert result[cache:cache+len(data)] == data, ('cache changed', stem)
                assert result[other:other+256] == memory[other:other+256], stem
                want = bytes(expected) + b'\xff'
                assert result[index:index+len(want)] == want, (
                    stem, list(result[index:index+len(want)]), list(want))
                assert result[index+len(want):index+256] == memory[index+len(want):index+256], stem
                pointer = int.from_bytes(result[0x8100:0x8102], 'little')
                assert pointer == cache + (expected[selected]*16 if rows else 0), stem
                cases += 1
    print(('PREPARED (CPU not started)' if args.prepare_only else 'NC SORT PASS'), cases)


if __name__ == '__main__':
    main()
