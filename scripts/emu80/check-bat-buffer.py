"""BAT в 0000h: 32-КБ граница и команда, которая затирает буфер."""
import argparse
import re
import runpy
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))['FileMachine']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path)
    args = parser.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужна отдельная установка Emu80-flash-test')
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    stop = 0x8700
    m.call('bp_set', addr=stop, type='exec')
    def call(address, **registers):
        m.write(0x8800, bytes((0xcd, address & 255, address >> 8, 0xc3, stop & 255, stop >> 8)))
        m.call('regs_set', pc=0x8800, sp=0x8f00, **registers)
        for _ in range(240):
            m.call('emu_run_for', ms=500)
            state = m.call('cpu_state')
            if state['registers']['pc']['dec'] == stop:
                return state
        raise RuntimeError(f'Операция не завершилась: {state}')
    def create(name, address, data):
        desc = bytearray(32)
        desc[:11] = name
        struct.pack_into('<H', desc, 18, address)
        struct.pack_into('<I', desc, 28, len(data) - 1)
        m.write(address, data)
        m.write(0x8500, desc)
        assert not call(0xC845, hl=0x8500)['flags']['C'], name
    assert not call(0xC842, af=0x0100, de=1)['flags']['C']
    # COM по 0000h портит начало BAT и ставит контрольный байт вне буфера.
    prog = bytes.fromhex('3e4232018f3e00320000c9')
    create(b'ZERO    COM', 0, prog)
    # Размер ровно 32 КБ; вторая строка выполняется после перезагрузки BAT.
    prefix = b'B:ZERO.COM\r\nA:ROM.COM'
    payload = prefix + b' ' * (32768 - len(prefix))
    create(b'LONG    BAT', 0, payload)
    m.write(0x8f01, b'\0')
    m.write(0x8400, b'B:LONG.BAT\0')
    result = call(0xC85D, hl=0x8400)
    assert not result['flags']['C'] and m.read(0x8f01, 1) == b'\x42'
    assert m.read(0xDE0C, 40) == (ROOT / 'bin/Rom.com').read_bytes()[12:52]
    assert m.read(0x8000, 1) == b'\xff'  # Терминатор вне буфера.
    # Сверх границы загрузка должна быть отклонена до записи в буфер.
    m.write(0x8500, b'LONG    BAT' + bytes(21))
    assert not call(0xC84B, hl=0x8500)['flags']['C']
    create(b'LONG    BAT', 0, b'X' * 32769)
    listing = (ROOT / 'bin/DOS.lst').read_text(encoding='utf-8-sig')
    match = re.search(r'^\s*\d+\s+([0-9A-F]{4})\s+(?:[0-9A-F]{2}\s+)*v_bootAutoexec:', listing, re.M)
    assert match
    m.write(int(match[1], 16), b'\1')  # Не ждать клавишу при штатной ошибке загрузки.
    m.write(0x8400, b'B:LONG.BAT\0')
    m.write(0x8f01, b'\x33')
    assert call(0xC85D, hl=0x8400)['flags']['C']
    assert m.read(0x8f01, 1) == b'\x33'
    print('32 KiB BAT OK, 0000h overwrite recovered, 32769 bytes rejected')


if __name__ == '__main__':
    main()
