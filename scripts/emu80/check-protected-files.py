"""Проверка защиты атрибутами DOS на тестовых дисках A: и H:."""
import argparse
import runpy
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
FileMachine = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))['FileMachine']


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
    payload = b'PROTECTION TEST!'
    m.write(0x4000, payload)
    for drive in (0, 7):
        assert not m.subroutine(0xC842, af=drive << 8, de=1)['flags']['C']
        for attrs in (0, 1, 4, 5):
            name = ('P%d%d     TXT' % (drive, attrs)).encode('ascii')
            desc = bytearray(32)
            desc[:11] = name
            desc[11] = attrs
            struct.pack_into('<H', desc, 18, 0x4000)
            struct.pack_into('<I', desc, 28, len(payload) - 1)
            m.write(0x7000, desc)
            assert not m.subroutine(0xC845, hl=0x7000)['flags']['C']
            m.write(0x7000, desc)
            before = (args.runtime / 'specialist-os' / ('system-flash.bin' if drive == 0 else 'flash-disk.bin')).read_bytes()
            deleted = m.subroutine(0xC84B, hl=0x7000)['flags']['C']
            assert deleted == bool(attrs), (drive, attrs, 'delete')
            if attrs:
                if attrs & 1:
                    assert m.subroutine(0xC845, hl=0x7000)['flags']['C'], (drive, attrs, 'overwrite')
                    assert m.subroutine(0xC88A, hl=0x7000, de=1)['flags']['C'], (drive, attrs, 'stream overwrite')
                    assert (args.runtime / 'specialist-os' / ('system-flash.bin' if drive == 0 else 'flash-disk.bin')).read_bytes() == before
                else:
                    assert not m.subroutine(0xC845, hl=0x7000)['flags']['C'], (drive, attrs, 'system overwrite')
                    get_file = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))['get_file']
                    disk = args.runtime / 'specialist-os' / ('system-flash.bin' if drive == 0 else 'flash-disk.bin')
                    assert get_file(disk.read_bytes(), name)[0][11] == attrs
                # Переименование поверх защищённого назначения также удаляет его.
                source = bytearray(desc)
                source[:11] = ('Q%d%d     TXT' % (drive, attrs)).encode('ascii')
                source[11] = 0
                m.write(0x7100, source)
                assert not m.subroutine(0xC845, hl=0x7100)['flags']['C']
                after_source = (args.runtime / 'specialist-os' / ('system-flash.bin' if drive == 0 else 'flash-disk.bin')).read_bytes()
                assert m.subroutine(0xC84E, hl=0x7100, de=0x7000)['flags']['C']
                assert (args.runtime / 'specialist-os' / ('system-flash.bin' if drive == 0 else 'flash-disk.bin')).read_bytes() == after_source
                assert not m.subroutine(0xC851, hl=0x7000)['flags']['C']
                assert not m.subroutine(0xC851, hl=0x7100)['flags']['C']
            else:
                assert m.subroutine(0xC851, hl=0x7000)['flags']['C']
    print('DOS: R/S deletion denied; R overwrite denied; S overwrite allowed and preserved on A: and H:')


if __name__ == '__main__':
    main()
