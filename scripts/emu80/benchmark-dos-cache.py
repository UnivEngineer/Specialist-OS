"""Сопоставимый замер кэша DOS: штатные процедуры DISKBEN, 8 КиБ, три прохода."""
import argparse
import json
import runpy
import struct
import shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def prepare_fixture(h, out):
    """Фиксируем образ и символы оболочки перед запуском измерений."""
    get_file=runpy.run_path(str(ROOT/'scripts/emu80/check-copy.py'))['get_file']
    for attempt in range(3):
        h['command'](['powershell.exe','-NoProfile','-File','scripts/images/build-image.ps1','SystemEmuWritable'])
        shutil.copyfile(ROOT/'bin/NC.sym',out/'NC.sym')
        shutil.copyfile(ROOT/'bin/NC.i80',out/'NC.i80')
        image=(ROOT/'images/SystemRom_emu80_writable.bin').read_bytes()
        if get_file(image,'NC.COM')[1][16:] == (out/'NC.i80').read_bytes():
            (out/'SystemRom.bin').write_bytes(image)
            break
    else:
        raise RuntimeError('NC changes during image preparation; repeat benchmark')
    return dict(system=out/'SystemRom.bin',nc_binary=out/'NC.i80',nc_symbols=out/'NC.sym')


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, required=True)
    args = p.parse_args()
    out = args.report.resolve()
    out.mkdir(parents=True, exist_ok=True)
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    h['Matrix'].__init__.__globals__.update(OUT=out, RUNTIME=ROOT/'.local/monitor2-001', PASSES=3)
    x = h['Matrix']()
    try:
        x.start('dos-cache',**prepare_fixture(h,out))
        x.console()
        for drive in (1, 0, 7):
            for mode in (1, 6):
                info = x.init_app(drive, mode)
                # Точка остановки вне кода DOS: обычные BP по низким адресам
                # иначе многократно срабатывают в другом банке.
                for label, target in (('file_results', 0x8E90), ('error', 0x8E91), ('file_exists', 0x8E91)):
                    x.m.write(x.s[label], b'\xC3'+struct.pack('<H', target))
                x.m.stop_at(0x8E90)
                x.m.call('bp_set', addr=0x8E91, type='exec')
                x.m.call('regs_set', pc=x.s['file_write_case'], sp=0x8FDF)
                for _ in range(180):
                    state = x.m.call('cpu_state')
                    pc = state['registers']['pc']['dec']
                    assert pc != 0x8E91, state
                    if pc == 0x8E90:
                        break
                    x.m.call('emu_run_for', ms=1000)
                else:
                    raise RuntimeError(state)
                assert x.m.read(x.s['v_fileOwned'], 1) == b'\0'
                assert x.m.read(x.s['v_pass'], 1) == b'\4'
                raw = x.m.read(x.s['statAll'], 40)
                metrics = dict(zip(('rawWrite', 'rawRead', 'fileWrite', 'fileRead'),
                                   (struct.unpack_from('<IIH', raw, i*10) for i in range(4))))
                row = dict(info, mode=mode, metrics=metrics)
                if drive == 0:
                    high = x.m.read(x.s['resultsHigh'], 40)
                    row['high'] = dict(zip(metrics, (struct.unpack_from('<IIH', high, i*10) for i in range(4))))
                x.report['rows'].append(row)
                x.save()
                print(info['drive'], mode, {k: round(24*7812.5/v[0], 2) for k, v in metrics.items()}, flush=True)
        x.report['integrity'] = 'DISKBEN сравнивает данные после каждого чтения, удаляет только свой файл'
        x.save()
    finally:
        x.stop()


if __name__ == '__main__':
    main()
