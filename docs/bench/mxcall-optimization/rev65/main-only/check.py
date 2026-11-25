import hashlib
import json
import os
from pathlib import Path
import runpy
import shutil
import time

ROOT=Path(__file__).resolve().parents[5]
OUT=Path(__file__).resolve().parent
BUNDLE=OUT.parent/'final2-bin'
OUT.mkdir(parents=True,exist_ok=True)
PORT=19781
slot=Path(os.environ['LOCALAPPDATA'])/'Specialist-OS/emu80'/str(PORT)
RUNTIME=slot/'rev65-main-only-hidden'
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
suite=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-mem-bank.py'))
symbols=suite['symbols']
b=symbols(BUNDLE/'bin/BIOS.sym')
e=symbols(BUNDLE/'bin/EDIT.sym')
t=symbols(BUNDLE/'bin/banked/TRANSPORT.sym')
n=symbols(BUNDLE/'bin/NC.sym')
m=h['Bench'](f'http://127.0.0.1:{PORT}/mcp')
def owner():
    return h['shell'](f'$c=Get-NetTCPConnection -State Listen -LocalPort {PORT} -ErrorAction SilentlyContinue; if($c){{$c[0].OwningProcess}};exit 0')
def ready(delay=500):
    m.call('bp_clear')
    m.call('emu_run_for',ms=delay)
def u16(symbol):
    return int.from_bytes(m.read(symbol,2),'little')
def text(command):
    names={' ':'space', ':':'colon', '/':'slash', '.':'period'}
    for char in command.lower():
        if char=='_':
            m.call('kbd_send',keys=['shift'],action='press')
            try:m.tap('0')
            finally:m.call('kbd_send',keys=['shift'],action='release')
        else:m.tap(names.get(char,char))
def wait_any(addresses, seconds=24):
    m.call('bp_clear')
    for addr in addresses:
        m.call('bp_set',addr=addr,type='exec')
    for _ in range(seconds):
        state=m.call('cpu_state')
        pc=state['registers']['pc']['dec']
        if pc in addresses:
            return pc
        m.call('emu_run_for',ms=1000)
    raise RuntimeError(m.call('cpu_state'))
def page(value):
    m.write(0xfffd if value else 0xfffc,bytes([value-1 if value else 0]))
def doc():
    begin,lo,hi,end=(u16(e[key]) for key in ('v_textBeginLo','v_gapLo','v_gapHi','v_textEnd'))
    return m.read(begin,lo-begin)+m.read(hi,end-lo)
assert not owner()
shutil.copytree(slot/'replica-hidden',RUNTIME,dirs_exist_ok=True)
suite['hidden_qt'](RUNTIME)
deployment=RUNTIME/'specialist-os'
for name in ('launch.conf','specialist-mx2.conf'):
    shutil.copyfile(OUT/name,deployment/name)
released=(BUNDLE/'images/C-Flash-512k-MMX-System-Emu80.bin').read_bytes()
autoexec=(OUT/'AUTOEXEC.BAT').read_bytes()
offset=27136
assert len(autoexec)==169 and released[offset:offset+5]==b'PATH '
assert b'RAMDISK.COM' in released[offset:offset+169] and b'NC_MN.COM' in autoexec
test_image=released[:offset]+autoexec+released[offset+len(autoexec):]
(deployment/'system-flash.bin').write_bytes(test_image)
exe=RUNTIME/'Emu80qt-mcp.exe'
quoted=suite['ps']
shell=suite['shell']
pid=int(shell(f"$p=Start-Process -FilePath {quoted(exe)} -ArgumentList '--conf-file','specialist-os/launch.conf',"
              f"'--platform','specialist-os' -WorkingDirectory {quoted(RUNTIME)} -WindowStyle Hidden -PassThru; $p.Id"))
