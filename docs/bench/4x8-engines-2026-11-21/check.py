"""Сетка FontBen через клавиатуру двух профилей NC, замер по таймеру ВИ53."""
import argparse
import json
import re
import runpy
import subprocess
import time
import urllib.request
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, required=True)
parser.add_argument('--runtime', type=Path, required=True)
parser.add_argument('--port', type=int, choices=range(19781, 19785), required=True)
parser.add_argument('--engine', choices=('Compact', 'Packed', 'Fast', 'FAST4'))
parser.add_argument('--polarity', choices=('normal', 'inverse'))
parser.add_argument('--file', action='store_true', help='Установить Fast с Fast.pre перед замером')
parser.add_argument('--output', type=Path)
args = parser.parse_args()
root = args.root.resolve()
runtime = args.runtime.resolve()
output = args.output.resolve() if args.output else Path(__file__).resolve().parent
output.mkdir(parents=True, exist_ok=True)
exe = runtime / 'Emu80qt-mcp.exe'
assert exe.is_file()

def symbols(name):
    return {label: int(value, 16) for label, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9a-fA-F]+)', (root / f'bin/{name}.sym').read_text(), re.M)}

def ps(source):
    return subprocess.run(['powershell.exe', '-NoProfile', '-Command', source], cwd=root,
                          capture_output=True, text=True, errors='replace', check=True).stdout.strip()

def literal(value):
    return "'" + str(value).replace("'", "''") + "'"

def owner():
    return ps(f'$listener = Get-NetTCPConnection -State Listen -LocalPort {args.port} '
              '-ErrorAction SilentlyContinue; if ($listener) { $listener.OwningProcess | Sort-Object -Unique }; exit 0')

def process_path(pid):
    return ps(f'(Get-Process -Id {pid} -ErrorAction SilentlyContinue).Path')

boot = symbols('NC_CORE')
nc = symbols('NC')
cli = symbols('CLI')
font_utility = symbols('FONT')
bench = symbols('FONTCOM')
bios = symbols('BIOS')
bank_page = symbols('banked/TRANSPORT')['bankPage']
Bench = runpy.run_path(str(root / 'source-apps/diskbench/check-emulator.py'))['Bench']
machine = Bench(f'http://127.0.0.1:{args.port}/mcp')
font = (root / 'assets/system-rom/fonts/4x8/Cpm4.pf').read_bytes()

def resume(address, page=0):
    machine.stop_at(address)
    machine.call('debug_step', mode='into')
    machine.until(address, 120, page=page)

def command(line, line_buffer, address, tag, image=None):
    machine.call('bp_clear')
    machine.call('emu_run_for', ms=500)
    machine.tap('esc')
    machine.call('emu_run_for', ms=250)
    keys = {' ': 'space', '/': 'slash', ':': 'colon', '.': 'period', '-': 'minus'}
    for char in line.lower():
        if char == '_':
            machine.call('kbd_send', keys=['shift'], action='press')
            try:
                machine.tap('0')
            finally:
                machine.call('kbd_send', keys=['shift'], action='release')
        else:
            machine.tap(keys.get(char, char))
    assert machine.read(line_buffer, len(line)).upper() == line.upper().encode(), line
    machine.screenshot(output / f'{tag}-command.png')
    machine.stop_at(address)
    machine.tap('enter')
    for _ in range(10):
        machine.until(address, 120, page=0)
        if image is None or machine.read(0, 16) == (root / f'bin/{image}.i80').read_bytes()[:16]:
            return
        machine.call('debug_step', mode='into')
    raise AssertionError((line, image))

