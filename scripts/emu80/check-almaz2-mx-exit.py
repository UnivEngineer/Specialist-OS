"""Almaz2MX: выход через клавиатуру и три полных сброса с проверкой bootstrap.

Штатный запуск использует текущий Games2M; --before требует образ из before-manifest.txt.
"""
from pathlib import Path
import runpy,re,json,argparse
R=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--before',action='store_true');a=p.parse_args()
O=R/'docs/compat/almaz2-mx-exit-2026-11-05'/('fixed' if not a.before else 'before');O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001';m=h['Matrix']()
def syms(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(),re.M)}
s=syms('DOS.sym');n=syms('NC.sym');game=syms('Almaz2-mx-optimized.sym');report={}
def tap(key):
 z=m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not z.get('stoppedByBreakpoint'):m.m.call('emu_run_for',ms=120)
def loaded_loop(pc,prefix,limit=120):
 for _ in range(limit):
  state=m.m.call('cpu_state')
  if state['registers']['pc']['dec']==pc:
   if m.m.read(0,len(prefix))==prefix:return
   m.m.call('bp_clear');m.m.call('emu_run_for',ms=20);m.m.stop_at(pc)
  m.m.call('emu_run_for',ms=500)
 raise AssertionError(('loop timeout',hex(pc),m.m.call('cpu_state')))
try:
 m.start('almaz-mx-exit',disk=R/'bin/nested-Games2M.bin')
 original=m.m.read(0xC800,234)
 command='A:/RUN.COM -MX H:/Almaz-2/MXOS/Almaz2MX.exe'
 tap('esc')
 keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}
 for ch in command:tap(keys.get(ch,ch.lower()))
 assert m.m.read(s['CMD_LINE'],len(command)).upper()==command.upper().encode()
 m.m.screenshot(O/'command.png');tap('enter');m.m.stop_at(game['game_frame'])
 prefix=(R/('assets/games/Almaz-2/Almaz2-mx-optimized.cpu' if not a.before else 'assets/games/originals/almaz2/Almaz2-mx-optimized-before-dosmod-2026-11-05.cpu.bin')).read_bytes()[:32]
 loaded_loop(game['game_frame'],prefix)
 report['moduleReadyInGame']=m.m.read(s['v_dosModuleReady'],1)[0]
 report['moduleHeaderInGame']=m.m.read(0x8000,16).hex()
 report['residentAbiSameInGame']=m.m.read(0xC800,234)==original
 m.m.screenshot(O/'game.png')
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=500)
 m.m.stop_at(game['audio_game_exit'])
 m.m.call('kbd_send',keys=['clear'],action='press')
 m.m.until(game['audio_game_exit'],20)
 m.m.call('kbd_send',keys=['clear'],action='release')
 report['exitKeyAccepted']=True
 m.m.stop_at(n['mainLoop'])
 returned=False
 for _ in range(30):
  state=m.m.call('cpu_state')
  if state['registers']['pc']['dec']==n['mainLoop'] and m.m.read(0,32)==(R/'bin/NC.i80').read_bytes()[:32]:returned=True;break
  m.m.call('emu_run_for',ms=500)
 report['returnedToNc']=returned
 report['afterExit']=m.m.call('cpu_state')
 report['residentAbiSameAfterExit']=m.m.read(0xC800,234)==original
 m.m.screenshot(O/'after-exit.png')
 if not a.before:assert returned,report
 # Сброс платформы должен заново скопировать DOS из bootstrap, до первого C000h.
 dos=(R/'bin/DOS.sys').read_bytes();report['resets']=[]
 for i in range(3):
  m.m.call('bp_clear');m.m.call('emu_reset');m.m.stop_at(0xC000);m.m.until(0xC000,30)
  equal=m.m.read(0xC000,len(dos))==dos;assert equal,('DOS not restored by reset',i)
  m.m.stop_at(n['mainLoop']);loaded_loop(n['mainLoop'],(R/'bin/NC.i80').read_bytes()[:32])
  report['resets'].append(dict(fullDosRestored=equal,nc=True))
  m.m.call('bp_clear');m.m.call('emu_run_for',ms=500)
 m.m.screenshot(O/'after-reset.png')
 (O/'result.json').write_text(json.dumps(report,indent=2)+'\n')
 print(json.dumps(report,indent=2),flush=True)
except Exception:
 (O/'failure.json').write_text(json.dumps(report,indent=2)+'\n')
 if m.pid:m.m.screenshot(O/'failure.png')
 raise
finally:m.stop()

