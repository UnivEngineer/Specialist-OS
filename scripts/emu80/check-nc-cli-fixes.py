"""NC: тихие команды, редактор, MkDir, Move и возврат тулбара после DIR."""
from pathlib import Path
import runpy,json,re
R=Path(__file__).resolve().parents[2]
O=R/'docs/cli/cli-fixes-2026-11-05/ui';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__
g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001'
m=h['Matrix']();s=h['symbols'](R/'bin/DOS.sym');n={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text(encoding='utf-8'),re.M)};c=h['symbols'](R/'bin/CLI.sym')
def tap(key):
    r=m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
    if not r.get('stoppedByBreakpoint'):m.m.call('emu_run_for',ms=120)
def text(value):
    keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}
    for char in value:tap(keys.get(char,char.lower()))
def line():return m.m.read(s['CMD_LINE'],128).split(b'\0')[0].upper()
def toolbar():return b''.join(m.m.read(0x9000+256*x+247,9) for x in range(48))
def quiet(value):
    before=toolbar();tap('esc');text(value);assert line()==value.upper().encode(),line()
    m.m.stop_at(n['mainLoop']);m.m.call('bp_set',addr=s['clearScreen'],type='exec');tap('enter')
    m.m.until(n['mainLoop'],30)
    assert m.m.call('cpu_state')['registers']['pc']['dec']==n['mainLoop']
    m.m.call('bp_clear');m.m.call('emu_run_for',ms=500)
    assert before==toolbar(),'toolbar changed by '+value
