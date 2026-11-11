"""NC: полная печать двух колонок и сравнение прокрутки с перерисовкой."""
import json
import re
import runpy
from pathlib import Path

R = Path(__file__).resolve().parents[2]
O = R/'.local/nc-list-check'
O.mkdir(parents=True, exist_ok=True)
h = runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=O, RUNTIME=R/'.local/monitor2-001')
x = h['Matrix']()
m = x.m
n = {a:int(b,16) for a,b in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)', (R/'bin/NC.sym').read_text('utf-8'), re.M)}
rows = []


def key(name):
    m.call('bp_clear')
    m.call('emu_run_for', ms=150)
    m.stop_at(n['mainLoop_cursorReady'])
    m.tap(name)
    m.until(n['mainLoop_cursorReady'], 40, page=0)


def panel(index):
    if m.read(n['state.activePanel'],1) != bytes((index,)):
        key('tab')
    assert m.read(n['state.activePanel'],1) == bytes((index,))
    key('home')


def pixels():
    return b''.join(m.read(0x9000+c*256+14,190) for c in range(48))


def compare(label):
    before = pixels()
    m.stop_at(n['mainLoop_cursorReady'])
    m.call('regs_set', pc=n['mainReloadActive'])
    m.until(n['mainLoop_cursorReady'], 40, page=0)
    after = pixels()
    diffs = [(i//190, i%190, a, b) for i,(a,b) in enumerate(zip(before,after)) if a != b]
    rows.append(dict(case=label, differences=len(diffs), firstDifferences=diffs[:20]))
    m.screenshot(O/(label+'.png'))
    (O/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    assert not diffs, (label, diffs[:20])
    print('PASS',label,flush=True)


try:
    x.start('NC list')
    panel(1)
    compare('H-initial')
    panel(0)
    compare('A-initial')
    count=m.read(n['state.panelA_filesCnt'],1)[0]
    names=[m.read(n['FILE_CACHE_A']+i*16,11).upper() for i in range(count)]
    assert b'TEST-MX COM' in names, names
    panel(1)
    for i in range(45):
        key('down')
    assert m.read(n['state.panelB_curFile'],1) == b'\x2D'
    assert m.read(n['state.panelB_firstFile'],1) != b'\0'
    compare('H-down-45')
    for i in range(40):
        key('up')
    assert m.read(n['state.panelB_curFile'],1) == b'\5'
    compare('H-up-40')
finally:
    x.stop()
