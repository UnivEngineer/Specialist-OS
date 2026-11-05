"""Yes/No при копировании из NC и CLI, только через клавиатуру Emu80."""
from pathlib import Path
import runpy,re,json
R=Path(__file__).resolve().parents[2];O=R/'docs/nc/copy-merge-2026-11-05/ui';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__
g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001';m=h['Matrix']()
def symbols(p):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
n=symbols(R/'bin/NC.sym');d=symbols(R/'bin/DOS.sym')
def tap(key):
 r=m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not r.get('stoppedByBreakpoint'):m.m.call('emu_run_for',ms=120)
def text(value):
 for c in value:tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(c,c.lower()))
def command(value,prompt=False):
 tap('esc');text(value)
 assert m.m.read(n['vars.cmdLine'],len(value)).upper()==value.upper().encode(),(value,m.m.read(n['vars.cmdLine'],len(value)),m.m.call('cpu_state'))
 target=n['ncOverwriteKey'] if prompt else n['mainLoop']
 m.m.stop_at(target);tap('enter');m.m.until(target,600)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=250)
def wait():m.m.call('emu_run_for',ms=1000)
def finish(key):
 m.m.stop_at(n['mainLoop']);tap(key);m.m.until(n['mainLoop'],120)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=250)
def error():return m.m.read(d['v_dosError'],1)[0]
try:
 m.start('copy-merge',disk=R/'bin/nested-Games2M.bin')
 command('COPY A:/README.TXT B:/ONE.TXT');assert error()==0,error()
 command('COPY A:/README.TXT B:/ONE.TXT',prompt=True)
 assert m.m.read(n['ncOverwriteChoice'],1)==b'\2'
 m.m.screenshot(O/'overwrite-default-no.png')
 tap('left');tap('left');assert m.m.read(n['ncOverwriteChoice'],1)==b'\0'
 m.m.screenshot(O/'overwrite-yes.png');finish('enter');assert error()==0,error()
 command('COPY A:/README.TXT B:/ONE.TXT',prompt=True);finish('esc');assert error()==0,error()
 command('B:');tap('tab');command('CD H:/')
 entries=m.m.read(n['FILE_CACHE_B'],n['FILE_CACHE_BYTES'])
 index=next(i for i in range(255) if entries[i*16:i*16+8].rstrip().upper()==b'BIGTEST')
 tap('home')
 for _ in range(index):tap('down')
 tap('f5');m.m.screenshot(O/'bigtest-copy-dialog.png')
 m.m.stop_at(n['mainLoop']);tap('enter');m.m.until(n['mainLoop'],600)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=250)
 assert error()==0,error();print('BigTest F5 completed',flush=True)
 command('CD B:/BigTest')
 count=m.m.read(n['state.panelB_filesCnt'],1)[0];assert count==57,count
 m.m.screenshot(O/'bigtest-copied.png')
 # Повторная копия объединяет каталог и спрашивает для каждого файла.
 command('COPY H:/BigTest B:/',prompt=True)
 m.m.stop_at(n['mainLoop']);tap('s');m.m.until(n['mainLoop'],120)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=250)
 print('BigTest merge/No to all completed',flush=True)
 # Переход к корню и родителю не открывает файловые диалоги.
 tap('esc');tap('home')
 for key in ['f5','f6','f8']:
  before=b''.join(m.m.read(0x9000+256*x,230) for x in range(48))
  tap(key)
  assert before==b''.join(m.m.read(0x9000+256*x,230) for x in range(48)),key
 (O/'result.json').write_text(json.dumps(dict(yesNo=True,escapeNo=True,bigTestEntries=count,pseudoEntryDialogsBlocked=True),indent=2))
 print('NC COPY Emu80 PASS',count,flush=True)
except Exception:
 if m.pid:m.m.screenshot(O/'failure.png')
 raise
finally:m.stop()