def oracle(corpus, raw, inverse):
    pixels = bytearray(12288)
    x = y = 0
    for code in corpus:
        if not raw and code == 13:
            x = 0
            continue
        if not raw and code == 10:
            y += 10
            continue
        for row in range(8):
            bits = font[code * 8 + row] ^ (15 if inverse else 0)
            pixels[x // 8 * 256 + y + row] |= bits << (4 if x % 8 == 0 else 0)
        x += 4
        if x == 384:
            x = 0
            y += 10
    return pixels, bytes([y + 7, x // 2])

assert not owner(), f'Порт {args.port} занят'
process = None
rows = []
try:
    process = int(ps(f'& scripts/emu80/run.ps1 -EmulatorDirectory {literal(runtime)} '
                     '-Flash -Mcp -Mute -ResetFlash -PassThru | Select-Object -ExpandProperty Id').splitlines()[-1])
    for _ in range(20):
        if owner() == str(process) and Path(process_path(process)) == exe:
            break
        time.sleep(.25)
    else:
        raise RuntimeError('Собственный Emu80 не занял MCP-порт')
    request = urllib.request.Request(machine.url, b'{"jsonrpc":"2.0","id":1,"method":"tools/list"}',
                                     {'Content-Type': 'application/json'})
    tools = json.load(urllib.request.urlopen(request, timeout=15))['result']['tools']
    assert any(tool['name'] == 'kbd_send' and 'duration_ms' in tool['inputSchema']['properties'] for tool in tools)
    conf = (runtime / 'specialist-os/specialist-mx2.conf').read_text(encoding='utf-8-sig')
    assert 'pit.frequency = 2000000' in conf and 'cpu.frequency = 2000000' in conf
    machine.call('debug_break')
    machine.stop_at(boot['ncBankInputPoll'])
    for _ in range(20):
        machine.until(boot['ncBankInputPoll'], 120)
        if machine.read(bank_page, 1)[0] > 1:
            break
        machine.call('debug_step', mode='into')
    else:
        raise AssertionError('Банковый NC не дошёл до ввода')
    machine.call('bp_clear')
    machine.call('emu_run_for', ms=500)
    machine.stop_at(cli['cliPrompt'])
    machine.tap('f10')
    machine.until(cli['cliPrompt'], 120, page=0)
    command('FONT 6x8/Rotate', cli['CMD_LINE'], font_utility['committed'], 'boot-rotate', 'FONT')
    resume(cli['cliPrompt'])
    command('A:NC_MN.COM', cli['CMD_LINE'], nc['mainLoop'], 'nc-main', 'NC')
    cases = (('FAST4', 0, 0),) if args.engine == 'FAST4' else (('Compact', 9, 0), ('Packed', 5, 0), ('Fast', 10, 2048))
    for name, form, size in cases:
        if args.engine and name != args.engine:
            continue
        option = ' -FILE' if args.file and name == 'Fast' else ''
        if name != 'FAST4':
            command(f'FONT 4x8/{name}{option}', nc['vars.cmdLine'], font_utility['committed'], name, 'FONT')
            assert machine.read(0xd825, 3) == bytes([4, 8, form]), name
            assert int.from_bytes(machine.read(0xd82c, 2), 'little') == size, name
            assert int.from_bytes(machine.read(bios['v_memTop'], 2), 'little') == 0x8bff - size, name
            if size:
                pre = (root / 'bin/fonts/Cpm4Fast.pre').read_bytes()
                assert machine.read(0x8400, size) == pre, name
                assert machine.read(font_utility['preparedError'], 1) == b'\0'
            resume(nc['mainLoop'])
        selection = ' -E FAST4' if name == 'FAST4' else ''
        for inverse in (False, True):
            label = f'{name}-{"inverse" if inverse else "normal"}'
            if args.polarity and inverse != (args.polarity == 'inverse'):
                continue
            command('H:/Bench/FontBen/FontBen.com -Q -N' + selection + (' -I' if inverse else ''),
                    nc['vars.cmdLine'], bench['gridMeasured'], label, 'FONTCOM')
            if name == 'FAST4':
                assert machine.read(0x5000, 2048) == (root / 'bin/fonts/Cpm4Fast.pre').read_bytes()
            corpus = machine.read(bench['gridText'], bench['GRID_TEXT_BYTES'])
            refs = [oracle(corpus, raw, inverse) for raw in (False, True)]
            for phase in range(18):
                kind = machine.read(bench['gridKind'], 1)[0]
                route = machine.read(bench['gridRoute'], 1)[0]
                expected, cursor = refs[kind in (1, 3, 4)]
                actual = machine.read(0x9000, 12288)
                assert all(actual[i] == expected[i] for i in range(12288) if i % 256 < 240), (label, kind, route)
                assert machine.read(bios['bios_vars.cursorY'], 2) == cursor, (label, kind, route, 'cursor')
                ticks = int.from_bytes(machine.read(bench['v_elapsed'], 4), 'little')
                assert 0 < ticks < 65536
                row = dict(engine=name, inverse=inverse, kind=kind, route=route, ticks=ticks, ms=round(ticks * .128, 3))
                rows.append(row)
                if route == 0:
                    print(row, flush=True)
                if phase < 17:
                    resume(bench['gridMeasured'])
            (output / 'results.json').write_text(json.dumps(rows, indent=2), encoding='utf8')
            resume(nc['mainLoop'])
            if size:
                assert machine.read(0x8400, size) == pre
            if name == 'FAST4':
                assert machine.read(0xd825, 3) == bytes([6, 8, 0]), 'FONT ABI restored'
    if args.engine in (None, 'Fast'):
        if not args.file:
            command('FONT 4x8/Fast -FILE', nc['vars.cmdLine'], font_utility['committed'], 'fast-file', 'FONT')
            assert machine.read(font_utility['preparedError'], 1) == b'\0'
            assert machine.read(0x8400, 2048) == pre
            resume(nc['mainLoop'])
        command('FONT 4x8/Packed', nc['vars.cmdLine'], font_utility['committed'], 'release', 'FONT')
        assert int.from_bytes(machine.read(bios['v_memTop'], 2), 'little') == 0x8bff
    print(f'PASS: {len(rows)} этапов FontBen', flush=True)
finally:
    if process and Path(process_path(process)) == exe:
        ps(f'Stop-Process -Id {process}')
