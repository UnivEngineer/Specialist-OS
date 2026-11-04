"""Нормализованные копии BUDI из неизменяемых исторических файлов legacy."""
import argparse
from hashlib import sha256
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
GAMES = ROOT / 'assets/games/BUDI'
SOURCES = (
    ('BUDI.rks', 'Budi-std.rks', '68b6b503233e6463cfc9558a39ae5d77d26d6ac2eda3c62e5464a8da5adfdb22'),
    ('BUDI2.rks', 'Budi2-color-f000.rks', '449588e8175e752819e24a37d1f9184bbb412e3f359b57d14378d7e6685425fc'),
    ('Budi-MX.cpu', 'Budi-ramfos.cpu', '569192f641e7e9bf612d8accd8cd4998c3554ba352a9947f23af0c64ba8348ff'),
)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    prepared = {}
    for source, target, expected in SOURCES:
        data = (GAMES / 'legacy' / source).read_bytes()
        if sha256(data).hexdigest() != expected:
            raise ValueError(f'Неизвестный исторический файл: {source}')
        prepared[target] = data
    patcher = runpy.run_path(str(Path(__file__).with_name('patch-budi2.py')))
    original = (GAMES / 'legacy/BUDI2.rks').read_bytes()
    if original != patcher['ORIGINAL'].read_bytes():
        raise ValueError('Оригиналы BUDI2 в legacy и originals различаются')
    prepared['Budi2-mx.rks'] = patcher['patch'](original)
    game_over = runpy.run_path(str(Path(__file__).with_name('patch-budi-game-over.py')))
    for name, data in prepared.items():
        data = game_over['patch'](data, name)
        target = GAMES / name
        if args.check:
            if not target.exists() or target.read_bytes() != data:
                raise ValueError(f'Рабочая копия отличается: {name}')
        else:
            target.write_bytes(data)
    print('OK: 3 исторических файла, 4 рабочих варианта с GAME OVER и патчем цвета BUDI2 MX')


if __name__ == '__main__':
    main()
