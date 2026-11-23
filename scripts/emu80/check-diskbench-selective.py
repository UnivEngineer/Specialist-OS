"""Selective DiskBen RAW/FILE samples plus batch-path and boot-echo checks."""
from pathlib import Path
import argparse,json,runpy,shutil,struct
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--runtime',type=Path,required=True);p.add_argument('--port',type=int,default=19782);p.add_argument('--profile-only',action='store_true');p.add_argument('--report',type=Path,default=R/'docs/diskbench/selective-2026-11-13');a=p.parse_args();a.report.mkdir(parents=True,exist_ok=True)
f=a.report/'input';f.mkdir(exist_ok=True)
for name in ('NC.i80','NC.sym','BIOS.sys','DOS.sys','DOS.sym','DISKBEN.i80','DISKMAIN.i80'):
 shutil.copy2(R/'bin'/name,f/name)
for name in ('DISKBEN','DISKMAIN'):shutil.copy2(R/f'source-apps/diskbench/build/{name}.sym',f/f'{name}.sym')
shutil.copy2(R/'bin/banked/KERNEL.sym',f/'KERNEL.sym');shutil.copy2(R/'images/C-Flash-512k-MMX-System-Emu80.bin',f/'system.bin')
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g.update(OUT=a.report,RUNTIME=a.runtime.resolve());g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp';sy=h['symbols'];ns=sy(f/'NC.sym');cs=sy(f/'DISKBEN.sym');ws=sy(f/'DISKMAIN.sym');ks=sy(f/'KERNEL.sym');rows=[];traces=[];phases=[];lookups=[]
def word(address):return int.from_bytes(m.read(address,2),'little')
def trap(*pcs):
 m.call('bp_clear')
 for pc in pcs:m.call('bp_set',addr=pc,type='exec')
 assert len(m.call('bp_list')['breakpoints'])==len(set(pcs))
def wait(pc,seconds=150,page=0):
 for _ in range(seconds):
  state=m.call('cpu_state');at=state['registers']['pc']['dec'];bank=m.read(x.pageAddress,1)[0]
  if at==pc and (page is None or bank==page):return state
  if bank==0 and at in (cs['error'],cs['bank_module_error']):raise RuntimeError(state)
  m.call('emu_run_for',ms=1000)
 raise RuntimeError(state)
def command(text):
 m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
 keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}
 for char in text:m.tap(keys.get(char,char))
 assert m.read(ns['CMD_LINE'],len(text)).lower()==text.encode()
def answer(menu,text,next_menu):
 wait(cs[menu]);m.call('bp_clear');m.call('emu_run_for',ms=500);trap(cs[next_menu],cs['error'],cs['bank_module_error']);m.line(text)
def trace_batch(drive):
 active=None;phase=None;lookup=None
 trap(ws['result'],ks['streamBatchStart'],ks['sbExecute'],ks['sbExecuted'],ws['begin'],ws['fileTransfer'],ws['bhClose'],ws['timer_sample'],ks['fileFindStart'],ks['fileFind_end'])
 for _ in range(400):
  state=m.call('cpu_state');pc=state['registers']['pc']['dec'];page=m.read(x.pageAddress,1)[0];tick=state['cpu_ticks']
  if page==0 and pc==ws['result']:return
  if page==0 and pc in (cs['error'],cs['bank_module_error']):raise RuntimeError(state)
  if page==0:
   if pc==ws['begin'] and m.read(ws['raw'],1)==b'\0':
    phase=dict(operation=m.read(ws['operation'],1)[0],begin=tick)
   elif pc==ws['fileTransfer'] and phase is not None:phase['transfer']=tick
   elif pc in (ws['flush'],ws['bhClose']) and phase is not None and 'transfer' in phase and 'dataEnd' not in phase:phase['dataEnd']=tick
   elif pc==ws['timer_sample'] and phase is not None:
    phase['end']=tick;phases.append(phase);phase=None
  if page==1 and m.read(ks['v_drive'],1)==bytes([drive]):
   if pc==ks['fileFindStart'] and phase is not None:lookup=dict(start=tick,operation=phase['operation'])
   elif pc==ks['fileFind_end'] and lookup is not None:
    lookup['cycles']=tick-lookup['start'];lookups.append(lookup);lookup=None
   if pc==ks['streamBatchStart']:
    active=dict(operation=m.read(ks['v_streamMode'],1)[0],remaining=word(ks['v_streamRemaining']),start=tick,client=m.read(ks['kdPage'],1)[0])
   elif pc==ks['sbExecute'] and active is not None:
    active.update(execute=tick,sectors=word(ks['sbCount']),buffer=word(ks['sbPacket']+2))
   elif pc==ks['sbExecuted'] and active is not None and 'execute' in active:
    active.update(done=tick,prepareCycles=active['execute']-active['start'],ioCycles=tick-active['execute']);traces.append(active);active=None
  if state.get('breaked'):m.call('debug_step',mode='into')
  m.call('emu_run_for',ms=500)
 raise RuntimeError(('batch trace timeout',state))
