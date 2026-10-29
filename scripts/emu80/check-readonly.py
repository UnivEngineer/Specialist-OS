"""Отказ изменений ROM A: без ложного удаления в кэше и NC."""
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
    args = parser.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужна отдельная установка Emu80-flash-test')
    m = FileMachine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    m.call('bp_set', addr=0x8100, type='exec')
    system_path = args.runtime / 'specialist-os/system.bin'
    physical = system_path.read_bytes()
    logical = physical[32768:] + physical[4:32768]
    entry, payload = get_file(logical, b'FONT    FNT')

    def select(drive):
        m.subroutine(0xC842, af=drive << 8, de=1)

    def stat(name):
        m.write(0x7000, name + bytes(21))
        return m.subroutine(0xC851, hl=0x7000)

    def check_original():
        assert not stat(b'FONT    FNT')['flags']['C']
        assert not m.subroutine(0xC866, hl=0x7000, de=0x4000)['flags']['C']
        assert m.read(0x4000, len(payload)) == payload

    select(0)
    m.write(0x7000, entry)
    assert m.subroutine(0xC84B, hl=0x7000)['flags']['C']
    assert m.read(0x8FF9, 1) == b'\1'
    check_original()  # Без переключения диска и повторного mount.
    m.write(0x7100, b'RORENAMEFNT' + bytes(21))
    assert m.subroutine(0xC84E, hl=0x7000, de=0x7100)['flags']['C']
    check_original()
    assert stat(b'RORENAMEFNT')['flags']['C']
    descriptor = bytearray(32)
    descriptor[:11] = b'RODEST  TXT'
    descriptor[18:20] = b'\0\x40'
    descriptor[28:30] = b'\xff\1'
    m.write(0x7000, descriptor)
    m.write(0x4000, bytes(range(256)) * 2)
    assert m.subroutine(0xC845, hl=0x7000)['flags']['C']
    assert stat(b'RODEST  TXT')['flags']['C']
    check_original()
    # Отказ перезаписи должен сохранить старый файл в доступном каталоге.
    m.write(0x7000, entry)
    m.write(0x4000, bytes(value ^ 255 for value in payload))
    assert m.subroutine(0xC845, hl=0x7000)['flags']['C']
    check_original()

    listing = (ROOT / 'bin/NC.lst').read_text(encoding='utf-8-sig')
    def label(name):
        pattern = r'^\s*\d+\+?\s+([0-9A-F]{4})\s+(?:[0-9A-F]{2}\s+)*' + name + r'[: ]'
        return int(re.search(pattern, listing, re.M)[1], 16)
    state = label('state')
    operand = re.search(r'21 ([0-9A-F]{2}) ([0-9A-F]{2})\s+ld\s+hl, vars.input', listing)
    input_addr = int(operand[2] + operand[1], 16)
    buffer_addr = input_addr + 64

    def selected(drive, descriptor):
        m.write(state + 1, b'\0')
        m.write(state + 4, bytes([drive]))
        m.write(state + 6, b'\1')
        m.write(state + 8, b'\0')
        m.write(state + 10, b'\0')
        m.write(buffer_addr, descriptor[:12] + descriptor[18:20] + descriptor[28:30])

    def expect_nc_error(address, text):
        m.call('bp_clear')
        m.call('bp_set', addr=label('copyFileShowError'), type='exec')
        m.write(0x8000, bytes([0xCD, address & 255, address >> 8, 0xC3, 0, 0x81]))
        m.call('regs_set', pc=0x8000, sp=0x8F00)
        for _ in range(120):
            m.call('emu_run_for', ms=500)
            cpu = m.call('cpu_state')['registers']
            if cpu['pc']['dec'] == label('copyFileShowError'):
                assert m.read(cpu['hl']['dec'], len(text)) == text
                break
        else:
            raise AssertionError('NC не показал отказ операции')
        m.call('bp_clear')
        m.call('bp_set', addr=0x8100, type='exec')

    select(0)
    selected(0, entry)
    expect_nc_error(label('loc_D595'), b'Delete failed')
    assert m.read(state + 6, 1) == b'\1'  # Отказ не уменьшает счётчик вручную.
    check_original()

    select(7)
    descriptor[:11] = b'ROFROM  TXT'
    m.write(0x7000, descriptor)
    m.write(0x4000, bytes(range(256)) * 2)
    assert not m.subroutine(0xC845, hl=0x7000)['flags']['C']
    assert not stat(b'ROFROM  TXT')['flags']['C']
    source = m.read(0x7000, 32)
    selected(7, source)
    m.write(input_addr, b'A:RODEST.TXT\0')
    expect_nc_error(label('copyFileInt'), b'Copy failed')
    select(0)
    assert stat(b'RODEST  TXT')['flags']['C']
    check_original()
    assert system_path.read_bytes() == physical
    report = {'checks': ['delete refusal and immediate stat/load', 'rename refusal',
                         'create refusal', 'overwrite refusal preserves original',
                         'NC Delete failed without decrement', 'NC H-to-A Copy failed without phantom entry'],
              'systemSha256': hashlib.sha256(physical).hexdigest()}
    (ROOT / '.codex-tmp/readonly-report.json').write_text(json.dumps(report, indent=2) + '\n')
    print('Read-only A: mutation errors, unchanged contents/cache view and NC messages OK')


if __name__ == '__main__':
    main()
