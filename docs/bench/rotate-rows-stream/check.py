from pathlib import Path
import json
import os
import runpy

ROOT = Path(__file__).resolve().parents[3]
OUT = Path(__file__).resolve().parent
os.environ['FONTBEN_REPORT_DIR'] = str(OUT)
runner = ROOT / 'source-apps/fontbench/check-resident.py'
ns = {'__file__': str(runner), '__name__': 'rotate_bench'}
exec(compile(runner.read_text(encoding='utf8').split('\ntry:\n')[0], str(runner), 'exec'), ns)
x, m, s, n, command, resume = [ns[k] for k in ('x', 'm', 's', 'n', 'command', 'resume')]
def command(cmd, pc):
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.tap('esc')
    m.call('emu_run_for', ms=250)
    keys = {':':'colon', '/':'slash', '.':'period', ' ':'space', '-':'minus', '=':'equals'}
    for c in cmd:
        m.tap(keys.get(c, c.lower()))
    assert m.read(n['vars.cmdLine'], len(cmd)).upper() == cmd.upper().encode(), cmd
    m.screenshot(OUT / 'command.png')
    m.stop_at(pc)
    for name in ('gridError', 'gridUsage', 'gridExit'):
        m.call('bp_set', addr=s[name], type='exec')
    m.tap('enter')
    expected_entry = (ROOT / 'bin/FONTCOM.i80').read_bytes()[:16]
    for attempt in range(60):
        state = m.call('cpu_state')
        at = state['registers']['pc']['dec']
        is_fontben = m.read(0, 16) == expected_entry
        if at == pc and is_fontben:
            m.stop_at(pc)
            return
        if is_fontben and at in (s['gridError'], s['gridUsage'], s['gridExit']):
            print('FAILED', cmd, state, 'path', m.read(s['enginePath'], 30),
                  'bank path', m.read(s['gridBankPath'], 40), 'id', m.read(s['engineId'], 1),
                  flush=True)
            m.call('bp_clear')
            m.call('emu_run_for', ms=500)
            m.screenshot(OUT / 'error.png')
            raise AssertionError(hex(at))
        if state['breaked']:
            m.call('debug_step', mode='into')
        m.call('emu_run_for', ms=500)
    raise AssertionError((cmd,state))


image = b''.join((ROOT / f'images/FlashDisk_chip{i}_512K.bin').read_bytes() for i in range(1, 5))
tree = runpy.run_path(str(ROOT / 'scripts/tests/check-nested-image.py'))['read_tree'](image)[0]
for disk_name, binary in [('FontBen.com', 'FONTCOM'), ('FontRot.dll', 'fonts/FontRot'),
                          ('RotRows.dll', 'fonts/RotRows'), ('Rot8Quad.dll', 'fonts/Rot8Quad'),
                          ('Rot8Strm.dll', 'fonts/Rot8Strm')]:
    assert tree['BENCH/FontBen/' + disk_name][32:] == (ROOT / f'bin/{binary}.i80').read_bytes()
disk = ROOT / '.local/fontben-rotate-games.bin'
disk.write_bytes(image)
rows = []


def check_stage(engine, phase, expect, cursor):
    kind = m.read(s['gridKind'], 1)[0]
    route = m.read(s['gridRoute'], 1)[0]
    actual = m.read(0x9000, 12288)
    differences = [i for i in range(12288) if i % 256 < 240 and actual[i] != expect[i]]
    assert not differences, (engine, phase, kind, route, 'pixels', differences[:12])
    assert m.read(s['bios_vars.cursorY'], 2) == cursor, (engine, phase, kind, route, 'cursor')
    ticks = int.from_bytes(m.read(s['v_elapsed'], 4), 'little')
    assert 0 < ticks < 65536
    rows.append(dict(engine=engine, phase=phase, kind=kind, route=route, ms=round(ticks * .128, 3)))
    (OUT / 'results.json').write_text(json.dumps(rows, indent=2))
    print(rows[-1], flush=True)


def pixels8(corpus, phase):
    image = bytearray(12288)
    for row in range(24):
        for col, code in enumerate(corpus[row * 48:row * 48 + 46]):
            x = phase * 2 + col * 8
            for y in range(8):
                bits = font8[code * 8 + y]
                for i in range(8):
                    if bits & (128 >> i):
                        px = x + i
                        image[(px // 8) * 256 + row * 10 + y] |= 128 >> (px % 8)
    return image


font6 = bytes(b & 63 for b in (ROOT / 'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes())
font8 = (ROOT / 'assets/system-rom/fonts/8x8/Win8.pf').read_bytes()
try:
    x.start('rotate-rows-stream', system=ROOT / 'images/SystemRom_emu80_writable.bin', disk=disk)
    for engine in ('6RotateQuads', '6RotateRows'):
        command('H:/BENCH/FontBen/FontBen.com -E ' + engine + ' -Q -N', s['gridMeasured'])
        corpus = m.read(s['gridText'], 1152)
        refs = [ns['oracle'](corpus, font6, 6, raw) for raw in (False, True)]
        for stage in range(18):
            kind = m.read(s['gridKind'], 1)[0]
            check_stage(engine, None, *refs[kind in (1, 3, 4)])
            if stage < 17:
                resume(s['gridMeasured'])
        resume(n['mainLoop'], True)
    for engine in ('8RotateQuads', '8RotateStream'):
        for phase in (0, 1, 2, 3):
            command(f'H:/BENCH/FontBen/FontBen.com -E {engine} -X{phase} -Q -N', s['gridMeasured'])
            expected = pixels8(m.read(s['gridText'], 1152), phase)
            for kind in range(5):
                assert m.read(s['gridKind'], 1)[0] == kind
                check_stage(engine, phase, expected, bytes([237, 184 + phase]))
                if kind < 4:
                    resume(s['gridMeasured'])
            resume(n['mainLoop'], True)
finally:
    x.stop()
