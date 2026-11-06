"""Запуск COPYBEN через NC и проверка сохранности страниц, хвостов и окна."""
from pathlib import Path
import runpy,re,json
ROOT=Path(__file__).resolve().parents[2]
import argparse
p=argparse.ArgumentParser();p.add_argument('--report',type=Path,default=ROOT/'docs/bench/copyben-rom-002');p.add_argument('--runtime',type=Path,default=ROOT/'.local/monitor2-001');args=p.parse_args()
out=args.report;out.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=out;g['RUNTIME']=args.runtime
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',(ROOT/'bin/COPYBEN.sym').read_text(),re.M)}
x=h['Matrix']()
try:
 x.start('copyben-native');m=x.m
 def tap(k):
  r=m.call('kbd_send',keys=[k],action='tap',duration_ms=100)
  if not r.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
 m.call('bp_clear');m.call('emu_run_for',ms=500);tap('esc');m.call('emu_run_for',ms=250)
 for c in 'copyben':tap(c)
 m.screenshot(out/'command.png')
 nc={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',(ROOT/'bin/NC.sym').read_text(),re.M)}
 assert m.read(nc['vars.cmdLine'],7).lower()==b'copyben', 'Unexpected keyboard input; refusing to execute'
 m.stop_at(s['entry']);tap('enter');m.until(s['entry'],20)
 oldwin=m.read(0xffc0,32);oldmain=m.read(0xff00,192)
 page=m.read(0x8ff8,1)[0]-1
 m.write(0xfffd,bytes([page]));oldbank=m.read(0xff00,192);olddata=m.read(0x3fff,8194);m.write(0xfffc,b'\0')
 if page>=1:
  m.write(0xfffd,bytes([page-1]));otherbank=m.read(0xff00,192);otherdata=m.read(0x3fff,8194);m.write(0xfffc,b'\0')
 m.stop_at(s['restored']);m.until(s['restored'],180)
 assert m.read(0xffc0,32)==oldwin
 assert m.read(0xff00,192)==oldmain
 r={k:int.from_bytes(m.read(s[k],n),'little') for k,n in [('directTicks',4),('blockTicks',4),('byteTicks',4),('byteOK',1),('directOK',1),('blockOK',1),('timerError',1),('ramErrors',1),('engineErrors',1)]}
 assert r['directOK']==r['blockOK']==r['byteOK']==r['timerError']==r['ramErrors']==r['engineErrors']==0,r
 m.write(0xfffd,bytes([page]));assert m.read(0xff00,192)==oldbank;assert m.read(0x3fff,8194)==olddata;m.write(0xfffc,b'\0')
 if page>=1:
  m.write(0xfffd,bytes([page-1]));assert m.read(0xff00,192)==otherbank;assert m.read(0x3fff,8194)==otherdata;m.write(0xfffc,b'\0')
 m.stop_at(s['waitKey']);m.until(s['waitKey'],10);m.screenshot(out/'results.png')
 r['engineTicks']=[int.from_bytes(m.read(s['engineTimings']+4*i,4),'little') for i in range(12)]
 (out/'result.json').write_text(json.dumps(r,indent=2));print(r)
 m.call('bp_clear');tap('space');m.call('emu_run_for',ms=2000)
finally:x.stop()
