"""Видимый Emu80: команды NC, файловые операции и маленькая игра через RUN."""
from pathlib import Path
import runpy,re,json,sys
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/(sys.argv[sys.argv.index('--report')+1] if '--report' in sys.argv else 'docs/bios-dos-split-2026-11-06/emulator');OUT.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g['OUT']=OUT;g['RUNTIME']=ROOT/'.local/monitor2-001'
def symbols(name):
 return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/f'bin/{name}.sym').read_text(),re.M)}
n=symbols('NC');d=symbols('DOS');r=symbols('RUN')
x=h['Matrix']();rows=[]
try:
 x.start('split',system=ROOT/'images/SystemRom_emu80_writable.bin')
 m=x.m
 def command(text,stop=None):
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
  for c in text:m.tap({' ': 'space',':':'colon','.':'period','-':'minus','/':'slash'}.get(c,c.lower()))
  assert m.read(n['vars.cmdLine'],len(text)).upper()==text.encode().upper(),text
  m.screenshot(OUT/('command-'+str(len(rows))+'.png'))
  m.stop_at(stop if stop is not None else n['mainLoop']);m.tap('enter')
  if stop is not None:
   for label in ['badImage','usage','notFound']:m.call('bp_set',addr=r[label],type='exec')
  for _ in range(20):
   state=m.call('cpu_state');pc=state['registers']['pc']['dec']
   if pc==(stop if stop is not None else n['mainLoop']):break
   if stop is not None and pc in [r[k] for k in ['badImage','usage','notFound']]:
    m.screenshot(OUT/'run-error.png');raise AssertionError((text,state,m.read(d['v_dosError'],1).hex(),{k:m.read(r[k],32).hex() for k in ['appName','descriptor','callerDrive']},m.read(d['v_drive'],1).hex()))
   m.call('emu_run_for',ms=500)
  else:raise AssertionError((text,state))
  rows.append(dict(command=text,ticks=state['cpu_ticks'],error=m.read(d['v_dosError'],1)[0]))
  m.call('bp_clear');return state
 if '--game-only' not in sys.argv:
  command('mkdir B:/SPLIT')
  command('copy A:/README.TXT B:/SPLIT/README.TXT')
  command('ren B:/SPLIT/README.TXT B:/SPLIT/TEST.TXT')
  command('cd B:/SPLIT')
  assert int.from_bytes(m.read(d['v_directories']+2,2),'little')>=2
  m.call('emu_run_for',ms=500);m.screenshot(OUT/'files.png')
  command('del B:/SPLIT/TEST.TXT')
  command('cd B:/')
 command('H:')
 if '--driver-write' in sys.argv:
  command('copy A:/README.TXT H:/ABI-TMP.TXT')
  command('copy H:/ABI-TMP.TXT B:/H-READ.TXT')
  command('del H:/ABI-TMP.TXT')
  command('del B:/H-READ.TXT')
  assert all(row['error']==0 for row in rows),rows
  assert int.from_bytes(m.read(d['v_drives']+14,2),'little')==d['DISK_DRIVER_ADDR']==0xE800
 # Реальная игра BALL: 5040 байт с 0000h, ниже временного DOS 4000h.
 for mode in ([] if '--files-only' in sys.argv else ['MX','STD','MON']):
  command('H:/Ball.bat' if mode=='MX' else 'RUN -'+mode+' H:/Ball.exe',r['ready'])
  m.stop_at(0);m.until(0,40)
  assert m.read(0,32)==(ROOT/'assets/games/BALL.RKS').read_bytes()[4:36]
  if mode in ['MX','STD']:assert m.read(d['bios_upper.koi7'],1)==b'\xff'
  m.call('bp_clear');m.call('emu_run_for',ms=2000);m.screenshot(OUT/('ball-'+mode.lower()+'.png'))
  m.tap('enter');m.call('emu_run_for',ms=1000);m.screenshot(OUT/('ball-menu-'+mode.lower()+'.png'))
  m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],40)
  assert m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]
 (OUT/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
 print('NC file commands PASS' if '--files-only' in sys.argv else 'NC commands and BALL MX/STD/MON launch PASS',rows)
finally:x.stop()
