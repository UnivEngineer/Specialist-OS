"""Платформенные контейнеры StarWars из неизменяемых исторических файлов.

Без аргументов обновляет только копии в корне StarWars; --check ничего не пишет.
Код игры и адрес загрузки не меняются. Неизвестные оригиналы отклоняются.
"""
import argparse
import hashlib
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
GAMES = ROOT / 'assets/games/StarWars'
SOURCES = {
    'StarWars-std-C800.exe': 'a8ac2914135e32f47aea3c2408565974dceac8d51a3c615515c93dc911d21bab',
    'StarWars-std.rks': '45f28d909c77d0bae25cdd69bf83c8b96e2f9bb90b0bf87becfdc24a8cd57177',
    'StarWars2-mx.exe': 'de9287ea25309a25bcc796246d55c1938729823ed76f4f977b9a742b78af2796',
    'StarWars2-mx.rks': '642326f85a1b6009a80a08d47b7c62a7fc0042e6c2f6c8f9076704ce4bf43b15',
    'StarWars2-ramfos.exe': 'b7dd022fc87f3280a7f54ea1a4d0fc87916d4f2d5cb91a80f8afa6c518f4cd5c',
    'StarWars2-std-tape.rks.bin': '51d6a9babdec423f8489db7fb0cb2a959ef0b273f42bc0824c2d6a3dcc3bbb8f',
    'StarWars2-std-germanovitsh.rks': 'b119f11eb438dcf7b15dfd96531b7ec95dcc5ab736287b92d0d0d19ad9992708',
}


def checksum(data):
    b = c = 0
    for i, value in enumerate(data):
        total = c + value
        c = total & 255
        if i + 1 < len(data):
            b = (b + value + (total >> 8)) & 255
    return (b << 8) | c


def rks(data):
    return b'\0\0' + (len(data) - 1).to_bytes(2, 'little') + data + checksum(data).to_bytes(2, 'little')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    original = {}
    for name, expected in SOURCES.items():
        data = (GAMES / 'legacy' / name).read_bytes()
        if hashlib.sha256(data).hexdigest() != expected:
            raise ValueError(f'Неизвестный оригинал: {name}')
        original[name] = data
    tape = original['StarWars2-std-tape.rks.bin']
    if tape[:3] != b'\xd9\xd9\xd9' or tape[778:782] != bytes.fromhex('00 00 ff 23'):
        raise ValueError('Неизвестный ленточный заголовок')
    payloads = {
        'StarWars-std-C800.rks': original['StarWars-std-C800.exe'],
        'StarWars-std.rks': original['StarWars-std.rks'][4:-2],
        'StarWars2-mx.exe': original['StarWars2-mx.exe'],
        'StarWars2-ramfos.exe': original['StarWars2-ramfos.exe'],
        'StarWars2-std.rks': tape[782:782 + 9216],
        'StarWars2-std-germanovitsh.rks': original['StarWars2-std-germanovitsh.rks'][4:-2],
    }
    if original['StarWars2-mx.rks'] != rks(payloads['StarWars2-mx.exe']):
        raise ValueError('Исторические EXE/RKS MX не соответствуют друг другу')
    if len({hashlib.sha256(data).digest() for data in payloads.values()}) != 6:
        raise ValueError('Ожидались шесть различных нагрузок')
    prepared = {name: rks(data) if name.endswith('.rks') else data
                for name, data in payloads.items()}
    for name, data in prepared.items():
        target = GAMES / name
        if args.check:
            if not target.exists() or target.read_bytes() != data:
                raise ValueError(f'Платформенная копия отличается: {name}')
        else:
            target.write_bytes(data)
    print('Проверены 7 исторических файлов и 6 платформенных копий; нагрузки сохранены.')


if __name__ == '__main__':
    main()
