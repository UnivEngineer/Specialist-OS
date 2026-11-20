"""EDIT 6.0: keyboard-only NC F4, TXT association, Shift-F4 and console launch."""
from pathlib import Path
import argparse,runpy,re,json
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782')
p.add_argument('--port',type=int,choices=range(19781,19785),default=19782)
a=p.parse_args();O=R/'.local/edit-launch-check';O.mkdir(exist_ok=True)
sy=lambda f:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/f).read_text(),re.M)}
s=sy('EDIT.sym');n=sy('NC.sym')
body=(R/'bin/EDIT.i80').read_bytes();readme=(R/'assets/system-rom/README.TXT').read_bytes()
credit=b'EDIT 6.0 (C) 2026 SPb SpaceEngineer\r\nBased on E 4.1 (C) 1992 Omsk KSOFT\r\n\0'
for artifact in ('EDIT','EDITCORE'):
 symbols=sy(artifact+'.sym')
 binary=(R/'bin'/f'{artifact}.i80').read_bytes()
 offset=symbols['editorCopyright']-(0x100 if artifact=='EDITCORE' else 0)
 assert binary[offset:offset+len(credit)]==credit,artifact
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g.update(OUT=O,RUNTIME=a.runtime.resolve())
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp'
def ready():m.call('bp_clear');m.call('emu_run_for',ms=500)
def tap(key):
 r=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not r.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
def prepare_nc():
 ready()
 if (m.read(n['uiModalCurrent'],2)==n['ncCatalogWarningModal'].to_bytes(2,'little')
     and m.read(n['ncDialogActive'],1)==b'\x01'):
  tap('enter');ready()
def select_readme():
 prepare_nc()
 if m.read(n['state.activePanel'],1)!=b'\0':tap('tab')
 assert m.read(n['state.activePanel'],1)==b'\0'
 cache=int.from_bytes(m.read(n['ncCatalogBases'],2),'little')
 count=m.read(n['state.panelA_filesCnt'],1)[0]
 indexes=m.read(n['FILE_INDEX_A'],count)
 target=next(i for i,v in enumerate(indexes) if m.read(cache+v*16,11).upper()==b'README  TXT')
 current=m.read(n['state.panelA_curFile'],1)[0]
 for _ in range(abs(target-current)):tap('down' if target>current else 'up')
 assert m.read(n['state.panelA_curFile'],1)==bytes([target])
def launch(key,shift=False,notice=True):
 prepare_nc()
 before=m.read(0x9000,0x3000)
 def notice_region(image):
  return b''.join(image[x*256+112:x*256+144] for x in range(16,32))
 first=n['ncEditorLoading'] if notice else s['editorLoadingDone']
 m.stop_at(first)
 if shift:
  m.call('kbd_send',keys=['shift'],action='press')
  m.call('emu_run_for',ms=200)
 tap(key)
 if shift:m.call('kbd_send',keys=['shift'],action='release')
 m.until(first,90,page=0)
 if notice:
  m.stop_at(s['editorLoadingDone'])
  m.until(s['editorLoadingDone'],90,page=0)
 assert m.read(s['keyInput'],3)==body[s['keyInput']:s['keyInput']+3]
 m.stop_at(s['editorPaintDocument'])
 m.until(s['editorPaintDocument'],90,page=0)
 assert m.read(s['editorInterfaceReady'],1)==b'\0'
 assert m.read(s['editorProgressVisible'],1)==b'\0'
 if notice:
  assert notice_region(m.read(0x9000,0x3000))!=notice_region(before)
  m.screenshot(O/(key+('-shift' if shift else '')+'-loading.png'))
 m.stop_at(s['editorStatusLoop'])
def arrive(label,expected):
 m.until(s['editorStatusLoop'],90,page=0)
 assert m.read(s['keyInput'],3)==body[s['keyInput']:s['keyInput']+3]
 start=int.from_bytes(m.read(s['v_textBeginLo'],2),'little');end=int.from_bytes(m.read(s['v_textEnd'],2),'little')
 lo=int.from_bytes(m.read(s['v_gapLo'],2),'little');hi=int.from_bytes(m.read(s['v_gapHi'],2),'little')
 assert m.read(start,lo-start)+m.read(hi,end-lo)==expected,(label,end-start,len(expected))
 assert m.read(s['v_insertMode'],1)==b'\xff'
 ready();m.screenshot(O/(label+'.png'))
 print('PASS',label,flush=True)
def leave():
 m.stop_at(n['mainLoop'])
 m.call('bp_set',addr=n['ncCatalogWarningKey'],type='exec')
 tap('clear')
 for _ in range(90):
  pc=m.call('cpu_state')['registers']['pc']['dec']
  if pc==n['mainLoop']:break
  if pc==n['ncCatalogWarningKey']:
   m.stop_at(n['mainLoop'])
   tap('enter')
  else:m.call('emu_run_for',ms=1000)
 else:raise AssertionError('NC did not resume after EDIT')
 prepare_nc()
try:
 x.start('edit-launch')
 select_readme();launch('f4');arrive('f4',readme);leave()
 select_readme();launch('enter');arrive('txt-association',readme);leave()
 launch('f4',shift=True)
 arrive('shift-f4',b'');leave()
 tap('esc')
 for ch in 'A:EDIT':tap('colon' if ch==':' else ch.lower())
 assert m.read(n['vars.cmdLine'],6).upper()==b'A:EDIT'
 m.screenshot(O/'console-command.png')
 launch('enter',notice=False);arrive('console-edit',b'');leave()
 print('EDIT 6.0 LAUNCH PASS',flush=True)
except Exception:
 if x.pid:m.call('debug_break');m.screenshot(O/'failure.png')
 raise
finally:x.stop()
