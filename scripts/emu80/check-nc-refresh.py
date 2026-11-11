"""Профиль NC: навигация, кэш путей, сохранение и точная очистка хвоста."""
from pathlib import Path
import runpy, json, sys, re, argparse
R=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser();parser.add_argument('--before',action='store_true');parser.add_argument('--report',type=Path,default=R/'docs/nc/refresh-2026-11-08');args=parser.parse_args()
phase='before' if args.before else 'after'
O=args.report/phase;O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001')
x=h['Matrix']();m=x.m;n={a:int(b,16) for a,b in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text('utf-8'),re.M)};k=h['symbols'](R/'bin/banked/KERNEL.sym');rows=[]
def bitmap():
 offset=24*m.read(n['state.activePanel'],1)[0]
 return b''.join(m.read(0x9000+256*(c+offset),230) for c in range(24))
def command(text, entry="ncCommandRun"):
 m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
 if entry=='ncCommandRun':
  for c in text:m.tap({' ':'space',':':'colon','/':'slash','.':'period','-':'minus'}.get(c,c.lower()))
  assert m.read(n['vars.cmdLine'],len(text)).upper()==text.upper().encode()
 m.stop_at(n[entry]);m.tap('enter');start=m.until(n[entry],40)['cpu_ticks']
 m.call('bp_clear')
 watch={n[z]:(z,0) for z in ('saveState','draw','refreshFilePanel','mainLoop')}
 watch[k['directoryGetCwd']]=('GETCWD',1)
 for addr in watch:m.call('bp_set',addr=addr,type='exec')
 counts={z:0 for z,_ in watch.values()}
 for _ in range(300):
  m.call('emu_run_for',ms=500);state=m.call('cpu_state');pc=state['registers']['pc']['dec']
  name,bank=watch.get(pc,('',255))
  if name and m.read(k['bankPage'],1)==bytes([bank]):
   if name=='draw' and state['registers']['hl']['dec']!=n['g_filePanel']:continue
   counts[name]+=1
   if name=='mainLoop':break
 else:raise AssertionError(state)
 actual=m.read(n['vars.path'],128).split(b'\0')[0].decode()
 row=dict(command=text,ms=(state['cpu_ticks']-start)/2000,path=actual,counts=counts)
 rows.append(row);print(row,flush=True)
 if phase=='after':
  assert counts['saveState']==0,row
  assert counts['GETCWD']<=1,row
  assert counts['draw']==0,row
  # Полная перерисовка того же состояния обязана дать тот же битмап.
  m.call('bp_clear');before=bitmap();m.write(n['ncKeepFrames'],b'\0')
  m.stop_at(n['mainLoop']);m.call('regs_set',pc=n['mainReloadActive']);m.until(n['mainLoop'],40)
  after=bitmap()
  if after!=before:
   (O/'incremental.bin').write_bytes(before);(O/'full.bin').write_bytes(after)
   print('DIFF',[(i//230,i%230,a,b) for i,(a,b) in enumerate(zip(before,after)) if a!=b][:45],flush=True)
   m.screenshot(O/'mismatch.png')
   raise AssertionError(('incremental/full bitmap mismatch',text))
 m.screenshot(O/(str(len(rows))+'.png'))
try:
 x.start('NC refresh '+phase)
 for text in ('CD A:/Bench','CD FontBen','CD ..','CD /','CD B:/','CD A:/Bench/FontBen','CD .'):
  command(text)
 if phase=='after':
  # Пункт корня и обычный Enter на каталоге, затем независимый кэш справа.
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('home')
  command('Enter root','enterDirectory')
  entries=m.read(n['FILE_CACHE_A'],n['FILE_CACHE_BYTES'])
  index=next(i//16 for i in range(0,255*16,16) if entries[i:i+8].rstrip().upper()==b'BENCH')
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('home')
  for _ in range(index):m.tap('down')
  command('Enter Bench','enterDirectory')
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('tab')
  command('CD A:/Bench/FontBen')
  command('CD .')
  # F10 сохраняет состояние один раз; свежий NC восстанавливает обе панели.
  m.call('bp_clear');m.call('emu_run_for',ms=500)
  m.stop_at(n['saveState']);m.tap('f10');m.until(n['saveState'],40)
  saved=m.read(n['state'],n['stateEnd']-n['state'])
  cli=h['symbols'](R/'bin/CLI.sym');m.stop_at(cli['cliPrompt']);m.until(cli['cliPrompt'],40)
  m.call('bp_clear');m.call('emu_run_for',ms=500)
  for c in 'NC':m.tap(c.lower())
  m.stop_at(n['mainLoop']);m.tap('enter');m.until(n['mainLoop'],40)
  assert m.read(n['state'],len(saved))==saved,'F10 did not persist panel state'
  rows.append(dict(case='F10-and-NC-restores-state',passed=True))
 (O/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
 print('NC REFRESH PASS',phase,len(rows),flush=True)
finally:x.stop()
