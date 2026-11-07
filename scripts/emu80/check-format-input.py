"""Клавиатурная проверка FORMAT и модальных окон NC в собственном Emu80."""
from pathlib import Path
import runpy,re,json
R=Path(__file__).resolve().parents[2];O=R/'docs/format/input-052';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001')
x=h['Matrix']();m=x.m
s=lambda f:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/f).read_text(),re.M)}
f=s('bin/FORMAT.sym');c=s('bin/CLI.sym');n=s('bin/NC.sym');rows=[]
def ready():m.call('bp_clear');m.call('emu_run_for',ms=500)
def text(t):
 for ch in t:m.tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(ch,ch.lower()))
def cmd(t,target):
 ready();m.tap('esc');text(t);m.stop_at(target);m.tap('enter');m.until(target,120,page=0)
def still(label):
 ready();before=m.read(0x9000,12288);m.screenshot(O/(label+'.png'));m.call('emu_run_for',ms=2000);assert m.read(0x9000,12288)==before,label
 rows.append(dict(case=label,bitmapStable=True))
try:
 x.start('after')
 # Модальные окна вызываются обычными командами и Shift+F1.
 m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=250);m.stop_at(n['ncModalGetch']);m.tap('f1');m.until(n['ncModalGetch'],120,page=0);m.call('kbd_send',keys=['shift'],action='release');still('tape-no-cursor');m.stop_at(n['mainLoop']);m.tap('enter');m.until(n['mainLoop'],120,page=0)
 cmd('copy A:/README.TXT B:/README.TXT',n['mainLoop'])
 cmd('copy A:/README.TXT B:/README.TXT',n['ncOverwriteKey']);still('overwrite-no-cursor')
 m.stop_at(n['ncOverwriteKey']);m.tap('right');m.until(n['ncOverwriteKey'],120,page=0);still('overwrite-no-to-all')
 m.stop_at(n['mainLoop']);m.tap('esc');m.until(n['mainLoop'],120,page=0)
 ready();x.console();cmd('format h',c['cliPrompt']);m.screenshot(O/'invalid-cli.png');rows.append(dict(case='invalid-cli',message='Invalid arguments'))
 cmd('format',f['lineEditLoop']);ready();m.tap('a');assert m.read(f['v_Answer'],2).lower()==b'a\0';m.screenshot(O/'drive-before-enter.png')
 m.stop_at(f['errSystemDrive']);m.tap('enter');m.until(f['errSystemDrive'],120,page=0);m.stop_at(c['cliPrompt']);m.until(c['cliPrompt'],120,page=0);m.screenshot(O/'system-drive-error.png');rows.append(dict(case='interactive-a',requiresEnter=True))
 cmd('format',f['lineEditLoop']);ready();text('b:');m.stop_at(f['ConfirmRequest']);m.tap('enter');m.until(f['ConfirmRequest'],120,page=0);m.stop_at(f['lineEditLoop']);m.until(f['lineEditLoop'],120,page=0);ready();m.tap('n');m.screenshot(O/'confirm-before-enter.png');m.stop_at(c['cliPrompt']);m.tap('enter');m.until(c['cliPrompt'],120,page=0);rows.append(dict(case='confirmation-n-enter',aborted=True))
 cmd('format',f['lineEditLoop']);ready();m.stop_at(c['cliPrompt']);m.tap('esc');m.until(c['cliPrompt'],120,page=0);rows.append(dict(case='interactive-escape',aborted=True))
 (O/'checks.json').write_text(json.dumps(rows,indent=2)+'\n');print(rows,flush=True)
finally:x.stop()
