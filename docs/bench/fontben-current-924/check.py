"""Замеры FontBen через собственный Emu80, с независимым эталоном для актуальных ядер."""
import argparse
import hashlib
import json
import re
import runpy
import subprocess
import time
import urllib.request
from pathlib import Path

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--root', type=Path, required=True)
parser.add_argument('--port', type=int, choices=range(19781, 19785), required=True)
parser.add_argument('--group', type=int, choices=range(4), required=True)
parser.add_argument('--only', nargs='+')
parser.add_argument('--pseudo', action='store_true')
args = parser.parse_args()
root = args.root.resolve()
out = Path(__file__).resolve().parent / f'{"pseudo" if args.pseudo else "worker"}-{args.group}'
out.mkdir(exist_ok=True)
exe = root / f'font4-runtime-{args.port}' / 'Emu80qt-mcp.exe'
assert exe.is_file()
Bench = runpy.run_path(str(root / 'source-apps/diskbench/check-emulator.py'))['Bench']
m = Bench(f'http://127.0.0.1:{args.port}/mcp')

def sym(name):
    return {label: int(value, 16) for label, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9a-fA-F]+)', (root / f'bin/{name}.sym').read_text(), re.M)}

def ps(source):
    report = subprocess.run(['powershell.exe', '-NoProfile', '-Command', source], cwd=root,
                            capture_output=True, text=True, errors='replace', check=True)
    return report.stdout.strip()

def owner():
    return ps(f'$l = Get-NetTCPConnection -LocalPort {args.port} -State Listen '
              '-ErrorAction SilentlyContinue; if ($l) { $l.OwningProcess | Sort-Object -Unique }; exit 0')

def process_path(pid):
    return Path(ps(f'(Get-Process -Id {pid} -ErrorAction SilentlyContinue).Path'))

bios, cli, boot, font_utility, bench = map(sym, ('BIOS', 'CLI', 'NC_CORE', 'FONT', 'FONTCOM'))
bank_page = sym('banked/TRANSPORT')['bankPage']
pos = bios['bios_vars.cursorY']
programs = {label: (root / f'bin/{label}.i80').read_bytes()[:12] for label in ('CLI', 'FONT', 'FONTCOM')}

def station(address):
    return 0 if address == boot['ncBankInputPoll'] else bank_page

def await_stop(addresses, timeout=50):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        state = m.call('cpu_state')
        pc = state['registers']['pc']['dec']
        if state['breaked']:
            if pc in addresses:
                expected = addresses[pc]
                if (not station(pc) and m.read(bank_page, 1)[0] > 1) or (
                        station(pc) and m.read(bank_page, 1) == b'\0' and
                        (not expected or m.read(0,12) == programs[expected])):
                    return pc
                m.call('debug_step', mode='into')
            m.call('emu_run')
        time.sleep(.02)
    m.screenshot(out / 'timeout.png')
    raise RuntimeError(('timeout', addresses, state))

def breaks(addresses):
    m.call('bp_clear')
    for address in addresses:
        m.call('bp_set', addr=address, type='exec')

def onwards(addresses):
    m.call('debug_step', mode='into')
    m.call('emu_run')
    return await_stop(addresses)

cli_stop = {cli['cliPrompt']: 'CLI'}
bench_stops = {bench['gridMeasured']: 'FONTCOM', bench['gridError']: 'FONTCOM',
               bench['gridInverseError']: 'FONTCOM', **cli_stop}

def command(line, addresses, screenshot=None):
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.tap('esc')
    m.call('emu_run_for', ms=250)
    keys = {' ': 'space', '/': 'slash', ':': 'colon', '.': 'period', '-': 'minus', ',': 'comma'}
    for char in line.lower():
        m.tap(keys.get(char, char))
    actual = m.read(cli['CMD_LINE'],len(line)).upper()
    assert actual == line.upper().encode(), (line, actual)
    if screenshot:
        m.screenshot(out / f'{screenshot}.png')
    breaks(addresses)
    m.tap('enter')
    return await_stop(addresses)

def install(path, prepared=False):
    stop = command('FONT ' + path + (' -FILE' if prepared else ''),
                   {font_utility['committed']: 'FONT', **cli_stop}, 'install-' + path.split('/')[-1])
    assert stop == font_utility['committed'], (path, 'FONT did not commit')
    assert m.read(font_utility['preparedError'], 1) == b'\0', path
    breaks(cli_stop)
    onwards(cli_stop)

def font(width, name):
    pf = {4: '4x8/Cpm4.pf', 6: '6x8/KOI8.pf', 8: '8x8/Win8.pf'}[width]
    data = (root / 'assets/system-rom/fonts' / pf).read_bytes()
    if name in ('COMPACT6', '6x8/Compact'):
        glyphs = (root / 'bin/fonts/compact-font.bin').read_bytes()
        indices = (root / 'bin/fonts/compact-map.bin').read_bytes()
        return b''.join(glyphs[i*8:i*8+8] for i in indices)
    return bytes(b & ((1 << width) - 1) for b in data)

