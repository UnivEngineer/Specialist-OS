"""Скомпилировать стандартный printf и проверить его в MAIN и EXEC через скрытый Emu80."""

import argparse
from contextlib import contextmanager
import os
from pathlib import Path
import re
import runpy
import shutil
import struct
import subprocess
import tempfile
import time


ROOT = Path(__file__).resolve().parents[2]
BIN = ROOT / 'bin'
LOAD = 0x0100
STUB, DONE, SP = 0x8e00, 0x8e10, 0x8fdf
PATH, DATA, HANDLE = 0x8500, 0x3000, 0x8400
TEXT64 = b'0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz+-'
TEXT256 = TEXT64 * 4
TEXT300 = TEXT256 + TEXT64[:44]


def word(value):
    return struct.pack('<H', value)


def build(out, z88dk):
    env = os.environ.copy()
    env['ZCCCFG'] = str(z88dk / 'lib/config')
    env['PATH'] = str(z88dk / 'bin') + os.pathsep + env['PATH']
    subprocess.run([
        str(z88dk / 'bin/zcc.exe'), '+cpm', '-clib=8080', '-compiler=sccz80',
        '-crt0=' + str(ROOT / 'sdk/runtime/crt0'), '-zorg=256', '-I' + str(ROOT / 'sdk/include'),
        *(str(ROOT / 'sdk/src' / source) for source in (
            'mxos_z88dk.asm', 'mxos_far.c', 'mxos_heap.c', 'mxos_printf.asm')),
        str(ROOT / 'sdk/tests/printf.c'),
        '-o', str(out / 'printf.bin'), '-m',
    ], check=True, cwd=ROOT, env=env, timeout=60)
    mapping = (out / 'printf.map').read_text(encoding='utf-8')
    symbols = {name: int(value, 16) for name, value in re.findall(
        r'^([\w.]+)\s*=\s*\$([0-9a-fA-F]+)', mapping, re.M)}
    binary = (out / 'printf_code_crt_init.bin').read_bytes()[:symbols['__BSS_head'] - LOAD]
    assert LOAD + len(binary) < DATA and symbols['__Start'] == LOAD
    call_main = binary.index(b'\xcd' + word(symbols['_main']) + b'\xc9',
                             symbols['crtLocalHeap'] - LOAD, symbols['crtHeapRefused'] - LOAD)
    executable = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))
    images = {name: executable['encode'](executable['Image'](
        binary, LOAD, LOAD, flags, bss=symbols['__BSS_END_tail'] - symbols['__BSS_head']), 'com')
        for name, flags in (('PRMAIN', 4), ('PREXEC', 2))}
    return binary, symbols['_main'], LOAD + call_main + 3, images


@contextmanager
def emulator(runtime_seed, port, bench):
    helpers = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
    shell, quote = helpers['shell'], helpers['ps']

    def owner():
        return shell(f'$p=Get-NetTCPConnection -State Listen -LocalPort {port} '
                     '-ErrorAction SilentlyContinue; if ($p) { $p[0].OwningProcess }; exit 0')

    assert not owner(), f'MCP-порт {port} занят'
    with tempfile.TemporaryDirectory(prefix='printf-emu80-') as temporary:
        runtime = Path(temporary) / 'runtime'
        shutil.copytree(runtime_seed, runtime)
        executable = runtime / 'Emu80qt-mcp.exe'
        assert executable.is_file()
        assert not owner(), f'MCP-порт {port} занят'
        command = (f'$p=& {quote(ROOT / "scripts/emu80/run.ps1")} -EmulatorDirectory '
                   f'{quote(runtime)} -Hidden -Flash -ResetFlash -FreshFloppy -Mcp -PassThru; '
                   '$p.Id')
        pid = int(shell(command).splitlines()[-1])
        try:
            for _ in range(30):
                listener = owner()
                if listener:
                    assert listener == str(pid), f'Порт {port} слушает PID {listener}'
                    break
                time.sleep(0.1)
            else:
                raise RuntimeError(f'Emu80 PID {pid} не закрепил MCP-порт {port}')
            assert Path(shell(f'(Get-Process -Id {pid}).Path')).resolve() == executable.resolve()
            assert shell(f'(Get-Process -Id {pid}).MainWindowHandle.ToInt64()') == '0'
            config = (runtime / 'specialist-os/launch.conf').read_text(encoding='utf-8')
            assert all(f'{device}.muted = yes' in config for device in ('pitSoundSource', 'tapeMonitor'))
            machine = bench(f'http://127.0.0.1:{port}/mcp')
            cpu = machine.call('sys_info')['cpu']
            assert cpu['speed_factor'] == 32 and not cpu['full_throttle'], cpu
            machine.call('debug_break')
            yield machine
        finally:
            shell(f'$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; '
                  f'if ($p -and $p.Path -eq {quote(executable)}) '
                  '{ Stop-Process -InputObject $p; $p.WaitForExit() }; exit 0')