try:
 x.start('diskbench-selective',system=f/'system.bin',nc_binary=f/'NC.i80',nc_symbols=f/'NC.sym',dos=f/'DOS.sys')
 cases=[('H',1,0,True)] if a.profile_only else [('B',0,0,False),('H',0,0,False),('H',1,0,False),('H',1,1,False),('H',1,0,True)]
 for index,(drive,offset,random,diagnostic) in enumerate(cases):
  if index==0 and drive=='B':
   command('a:/bench/diskben/diskben.com');trap(cs['menu_drive'],cs['menu_executor']);m.tap('enter');wait(cs['menu_drive'])
   m.screenshot(a.report/'first-menu-drive.png');answer('menu_drive','b','menu_executor');answer('menu_executor','main','menu_verify')
  else:
   command('a:/bench/diskben/diskben.com '+drive.lower()+': -c:main -m:all -b:'+('unaligned' if offset else 'aligned'))
   trap(cs['menu_verify'],cs['error'],cs['bank_module_error']);m.tap('enter')
  for menu,value,next_menu in [('menu_verify','0','menu_writes'),('menu_writes','1','menu_size'),('menu_size','8','menu_fragment'),('menu_fragment',str(random),'moduleSelect')]:answer(menu,value,next_menu)
  wait(cs['moduleSelect']);trap(ws['pitTimerInit'],cs['error'],cs['bank_module_error']);m.call('emu_run_for',ms=500);wait(ws['pitTimerInit'])
  assert m.read(ws['config']+10,1)==bytes([offset]) and m.read(ws['config']+1,1)==b'\1'
  if diagnostic:trace_batch(ord(drive)-65)
  else:
   trap(ws['result'],cs['error'],cs['bank_module_error']);m.call('emu_run_for',ms=1000);wait(ws['result'])
  raw=m.read(ws['stats'],60);ticks=[struct.unpack_from('<I',raw,n)[0] for n in (0,10,20,30)];assert all(ticks)
  row=dict(drive=drive,offset=offset,random=random,diagnostic=diagnostic,ticks=ticks,rawWrite=62500/ticks[0],rawRead=62500/ticks[1],fileWrite=62500/ticks[2],fileRead=62500/ticks[3]);rows.append(row)
  trap(cs['exit_wait']);m.call('emu_run_for',ms=500);wait(cs['exit_wait']);m.screenshot(a.report/f'results-{index}.png')
  trap(cs['finished']);m.call('emu_run_for',ms=500);m.tap('x');wait(cs['finished']);trap(ns['mainLoop']);m.call('debug_step',mode='into');m.until(ns['mainLoop'],120,page=0)
  (a.report/'results.json').write_text(json.dumps(dict(rows=rows,traces=traces,phases=phases,lookups=lookups),indent=2)+'\n');print('SAMPLE',row,flush=True)
 # Cold/warm boot command echo: stop after printing each command, before execution.
 trap(ks['batExecute']);m.call('emu_reset')
 for i in range(3):
  wait(ks['batExecute'],page=1);m.screenshot(a.report/f'autoexec-{i}.png');m.call('debug_step',mode='into');m.call('emu_run_for',ms=500)
 trap(ns['mainLoop']);m.call('emu_run_for',ms=500);m.until(ns['mainLoop'],120,page=0)
 print('AUTOEXEC ECHO / DRIVE-FIRST / SELECTIVE RAW+FILE PASS',flush=True)
finally:x.stop()
