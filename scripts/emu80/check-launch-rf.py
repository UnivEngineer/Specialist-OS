"""RUN -RF -> RAMFOS в отдельном тестовом runtime Emu80.

Проверяются чтения файлов, замена/инициализация ядра, нагрузка и аргументы.
Вместо исполнения игры в RAM ставится RET для проверки возврата; файлы игр
не меняются. Игровой процесс этим не проверяется.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
Machine = runpy.run_path(str(Path(__file__).with_name('check-flash.py')))['Machine']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path)
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    if runtime != ROOT / '.local/monitor-compat-001':
        raise ValueError('Разрешён только отдельный .local/monitor-compat-001')

    def owner():
        query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
        path = subprocess.run(['powershell.exe', '-NoProfile', '-Command', query],
                              capture_output=True, text=True, check=True).stdout.strip()
        if Path(path).resolve() != runtime / 'Emu80qt-mcp.exe':
            raise ValueError('MCP принадлежит другой машине')

    owner()
    m = Machine('http://127.0.0.1:19781/mcp')
    image = (ROOT / 'images/SystemRom_emu80_writable.bin').read_bytes()
    kernel = (ROOT / 'bin/RAMFOS.mon').read_bytes()
    cases = [('H:BUDI2-MX.EXE', 'assets/games/BUDI/Budi2-mx.rks'),
             ('H:BURAN-RF.EXE', 'assets/games/Buran/Buran-ramfos.cpu')]
    results = {'system_sha256': hashlib.sha256(image).hexdigest(), 'cases': []}
    for name, source in cases:
        owner()
        m.call('bp_clear')
        m.call('emu_reset')
        assert m.read(0, 32768) == image[:32768]
        m.call('emu_run_for', ms=2500)
        assert m.read(SYSTEM['NC_LOAD_ADDR'], 32) == (ROOT / 'bin/NC.i80').read_bytes()[:32]
        command = f'A:RUN.COM -RF {name} /X one\0'.encode('ascii')
        m.write(0x7300, command)
        m.write(0x8000, bytes.fromhex('21 00 73 cd 5d c8 c3 00 81'))
        m.call('bp_set', addr=0x8100, type='exec')
        m.call('bp_set', addr=0x8C00, type='exec')
        m.call('regs_set', pc=0x8000, sp=SYSTEM['STACK_ADDR'])
        for _ in range(120):
            m.call('emu_run_for', ms=500)
            state = m.call('cpu_state')['registers']
            if state['pc']['dec'] in (0x8100, 0x8C00):
                break
        assert state['pc']['dec'] == 0x8C00, (name, state)
        assert m.read(0x9000, len(kernel)) == kernel
        # F85Ah сначала занят кодом RAMRUN: ставить этот breakpoint можно
        # лишь после достижения переходника 8C00h.
        m.call('bp_clear')
        m.call('bp_set', addr=0xF85A, type='exec')
        m.call('emu_run_for', ms=500)
        state = m.call('cpu_state')['registers']
        assert state['pc']['dec'] == 0xF85A, (name, state)
        assert m.read(0xC000, 0x1300) == kernel[:0x1300]
        assert m.read(0xF800, 0x7E0) == kernel[0x1300:]
        expected = (ROOT / source).read_bytes()
        if source.endswith('.rks'):
            expected = expected[4:-2]
        assert m.read(0, len(expected)) == expected
        actual_args = m.read(0x8E00, 128).split(b'\0', 1)[0]
        assert actual_args.lstrip(b' ') == b'/X one', actual_args
        assert state['hl']['dec'] == 0 and state['de']['dec'] == 0x8E00
        m.call('bp_clear')
        m.call('bp_set', addr=0, type='exec')
        m.call('emu_run_for', ms=500)
        state = m.call('cpu_state')['registers']
        assert state['pc']['dec'] == 0 and state['de']['dec'] == 0x8E00
        assert m.read(0, 16) == expected[:16]
        m.write(0, b'\xC9')
        m.call('bp_clear')
        m.call('emu_run_for', ms=2500)
        assert m.read(SYSTEM['NC_LOAD_ADDR'], 32) == (ROOT / 'bin/NC.i80').read_bytes()[:32]
        results['cases'].append({'file': name, 'payload_bytes': len(expected),
            'kernel_matches': True, 'payload_matches': True,
            'args': actual_args.decode('ascii'), 'init_and_entry': True,
            'ret_reboots_to_nc': True})
    out = ROOT / 'docs/compat/launch-rf-001'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'emu80.json').write_text(json.dumps(results, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
