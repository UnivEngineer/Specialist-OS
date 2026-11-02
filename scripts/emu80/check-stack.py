"""Проверка нижнего системного стека в отдельном Emu80 без записи на диски."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(Path(__file__).with_name('check-flash.py')))['Machine']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', type=Path, required=True)
    args = p.parse_args()
    runtime = args.runtime.resolve()
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    owner = subprocess.run(['powershell.exe', '-NoProfile', '-Command', query],
                           capture_output=True, text=True, check=True).stdout.strip()
    assert Path(owner).resolve() == runtime / 'Emu80qt-mcp.exe'
    assert runtime == ROOT / '.local/monitor-compat-001'
    image = (ROOT / 'images/SystemRom_emulator.bin').read_bytes()
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    m.call('emu_reset')
    assert m.read(0, 32768) == image[32768:]
    results = {'image_sha256': hashlib.sha256(image).hexdigest(), 'rom_verified': True}
    try:
        m.call('emu_run_for', ms=2000)
        assert m.read(0xE800, 32) == (ROOT / 'bin/NC.com').read_bytes()[:32]
        state = m.call('cpu_state')
        assert 0x8DDF <= state['registers']['sp']['dec'] < 0x8FDF
        results['nc_boot_sp'] = state['registers']['sp']['hex']

        def execute(code, **regs):
            m.call('bp_clear')
            m.write(0x8200, code)
            m.call('bp_set', addr=0x8300, type='exec')
            m.call('regs_set', pc=0x8200, sp=0x8FDF, **regs)
            m.call('emu_run_for', ms=500)
            s = m.call('cpu_state')
            assert s['registers']['pc']['dec'] == 0x8300, s
            assert s['registers']['sp']['dec'] == 0x8FDF, s
            return s

        s = execute(bytes.fromhex('CD 30 C8 C3 00 83'))
        assert s['registers']['hl']['dec'] == 0x8DDE
        results['mem_top'] = s['registers']['hl']['hex']
        # Имя готовится штатно; затем весь NC читается с A: в нижнее RAM.
        m.write(0x8400, b'A:NC.COM\0' + bytes(32))
        s = execute(bytes.fromhex('21 00 84 11 00 85 CD 5A C8 21 00 85 11 00 40 CD 66 C8 C3 00 83'))
        assert not s['flags']['C']
        nc = (ROOT / 'bin/NC.com').read_bytes()
        assert m.read(0x4000, len(nc)) == nc
        results['file_load2_A_NC'] = True
        # STD: код выше 8000h остаётся видимым; сумма не обращается к портам.
        m.write(0x8100, bytes((1, 2, 3)))
        code = bytes.fromhex('3E 03 32 F8 FF 32 FF FF 21 00 81 11 02 81 CD 2A C8 32 FC F7 3E F0 32 F8 FF C3 00 83')
        s = execute(code)
        assert s['registers']['bc']['dec'] == 0x0306
        results['std_call_ret_checksum'] = s['registers']['bc']['hex']
        # DEBUG: не перекрывает собственные переменные новым верхом стека.
        debug = (ROOT / 'bin/Debug.com').read_bytes()
        m.write(0xF100, debug)
        m.call('regs_set', pc=0xF100, sp=0x8FDF)
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        s = m.call('cpu_state')
        assert 0x8DDF <= s['registers']['sp']['dec'] < 0x8F60
        results['debug_prompt_sp'] = s['registers']['sp']['hex']
    finally:
        m.call('bp_clear')
        m.call('emu_reset')
        m.call('emu_run_for', ms=1500)
        m.call('debug_break')
    out = ROOT / 'docs/dos/stack-001'
    out.mkdir(parents=True, exist_ok=True)
    (out / 'checks.json').write_text(json.dumps(results, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(results, indent=2))


if __name__ == '__main__':
    main()
