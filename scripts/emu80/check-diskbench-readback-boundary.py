"""Проверка прямого HIGH и полной проверки файла на границах 1/24 КиБ."""
import json, re, runpy, struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ns = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
Matrix = ns['Matrix']
out = ROOT / 'docs/diskbench/readback-boundary-001'
out.mkdir(parents=True, exist_ok=True)
Matrix.__init__.__globals__['OUT'] = out
x = Matrix()
rows = []
try:
    x.start('boundary')
    x.install('FlashSys')
    for drive in [0, 1]:
        if drive == 1:
            x.format_ram()
        for kib in [1, 24]:
            x.init_app(drive, 6)
            s, m = x.s, x.m
            sectors = kib * 4
            m.write(s['v_fileKiB'], bytes([kib]))
            m.write(s['v_fileSectors'], bytes([sectors]))
            m.write(s['v_bufferEndHi'], bytes([0x20 + sectors]))
            m.write(s['v_budget'], b'\1')
            # Поле размера дескриптора хранит длину минус один.
            offset = int(re.search(r'^FILE_DESCRIPTOR\.size: EQU 0x([0-9A-Fa-f]+)',
                                  (ROOT / 'bin/DOS.sym').read_text(), re.M)[1], 16)
            m.write(s['tmpDescriptor'] + offset, (kib * 1024 - 1).to_bytes(4, 'little'))
            disk = ns['RUNTIME'] / 'specialist-os/system-flash.bin'
            before = disk.read_bytes() if drive == 0 else x.snapshot_metadata()
            x.traps(s['file_cleanup'])
            m.call('regs_set', pc=s['file_write_case'], sp=0x8FDF)
            x.until(s['file_cleanup'])
            expected = bytes(1 ^ (a >> 8) ^ (a & 255) for a in range(0x2000, 0x2000 + kib * 1024))
            assert m.read(0x2000, len(expected)) == expected
            high = m.read(s['resultsHigh'], 40)
            assert (struct.unpack_from('<I', high, 0)[0] > 0) == (drive == 0)
            assert struct.unpack_from('<IIH', high, 20) == (0, 0, 0)
            if drive == 0:
                assert struct.unpack_from('<I', high, 10)[0] > 0
                assert struct.unpack_from('<I', high, 30)[0] > 0
            assert all(int.from_bytes(m.read(s[k], 4), 'little') > 0 for k in ['statRawVerify', 'statFileVerify'])
            x.traps(s['file_results'])
            x.until(s['file_results'])
            assert m.read(s['v_fileOwned'], 1) == b'\0'
            x.m.stop_at(s['exit_wait'])
            x.m.until(s['exit_wait'])
            x.m.screenshot(out / f'{drive}-{kib}.png')
            after = disk.read_bytes() if drive == 0 else x.snapshot_metadata()
            x.check_integrity(before, after)
            rows.append(dict(drive=chr(65 + drive), sizeKiB=kib, originalPreserved=True, integrity=True))
            print(rows[-1], flush=True)
    (out / 'results.json').write_text(json.dumps(rows, indent=2) + '\n')
finally:
    x.stop()
