"""Проверки FlashSys и образа системной AT29C040A в тестовой установке."""
import argparse
import hashlib
import json
import re
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
helpers = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))
FileMachine, get_file = helpers['FileMachine'], helpers['get_file']


def entry(name):
    listing = (ROOT / 'bin' / (name + '.lst')).read_text(encoding='utf-8-sig')
    return int(re.search(r'^\s*\d+\s+([0-9A-F]{4})\s+Driver:\s*$', listing, re.M)[1], 16)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--verify-persisted', action='store_true')
    parser.add_argument('--locked', action='store_true')
    parser.add_argument('--report', type=Path, default=ROOT / '.codex-tmp/system-flash-report.json')
    args = parser.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужна отдельная установка Emu80-flash-test')
    image = args.runtime / 'specialist-os/system-flash.bin'
    disk_h = args.runtime / 'specialist-os/flash-disk.bin'
    m = FileMachine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    m.call('bp_set', addr=0x8100, type='exec')
    assert m.read(0xDC00, 8) == (ROOT / 'bin/FlashSys.com').read_bytes()[:8], 'FlashSys не установлен при загрузке'
    m.subroutine(0xC842, af=0x0100, de=1)
    m.subroutine(0xC842, af=0x0000, de=1)
    driver = entry('FlashSys')
    if args.locked:
        before = image.read_bytes()
        sector = 0x7FF
        original = before[sector * 256:(sector + 1) * 256]
        m.write(0x4000, bytes(value ^ 255 for value in original))
        m.write(0x8FF9, b'\0')
        state = m.subroutine(driver, bc=0x0100, de=sector, hl=0x4000)
        assert state['flags']['C'] and m.read(0x8FF9, 1) == b'\1'
        assert image.read_bytes() == before
        print('J4 read-only: FlashSys write rejected, image unchanged')
        return
    if args.verify_persisted:
        report = json.loads(args.report.read_text())
        for case in report['writes']:
            assert not m.subroutine(driver, bc=0x0200, de=case['sector'], hl=0x4000)['flags']['C']
            assert hashlib.sha256(m.read(0x4000, 256)).hexdigest() == case['sha256']
        assert image.with_suffix('.bin.sdp').read_bytes() == b'\1'
        report['persistentAfterRestart'] = True
        args.report.write_text(json.dumps(report, indent=2) + '\n')
        print('FlashSys: persisted sectors and SDP after restart OK')
        return
    before = image.read_bytes()
    h_hash = hashlib.sha256(disk_h.read_bytes()).hexdigest()
    assert len(before) == 524288
    for sector in [0, 1, 0x7F, 0x80, 0xFF, 0x100, 0x3FF, 0x7FF]:
        assert not m.subroutine(driver, bc=0x0200, de=sector, hl=0x4000)['flags']['C']
        assert m.read(0x4000, 256) == before[sector * 256:(sector + 1) * 256]
    writes = []
    for sector in [0x7F, 0x80, 0xFF, 0x100, 0x37F, 0x380, 0x7FE, 0x7FF]:
        payload = bytes((i * 71 + sector * 13 + (sector >> 8)) & 255 for i in range(256))
        m.write(0x4000, payload)
        state = m.subroutine(driver, bc=0x0100, de=sector, hl=0x4000)
        assert not state['flags']['C'], f'Ошибка записи сектора {sector:04X}'
        assert not m.subroutine(driver, bc=0x0200, de=sector, hl=0x4100)['flags']['C']
        assert m.read(0x4100, 256) == payload
        with image.open('rb') as f:
            f.seek(sector * 256)
            assert f.read(256) == payload
        writes.append({'sector': sector, 'sha256': hashlib.sha256(payload).hexdigest()})
    assert hashlib.sha256(disk_h.read_bytes()).hexdigest() == h_hash
    report = {'writes': writes, 'seedReadSectors': [0, 1, 127, 128, 255, 256, 1023, 2047],
              'hUnchanged': True, 'persistentAfterRestart': False}
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('FlashSys: 8 reads, 8 writes across page/chip boundaries, H: unchanged OK')


if __name__ == '__main__':
    main()
