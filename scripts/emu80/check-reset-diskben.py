"""Аппаратный Reset из меню DISKBEN, запуск через штатную строку NC."""
from pathlib import Path
import runpy,json,argparse
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'docs/font-arena-044/reset';OUT.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=OUT,RUNTIME=ROOT/'.local/monitor2-001')
x=h['Matrix']();m=x.m;n=h['symbols'](ROOT/'bin/NC.sym');k=h['symbols'](ROOT/'bin/banked/KERNEL.sym');rows=[]
p=argparse.ArgumentParser();p.add_argument('--after-write-only',action='store_true');args=p.parse_args()
def launch():
 m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc')
 for c in 'diskben':m.tap(c)
 m.stop_at(x.s['entry']);m.tap('enter');m.until(x.s['entry'],120,page=0)
 m.screenshot(OUT/'launch-command.png')
 m.stop_at(x.s['menu_drive']);m.until(x.s['menu_drive'],120,page=0)
def answer(text,target):
 m.call('bp_clear');m.call('emu_run_for',ms=500)
 for c in text:m.tap(c)
 m.stop_at(x.s[target]);m.tap('enter');m.until(x.s[target],120,page=0)
try:
 x.start('reset-diskben')
 cases=[('A-after-write','a',[('0','menu_writes'),('3','menu_size'),('8','exit_wait')]),('B-after-write','b',[('0','menu_writes'),('3','menu_size'),('8','exit_wait')])] if args.after_write_only else [('drive',None,[]),('A-verify','a',[]),('A-writes','a',[('0','menu_writes')]),('A-size','a',[('0','menu_writes'),('3','menu_size')]),('H-verify','h',[]),('B-size','b',[('0','menu_writes'),('3','menu_size')])]
 for phase,drive,answers in cases:
  launch()
  if drive:answer(drive,'menu_verify')
  for text,target in answers:answer(text,target)
  m.screenshot(OUT/(phase+'.png'));m.call('bp_clear');m.call('emu_reset')
  m.stop_at(n['mainLoop']);m.call('bp_set',addr=k['badCommand'],type='exec')
  for i in range(120):
   m.call('emu_run_for',ms=500);cpu=m.call('cpu_state');pc=cpu['registers']['pc']['dec'];page=m.read(k['bankPage'],1)[0]
   if pc==k['badCommand'] and page==1:break
   if pc==n['mainLoop'] and page==0 and m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]:break
  m.screenshot(OUT/(phase+'-reset.png'));row=dict(phase=phase,cpu=cpu)
  if pc==k['badCommand']:
   row['cmd']=m.read(k['CMD_LINE'],130).hex();row['cliLine']=m.read(k['cliLine'],128).hex();row['error']=m.read(k['v_dosError'],1).hex()
  rows.append(row);(OUT/('after-write.json' if args.after_write_only else 'result.json')).write_text(json.dumps(rows,indent=2))
  assert pc==n['mainLoop'] and m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32],row
  print('RESET DISKBEN PASS',phase,flush=True)
finally:x.stop()
