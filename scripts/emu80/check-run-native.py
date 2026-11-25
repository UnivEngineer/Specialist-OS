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
core = symbols(R/'bin/NC_CORE.sym')
cli = symbols(R/'bin/CLI.sym')
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
    api=runpy.run_path(str(R/'scripts/emu80/file-api.py'))
    callback=lambda op,hl=0,a=0:api['main_call'](m,k,op,hl,a)
    api['path_io'](m,callback,k,0x3200,True)

def type_line(command, address):
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.tap('esc')
    m.call('emu_run_for', ms=250)
    for c in command:
        if c == '_':
            m.call('kbd_send', keys=['shift'], action='press')
            try:
                m.tap('0')
            finally:
                m.call('kbd_send', keys=['shift'], action='release')
        else:
            m.tap({' ':'space', ':':'colon', '/':'slash', '.':'period', '-':'minus'}.get(c,c.lower()))
    typed = m.read(address, len(command))
    assert typed.upper() == command.upper().encode(), (command, typed)

def ready_main():
    poll = core['ncBankInputPoll']
    main_loop = n['mainLoop']
    main_code = (R/'bin/NC.i80').read_bytes()[:32]
    core_code = (R/'bin/NC_CORE.i80').read_bytes()[poll-0x100:poll-0x100+3]
    m.stop_at(main_loop)
    m.call('bp_set', addr=poll, type='exec')
    for _ in range(120):
        state = m.call('cpu_state')
        pc = state['registers']['pc']['dec']
        bank = m.read(x.pageAddress, 1)[0]
        if pc == main_loop and bank == 0 and m.read(0, 32) == main_code:
            return
        if pc == poll and bank > 1 and m.read(poll, 3) == core_code:
            m.call('bp_clear')
            m.call('emu_run_for', ms=500)
            x.console()
            type_line('C:/DOS/NC_MN.COM', cli['CMD_LINE'])
            m.stop_at(main_loop)
            m.tap('enter')
            m.until(main_loop, 120, page=0)
            assert m.read(0, 32) == main_code
            return
        if state['breaked']:
            m.call('debug_step', mode='into')
        m.call('emu_run_for', ms=1000)
    raise AssertionError(('NC did not enter MAIN', m.call('cpu_state')))

def resources():
    page(1)
    shell_owner = m.read(k['appOwner'], 1)[0]
    allocations = m.read(k['MEM_ALLOC_TABLE'], k['MEM_ALLOC_RECORDS'] * 6)
    size = k['MODULE_RECORD_SIZE']
    modules = m.read(k['MODULE_TABLE'], k['MODULE_MAX'] * size)
    page(0)
    # При тёплом старте NC может вернуться на другую глубину FILE_EXEC.
    # Сверяем банк, адрес и длину его областей и точных владельцев остальных записей.
    blocks = set()
    for i in range(0, len(allocations), 6):
        if allocations[i] != 255:
            owner = allocations[i+5]
            blocks.add((allocations[i:i+5], None if owner == shell_owner else owner))
    return (blocks,
            {modules[i:i+2] for i in range(0, len(modules), size) if modules[i:i+2] != bytes(2)})

def enter(command, menu=False, direct=False):
    page(0)
    m.write(0x8800, word(0))
    prior_allocations, prior_modules = resources()
    type_line(command, n['vars.cmdLine'])
    stem = str(len(rows)+1)
    m.screenshot(O/(stem+'-command.png'))
    returned = n['mainLoop'] if direct else 0xc000
    m.stop_at(r['modeKey'] if menu else returned)
    m.tap('enter')
    if menu:
        m.until(r['modeKey'], 120, page=0)
        m.screenshot(O/(stem+'-menu.png'))
        m.stop_at(0xc000)
        m.call('emu_run_for', ms=500) # Дать getch завершить отпускание Enter.
        m.tap('1')  # MX
    m.until(returned, 120, page=0)
    assert m.read(0x8800, 2) == word(0x55aa), (command, m.read(0x8800,2).hex())
    m.screenshot(O/(stem+'-result.png'))
    if not direct:
        ready_main()
    allocations, modules = resources()
    assert allocations == prior_allocations, (command, allocations^prior_allocations)
    assert modules == prior_modules, (command, modules^prior_modules)
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
    neutral.mode=1  # В заголовке рекомендован MX, но RUN без ключа показывает меню.
    for ext in ('COM','EXE'):
        save('UNKNOWN.'+ext, e['encode'](neutral, ext))
    extensions = {line.split()[0] for line in (R/'assets/system-rom/CLI.EXT').read_bytes().splitlines() if line.split()}
    assert not extensions & {b'COM', b'EXE'}
    assert {b'RKS', b'MON'} <= extensions
    page(0)
    m.stop_at(n['mainLoop'])
    m.call('emu_reset')
    ready_main()
    if '--std-only' in sys.argv:
        enter('RUN B:/UNKNOWN.COM -STD Test')
    else:
        for name in ('MAINAPP','BANKAPP','DOSAPP'):
            enter('RUN B:/'+name+'.EXE Test')
        enter('RUN B:/MAINAPP.COM Test')
        enter('RUN B:/UNKNOWN.COM Test', menu=True)
        enter('RUN B:/UNKNOWN.EXE Test', menu=True)
        enter('B:/UNKNOWN.EXE Test', direct=True)
        enter('B:/UNKNOWN.COM Test', direct=True)
        enter('RUN B:/UNKNOWN.COM -MX Test')
    (O/'result.json').write_text(json.dumps(rows, indent=2)+'\n', 'utf-8')
    print('RUN NATIVE PASS', len(rows), flush=True)
finally:
    x.stop()
