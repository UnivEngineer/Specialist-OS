"""Видимый Emu80: запрос всех банков из NC, проверка пустого и занятого B:."""
import argparse
import os
from pathlib import Path
import re
import runpy
import shutil
import struct
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/resident-pressure'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, choices=range(19781, 19785), required=True)
    port = parser.parse_args().port
    OUT.mkdir(parents=True, exist_ok=True)
    h = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
    k = h['symbols'](ROOT / 'bin/banked/KERNEL.sym')
    n = {name: int(value, 16) for name, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (ROOT / 'bin/NC_CORE.sym').read_text('utf-8'), re.M)}
    probe = OUT / 'probe'
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080',
                    f'--raw={probe}.i80', f'--sym={probe}.sym',
                    (ROOT / 'scripts/emu80/fixtures/pressure-probe.asm').as_posix()],
                   check=True, cwd=ROOT)
    p = h['symbols'](probe.with_suffix('.sym'))
    probe.with_suffix('.cpu').write_text('0000\n0000\nmxos2.mon\nFLAGS=04\n', encoding='ascii')
    manifest = OUT / 'H-Flash-2M-Pressure-Test.txt'
    manifest.write_text('@label PRESSURE\nPROBE.COM ../.local/resident-pressure/probe.cpu\n', encoding='ascii')
    h['command'](['powershell.exe', '-NoProfile', '-File', 'scripts/images/build-image.ps1',
                  'Games2M', '-ManifestFile', manifest])

    runtime = Path(os.environ['LOCALAPPDATA']) / 'Specialist-OS' / 'emu80' / str(port)
    exe = runtime / 'Emu80qt-mcp.exe'
    machine = h['Bench'](f'http://127.0.0.1:{port}/mcp')

    def listener():
        return h['shell'](
            f'$p=Get-NetTCPConnection -State Listen -LocalPort {port} -ErrorAction SilentlyContinue; '
            'if ($p) { $p[0].OwningProcess }; exit 0')

    assert exe.is_file() and not listener(), 'Требуется свободный тестовый слот Emu80'
    h['command'](['powershell.exe', '-NoProfile', '-File', 'scripts/emu80/run.ps1',
                  '-EmulatorDirectory', runtime, '-Flash', '-Mcp', '-PrepareOnly',
                  '-UpdateSystemFlash', '-TestFlashSeed'])
    shutil.copyfile(ROOT / 'images' / manifest.with_suffix('.bin').name,
                    runtime / 'specialist-os/test-flash.bin')
    process = subprocess.Popen([str(exe), '--conf-file', 'specialist-os/launch.conf',
                                '--platform', 'specialist-os'], cwd=runtime)

    def page():
        return machine.read(k['bankPage'], 1)[0]

    def select(bank):
        machine.write(0xfffd if bank else 0xfffc, bytes([bank - 1 if bank else 0]))

    def bank_read(bank, address, size):
        before = page()
        select(bank)
        try:
            return machine.read(address, size)
        finally:
            select(before)

    def dos(name, size=1):
        return bank_read(1, k[name], size)

    def nc_handle():
        keys = dos('residentKeys', 2 * k['MODULE_MAX'])
        key = struct.pack('<H', k['NC_BANK_KEY'])
        for slot in range(k['MODULE_MAX']):
            if keys[2 * slot:2 * slot + 2] == key:
                address = k['MODULE_TABLE'] + slot * k['MODULE_RECORD_SIZE']
                return int.from_bytes(bank_read(1, address, 2), 'little')
        return 0

    def wait_nc():
        address = n['ncBankInputPoll']
        machine.stop_at(address)
        for _ in range(16):
            machine.until(address, 90)
            if page() > 1:
                return
            machine.call('debug_step', mode='into')
        raise AssertionError('Банковый NC не дошёл до опроса клавиатуры')

    def command(value, target):
        machine.call('bp_clear')
        machine.call('emu_run_for', ms=500)
        machine.tap('esc')
        machine.call('emu_run_for', ms=250)
        keys = {' ': 'space', ':': 'colon', '.': 'period', '/': 'slash'}
        for char in value.lower():
            machine.tap(keys.get(char, char))
        assert machine.read(n['vars.cmdLine'], len(value)).lower() == value.lower().encode()
        machine.stop_at(target)
        machine.tap('enter')
        machine.until(target, 90)

    def run_probe():
        command('H:/PROBE.COM', p['probeResult'])
        assert page() == 0 and not nc_handle()
        return struct.unpack('<BBHHH', machine.read(p['probeBanks'], 8))

    def continue_nc():
        machine.call('bp_clear')
        machine.call('debug_step', mode='into')
        wait_nc()

    try:
        for _ in range(30):
            if listener() == str(process.pid):
                break
            if process.poll() is not None:
                raise RuntimeError('Emu80 завершился до запуска MCP')
            time.sleep(.2)
        else:
            raise RuntimeError('Emu80 не занял свой MCP-порт')
        machine.call('debug_break')
        h['set_benchmark_speed'](machine, process.pid)
        wait_nc()
        pool = dos('memoryRamPages')[0]
        total = dos('bankPageCount')[0] - 1
        initial = nc_handle()
        assert initial and pool == 4
        default_panel = machine.read(n['state.activePanel'], 1)
        machine.call('bp_clear')
        machine.call('emu_run_for', ms=500)
        machine.tap('tab')
        wait_nc()
        panel = machine.read(n['state.activePanel'], 1)
        assert panel != default_panel
        count, status, *_ = run_probe()
        assert (count, status) == (total, k['MEM_ERROR_NOMEM']), (count, status, total)
        assert dos('pressurePages') == bytes([pool]) and dos('memoryRamPages') == b'\0'
        assert dos('pressureStateSize') == b'\1'
        continue_nc()
        assert nc_handle() not in (0, initial)
        assert dos('memoryRamPages') == bytes([pool]) and dos('pressurePages') == b'\0'
        assert machine.read(n['state.activePanel'], 1) == panel
        print(f'PASS: NC вытеснен, выдано {count} банков, B: и NC восстановлены', flush=True)

        command('MD B:/KEEP', k['cliMkdir'])
        continue_nc()
        before = bank_read(2, 0, 4096)
        assert b'KEEP       ' in before
        count, status, detail, name, reason = run_probe()
        assert (count, status) == (total - pool, k['RESIDENT_APP_CANT_UNLOAD'])
        bank, code = detail >> 8, detail & 255
        assert code == 1 and bank_read(bank, name, 12) == b'RAMDISK.COM\0'
        assert bank_read(bank, reason, 22) == b'RAM disk is not empty\0'
        assert dos('memoryRamPages') == bytes([pool]) and dos('pressurePages') == b'\0'
        continue_nc()
        assert nc_handle() and bank_read(2, 0, 4096) == before
        print('PASS: E8h и RAMDISK.COM, содержимое B: сохранено, NC восстановлен', flush=True)
    finally:
        if process.poll() is None:
            process.terminate()
            process.wait(timeout=10)


if __name__ == '__main__':
    main()
