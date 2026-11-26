"""DiskBen alignment menu/key: real RAW/FILE, MAIN/DOS/BANK, guard bytes and NC return."""
from pathlib import Path
import argparse,json,runpy,shutil,struct
R=Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime',type=Path,required=True)
p.add_argument('--port',type=int,default=19782)
p.add_argument('--disk',type=Path,default=R/'images/DiskBench-H.bin', help='writable H: fixture with a free root entry')
p.add_argument('--report',type=Path,default=R/'docs/diskbench/h-write-unroll-2026-11-13/alignment-emulator')
a = p.parse_args()
a.report.mkdir(parents=True,exist_ok=True)
if not a.disk.is_file():
 p.error('Build H: with scripts/images/build-image.ps1 Games2M -ManifestFile scripts/emu80/fixtures/DiskBench-H.txt or pass --disk')
f=a.report/'input';f.mkdir(exist_ok=True)
shutil.copy2(a.disk,f/'disk.bin')
for name in ('NC.i80','NC.sym','BIOS.sys','DOS.sys','DOS.sym','DISKBEN.i80','DISKMAIN.i80','DISKDOS.i80','DISKBANK.i80'):
 shutil.copy2(R/'bin'/name,f/name)
for name in ('DISKBEN','DISKMAIN','DISKDOS','DISKBANK'):shutil.copy2(R/f'source-apps/diskbench/build/{name}.sym',f/f'{name}.sym')
# DiskDos needs the interval that the optional floppy service normally occupies.
system=(R/'images/C-Flash-512k-MMX-System-Emu80.bin').read_bytes()
floppy=b"C:\\DRIVERS\\FLOPPY.COM  -LETTER A"
assert system.count(floppy)==1,'Unexpected AUTOEXEC in system image'
(f/'system.bin').write_bytes(system.replace(floppy,b'REM '.ljust(len(floppy))))
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g.update(OUT=a.report,RUNTIME=a.runtime.resolve());g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp';sy=h['symbols'];ns=sy(f/'NC.sym');cs=sy(f/'DISKBEN.sym');ws={name:sy(f/f'DISK{name}.sym') for name in ('MAIN','DOS','BANK')};rows=[]
timer_owner=sy(R/'bin/banked/KERNEL.sym')['timerOwner']
def trap(*pcs):
 m.call('bp_clear')
 for pc in pcs:m.call('bp_set',addr=pc,type='exec')
def wait(pc,seconds=120,page=None):
 for _ in range(seconds):
  state=m.call('cpu_state');at=state['registers']['pc']['dec'];bank=m.read(x.pageAddress,1)[0]
  if at==pc and (page is None or bank==page):return bank
  if at in (cs['error'],cs['bank_module_error'],cs['moduleUnload']) and bank==0:raise RuntimeError(state)
  if state.get('breaked') and at==pc:m.call('debug_step',mode='into')
  m.call('emu_run_for',ms=1000)
 raise RuntimeError(state)
def command(text):
 m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
 keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus','=':'equals'}
 for char in text:m.tap(keys.get(char,char))
 actual=m.read(ns['CMD_LINE'],len(text)).lower()
 if actual!=text.encode():
  m.screenshot(a.report/'command-error.png');raise AssertionError((text,actual))
try:
 for offset,interactive in ((0,False),(1,True)):
  for name,target in (('MAIN',3),('DOS',2),('BANK',1)):
   x.start(f'diskbench-alignment-{name}-{offset}',system=f/'system.bin',disk=f/'disk.bin',nc_binary=f/'NC.i80',nc_symbols=f/'NC.sym',dos=f/'DOS.sys')
   text=f'h:/bench/diskben/diskben.com h: -c:{name.lower()} -m:all'
   if not interactive:text+=' -b:aligned'
   command(text)
   trap(cs['menu_alignment'] if interactive else cs['menu_verify'],cs['error'],cs['bank_module_error'])
   m.tap('enter')
   if interactive:
    wait(cs['menu_alignment'],page=0);m.call('bp_clear');m.call('emu_run_for',ms=500)
    m.screenshot(a.report/f'alignment-menu-{name}.png')
    trap(cs['menu_verify'],cs['error'],cs['bank_module_error']);m.line('1')
   for menu,value,next_menu in [('menu_verify','2','menu_writes'),('menu_writes','1','menu_size'),('menu_size','8','menu_fragment'),('menu_fragment','1','moduleSelect')]:
    wait(cs[menu],page=0);m.call('bp_clear');m.call('emu_run_for',ms=500)
    trap(cs[next_menu],cs['error'],cs['bank_module_error']);m.line(value)
   wait(cs['moduleSelect'],page=0)
   assert m.read(cs['argBank'],1)==bytes([target])
   worker=ws[name]
   trap(worker['pitTimerInitDivisor'],cs['error'],cs['bank_module_error']);m.call('emu_run_for',ms=500)
   bank=wait(worker['pitTimerInitDivisor'])
   assert bank==(0 if name=='MAIN' else 1 if name=='DOS' else bank) and (name!='BANK' or bank>1)
   address=int.from_bytes(m.read(worker['dataBuffer'],2),'little')
   assert address&255==offset,(name,hex(address),offset)
   assert m.read(worker['config']+10,1)==bytes([offset])
   if bank==1:assert m.read(timer_owner,1)!=b'\0','DOS worker has no timer owner'
   m.write(address-1,b'\xa5')
   m.write(address+8192,b'\x5a')
   # A MAIN error address can be hot DOS code. Stop on the worker result
   # or the first controller cleanup after a failed worker call.
   trap(worker['result'],cs['moduleUnload']);m.call('emu_run_for',ms=1000)
   wait(worker['result'],page=bank)
   assert m.read(address-1,1)==b'\xa5' and m.read(address+8192,1)==b'\x5a',(name,offset,'guards')
   stats=m.read(worker['stats'],60)
   ticks=[struct.unpack_from('<I',stats,n)[0] for n in (0,10,20,30)]
   assert all(ticks),(name,offset,stats.hex())
   trap(cs['exit_wait'],cs['error'],cs['bank_module_error']);m.call('emu_run_for',ms=500)
   wait(cs['exit_wait'],page=0)
   m.write(0xfffd,b'\0')
   try:assert m.read(timer_owner,1)==b'\0','timer lease retained on exit'
   finally:m.write(0xfffc,b'\0')
   m.screenshot(a.report/f'results-{name}-{offset}.png')
   trap(cs['finished']);m.call('emu_run_for',ms=500);m.tap('x');wait(cs['finished'],page=0)
   trap(ns['mainLoop']);m.call('debug_step',mode='into');m.until(ns['mainLoop'],120,page=0)
   rows.append(dict(caller=name,offset=offset,buffer=address,page=bank,bytes=8192,random=True,sectorVerify=True,guards=True,ticks=ticks,timerReleased=True,returnedToNC=True))
   print('ALIGNMENT PASS',name,offset,'RAW/FILE, guards, timer released, NC return',flush=True)
 (a.report/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
finally:x.stop()