def check(machine, symbols, entry, exit_main, images):
    k = symbols(BIN / 'banked/KERNEL.sym')
    b = symbols(BIN / 'BIOS.sym')
    nc = symbols(BIN / 'NC_CORE.sym')
    poll = nc['ncBankInputPoll']
    machine.stop_at(poll)
    for _ in range(30):
        machine.until(poll, 30)
        if machine.read(k['bankPage'], 1)[0] > k['DOS_IO_PAGE']:
            break
        machine.call('debug_step', mode='into')
    else:
        raise RuntimeError('NC EXEC не дошёл до опроса клавиатуры')
    machine.write(0xfffc, b'\0')

    def setup(operation, **registers):
        machine.write(STUB, b'\xcd' + word(b['mx_callRegs']) + word(k[operation]) + b'\xc3' + word(DONE))
        machine.call('regs_set', pc=STUB, sp=SP, **registers)

    def invoke(operation, **registers):
        machine.stop_at(DONE)
        setup(operation, **registers)
        state = machine.until(DONE, 30, page=0)
        assert not state['flags']['C'], (operation, state)
        return state

    invoke('MXOS_DOS_FILE_SELECT_DRIVE', af=0x100, de=1)
    for name, image in images.items():
        handle = bytearray(64)
        handle[:11] = name.ljust(8).encode('ascii') + b'COM'
        handle[40:42] = bytes((1, 15))
        machine.write(HANDLE, handle)
        invoke('MXOS_DOS_FILEHANDLE', af=0, hl=HANDLE)
        machine.write(DATA, image)
        machine.write(HANDLE + 49, struct.pack('<HH', DATA, len(image)))
        invoke('MXOS_DOS_FILEHANDLE', af=0x0200, hl=HANDLE)
        assert machine.read(HANDLE + 51, 2) == word(len(image))
        invoke('MXOS_DOS_FILEHANDLE', af=0x0500, hl=HANDLE)

    print_entry = b['bios_printStringLim']
    copy_entry = b['mx_memcpyFar']
    for name in images:
        machine.write(PATH, f'B:/{name}.COM\0'.encode('ascii'))
        machine.stop_at(entry)
        setup('MXOS_DOS_FILE_EXEC_NATIVE', hl=PATH)
        for _ in range(30):
            machine.until(entry, 30)
            page = machine.read(k['bankPage'], 1)[0]
            if page == 0 or page > k['DOS_IO_PAGE']:
                break
            machine.call('debug_step', mode='into')
        else:
            raise RuntimeError(f'{name} не достиг main')
        assert (page == 0) == (name == 'PRMAIN'), (name, page)
        prefix = b'[-32768/65535/MXOS/X/%]\r\nA\0B\nC\rD'
        long_text = TEXT256 * 2 + b'<' + TEXT256 + b'>' + TEXT256 + b'!'
        expected = prefix + long_text + (TEXT300 * 2 + b'!\0!' + TEXT256 if page else b'')
        expected += f'\r\npage={page}\r\n'.encode('ascii')
        machine.stop_at(exit_main)
        machine.call('bp_set', addr=print_entry, type='exec')
        machine.call('bp_set', addr=copy_entry, type='exec')
        actual = bytearray()
        chunks = []
        copies = []
        for _ in range(len(expected) + 30):
            machine.call('emu_run_for', ms=1000)
            regs = {name: reg['dec'] for name, reg in machine.call('cpu_state')['registers'].items()}
            pc = regs['pc']
            if pc not in (exit_main, print_entry, copy_entry):
                continue
            active = machine.read(k['bankPage'], 1)[0]
            if pc == exit_main and active == page:
                assert regs['hl'] == 0, (name, 'main вернул ошибку', regs['hl'], bytes(actual[:40]))
                break
            if pc == print_entry and active == 0:
                chunks.append((len(actual), regs['hl'], regs['bc']))
                actual.extend(machine.read(regs['hl'], regs['bc']))
            if pc == copy_entry and active == page:
                source_page, _, target_page, target, count = struct.unpack('<BHBHH', machine.read(regs['hl'], 8))
                copies.append((source_page, target_page, target, count))
            machine.call('debug_step', mode='into')
        else:
            raise RuntimeError(f'{name} не завершил main')
        assert actual == expected, (name, len(actual), len(expected), bytes(actual[:40]))
        if page:
            bound_address = 0x7400
            bound_start = len(prefix) + len(long_text)
            bound_end = bound_start + 603
            bound = [(offset, length) for offset, address, length in chunks if address == bound_address]
            assert bound == [(bound_start, 300), (bound_start + 300, 300), (bound_start + 600, 3)], bound
            assert copies == [(page, 0, bound_address, length) for _, length in bound], copies
            window = b['BANK_WINDOW']
            assert all(address == window and 1 <= length <= 32
                       for offset, address, length in chunks if not bound_start <= offset < bound_end), chunks
            assert (bound_end, window, 32) in chunks, chunks
        else:
            assert not copies, copies
        machine.stop_at(DONE)
        state = machine.until(DONE, 30, page=0)
        assert not state['flags']['C'] and state['registers']['sp']['dec'] == SP, (name, state)
        print(f'PASS {name}: page {page}, {len(actual)} bytes, printf returns and stack', flush=True)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True,
                        help='Шаблон Emu80 MCP с фиксированной скоростью 32x и Qt offscreen')
    parser.add_argument('--port', type=int, choices=range(19781, 19785), required=True)
    parser.add_argument('--z88dk', type=Path, default=Path(os.environ.get('SPECIALIST_Z88DK_DIR',
                        'D:/Projects/RetroPC/Specialist/MySoft/z88dk')))
    args = parser.parse_args()
    helpers = runpy.run_path(str(ROOT / 'source-apps/diskbench/check-emulator.py'))
    with tempfile.TemporaryDirectory(prefix='printf-8080-') as temporary:
        binary, entry, exit_main, images = build(Path(temporary), args.z88dk.resolve())
        print(f'sccz80/8080: {len(binary)} bytes', flush=True)
        with emulator(args.runtime.resolve(), args.port, helpers['Bench']) as machine:
            check(machine, helpers['symbols'], entry, exit_main, images)


if __name__ == '__main__':
    main()
