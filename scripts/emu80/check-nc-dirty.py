"""NC: команды обновляют только затронутые панели; следы окна не сбрасывают кэш."""
from pathlib import Path
import runpy,re,json
R=Path(__file__).resolve().parents[2];O=R/'docs/nc/dirty-2026-11-05/ui';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001';m=h['Matrix']()
def symbols(p):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
n=symbols(R/'bin/NC.sym');d=symbols(R/'bin/DOS.sym');dm=symbols(R/'bin/DOSMOD.sym');report=[]
def tap(key):
 r=m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not r.get('stoppedByBreakpoint'):m.m.call('emu_run_for',ms=120)
def text(value):
 for c in value:tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(c,c.lower()))
def panels():return b''.join(m.m.read(0x9000+x*256,230) for x in range(48))
def observe(key,target=None):
 target=target or n['mainLoop'];frames=[];reads=[]
 m.m.stop_at(target)
 for label in ['loadAndPrintPanel','lf_ReadCatalog']:m.m.call('bp_set',addr=n[label],type='exec')
 tap(key)
 for _ in range(180):
  state=m.m.call('cpu_state');pc=state['registers']['pc']['dec']
  if pc==target:break
  if pc==n['loadAndPrintPanel']:frames.append(state['registers']['bc']['dec']&255)
  if pc==n['lf_ReadCatalog']:reads.append(m.m.read(n['state.activePanel'],1)[0])
  m.m.call('emu_run_for',ms=500)
 else:raise AssertionError(state)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=250)
 return frames,reads

def command(value,frames,reads,prompt=False):
 before=panels();tap('esc');text(value)
 assert m.m.read(n['vars.cmdLine'],len(value)).upper()==value.upper().encode()
 actual=observe('enter',n['ncOverwriteKey'] if prompt else None)
 assert actual==(frames,reads),(value,actual,(frames,reads))
 if not prompt:
  assert m.m.read(d['v_dosError'],1)==b'\0'
  if not frames:assert before==panels(),('unaffected panel bitmap changed',value)
 report.append(dict(command=value,frames=actual[0],catalogReads=actual[1]))
 print(value,actual,flush=True)
try:
 m.start('dirty',disk=R/'bin/nested-Games2M.bin')
 command('COPY A:/README.TXT B:/ONE.TXT',[],[])
 command('MKDIR B:/D1',[],[])
 command('REN B:/ONE.TXT B:/TWO.TXT',[],[])
 command('MOVE B:/TWO.TXT B:/D1/THREE.TXT',[],[])
 command('DEL B:/D1/THREE.TXT',[],[])
 command('CD B:/D1',[0],[0])
 command('COPY A:/README.TXT B:/D1/ONE.TXT',[0],[0])
 command('MD B:/OTHER',[],[])
 command('COPY A:/README.TXT B:/OTHER/TWO.TXT',[],[])
 command('CD B:/OTHER',[0],[0])
 tap('tab');command('CD B:/OTHER',[1],[1])
 command('REN TWO.TXT THREE.TXT',[0,1],[0,1])
 command('COPY A:/README.TXT B:/D1/ONE.TXT',[],[],prompt=True)
 assert observe('n')==([0,1],[]),'No must restore cached panels'
 m.m.screenshot(O/'both-panels.png')
 tap('f7');assert observe('esc')==([0,1],[]),'cancelled dialog must reuse caches'
 # F5 пишет в невидимый каталог: только восстановить окно из кэшей.
 tap('home');tap('down');tap('down');tap('f5')
 value=m.m.read(n['vars.input'],128).split(b'\0')[0]
 for _ in value:tap('backspace')
 text('B:/D1/FOUR.TXT')
 assert observe('enter')==([0,1],[]),'F5 must not reload unrelated directories'
 # При замене файла на обеих видимых панелях обновляются кэши и строки сведений.
 command('COPY A:/FONTKOI.FNT B:/OTHER/THREE.TXT',[],[],prompt=True)
 assert observe('y')==([0,1],[0,1])
 cached=m.m.read(n['FILE_CACHE_B'],n['FILE_CACHE_BYTES'])
 entry=next(cached[i:i+16] for i in range(0,255*16,16) if cached[i:i+11].upper()==b'THREE   TXT')
 assert int.from_bytes(entry[14:16],'little')==1280,entry
 m.m.screenshot(O/'overwritten-size.png')
 # Перед выгрузкой NC отключает указатель на свой код.
 m.m.stop_at(n['ncExitToCli']);tap('f10');m.m.until(n['ncExitToCli'],30)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=2000)
 assert m.m.read(dm['cliChangeHook'],2)==b'\0\0'
 (O/'result.json').write_text(json.dumps(report,indent=2))
 print('NC DIRTY Emu80 PASS',len(report),flush=True)
except Exception:
 if m.pid:m.m.screenshot(O/'failure.png')
 raise
finally:m.stop()
