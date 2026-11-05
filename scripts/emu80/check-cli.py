"""Самостоятельный CLI и NC F7 через обычный клавиатурный ввод Emu80."""
from pathlib import Path
import runpy,re,json
R=Path(__file__).resolve().parents[2]
O=R/'docs/cli/cli-001/emulator';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=O;g['RUNTIME']=R/'.local/monitor2-001'
m=h['Matrix']();s=h['symbols'](R/'bin/DOS.sym');c=h['symbols'](R/'bin/CLI.sym');n={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text(),re.M)}
def tap(key):
 result=m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not result.get('stoppedByBreakpoint'):m.m.call('emu_run_for',ms=120)
def type_text(text):
 keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus',';':'semicolon'}
 for ch in text:tap(keys.get(ch,ch.lower()))
 assert m.m.read(s['CMD_LINE'],len(text)).upper()==text.upper().encode(),repr(m.m.read(s['CMD_LINE'],128))
def enter(text):
 type_text(text);m.m.stop_at(c['cliPrompt']);m.m.call('bp_set',addr=c['cliError'],type='exec');tap('enter')
 for _ in range(120):
  pc=m.m.call('cpu_state')['registers']['pc']['dec']
  assert pc!=c['cliError'],(text,m.m.read(s['v_dosError'],1).hex())
  if pc==c['cliPrompt']:break
  m.m.call('emu_run_for',ms=1000)
 else:raise AssertionError(('command timeout',text))
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=500)
def cwd(want):
 got=m.m.read(c['cliPromptPath'],128).split(b'\0')[0].decode();assert got.upper()==want.upper(),(got,want)
 print('CLI PATH',got,flush=True)
try:
 m.start('cli',disk=R/'bin/nested-Games2M.bin')
 m.m.call('bp_set',addr=c['cliPrompt'],type='exec');tap('f10');m.m.until(c['cliPrompt'],40)
 assert m.m.call('cpu_state')['registers']['pc']['dec']==c['cliPrompt']
 assert m.m.read(0,len((R/'bin/CLI.i80').read_bytes()))==(R/'bin/CLI.i80').read_bytes()
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=500);cwd('A:\\')
 enter('B:');cwd('B:\\')
 # Вставка, Backspace, Home/End и горизонтальное окно не портят строку.
 type_text('ECHO ABCD');tap('left');tap('backspace');tap('x')
 assert m.m.read(s['CMD_LINE'],128).split(b'\0')[0].upper()==b'ECHO ABXD'
 tap('esc')
 type_text('ECHO '+('X'*110));tap('home');tap('right');tap('end');tap('backspace')
 assert len(m.m.read(s['CMD_LINE'],128).split(b'\0')[0])==114
 m.m.call('emu_run_for',ms=1500)
 assert len(m.m.read(s['CMD_LINE'],128).split(b'\0')[0])==114
 tap('esc')
 enter('MD TESTDIR');enter('CD TESTDIR');cwd('B:\\TESTDIR')
 enter('MKDIR INNER');enter('CD INNER');cwd('B:\\TESTDIR\\INNER')
 enter('CD /');cwd('B:\\')
 enter('COPY A:/README.TXT B:/TESTDIR/SHOW.TXT');cwd('B:\\')
 enter('TYPE B:/TESTDIR/SHOW.TXT')
 enter('REN B:/TESTDIR/SHOW.TXT B:/TESTDIR/RENAMED.TXT')
 enter('MOVE B:/TESTDIR/RENAMED.TXT B:/MOVED.TXT')
 enter('DIR');m.m.screenshot(O/'cli-dir.png')
 enter('DEL B:/MOVED.TXT');enter('RD TESTDIR/INNER');enter('RD TESTDIR')
 enter('PATH B:/MISSING;A:/');assert m.m.read(s['v_cliPath'],128).split(b'\0')[0].upper()==b'B:/MISSING;A:/'
 # Голая COM в A:\ находится из B:, затем возврат происходит в тот же CLI/CWD.
 ck=h['symbols'](R/'bin/CHKDSK.sym')
 type_text('CHKDSK B:');m.m.stop_at(ck['waitKey']);tap('enter');m.m.until(ck['waitKey'],40)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=500);m.m.stop_at(s['reboot3_2']);tap('enter');m.m.until(s['reboot3_2'],20)
 m.m.call('bp_clear');m.m.stop_at(c['cliPrompt']);m.m.until(c['cliPrompt'],40)
 assert m.m.read(0,len((R/'bin/CLI.i80').read_bytes()))==(R/'bin/CLI.i80').read_bytes()
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=500);cwd('B:\\')
 assert m.m.read(s['v_cliPath'],128).split(b'\0')[0].upper()==b'B:/MISSING;A:/'
 type_text('EXIT');m.m.call('bp_set',addr=n['mainLoop'],type='exec');tap('enter');m.m.until(n['mainLoop'],40)
 assert m.m.call('cpu_state')['registers']['pc']['dec']==n['mainLoop']
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=500)
 # NC использует тот же MKDIR из F7. Выбираем B: через общую команду.
 def toolbar():return b''.join(m.m.read(0x9000+256*x+247,9) for x in range(48))
 toolbar_before=toolbar()
 type_text('B:');m.m.call('bp_set',addr=s['clearScreen'],type='exec');tap('enter');m.m.call('emu_run_for',ms=1500)
 assert m.m.call('cpu_state')['registers']['pc']['dec']!=s['clearScreen'],'quiet command cleared screen'
 assert toolbar()==toolbar_before,'quiet command changed toolbar'
 m.m.call('bp_clear');tap('f7');type_text_marker='NCDIX'
 # Диалог имеет собственный буфер, а не CMD_LINE.
 for ch in type_text_marker:tap(ch.lower())
 tap('backspace');tap('r');tap('left');tap('right')
 assert m.m.read(n['vars.input'],128).split(b'\0')[0].upper()==b'NCDIR'
 m.m.screenshot(O/'nc-mkdir-dialog.png')
 tap('enter');m.m.call('emu_run_for',ms=1200)
 assert b'NCDIR' in m.m.read(n['FILE_CACHE_A'],n['FILE_CACHE_BYTES']).upper() or b'NCDIR' in m.m.read(n['FILE_CACHE_B'],n['FILE_CACHE_BYTES']).upper()
 m.m.screenshot(O/'nc-mkdir.png')
 (O/'result.json').write_text(json.dumps(dict(standaloneCli=True,commands=True,path=True,returnAfterCom=True,ncMkdir=True),indent=2)+'\n')
 print('CLI UI PASS',flush=True)
except Exception:
 m.m.screenshot(O/'failure.png');print(m.m.call('cpu_state'),flush=True);raise
finally:m.stop()
