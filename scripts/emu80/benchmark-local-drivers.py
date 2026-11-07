"""Сопоставимый DISKBEN до/после изменения драйверов банковой DOS."""
from pathlib import Path
import argparse
import hashlib
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--runtime', type=Path, default=ROOT/'.local/monitor2-001')
    args = parser.parse_args()
    out = args.report.resolve()
    if (out/'raw.json').exists():
        parser.error('Choose a new report directory')
    out.mkdir(parents=True, exist_ok=True)
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    g = h['Matrix'].__init__.__globals__
    g.update(OUT=out, RUNTIME=args.runtime.resolve(), PASSES=1)
    matrix = h['Matrix']()
    matrix.report['commit'] = subprocess.check_output(['git', 'rev-parse', 'HEAD'], cwd=ROOT, text=True).strip()
    matrix.report['workingTreeDiff'] = subprocess.check_output(['git', 'diff', '--stat'], cwd=ROOT, text=True)
    matrix.report['sha256'] = {name: hashlib.sha256((ROOT/name).read_bytes()).hexdigest() for name in (
        'bin/BIOS.sys', 'bin/DOS.sys', 'bin/DISKBEN.i80', 'images/SystemRom_emu80_writable.bin')}
    try:
        matrix.start('local-drivers')
        matrix.console()
        for drive, name, mode in ((1, 'Bank-CPM16', 1), (1, 'Bank-CPM16', 6),
                                  (0, 'FlashSys', 1), (7, 'Flash', 1)):
            matrix.writable(drive, name, mode)
        matrix.install('FlashFst')
        matrix.writable(7, 'FlashFst', 1)
        matrix.writable(7, 'FlashFst', 6)
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
