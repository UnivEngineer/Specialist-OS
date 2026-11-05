"""Четыре кнопки подтверждения и действие All до конца одной операции."""
from pathlib import Path
import runpy,re,json
R=Path(__file__).resolve().parents[2];O=R/'docs/nc/overwrite-all-2026-11-05/ui';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001';m=h['Matrix']()
n={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text(),re.M)}
def tap(key):
 r=m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not r.get('stoppedByBreakpoint'):m.m.call('emu_run_for',ms=120)
def text(value):
 for c in value:tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(c,c.lower()))
def finish(key,dialog=False):
 target=n['ncOverwriteKey'] if dialog else n['mainLoop']
 m.m.stop_at(target)
 if not dialog:m.m.call('bp_set',addr=n['ncOverwritePrompt'],type='exec')
 tap(key)
 for _ in range(120):
  state=m.m.call('cpu_state');pc=state['registers']['pc']['dec']
  if pc==target:break
  assert pc!=n['ncOverwritePrompt'],('unexpected repeated confirmation',state)
  m.m.call('emu_run_for',ms=500)
 else:raise AssertionError(state)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=250)
def command(value,dialog=False):
 tap('esc');text(value)
 assert m.m.read(n['vars.cmdLine'],len(value)).upper()==value.upper().encode()
 finish('enter',dialog)
 print(value,flush=True)
def size():
 command('CD B:/DST/SRC')
 cached=m.m.read(n['FILE_CACHE_A'],n['FILE_CACHE_BYTES'])
 entry=next(cached[i:i+16] for i in range(0,255*16,16) if cached[i:i+11].upper()==b'ONE        ')
 return int.from_bytes(entry[14:16],'little')
try:
 m.start('overwrite-all',disk=R/'bin/nested-Games2M.bin')
 command('MD B:/SRC');command('MD B:/DST')
 command('COPY A:/README.TXT B:/SRC/ONE')
 command('COPY A:/FONTKOI.FNT B:/SRC/TWO')
 command('ATTRIB -RS B:/SRC/TWO')
 command('COPY B:/SRC B:/DST')
 command('COPY A:/FONTKOI.FNT B:/SRC/ONE',True);finish('y')
 command('COPY B:/SRC B:/DST',True)
 assert m.m.read(n['ncOverwriteChoice'],1)==b'\2'
 m.m.screenshot(O/'default-no.png')
 tap('left');assert m.m.read(n['ncOverwriteChoice'],1)==b'\1'
 m.m.screenshot(O/'yes-to-all.png');finish('enter')
 assert size()==1280
 command('COPY A:/README.TXT B:/SRC/ONE',True);finish('y')
 command('COPY B:/SRC B:/DST',True)
 tap('right');assert m.m.read(n['ncOverwriteChoice'],1)==b'\3'
 m.m.screenshot(O/'no-to-all.png');finish('enter')
 assert size()==1280,'No to all replaced existing data'
 # Следующая независимая операция снова спрашивает.
 command('COPY B:/SRC/ONE B:/DST/SRC/ONE',True);finish('esc')
 (O/'result.json').write_text(json.dumps(dict(yesToAll=True,noToAll=True,onePromptPerTree=True,policyResets=True,defaultNo=True),indent=2))
 print('OVERWRITE ALL Emu80 PASS',flush=True)
except Exception:
 if m.pid:m.m.screenshot(O/'failure.png')
 raise
finally:m.stop()
