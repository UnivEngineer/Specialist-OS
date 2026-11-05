"""Проверка NC и путей через клавиатуру собственного Emu80."""
from pathlib import Path
import runpy,json,re
R=Path(__file__).resolve().parents[2]
out=R/'docs/fat/directories-002';out.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=out;g['RUNTIME']=R/'.local/monitor2-001'
tree=runpy.run_path(str(R/'scripts/tests/check-nested-image.py'))['read_tree']((R/'bin/nested-Games2M.bin').read_bytes())[1]
base=next(k.split('/')[0] for k in tree if k.endswith('/MXOS'))
m=h['Matrix']();s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text(),re.M)};d=h['symbols'](R/'bin/DOS.sym')
def tap(key):
    result=m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
    if not result.get('stoppedByBreakpoint'):m.m.call('emu_run_for',ms=120)
def path():return m.m.read(s['vars.path'],128).split(b'\0')[0].decode('ascii',errors='backslashreplace')
def check(want):
    want=want.replace("Almaz",base)
    actual=path();assert actual.upper()==want.upper(),(actual,want)
    print('PATH',actual,flush=True)
def command(text):
    text=re.sub(r'([/\\])Almaz(?=[/\\]|$)',lambda match:match[1]+base,text)
    tap('esc');m.m.call('emu_run_for',ms=250)
    keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}
    for c in text:tap(keys.get(c,c.lower()))
    assert m.m.read(s['vars.cmdLine'],len(text)).upper()==text.upper().encode()
    m.m.screenshot(out/'command-before-enter.png')
    tap('enter');m.m.call('emu_run_for',ms=1500)
try:
    m.start('nc-directories',disk=R/'bin/nested-Games2M.bin')
    check('A:\\')
    # Стрелки панели не должны перерисовывать приглашение.
    m.m.call('bp_set',addr=s['redrawCommandLine'],type='exec')
    tap('down');tap('up')
    assert m.m.call('cpu_state')['registers']['pc']['dec']!=s['redrawCommandLine']
    m.m.call('bp_clear')
    tap('tab');check('H:\\')
    command('CD H:/Almaz');check('H:\\Almaz')
    tap('down');tap('down');tap('enter');m.m.call('emu_run_for',ms=1000);check('H:\\Almaz\\MXOS')
    # Пункт корня в первой строке, а CD . сохраняет текущий каталог.
    command('CD .');check('H:\\Almaz\\MXOS')
    tap('enter');m.m.call('emu_run_for',ms=1000);check('H:\\')
    command('CD H:/Almaz/MXOS');check('H:\\Almaz\\MXOS')
    command('CD ../STD');check('H:\\Almaz\\STD')
    tap('tab');check('A:\\')
    command('CHDIR H:/Almaz/RAMFOS');check('H:\\Almaz\\RAMFOS')
    tap('tab');check('H:\\Almaz\\STD')
    m.m.screenshot(out/'panels.png')
    command('CD /');check('H:\\')
    rs=h['symbols'](R/'bin/RUN.sym')
    m.m.call('bp_set',addr=rs['rksOpen'],type='exec')
    command('H:/Almaz/STD/Almaz2.rks')
    state=m.m.call('cpu_state')
    if state['registers']['pc']['dec']!=rs['rksOpen']:
        m.m.screenshot(out/'run-failure.png')
        raise AssertionError((state,m.m.read(s['vars.cmdLine'],128)))
    assert m.m.read(rs['appName'],128).split(b'\0')[0].upper()==('H:/'+base+'/STD/ALMAZ2.RKS').upper().encode()
    m.m.screenshot(out/'run-full-path.png')
    (out/'nc-result.json').write_text(json.dumps({'keyboardNavigation':True,'cd':True,'chdir':True,'independentPanelsSameDrive':True,'rootShortcut':True,'dotStaysInDirectory':True,'arrowDoesNotRedrawPrompt':True,'commandRksAssociation':True},indent=2)+'\n')
    print('NC DIRECTORY UI PASS; RUN received full path',flush=True)
except Exception:
    m.m.screenshot(out/'interface-failure.png')
    print(m.m.call('cpu_state'),flush=True)
    ms=h['symbols'](R/'bin/DOSMOD.sym')
    print('DOSERR',m.m.read(d['v_dosError'],1).hex(),'CLIPATH',m.m.read(ms['cliPath1'],128),'CLILINE',m.m.read(ms['cliLine'],128),flush=True)
    raise
finally:m.stop()
