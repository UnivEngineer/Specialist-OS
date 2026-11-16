"""Owned visible/muted Emu80: NC switches font via real command editing."""
from pathlib import Path
import runpy,json,sys
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'.local/nc-layout-ui';OUT.mkdir(exist_ok=True)
def main():
 h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
 h['Matrix'].__init__.__globals__.update(RUNTIME=ROOT/'.local/nc-sort-benchmark-runtime',OUT=OUT)
 machine=h['Matrix']();n=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-sort.py'))['symbols'](ROOT/'bin/NC.sym')
 try:
  machine.start('adaptive-font',disk=ROOT/'images/FlashDisk_emu80_seed.bin')
  m=machine.m
  def byte(k):return m.read(n[k],1)[0]
  def text():
   return m.read(n['vars.cmdLine'],127).split(b'\0',1)[0]
  keys={' ':'space',':':'colon','.':'period','-':'minus','/':'slash','\\':'backslash'}
  def type_text(s):
   for c in s:m.tap(keys.get(c,c.lower()))
  def command(s):
   m.tap('esc');type_text(s);assert text().upper()==s.encode().upper(),(s,text())
   m.tap('enter')
   if s.upper().startswith('FONT '):
    target=n['ncReadKey'];m.stop_at(target)
    expected=(ROOT/'bin/NC.i80').read_bytes()[:32]
    for attempt in range(120):
     m.until(target,120,page=0)
     if (m.read(0,32)==expected and
         int.from_bytes(m.read(n['vars.cmdLineEnd'],2),'little')==n['vars.cmdLine'] and
         byte('ncFontWidth')==m.read(n['FONT_ENGINE_HEADER']+5,1)[0]):break
     m.call('debug_step',mode='into')
    else:raise AssertionError('NC did not finish FONT return')
    m.call('bp_clear');m.call('emu_run_for',ms=500)
   else:m.call('emu_run_for',ms=1500)
  def shot(s):
   m.call('emu_run_for',ms=1500)
   m.screenshot(OUT/(s+'.png'))
  def close_dialog():
   m.tap('esc');m.call('emu_run_for',ms=1500)
  visual='--visual-only' in sys.argv
  focus='--focus' in sys.argv
  for font,width in [('Rotate',6),('Cpm4',4),('Fast8',8),('Compact',6)]:
   command('FONT '+font)
   assert byte('ncFontWidth')==width,(font,byte('ncFontWidth'))
   shot(font+'-panels')
   if focus:
    for key in ('f1','f2'):
     m.tap(key);shot(font+'-'+key+'-fixed');close_dialog()
    continue
   m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=700);shot(font+'-shift')
   m.tap('f1');m.call('kbd_send',keys=['shift'],action='release');shot(font+'-sort');close_dialog()
   m.tap('f1');shot(font+'-drives');close_dialog()
   m.tap('f7');m.call('emu_run_for',ms=1500);type_text('longname.txt');shot(font+'-input');close_dialog()
   m.tap('f3');m.call('emu_run_for',ms=1500);shot(font+'-info');m.tap('f3');m.call('emu_run_for',ms=1500);m.tap('esc')
   if visual:continue
   if byte('state.activePanel')==0:m.tap('tab');m.call('emu_run_for',ms=1000)
   long='echo '+('abcdefghi'*8)
   type_text(long);assert text().upper()==long.encode().upper(),(font,text(),long)
   for _ in range(8):m.tap('left')
   pos=byte('lineEditPosition');off=byte('lineEditOffset');assert pos==len(long)-8
   m.tap('x');m.tap('backspace');assert text().upper()==long.encode().upper()
   # Up/Down must preserve the middle text position even across panel scrolling.
   for _ in range(byte('ncVisibleCount')+1):m.tap('down')
   assert byte('lineEditPosition')==pos and text().upper()==long.encode().upper()
   assert byte('bios_vars.cursorX')==m.read(n['lineEditOrigin']+1,1)[0]+(pos-byte('lineEditOffset'))*width//2
   shot(font+'-command');m.tap('esc')
   print(font,'menus, command editing, panel navigation PASS',flush=True)
  def observed_command(value,expect_refresh):
   m.tap('esc');type_text(value)
   assert text().upper()==value.encode().upper()
   before=b''.join(m.read(0x9000+x*256,230) for x in range(48))
   labels=['ncReadKey','ncDiskChanged','loadAndPrintPanel','drawCursor']
   m.call('bp_clear')
   for label in labels:m.call('bp_set',addr=n[label],type='exec')
   m.tap('enter');events=[]
   for attempt in range(300):
    state=m.call('cpu_state');pc=state['registers']['pc']['dec']
    page=m.read(machine.pageAddress,1)[0]
    if page==0 and pc==n['ncReadKey'] and int.from_bytes(m.read(n['vars.cmdLineEnd'],2),'little')==n['vars.cmdLine']:break
    if page==0:
     for label in labels[1:]:
      if pc==n[label]:events.append((label,state['registers']['af']['dec']>>8,state['registers']['bc']['dec'],state['registers']['de']['dec']))
    if pc in [n[label] for label in labels]:m.call('debug_step',mode='into')
    m.call('emu_run_for',ms=500)
   else:raise AssertionError(('command timeout',value,events))
   m.call('bp_clear');m.call('emu_run_for',ms=500)
   after=b''.join(m.read(0x9000+x*256,230) for x in range(48))
   if not expect_refresh:
    assert not any(row[0] in ('loadAndPrintPanel','drawCursor') for row in events),(value,events)
    assert before==after,(value,'unrelated panel bitmap changed')
   else:assert any(row[0]=='loadAndPrintPanel' for row in events),(value,events)
   print('QUIET',value,events,flush=True)
  if focus:
   command('B:')
   observed_command('MD B:/PARENT',True)
   observed_command('MD B:/PARENT/CHILD',False)
   observed_command('MD B:/PARENT/CHILD/LEAF',False)
   observed_command('COPY A:/README.TXT B:/PARENT/CHILD/FILE.TXT',False)
   observed_command('DEL B:/PARENT/CHILD/FILE.TXT',False)
   print('NC drive title and unrelated-directory refresh PASS',flush=True)
   return
  command('B:')
  for p in ['B:/FOLDER1','B:/FOLDER1/FOLDER2','B:/FOLDER1/FOLDER2/FOLDER3']:
   command('MD '+p)
  command('CD B:/FOLDER1/FOLDER2/FOLDER3')
  for font,width in [('Cpm4',4),('Fast8',8),('Rotate',6)]:
   command('FONT '+font);assert byte('ncFontWidth')==width
   type_text('echo testing');shot(font+'-long-path');m.tap('esc')
  print('NC ADAPTIVE FONT UI PASS',flush=True)
 except Exception:
  try:
   machine.m.call('debug_break')
   (OUT/'failure-state.json').write_text(json.dumps(machine.m.call('cpu_state'),indent=2))
   (OUT/'failure-memory.bin').write_bytes(machine.m.read(0,65536))
  except Exception:pass
  raise
 finally:
  try:machine.m.screenshot(OUT/'last.png')
  except Exception:pass
  machine.stop()
if __name__=='__main__':main()

