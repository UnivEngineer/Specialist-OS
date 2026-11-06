"""Штатный bootstrap: BIOS в main, DOS в банке 0; холодный и тёплый сброс."""
from pathlib import Path
import runpy, json, argparse
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--normal',action='store_true',help='Сохранён для прежних команд; сборка всегда штатная');p.add_argument('--report',type=Path,default=ROOT/'docs/banked/bootstrap-current');a=p.parse_args();a.report.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['RUNTIME']=ROOT/'.local/monitor2-001';g['OUT']=a.report.resolve()
x=h['Matrix']();m=x.m;s=h['symbols'](ROOT/'bin/bootstrap/BOOT.sym');b=h['symbols'](ROOT/'bin/BIOS.sym');k=h['symbols'](ROOT/'bin/banked/KERNEL.sym');n=h['symbols'](ROOT/'bin/NC.sym');rows=[]
def page(value):m.write(0xFFFD if value else 0xFFFC,bytes([value-1 if value else 0]))
def nc():
 for _ in range(20):
  m.stop_at(n['mainLoop']);state=m.until(n['mainLoop'],120)
  if m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]:return state
  m.call('debug_step',mode='into')
 raise AssertionError('NC was not restored')
try:
 x.start('bootstrap')
 m.call('debug_break');page(1);m.write(k['v_dosModuleReady'],b'\0');page(0)
 for mode in ('cold','warm'):
  m.stop_at(s['stage'])
  m.call('emu_reset')
  m.until(s['stage'],40);ticks={}
  for label in ('stage','biosLoaded','dosLoaded','bootDone'):
   m.stop_at(s[label]);state=m.until(s[label],40);ticks[label]=state['cpu_ticks']
   if label=='biosLoaded':
    image=(ROOT/'bin/BIOS.sys').read_bytes();assert m.read(0xC000,len(image))==image
   if label=='dosLoaded':
    page(1)
    if mode=='cold':
     image=(ROOT/'bin/DOS.sys').read_bytes();assert m.read(0x4000,len(image))==image
    else:
     assert m.read(0xA800,17)==b'DOS survives warm'
     page(2);assert m.read(0xE000,17)==b'RAM disk survives'
    page(0)
  state=nc();ticks['nc']=state['cpu_ticks'];assert m.read(b['v_memTop'],2)==bytes.fromhex('ff8b')
  page(1);assert m.read(k['v_dosModuleReady'],1)==b'\1';m.write(0xA800,b'DOS survives warm')
  page(2)
  if mode!='cold':assert m.read(0xE000,17)==b'RAM disk survives'
  m.write(0xE000,b'RAM disk survives');page(0)
  row=dict(mode=mode,ticks=ticks,biosMs=(ticks['biosLoaded']-ticks['stage'])/2000,dosAndProbeMs=(ticks['dosLoaded']-ticks['biosLoaded'])/2000,bootMs=(ticks['nc']-ticks['stage'])/2000,dosReloaded=mode=='cold');rows.append(row);print(row,flush=True);m.screenshot(a.report/(mode+'.png'))
 # Программные входы сохраняют BIOS; выбор оболочки принадлежит DOS.
 cli=h['symbols'](ROOT/'bin/CLI.sym')
 t=h['symbols'](ROOT/'bin/banked/TRANSPORT.sym')
 marker=b['biosVersion']+5;original=m.read(marker,1);m.write(marker,b'9')
 driver=m.read(0xF600,16)
 for shell in ('NC','CLI'):
  if shell=='CLI':
   m.call('bp_clear');m.call('emu_run_for',ms=500)
   m.stop_at(cli['cliPrompt']);m.tap('f10');m.until(cli['cliPrompt'],120)
  target=n['mainLoop'] if shell=='NC' else cli['cliPrompt']
  binary=(ROOT/f'bin/{shell}.i80').read_bytes()[:32]
  for entry,std in ((0xC000,False),(0xC800,False),(0xC800,True)):
   # Изолированная проверка ABI; возврат реального COM проверяет system-019.
   m.stop_at(target);start=m.call('cpu_state')['cpu_ticks']
   if std:m.write(0xFFFF,b'\0')
   m.call('regs_set',pc=entry,sp=0x8FDF);state=m.until(target,120)
   assert m.read(0,32)==binary
   assert m.read(marker,1)==b'9','Software restart reloaded BIOS'
   assert m.read(0xF600,16)==driver
   page(1);assert m.read(k['v_shell'],1)==bytes([shell=='CLI'])
   assert m.read(0xA800,17)==b'DOS survives warm'
   assert m.read(t['bankDepth'],1)==b'\1'
   page(2);assert m.read(0xE000,17)==b'RAM disk survives';page(0)
   row=dict(mode=f'{entry:04X}',std=std,shell=shell,biosReloaded=False,dosReloaded=False,returnMs=(state['cpu_ticks']-start)/2000)
   rows.append(row);print(row,flush=True)
  m.screenshot(a.report/('software-'+shell+'.png'))
 # Reset обязан восстановить даже повреждённый драйвер main, сохранив CLI.
 m.write(0xF600,b'\x76');m.stop_at(cli['cliPrompt']);m.call('emu_reset');m.until(cli['cliPrompt'],120)
 assert m.read(0,32)==(ROOT/'bin/CLI.i80').read_bytes()[:32]
 assert m.read(marker,1)==original and m.read(0xF600,16)==driver
 page(1);assert m.read(k['v_shell'],1)==b'\1';assert m.read(0xA800,17)==b'DOS survives warm'
 page(2);assert m.read(0xE000,17)==b'RAM disk survives';page(0)
 rows.append(dict(mode='reset-corrupt-main',shell='CLI',biosRestored=True,driverRestored=True,dosReloaded=False))
 print('RESET RESTORED BIOS/DRIVER; CLI/DOS/B: PRESERVED',flush=True)
 (a.report/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
finally:x.stop()