try:
    for _ in range(30):
        if owner()==str(pid):
            break
        time.sleep(.2)
    else:
        raise RuntimeError('MCP ownership did not match')
    m.call('debug_break')
    assert Path(shell(f'Get-Process -Id {pid} | Select-Object -ExpandProperty Path'))==exe
    assert shell(f'(Get-Process -Id {pid}).MainWindowHandle.ToInt64()')=='0'
    info=m.call('sys_info')
    assert info['cpu']['speed_factor']==32 and not info['cpu']['full_throttle']
    assert 'pitSoundSource.muted = yes' in (RUNTIME/'specialist-os/launch.conf').read_text()
    assert 'tapeMonitor.muted = yes' in (RUNTIME/'specialist-os/launch.conf').read_text()
    m.call('bp_clear')
    m.stop_at(b['biosBootWaitEnter'])
    m.call('emu_reset')
    addresses=[b['biosBootWaitEnter'],b['biosBankShell'],b['biosRecoveryStart'],n['mainLoop']]
    boots=[]
    for _ in range(8):
        pc=wait_any(addresses)
        state=m.read(t['bankPage'],1)[0]
        boots.append([hex(pc),state])
        print('boot',hex(pc),state,flush=True)
        if state==0 and pc==b['biosBootWaitEnter']:
            m.screenshot(OUT/'boot-prompt.png')
            m.call('bp_clear')
            m.tap('enter')
        elif state==0 and pc==n['mainLoop'] and m.read(0,3)==(BUNDLE/'bin/NC.i80').read_bytes()[:3]:
            break
        else:
            m.call('debug_step',mode='into')
    else:
        raise RuntimeError('NC MAIN did not start')
    assert m.read(b['biosBootPages'],1)==b'\x01'
    assert m.read(t['bankPageCount'],1)==b'\x01'
    page(1)
    dos_count=m.read(t['bankPageCount'],1)[0]
    assert dos_count==1
    page(0)
    m.screenshot(OUT/'nc-main.png')
    ready()
    m.tap('esc')
    m.call('emu_run_for',ms=250)
    (OUT/'nc-empty.vram').write_bytes(m.read(0x9000,0x3000))
    m.screenshot(OUT/'nc-empty.png')
    command='C:/DOS/EDIT_MN.COM C:/AUTOEXEC.BAT'
    text(command)
    m.call('emu_run_for',ms=350)
    assert m.read(n['vars.cmdLine'],len(command)).upper()==command.upper().encode()
    (OUT/'nc-command.vram').write_bytes(m.read(0x9000,0x3000))
    m.screenshot(OUT/'nc-command.png')
    m.stop_at(e['keyInput'])
    m.tap('enter')
    m.until(e['keyInput'],24,page=0)
    original=doc()
    assert original
    m.screenshot(OUT/'edit-main.png')
    ready()
    m.stop_at(e['keyInput'])
    m.tap('x')
    m.until(e['keyInput'],24,page=0)
    m.stop_at(b['bios_getch'])
    m.until(b['bios_getch'],24,page=0)
    edited=doc()
    print('document',len(original),len(edited),original[:16],edited[:16],flush=True)
    assert edited[:1].lower()==b'x' and edited[1:]==original
    assert u16(e['v_column'])==1 and m.read(e['v_documentDirty'],1)==b'\x01'
    m.screenshot(OUT/'edit-input.png')
    ready()
    m.stop_at(e['editorSaveWait'])
    m.tap('f10')
    m.until(e['editorSaveWait'],24,page=0)
    ready()
    m.stop_at(e['editorDiscardFile'])
    m.tap('d')
    try:
        m.until(e['editorDiscardFile'],5,page=0)
    except RuntimeError:
        ready()
        m.tap('tab')
        assert m.read(e['uiModalFocusIndex'],1)==b'\x01'
        m.stop_at(e['editorDiscardFile'])
        m.tap('enter')
        m.until(e['editorDiscardFile'],5,page=0)
    m.stop_at(n['mainLoop'])
    m.until(n['mainLoop'],24,page=0)
    assert m.read(0,3)==(BUNDLE/'bin/NC.i80').read_bytes()[:3]
    assert m.read(t['bankPageCount'],1)==b'\x01'
    m.screenshot(OUT/'nc-return.png')
    report={'release_sha256':hashlib.sha256(released).hexdigest(),'test_sha256':hashlib.sha256(test_image).hexdigest(),
            'port':PORT,'detected_pages':1,'dos_page_count':dos_count,'free_exec_pages':0,
            'hidden':True,'muted':True,'cpu':info['cpu'],'entry':'C:/DOS/NC_MN.COM',
            'command':command,'document_bytes':len(original),'edited_bytes':len(edited),
            'inserted':chr(edited[0]),'exit':'F10, discard changes, NC MAIN','boot_breakpoints':boots}
    (OUT/'result.json').write_text(json.dumps(report,indent=2)+'\n')
    print('PASS',report,flush=True)
except Exception:
    m.screenshot(OUT/'failure.png')
    raise
finally:
    shell(f'$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; if($p -and $p.Path -eq {quoted(exe)}) '
          '{ Stop-Process -InputObject $p; $p.WaitForExit() }; exit 0')
