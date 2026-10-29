"""Регрессии NC и независимых MX/STD-цветов в отдельном Emu80-flash-test."""
import argparse
import base64
import io
import json
import re
import runpy
import urllib.request
from pathlib import Path
from PIL import Image

ROOT = Path(__file__).resolve().parents[2]
FileMachine = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))['FileMachine']


def screen(machine):
    request = urllib.request.Request(machine.url, json.dumps({
        'jsonrpc': '2.0', 'id': 1, 'method': 'tools/call',
        'params': {'name': 'screen', 'arguments': {}},
    }).encode(), {'Content-Type': 'application/json'})
    blocks = json.load(urllib.request.urlopen(request, timeout=30))['result']['content']
    block = next(item for item in blocks if item['type'] == 'image')
    return Image.open(io.BytesIO(base64.b64decode(block['data'])))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    args = parser.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужна отдельная установка Emu80-flash-test')
    m = FileMachine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    m.call('bp_set', addr=0x8100, type='exec')
    m.write(0xFFF8, b'\xB1')
    m.write(0xFFE3, b'\x82')
    for index, color in enumerate([0x00, 0x20, 0x40, 0x80, 0xD0]):
        m.write(0xFFE2, bytes([color]))
        m.write(0x9000 + index * 256, b'\xFF')
    pic = screen(m)
    cyan = (0, 255, 255, 255)
    assert all(pic.getpixel((i * 8, 0)) == cyan for i in range(5)), 'ВВ55 изменил MX-цвет'
    m.write(0xFFFF, b'\0')
    m.write(0xF7F8, b'\2')  # STD, восемь цветов.
    m.write(0xF803, b'\x82')
    m.write(0xF802, b'\0')
    m.write(0x9000, b'\xFF')
    m.write(0xF802, b'\xD0')
    m.write(0x9100, b'\xFF')
    pic = screen(m)
    assert pic.getpixel((0, 0)) != pic.getpixel((8, 0)), 'STD-цвет перестал работать'
    m.write(0xF7FC, b'\0')
    m.write(0x9000, b'\xFF')
    assert screen(m).getpixel((0, 0)) == cyan, 'MX-защёлка не сохранилась после STD'

    listing = (ROOT / 'bin/NC.lst').read_text(encoding='utf-8-sig')
    def label(name):
        pattern = r'^\s*\d+\+?\s+([0-9A-F]{4})\s+(?:[0-9A-F]{2}\s+)*' + name + r'[: ]'
        return int(re.search(pattern, listing, re.M)[1], 16)
    state = label('state')
    load = label('loadFiles')
    m.write(state + 1, b'\0')
    m.write(state + 4, b'\x07')
    m.write(state + 8, b'\xFF')
    m.write(state + 10, b'\xFF')
    m.subroutine(load, af=0x0700)
    values = m.read(state, 12)
    assert values[6] > 0 and values[10] == values[6] - 1 and values[8] <= values[10]
    # Создаём и удаляем два файла на пустом RAM-диске B:.
    m.subroutine(0xC842, af=0x0100, de=1)
    for name in [b'UITEST1 TXT', b'UITEST2 TXT']:
        descriptor = bytearray(32)
        descriptor[:11] = name
        descriptor[18:20] = b'\0\x40'
        descriptor[28] = 255
        m.write(0x7000, descriptor)
        m.write(0x4000, bytes(range(256)))
        assert not m.subroutine(0xC845, hl=0x7000)['flags']['C']
    m.write(state + 4, b'\x01')
    m.write(state + 8, b'\0')
    m.write(state + 10, b'\x01')
    assert not m.subroutine(0xC84B, hl=0x7000)['flags']['C']
    m.subroutine(load, af=0x0100)
    values = m.read(state, 12)
    assert values[6] == 1 and values[10] == 0
    m.write(0x7000, b'UITEST1 TXT')
    assert not m.subroutine(0xC84B, hl=0x7000)['flags']['C']
    m.subroutine(load, af=0x0100)
    values = m.read(state, 12)
    assert values[6] == values[8] == values[10] == 0
    print('MX latch independent from keyboard, STD colors, MX restore, NC cursor after deletion OK')


if __name__ == '__main__':
    main()
