"""RUN: нативные MAIN/DRIVER/BANK и меню COM/EXE без признака совместимости."""
from pathlib import Path
import json, re, runpy, struct, sys

R = Path(__file__).resolve().parents[2]
O = R/'docs/banked/run-native-057'
if '--std-only' in sys.argv: O = O/'std'
O.mkdir(parents=True, exist_ok=True)
symbols = lambda p: {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', p.read_text('utf-8'), re.M)}
k = symbols(R/'bin/banked/KERNEL.sym')
n = symbols(R/'bin/NC.sym')
r = symbols(R/'bin/RUN.sym')
helpers = runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
helpers['Matrix'].__init__.__globals__.update(OUT=O, RUNTIME=R/'.local/monitor2-001')
x = helpers['Matrix']()
m = x.m
rows = []
word = lambda v: struct.pack('<H', v)
e = runpy.run_path(str(R/'scripts/images/executable.py'))

def page(v):
    m.write(0xfffd if v else 0xfffc, bytes([v-1 if v else 0]))

def save(name, data):
    page(0)
    m.write(0x3400, ('B:\\'+name).encode()+bytes(128-len(name)-3))
    m.write(0x4000, data)
    m.write(0x3200, word(0x3400)+word(0x4000)+word(len(data))+bytes(34))
    m.write(0x3000, struct.pack('<4H', 0x3200, 0, 0, 2))
    m.write(0x2000, b'\xcd\x00\xf8\xc3\x00\x70')
    m.stop_at(0x7000)
    m.call('regs_set', pc=0x2000, sp=0x8fdf, bc=k['MXOS_DOS_FILE_SAVE_PATH'], de=0x3000)
    result = m.until(0x7000, 40)
    assert not result['registers']['af']['dec'] & 1, result

def enter(command, menu=False):
    page(0)
    m.write(0x5200, word(0))
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.tap('esc')
    m.call('emu_run_for', ms=250)
    for c in command:
        m.tap({' ':'space', ':':'colon', '/':'slash', '.':'period', '-':'minus'}.get(c,c.lower()))
    assert m.read(n['vars.cmdLine'], len(command)).upper() == command.upper().encode()
    stem = str(len(rows)+1)
    m.screenshot(O/(stem+'-command.png'))
    m.stop_at(r['modeKey'] if menu else 0xc000)
    m.tap('enter')
    if menu:
        m.until(r['modeKey'], 120, page=0)
        m.screenshot(O/(stem+'-menu.png'))
        m.stop_at(0xc000)
        m.call('emu_run_for', ms=500) # Дать getch завершить отпускание Enter.
        m.tap('1')  # MX
    m.until(0xc000, 120, page=0)
    assert m.read(0x5200, 2) == word(0x55aa), (command, m.read(0x5200,2).hex())
    m.screenshot(O/(stem+'-result.png'))
    page(1)
    allocations=m.read(k['MEM_ALLOC_TABLE'], 32*6)
    modules=m.read(k['MODULE_TABLE'], k['MODULE_MAX']*k['MODULE_RECORD_SIZE'])
    assert all(allocations[i]==255 for i in range(0,len(allocations),6)), command
    assert all(modules[i:i+2]==bytes(2) for i in range(0,len(modules),10)), command
    page(0)
    m.stop_at(n['mainLoop'])
    m.until(n['mainLoop'], 120, page=0)
    rows.append(dict(command=command, menu=menu, returned=True, resourcesReleased=True))
    print('PASS', command, 'menu' if menu else 'native', flush=True)

try:
    x.start('RUN native dispatch')
    m.call('debug_break')
    for name in ('BANKAPP','MAINAPP','DOSAPP'):
        data=(R/f'bin/moduletest/{name}.COM').read_bytes()
        save(name+'.COM', data)
        save(name+'.EXE', data)
    neutral=e['decode']((R/'bin/moduletest/MAINAPP.COM').read_bytes(), 'com')
    neutral.flags=0
    for ext in ('COM','EXE'):
        save('UNKNOWN.'+ext, e['encode'](neutral, ext))
    # Подтверждаем, что EXE→RUN включена именно в проверяемом системном образе.
    assert b'EXE A:\\RUN.COM' in (R/'assets/system-rom/cli.ext').read_bytes()
    page(0)
    m.stop_at(n['mainLoop'])
    m.call('emu_reset')
    m.until(n['mainLoop'], 120, page=0)
    if '--std-only' in sys.argv:
        enter('RUN B:/UNKNOWN.COM -STD Test')
    else:
        for name in ('MAINAPP','BANKAPP','DOSAPP'):
            enter('RUN B:/'+name+'.EXE Test')
        enter('RUN B:/MAINAPP.COM Test')
        enter('RUN B:/UNKNOWN.COM Test', menu=True)
        enter('B:/UNKNOWN.EXE Test', menu=True)
        enter('B:/UNKNOWN.COM Test', menu=True)
        enter('RUN B:/UNKNOWN.COM -MX Test')
    (O/'result.json').write_text(json.dumps(rows, indent=2)+'\n', 'utf-8')
    print('RUN NATIVE PASS', len(rows), flush=True)
finally:
    x.stop()