def cache():return m.m.read(n['FILE_CACHE_A'],n['FILE_CACHE_BYTES']).upper()
try:
    m.start('nc-ui',disk=R/'bin/nested-Games2M.bin')
    normal_toolbar=toolbar()
    m.m.call('kbd_send',keys=['shift'],action='press')
    m.m.call('emu_run_for',ms=500)
    assert m.m.read(n['ncShiftState'],1)==b'\0'
    assert toolbar()!=normal_toolbar
    m.m.screenshot(O/'shift-toolbar.png')
    tap('f6')
    assert b':' not in m.m.read(n['vars.input'],128).split(b'\0')[0]
    m.m.screenshot(O/'rename-dialog.png')
    m.m.call('kbd_send',keys=['shift'],action='release')
    tap('esc');m.m.call('emu_run_for',ms=1000)
    assert toolbar()==normal_toolbar
    tap('down');selected=m.m.read(n['state.panelA_curFile'],1)
    text('CD /');tap('left');tap('backspace');tap('space')
    assert line()==b'CD /',line()
    assert m.m.read(n['state.panelA_curFile'],1)==selected
    tap('esc');tap('up');assert m.m.read(n['state.panelA_curFile'],1)!=selected
    quiet('B:')
    m.m.screenshot(O/'nc-empty-panel.png')
    # Отпущенная точка с запятой не должна повторяться; Escape отменяет диалог.
    tap('f7');tap('semicolon')
    typed=m.m.read(n['vars.input'],128).split(b'\0')[0]
    assert typed==b';',typed
    m.m.call('emu_run_for',ms=1500)
    assert m.m.read(n['vars.input'],128).split(b'\0')[0]==typed
    tap('esc');m.m.call('emu_run_for',ms=1000);tap('f7')
    assert m.m.read(n['vars.input'],1)==b'\0'
    tap('esc');m.m.call('emu_run_for',ms=1000)
    tap('f7');text('SUBX');tap('backspace');tap('d');tap('home');tap('end')
    assert m.m.read(n['vars.input'],128).split(b'\0')[0].upper()==b'SUBD'
    m.m.screenshot(O/'mkdir.png');tap('enter');m.m.call('emu_run_for',ms=1000)
    quiet('COPY A:/README.TXT B:/TEST.TXT')
    tap('home');tap('down')
    m.m.call('kbd_send',keys=['shift'],action='press');m.m.call('emu_run_for',ms=300)
    tap('f6');m.m.call('kbd_send',keys=['shift'],action='release')
    old=m.m.read(n['vars.input'],128).split(b'\0')[0]
    for _ in old:tap('backspace')
    text('RENAMED.TXT');tap('enter');m.m.call('emu_run_for',ms=1000)
    assert b'RENAMED TXT' in cache()
    quiet('REN RENAMED.TXT TEST.TXT')
    # Вторую панель направляем в каталог назначения на том же B:.
    tap('tab');quiet('CD B:/SUBD');tap('tab');tap('home');tap('down')
    tap('f6')
    assert m.m.read(n['vars.input'],128).split(b'\0')[0].upper()==b'B:\\SUBD\\TEST.TXT'
    m.m.screenshot(O/'move.png');tap('enter');m.m.call('emu_run_for',ms=1500)
    assert b'TEST    TXT' not in cache()
    quiet('CD SUBD');assert b'TEST    TXT' in cache()
    m.m.screenshot(O/'move-result.png')
    # Дерево: COPY RAM -> Flash, затем MOVE Flash -> RAM с удалением источника.
    quiet('CD B:/');quiet('MD TREE');quiet('MD TREE/CHILD')
    quiet('COPY A:/README.TXT B:/TREE/CHILD/ONE.TXT')
    quiet('COPY A:/README.TXT B:/TREE/CHILD/TWO.TXT')
    quiet('COPY B:/TREE A:/TREE')
    quiet('MOVE A:/TREE B:/MOVED')
    quiet('CD B:/MOVED/CHILD')
    assert b'ONE     TXT' in cache() and b'TWO     TXT' in cache()
    quiet('CD A:/');assert b'TREE       ' not in cache()
    quiet('CD B:/')
    before=toolbar();text('DIR');tap('enter');m.m.call('emu_run_for',ms=1000)
    m.m.screenshot(O/'dir-from-nc.png');tap('enter');m.m.call('emu_run_for',ms=1500)
    assert toolbar()==before,'DIR did not restore toolbar'
    m.m.screenshot(O/'after-dir.png')
    # Полный экран консоли: редактирование не сдвигает предыдущие строки.
    m.m.stop_at(c['cliPrompt']);tap('f10');m.m.until(c['cliPrompt'],30)
    m.m.call('bp_clear');m.m.call('emu_run_for',ms=500)
    text('DIR H:/');m.m.stop_at(c['cliPrompt']);tap('enter');m.m.until(c['cliPrompt'],90)
    m.m.call('bp_clear');m.m.call('emu_run_for',ms=500)
    screen=m.m.read(0x9000,12288)
    text('ECHO ABCD');tap('left');tap('backspace');tap('x')
    assert line()==b'ECHO ABXD'
    m.m.call('emu_run_for',ms=1500);assert line()==b'ECHO ABXD','key repeated after release'
    edited=m.m.read(0x9000,12288)
    assert all(screen[x*256:x*256+238]==edited[x*256:x*256+238] for x in range(48))
    m.m.screenshot(O/'cli-bottom-edit.png')
    tap('esc');text('NC');tap('enter');m.m.call('emu_run_for',ms=2000)
    m.m.call('kbd_send',keys=['shift'],action='press');m.m.call('emu_run_for',ms=500)
    e=h['symbols'](R/'bin/E.sym')
    m.m.stop_at(e['keyInput']);tap('f4');m.m.until(e['keyInput'],45)
    m.m.call('kbd_send',keys=['shift'],action='release')
    assert m.m.read(s['inputHook'],2)==b'\0\0'
    begin=int.from_bytes(m.m.read(e['v_textBeginLo'],2),'little')
    assert m.m.read(begin,1)==b'\xff','New editor document is not empty'
    m.m.screenshot(O/'new-editor.png')
    (O/'result.json').write_text(json.dumps(dict(panelArrows=True,textArrows=True,mkdir=True,move=True,quietCommands=True,toolbar=True,bottomEditing=True,shiftToolbar=True,rename=True,treeCopyMove=True,newEditor=True),indent=2)+'\n')
    print('NC/CLI fixes UI PASS',flush=True)
except Exception:
    m.m.screenshot(O/'failure.png');print(m.m.call('cpu_state'),flush=True);raise
finally:m.stop()
