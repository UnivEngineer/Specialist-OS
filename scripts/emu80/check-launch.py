"""Режимы LAUNCH и стек редактора в отдельной flash-машине Emu80."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(Path(__file__).with_name('check-flash.py')))['Machine']


def symbols(name):
    return {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
            (ROOT / f'bin/{name}.sym').read_text(encoding='utf-8-sig'), re.M)}


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', type=Path, required=True)
    args = p.parse_args()
    runtime = args.runtime.resolve()
    assert runtime == ROOT / '.local/monitor-compat-001'
    def verify_owner():
        query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
        owner = subprocess.run(['powershell.exe', '-NoProfile', '-Command', query],
                               capture_output=True, text=True, check=True).stdout.strip()
        assert Path(owner).resolve() == runtime / 'Emu80qt-mcp.exe'
    verify_owner()
    image = (ROOT / 'images/SystemRom_emu80_writable.bin').read_bytes()
    m = Machine('http://127.0.0.1:19781/mcp')
    nc, ed = symbols('NC'), symbols('E')
    results = {'image_sha256': hashlib.sha256(image).hexdigest()}

    def boot():
        verify_owner()
        m.call('bp_clear')
        m.call('emu_reset')
        assert m.read(0, 32768) == image[:32768]
        m.call('emu_run_for', ms=2500)
        assert m.read(0xE800, 32) == (ROOT / 'bin/NC.com').read_bytes()[:32]

    def stop_at(addr):
        m.call('bp_clear')
        m.call('bp_set', addr=addr, type='exec')

    def until(addr):
        for _ in range(5):
            verify_owner()
            m.call('emu_run_for', ms=2000)
            s = m.call('cpu_state')
            if s['registers']['pc']['dec'] == addr:
                break
        assert s['registers']['pc']['dec'] == addr, s
        return s

    def command(text, target):
        verify_owner()
        m.write(0x8400, text.encode('ascii') + b'\0')
        m.write(0x8200, bytes.fromhex('21 00 84 CD 5D C8 C3 00 83'))
        stop_at(target)
        m.call('regs_set', pc=0x8200, sp=0x8FDF)
        return until(target)

    try:
        for mode in ('MX', 'STD', 'MON', 'MON:Monitor2'):
            print(f'Checking {mode}', flush=True)
            boot()
            s = command(f'A:LAUNCH.COM -{mode} A:CHKDSK.COM', 0)
            print(f'Entry SP={s["registers"]["sp"]["hex"]}', flush=True)
            swapped = mode.startswith('MON')
            assert m.read(0xC000, 16) == ((ROOT / 'assets/programs/Monitor2.mon').read_bytes()[:16]
                    if swapped else (ROOT / 'bin/DOS.sys').read_bytes()[:16])
            assert (m.read(0xF800, 8) == (ROOT / 'bin/LAUNCH.com').read_bytes()[:8]) == (mode == 'MX')
            # Заменяем только загруженную тестовую программу в RAM на RET.
            m.write(0, b'\xC9')
            target = nc['mainLoop'] if swapped else 0x8300
            stop_at(target)
            returned = until(target)
            if not swapped:
                assert returned['registers']['sp']['dec'] == 0x8FDF
                assert m.read(0xF800, 8) == (ROOT / 'bin/LAUNCH.com').read_bytes()[:8]
            results[mode] = {'entry_sp': s['registers']['sp']['hex'], 'return_pc': hex(target)}
        for option in ('-WRONG', '-MON:', '-MON:TOOLONGNAME', '-MON:Monitor1'):
            boot()
            # fileExec очищает CF в defAppReturn; проверяем контракт LAUNCH прямо.
            m.write(0xF800, (ROOT / 'bin/LAUNCH.com').read_bytes())
            m.write(0x8400, f'{option} A:CHKDSK.COM\0'.encode('ascii'))
            m.write(0x8200, bytes.fromhex('11 00 84 CD 00 F8 C3 00 83'))
            stop_at(0x8300)
            m.call('regs_set', pc=0x8200, sp=0x8FDF)
            s = until(0x8300)
            assert s['flags']['C'], s
            results[option] = 'rejected'
        boot()
        s = command('A:E.COM A:README.TXT', ed['keyInput'])
        saved = int.from_bytes(m.read(ed['smc_resetSessionStack'] + 1, 2), 'little')
        assert saved == 0x8F50 and 0x8DDF <= s['registers']['sp']['dec'] < saved
        results['editor'] = {'saved_sp': hex(saved), 'waiting_sp': s['registers']['sp']['hex']}
        # Реальный цикл NC копирует обработчик целиком, включая ключ -MON.
        boot()
        m.write(0x8400, b'A:LAUNCH.COM -MON\r')
        stop_at(nc['Enter_stdExec'])
        m.call('regs_set', pc=nc['Enter_5'], de=0x8400, sp=0x8FDF)
        until(nc['Enter_stdExec'])
        prefix = b'A:LAUNCH.COM -MON \0'
        assert m.read(nc['vars.cmdLine'], len(prefix)) == prefix
        results['association_prefix'] = 'A:LAUNCH.COM -MON '
    finally:
        boot()
        m.call('debug_break')
    out = ROOT / 'docs/compat/launch-001'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'checks.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