def oracle(corpus, width, name, kind, inverse, phase, pseudo=False):
    bitmap = font(width, name)
    pixels = bytearray(12288)
    x = y = 0
    text_rows = [corpus] if phase is None else [corpus[i:i+42] for i in range(0,len(corpus),44)]
    for row, text in enumerate(text_rows):
        if phase is not None:
            x, y = phase*2, row*10 + int(pseudo)
        for code in text:
            if kind in (0, 2) and code < 32:
                if code in (10, 13): x = 0
                if code == 10: y += 10
                continue
            glyph = bitmap[code*8:code*8+8]
            scans = tuple(enumerate(glyph))
            if kind == 5:
                box = name != '6x8/Compact' and 0x2500 <= ord(bytes([code]).decode('koi8-r')) <= 0x259f
                scans = ((-1,glyph[0] if box else 0),*scans,(8,glyph[-1] if box else 0))
            for scan, bits in scans:
                bits ^= (1<<width)-1 if inverse else 0
                for bit in range(width):
                    p = x+bit
                    if bits & (1<<(width-1-bit)):
                        pixels[p//8*256+y+scan] |= 128>>(p%8)
            x += width
            if x == 384: x,y = 0,y+10
    return b''.join(pixels[i:i+240] for i in range(0,12288,256)), bytes([y+7,x//2])

def engine_info():
    index = m.read(bench['engineId'], 1)[0]
    rec = m.read(bench['engineRecords'] + index*8, 8)
    name = m.read(int.from_bytes(rec[:2], 'little'), 16).split(b'\0')[0].decode('ascii')
    return name, bool(rec[7] & 128)

def byte(symbol):
    return m.read(bench[symbol], 1)[0]

results = []
errors = []
result_path = out/'results.json'
if args.only and result_path.exists():
    results = json.loads(result_path.read_text(encoding='utf8'))
metadata = json.loads((out/'metadata.json').read_text()) if args.only and (out/'metadata.json').exists() else {}

def benchmark(names, installed=None, inverse=False, phase=None, legacy=False, pseudo=False):
    case = installed if installed else names.replace(',', '+')
    tag = case.replace('/','-') + ('-I' if inverse else '') + (f'-X{phase}' if phase is not None else '')
    line = 'H:/BENCH/FONTBEN/FONTBEN.COM -Q -N'
    if not installed: line += ' -E ' + names
    if pseudo: line += ' -R'
    elif phase is not None: line += f' -X{phase}'
    if inverse: line += ' -I'
    print('START', tag, flush=True)
    current = command(line, bench_stops, tag)
    references = {}
    legacy_reference = {}
    report_rows = []
    while current == bench['gridMeasured']:
        name, direct = engine_info()
        name = installed or name
        width = byte('engineWidth')
        kind, route, pre = (byte(key) for key in ('gridKind','gridRoute','gridPre'))
        ticks = int.from_bytes(m.read(bench['v_elapsed'], 4), 'little')
        assert 0 < ticks < 65536, (name, kind, route, ticks)
        key = (name, kind)
        if key not in references:
            corpus = m.read(bench['gridText'], 924 if phase is None else 22*44)
            corpus_hash = hashlib.sha256(corpus).hexdigest()
            meta_key = 'pseudo' if pseudo else 'ordinary' if phase is None else 'phase'
            assert metadata.setdefault(meta_key, corpus_hash) == corpus_hash
            references[key] = oracle(corpus, width, name, kind, inverse, phase, pseudo) if direct or installed else None
        frame = m.read(0x9000, 12288)
        frame = b''.join(frame[i:i+240] for i in range(0,12288,256))
        cursor = m.read(pos,2)
        expected = references[key]
        if expected:
            pixel_check = 'oracle' if (frame,cursor) == expected else 'oracle-difference'
        else:
            ref = legacy_reference.setdefault((name, kind, pre), (frame,cursor))
            pixel_check = 'routes' if ref == (frame,cursor) else 'route-difference'
        report_rows.append(dict(engine=name, width=width, mode='pseudo' if pseudo else 'bios' if installed else 'legacy' if legacy else 'standalone',
                                inverse=inverse, phase=phase, kind=kind, route=route, pre=pre,
                                ticks=ticks, ms=round(ticks*.128,3), pixels=pixel_check))
        current = onwards(bench_stops)
    if current != cli['cliPrompt']:
        m.screenshot(out / f'{tag}-error.png')
        errors.append((tag, hex(current), len(report_rows)))
        breaks(cli_stop)
        onwards(cli_stop)
    if not report_rows:
        errors.append((tag, 'no-results', 0))
    results.extend(report_rows)
    result_path.write_text(json.dumps(results,indent=2)+'\n',encoding='utf8')
    (out/'metadata.json').write_text(json.dumps(metadata,indent=2)+'\n',encoding='utf8')
    (out/'errors.json').write_text(json.dumps(errors,indent=2)+'\n',encoding='utf8')
    print('DONE', tag, len(report_rows), sorted(set(r['pixels'] for r in report_rows)), flush=True)

# Резидентные графические движки проверяются на всех фазах, плюс полная сетка при X=0.
resident = (
    [('6x8/Rotate','6x8/Rotate'), ('6x8/Packed','6x8/Packed'), ('6x8/Compact','H:/BENCH/FONTBEN/CMP6.FNT')],
    [('8x8/Fast','8x8/Fast'), ('6x8/Fast','6x8/Fast')],
    [('8x8/Rotate','8x8/Rotate'), ('8x8/Aligned','H:/BENCH/FONTBEN/FAST8.FNT')],
    [('8x8/Packed','8x8/Packed'), ('8x8/Row','H:/BENCH/FONTBEN/FAST8ROW.FNT')],
)
standalone = (
    ['6ROTATEROWS,6ROTATEQUADS,6PACKEDQUADS,6FULLQUADS'],
    ['COMPACT6,CPM4'],
    ['FAST8,8ROTATEQUADS'],
    ['8ROTATESTREAM'],
)
legacy_sets = (
    ['MXOS1,OPT,SCALAR12,CACHE,QUAD,COLD'],
    ['WARM,ROTATE,HALF,PACKED,ROW6,ROW8'],
    ['4RAW,4PACK,4DUP,REMAP32,BLOCK32,MON2'],
    ['RAMFOS,ROTLINE,FULLFAST,COL6PRE,TRIPACK,WIN8'],
)

def selected(case):
    return not args.only or case in args.only

assert not owner(), f'Порт {args.port} занят'
pid = None
try:
    quoted = "'" + str(exe.parent).replace("'", "''") + "'"
    pid = int(ps(f'& scripts/emu80/run.ps1 -EmulatorDirectory {quoted} '
                 '-Flash -Mcp -Mute -ResetFlash -PassThru | Select-Object -ExpandProperty Id').splitlines()[-1])
    for _ in range(25):
        if owner() == str(pid) and process_path(pid) == exe:
            break
        time.sleep(.25)
    else:
        raise RuntimeError('Свой Emu80 не получил MCP-порт')
    request = urllib.request.Request(m.url, b'{"jsonrpc":"2.0","id":1,"method":"tools/list"}',
                                     {'Content-Type':'application/json'})
    tool_names = {item['name'] for item in json.load(urllib.request.urlopen(request,timeout=15))['result']['tools']}
    assert {'kbd_send','emu_run','bp_set'} <= tool_names
    conf = (exe.parent/'specialist-os/launch.conf').read_text(encoding='utf-8-sig')
    assert 'pitSoundSource.muted = yes' in conf and 'tapeMonitor.muted = yes' in conf
    m.call('debug_break')
    boot_stop = {boot['ncBankInputPoll']:None}
    breaks(boot_stop)
    m.call('emu_run')
    await_stop(boot_stop)
    m.call('bp_clear')
    m.call('emu_run_for',ms=500)
    breaks(cli_stop)
    m.tap('f10')
    await_stop(cli_stop)
    if args.pseudo:
        extra = (None, ('4x8/Compact','4x8/Compact'), ('4x8/Packed','4x8/Packed'), ('4x8/Fast','4x8/Fast'))[args.group]
        for name, path in resident[args.group] + ([extra] if extra else []):
            if selected(name):
                install(path, name in ('6x8/Packed','6x8/Fast','8x8/Packed','8x8/Fast','4x8/Fast'))
                for inverse in (False,True):
                    benchmark('',name,inverse,phase=0,pseudo=True)
        print('COMPLETE PSEUDO', args.group, len(results), errors, flush=True)
        raise SystemExit
    for name, path in resident[args.group]:
        if not selected(name):
            continue
        install(path, name in ('6x8/Packed','6x8/Fast','8x8/Packed','8x8/Fast'))
        for inverse in (False,True):
            benchmark('', name, inverse)
            if name in ('8x8/Fast','8x8/Packed','8x8/Rotate'):
                for phase in range(4):
                    benchmark('', name, inverse, phase)
    if not args.only or any(selected(n) for n in standalone[args.group]+legacy_sets[args.group]+['standalone-phases']):
        install('6x8/Rotate')
    for names in standalone[args.group]:
        if selected(names):
            for inverse in (False,True):
                benchmark(names,inverse=inverse)
    if args.group in (2,3) and selected('standalone-phases'):
        engine = '8ROTATEQUADS' if args.group == 2 else '8ROTATESTREAM'
        for inverse in (False,True):
            for phase in range(4):
                benchmark(engine,inverse=inverse,phase=phase)
    for names in legacy_sets[args.group]:
        if selected(names):
            benchmark(names,legacy=True)
    print('COMPLETE', args.group, len(results), 'results', errors, flush=True)
finally:
    if pid and process_path(pid) == exe:
        ps(f'Stop-Process -Id {pid}')
