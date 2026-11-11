"""Полная матрица DISKBEN через штатный ввод NC, только отдельный Emu80."""
import argparse,hashlib,json,runpy,struct,time,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]

def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--runtime',type=Path,required=True)
 p.add_argument('--output',type=Path,default=ROOT/'docs/diskbench/matrix-2026-11-09')
 p.add_argument('--rom-image',type=Path,required=True)
 p.add_argument('--resume',action='store_true')
 p.add_argument('--rom-nc-binary',type=Path,default=ROOT/'bin/NC.i80')
 p.add_argument('--rom-nc-symbols',type=Path,default=ROOT/'bin/NC.sym')
 a=p.parse_args();runtime=a.runtime.resolve();out=a.output.resolve();out.mkdir(parents=True,exist_ok=True)
 assert runtime.is_relative_to(ROOT/'.local') or runtime.name=='Emu80-flash-test'
 for source,target in [(a.rom_nc_binary,out/'rom-NC.i80'),(a.rom_nc_symbols,out/'rom-NC.sym')]:
  if not a.resume or not target.exists():
   if source.resolve()!=target.resolve():shutil.copyfile(source,target)
 g=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));C=g['Matrix'];C.__init__.__globals__.update(OUT=out,RUNTIME=runtime)
 x=C();m=x.m;s=x.s;nc=g['symbols'](ROOT/'bin/NC.sym');rows=[]
 if a.resume and (out/'matrix.json').exists():rows=json.loads((out/'matrix.json').read_text())['rows']
 def save():
  (out/'matrix.json').write_text(json.dumps(dict(cpuHz=2000000,pitHz=2000000,fileBytes=8192,passes=3,bufferBytes=256,units='KiB/s',rows=rows),indent=2)+'\n')
 def trap(targets):
  m.call('bp_clear')
  for pc in targets:m.call('bp_set',addr=pc,type='exec')
 def until(target,seconds=300,errors=()):
  for _ in range(seconds):
   st=m.call('cpu_state');pc=st['registers']['pc']['dec']
   if pc in (target,*errors) and m.read(x.pageAddress,1)==b'\0':
    if pc==target:return st
    m.screenshot(out/'failure.png');raise RuntimeError(('DISKBEN failure',st,m.read(s['bankModuleRegs'],8).hex()))
   m.call('emu_run_for',ms=1000)
  m.screenshot(out/'timeout.png');raise RuntimeError(('timeout',m.call('cpu_state')))
 def type_command(command,key):
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
  keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}
  for c in command:m.tap(keys.get(c,c.lower()))
  assert m.read(nc['CMD_LINE'],len(command)).decode('ascii').lower()==command.lower()
  m.screenshot(out/(key+'-command.png'))
 def resume_nc():
  trap([nc['mainLoop']]);m.until(nc['mainLoop'],120,page=0)
  m.call('bp_clear');m.call('emu_run_for',ms=500)
 def install(name):
  type_command('a:/drivers/'+name+'.com','install-'+name)
  trap([nc['mainLoop']]);m.tap('enter');m.until(nc['mainLoop'],120,page=0)
  m.call('bp_clear');m.call('emu_run_for',ms=500)
 def bench(drive,driver,readonly=False):
  key=drive+'-'+driver
  if len([r for r in rows if r['drive']==drive.upper() and r['driver']==driver])==3:
   print('SKIP',key,flush=True);return
  print('RUN',key,flush=True)
  type_command('a:/bench/diskben/diskben.com '+drive+': -a',key)
  errors=[s['error'],s['bank_module_error']]
  first=s['exit_wait'] if readonly else s['menu_verify']
  trap([first,*errors]);m.tap('enter');until(first,errors=errors)
  if not readonly:
   for label,value in [('menu_writes','1'),('menu_size','3'),('exit_wait','8')]:
    trap([s[label],*errors]);m.line(value);until(s[label],errors=errors)
  block=m.read(s['batchStats'],3*60);m.screenshot(out/(key+'-results.png'))
  assert m.read(s['argBank'],1)==b'\0' # все три размещения завершены
  for i,context in enumerate(['MAIN','DOS','BANK']):
   raw=block[i*60:(i+1)*60];metric=[struct.unpack_from('<IIH',raw,n*10) for n in range(4)]
   verify=struct.unpack_from('<I',raw,44)[0]
   assert all(v[0]>0 for n,v in enumerate(metric) if not readonly or n in (1,3)),(key,context,metric)
   if readonly:assert metric[0][0]==metric[2][0]==verify==0
   else:assert verify>0,(key,context)
   ticks=dict(rawRead=metric[1][0],rawWrite=metric[0][0],dosRead=metric[3][0],dosWrite=metric[2][0],dosWriteVerify=metric[2][0]+verify)
   highWrite,highRead=struct.unpack_from('<II',raw,48)
   if highRead:ticks['rawReadHigh']=highRead
   if highWrite:
    ticks['rawWriteHigh']=highWrite
    highVerify=struct.unpack_from('<I',raw,56)[0]
    if not readonly:assert highVerify>0,(key,context,'HIGH verification')
    ticks['rawWriteHighVerify']=highWrite+highVerify
   rates={k:(None if readonly and k in ('rawWrite','dosWrite','dosWriteVerify') else (96*1953*100//v)/100) for k,v in ticks.items() if v or readonly}
   rows.append(dict(drive=drive.upper(),driver=driver,context=context,ticks=ticks,rates=rates,verifyTicks=verify,rawWriteFlush=metric[0][1:],dosWriteFlush=metric[2][1:]))
  save();print('DONE',key,flush=True)
  trap([s['finished']]);m.call('emu_run_for',ms=500);m.tap('x');until(s['finished'],120)
  resume_nc()
 try:
  if len(rows)<12:
   x.start('flash-matrix',system=ROOT/'images/SystemRom_emu80_writable.bin')
   bench('h','FlashFst')
   bench('a','FlashSys')
   bench('b','RAMDisk')
   install('AT2MX');bench('h','Flash')
   x.stop()
  nc=g['symbols'](out/'rom-NC.sym')
  x.start('rom-matrix',system=a.rom_image.resolve(),locked=True,nc_binary=out/'rom-NC.i80',nc_symbols=out/'rom-NC.sym')
  image=runtime/'specialist-os/system-flash.bin';before=hashlib.sha256(image.read_bytes()).hexdigest()
  bench('a','ROM',True)
  assert hashlib.sha256(image.read_bytes()).hexdigest()==before,'ROM image was modified'
  print('PASS: 15 context/driver rows; ROM image unchanged',flush=True)
 finally:x.stop();save()

if __name__=='__main__':main()
