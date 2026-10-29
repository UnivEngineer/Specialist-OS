"""Копирование NC с H: на системную flash A: и проверка сохранности."""
import argparse
import hashlib
import json
import re
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
helpers = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))
FileMachine, get_file = helpers['FileMachine'], helpers['get_file']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--verify-persisted', action='store_true')
    args = parser.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужна отдельная установка Emu80-flash-test')
    a_path = args.runtime / 'specialist-os/system-flash.bin'
    h_path = args.runtime / 'specialist-os/flash-disk.bin'
    report_path = ROOT / '.codex-tmp/system-flash-files.json'
    m = FileMachine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    m.call('bp_set', addr=0x8100, type='exec')
    target = b'SYSCPY  TXT'
    if not args.verify_persisted:
        source_entry, expected = get_file(h_path.read_bytes(), b'COPYCHK TXT')
        old_h = hashlib.sha256(h_path.read_bytes()).hexdigest()
        listing = (ROOT / 'bin/NC.lst').read_text(encoding='utf-8-sig')
        def label(name):
            pattern = r'^\s*\d+\+?\s+([0-9A-F]{4})\s+(?:[0-9A-F]{2}\s+)*' + name + r'[: ]'
            return int(re.search(pattern, listing, re.M)[1], 16)
        state = label('state')
        operand = re.search(r'21 ([0-9A-F]{2}) ([0-9A-F]{2})\s+ld\s+hl, vars.input', listing)
        input_addr = int(operand[2] + operand[1], 16)
        m.write(state + 1, b'\0')
        m.write(state + 4, b'\x07')
        m.write(state + 8, b'\0')
        m.write(state + 10, b'\0')
        m.write(input_addr + 64, source_entry[:12] + source_entry[18:20] + source_entry[28:30])
        m.write(input_addr, b'A:SYSCPY.TXT\0')
        # Копирование на A: должно завершаться без дополнительного запроса.
        m.subroutine(label('copyFileInt'))
        assert m.read(0x8FF9, 1) == b'\0'
        assert hashlib.sha256(h_path.read_bytes()).hexdigest() == old_h
        assert get_file(a_path.read_bytes(), target)[1] == expected
        report = {'source': 'H:COPYCHK.TXT', 'target': 'A:SYSCPY.TXT',
                  'size': len(expected), 'sha256': hashlib.sha256(expected).hexdigest(),
                  'hUnchanged': True, 'persistentAfterRestart': False}
    else:
        report = json.loads(report_path.read_text())
        expected = get_file(a_path.read_bytes(), target)[1]
        assert hashlib.sha256(expected).hexdigest() == report['sha256']
        report['persistentAfterRestart'] = True

    m.subroutine(0xC842, af=0x0100, de=1)
    m.subroutine(0xC842, af=0x0000, de=1)
    m.write(0x7000, target + bytes(21))
    assert not m.subroutine(0xC851, hl=0x7000)['flags']['C']
    assert not m.subroutine(0xC866, hl=0x7000, de=0x4000)['flags']['C']
    assert m.read(0x4000, len(expected)) == expected
    report_path.write_text(json.dumps(report, indent=2) + '\n')
    print('NC H: -> A: copy, FAT and DOS readback OK' + (' after restart' if args.verify_persisted else ''))


if __name__ == '__main__':
    main()
