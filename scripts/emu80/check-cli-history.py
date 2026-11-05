"""DIR A:, история CLI и обновление одной панели NC через обычную клавиатуру."""
from pathlib import Path
import runpy,json,re

R=Path(__file__).resolve().parents[2]
O=R/'docs/cli/history-2026-11-05';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001'
def symbols(name):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(encoding='utf-8'),re.M)}
m=h['Matrix']();s=symbols('DOS.sym');c=symbols('CLI.sym');n=symbols('NC.sym')

def tap(key):
    result=m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
    if not result.get('stoppedByBreakpoint'):m.m.call('emu_run_for',ms=120)

def text(value):
    keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}
    for ch in value:tap(keys.get(ch,ch.lower()))

def line():return m.m.read(s['CMD_LINE'],128).split(b'\0')[0].upper()

def resume():
    m.m.call('bp_clear');m.m.call('emu_run_for',ms=500)

def cli(value):
    text(value);assert line()==value.upper().encode()
    m.m.stop_at(c['cliPrompt']);m.m.call('bp_set',addr=c['cliError'],type='exec');tap('enter')
    for _ in range(90):
        pc=m.m.call('cpu_state')['registers']['pc']['dec']
        assert pc!=c['cliError'],(value,m.m.read(s['v_dosError'],1))
        if pc==c['cliPrompt']:break
        m.m.call('emu_run_for',ms=500)
    else:raise AssertionError(('CLI timeout',value))
    resume();print('CLI',value,flush=True)

def panel(index):return b''.join(m.m.read(0x9000+(x+24*index)*256,230) for x in range(24))

def nc_change(value=None,key=None):
    active=m.m.read(n['state.activePanel'],1)[0];before=panel(active^1)
    if value is not None:text(value);assert line()==value.upper().encode()
    m.m.stop_at(n['redrawFilePanel']);m.m.call('bp_set',addr=n['mainLoop'],type='exec')
    tap(key or 'enter');visits=[]
    for _ in range(90):
        pc=m.m.call('cpu_state')['registers']['pc']['dec']
        if pc==n['redrawFilePanel']:
            visits.append(m.m.read(n['state.activePanel'],1)[0])
            m.m.call('bp_clear');m.m.stop_at(n['mainLoop'])
            # Одна трассировка следующего вызова: эмулятор продолжает с текущего BP.
            m.m.call('emu_run_for',ms=1)
            m.m.call('bp_set',addr=n['redrawFilePanel'],type='exec')
        elif pc==n['mainLoop']:break
        else:m.m.call('emu_run_for',ms=500)
    else:raise AssertionError(('NC timeout',value,key))
    assert visits==[active],(value,key,visits)
    resume();after=panel(active^1)
    if after!=before:
        (O/'panel-before.bin').write_bytes(before);(O/'panel-after.bin').write_bytes(after)
        changes=[(i//230,i%230,a,b) for i,(a,b) in enumerate(zip(before,after)) if a!=b]
        raise AssertionError(('other panel changed',value,key,len(changes),changes[:20]))
    print('NC one panel',value or key,flush=True)

try:
    m.start('history',disk=R/'bin/nested-Games2M.bin')
    nc_change('B:');nc_change('CD /')
    # Каталог создаём командой, затем открываем Enter и через CD на обеих панелях.
    text('MD HISTORY');tap('enter');m.m.call('emu_run_for',ms=1000)
    tap('home');nc_change(key='enter')
    nc_change('CD /');nc_change('H:');nc_change('A:')
    tap('tab');m.m.call('emu_run_for',ms=1000)
    nc_change('B:');nc_change('CD HISTORY');nc_change('CD /')
    m.m.screenshot(O/'nc-one-panel.png')
    m.m.stop_at(c['cliPrompt']);tap('f10');m.m.until(c['cliPrompt'],40);resume()
    cli('H:');cli('DIR A:')
    assert m.m.read(c['cliPromptPath'],128).split(b'\0')[0].upper()==b'H:\\'
    cli('ECHO ONE');cli('ECHO TWO');cli('ECHO TWO')
    count=m.m.read(c['cliHistoryCount'],1)[0]
    text('ECHO DRAFT');tap('up');assert line()==b'ECHO TWO'
    tap('up');assert line()==b'ECHO ONE'
    tap('down');assert line()==b'ECHO TWO'
    tap('down');assert line()==b'ECHO DRAFT'
    tap('down');assert line()==b'ECHO DRAFT'
    tap('up');tap('end');tap('backspace');tap('x');assert line()==b'ECHO TWX'
    tap('esc');tap('up');assert line()==b'ECHO TWO'
    tap('esc');assert line()==b''
    assert m.m.read(c['cliHistoryCount'],1)[0]==count
    m.m.screenshot(O/'history-edit.png')
    # Программа CHKDSK затирает CLI; после возврата история загружается с B:.
    ck=h['symbols'](R/'bin/CHKDSK.sym')
    text('CHKDSK B:');m.m.stop_at(ck['waitKey']);tap('enter');m.m.until(ck['waitKey'],60)
    resume();m.m.stop_at(s['defAppReturn']);tap('space');m.m.until(s['defAppReturn'],30)
    assert m.m.read(s['bios_vars.cursorX'],1)==b'\0','CHKDSK did not finish its output line'
    m.m.screenshot(O/'chkdsk-exit.png')
    m.m.stop_at(s['reboot3_2']);m.m.until(s['reboot3_2'],30)
    m.m.call('bp_clear');m.m.stop_at(c['cliPrompt']);m.m.until(c['cliPrompt'],40);resume()
    tap('up');assert line()==b'CHKDSK B:'
    tap('up');assert line()==b'ECHO TWO'
    tap('esc');m.m.screenshot(O/'history-after-com.png')
    # Редактор уже перевёл строку: BAD COMMAND печатается без пустой строки.
    text('ZZMISSING');m.m.stop_at(s['badCommand']);tap('enter');m.m.until(s['badCommand'],30)
    y=m.m.read(s['bios_vars.cursorY'],1)
    m.m.stop_at(s['execCommandErrorDone']);m.m.until(s['execCommandErrorDone'],20)
    assert m.m.read(s['bios_vars.cursorY'],1)==y,'extra newline before BAD COMMAND'
    m.m.screenshot(O/'bad-command.png')
    m.m.stop_at(c['cliPrompt']);m.m.until(c['cliPrompt'],40);resume()
    (O/'result.json').write_text(json.dumps(dict(dirDrive=True,history=True,draft=True,deduplicate=True,afterCom=True,singlePanel=True,badCommandNoBlank=True,chkdskNewline=True),indent=2)+'\n')
    print('CLI history / DIR drive / NC single panel PASS',flush=True)
except Exception:
    m.m.screenshot(O/'failure.png');print(m.m.call('cpu_state'),flush=True);raise
finally:m.stop()
