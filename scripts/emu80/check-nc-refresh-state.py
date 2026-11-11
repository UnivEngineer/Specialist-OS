"""NC: восстановление экрана, инвалидирование путей и сохранение перед запуском."""
from pathlib import Path
import runpy, re, json
R=Path(__file__).resolve().parents[2];O=R/'docs/nc/refresh-2026-11-08/state';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001')
x=h['Matrix']();m=x.m
n={a:int(b,16) for a,b in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text('utf-8'),re.M)}
rows=[]
def ready():m.call('bp_clear');m.call('emu_run_for',ms=500)
def type_text(text):
 for c in text:m.tap({' ':'space',':':'colon','/':'slash','.':'period','-':'minus'}.get(c,c.lower()))
def command(text):
 ready();m.tap('esc');m.call('emu_run_for',ms=250);type_text(text)
 assert m.read(n['vars.cmdLine'],len(text)).upper()==text.upper().encode()
 m.stop_at(n['mainLoop']);m.tap('enter');m.until(n['mainLoop'],60)
def pixels():return b''.join(m.read(0x9000+c*256,208) for c in range(48))
def toolbar():return b''.join(m.read(0x9000+c*256+247,9) for c in range(48))
def path(name):return m.read(n[name],128).split(b'\0')[0].upper()
def check(name):rows.append(name);print('PASS',name,flush=True)
try:
 x.start('NC refresh state')
 # Сравниваем одинаковую фазу до рисования выделения, а не его инверсию.
 m.stop_at(n['mainLoop']);m.tap('home');m.until(n['mainLoop'],40)
 original=pixels();bar=toolbar()
 ready();m.tap('f7');m.stop_at(n['mainLoop']);m.tap('esc');m.until(n['mainLoop'],40)
 if pixels()!=original:
  print('DIFF',[(i//208,i%208,a,b) for i,(a,b) in enumerate(zip(original,pixels())) if a!=b][:30],flush=True)
  m.screenshot(O/'failure.png')
  raise AssertionError('F7 cancel changed file area')
 check('F7-cancel-restores-panels')
 ready();type_text('DIR A:');m.stop_at(n['ncCommandRestoreScreen']);m.tap('enter')
 m.call('emu_run_for',ms=1500);m.tap('enter');m.until(n['ncCommandRestoreScreen'],40)
 m.stop_at(n['mainLoop']);m.until(n['mainLoop'],40)
 assert pixels()==original and toolbar()==bar,'DIR restore changed screen'
 check('DIR-restores-panels-and-toolbar')
 for text in ('MD B:/PARENT','MD B:/PARENT/CHILD','CD B:/PARENT/CHILD'):command(text)
 ready();m.tap('tab');command('CD B:/PARENT/CHILD')
 assert path('vars.pathA')==path('vars.pathB')==b'B:\\PARENT\\CHILD'
 command('REN B:/PARENT B:/RENAMED')
 assert path('vars.pathA')==path('vars.pathB')==b'B:\\RENAMED\\CHILD',(path('vars.pathA'),path('vars.pathB'))
 check('ancestor-rename-invalidates-both-path-caches')
 state=m.read(n['state'],n['stateEnd']-n['state'])
 ready();type_text('NC');m.stop_at(n['saveState']);m.tap('enter');m.until(n['saveState'],40)
 m.stop_at(n['mainLoop']);m.until(n['mainLoop'],60)
 assert m.read(n['state'],len(state))==state,'program launch did not persist panel state'
 check('program-launch-saves-and-restores-state')
 m.screenshot(O/'panels.png');(O/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
finally:x.stop()
