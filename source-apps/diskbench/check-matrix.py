"""Current full DiskBen matrix: frozen inputs, normal NC keyboard entry, one emulator per image."""
import argparse,hashlib,json,runpy,struct,shutil,random
from datetime import datetime
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True);p.add_argument('--output',type=Path,required=True);p.add_argument('--port',type=int,default=19782);p.add_argument('--resume',action='store_true');p.add_argument('--only-a',action='store_true');a=p.parse_args();expected_rows=24 if a.only_a else 114
 out=a.output.resolve();out.mkdir(parents=True,exist_ok=True);f=out/'input';f.mkdir(exist_ok=True)
 if not a.resume:
  assert not (out/'matrix.json').exists(),'Use a new output directory or --resume'
  (f/'bin').mkdir(exist_ok=True)
  for binary in (ROOT/'bin').iterdir():
   if binary.is_file() and binary.suffix.lower() in ('.bin','.sys','.sym','.i80','.cpu','.inc','.json'):shutil.copy2(binary,f/'bin'/binary.name)
  for directory in ('banked','bootstrap'):shutil.copytree(ROOT/'bin'/directory,f/'bin'/directory,dirs_exist_ok=True)
  for name in ('DISKBEN','DISKMAIN','DISKDOS','DISKBANK'):shutil.copy2(ROOT/f'source-apps/diskbench/build/{name}.sym',f/f'{name}.sym')
  shutil.copy2(ROOT/'images/SystemRom_emu80_writable.bin',f/'system.bin');shutil.copy2(ROOT/'images/FlashDisk_emu80_seed.bin',f/'h.bin')
  make_rom(f)
  inputs={str(q.relative_to(f)):hashlib.sha256(q.read_bytes()).hexdigest() for q in f.rglob('*') if q.is_file()}
  (out/'sha256.json').write_text(json.dumps(inputs,indent=2)+'\n')
 else:
  for name,digest in json.loads((out/'sha256.json').read_text()).items():assert hashlib.sha256((f/name).read_bytes()).hexdigest()==digest,name
 g=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));C=g['Matrix'];C.__init__.__globals__.update(OUT=out,RUNTIME=a.runtime.resolve())
 C.__init__.__globals__['mcp_owner']=lambda:g['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
 x=C();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp';s=g['symbols'](f/'DISKBEN.sym');nc=g['symbols'](f/'bin/NC.sym');ws=g['symbols'](f/'DISKMAIN.sym');x.s=s;x.pageAddress=g['symbols'](f/'bin/banked/TRANSPORT.sym')['bankPage']
 rows=json.loads((out/'matrix.json').read_text())['rows'] if a.resume and (out/'matrix.json').exists() else []
 def save(complete=False):
  (out/'matrix.json').write_text(json.dumps(dict(date=datetime.now().strftime('%d.%m.%Y'),complete=complete,cpuHz=2000000,pitHz=2000000,fileKiB=8,passes=1,romReadPasses=3,units='KiB/s',rows=rows),indent=2)+'\n')
 def trap(*pcs):
  m.call('bp_clear')
  for pc in set(pcs):m.call('bp_set',addr=pc,type='exec')
 def wait(pc):
  for _ in range(300):
   st=m.call('cpu_state');at=st['registers']['pc']['dec'];page=m.read(x.pageAddress,1)[0]
   controller=page==0 and m.read(0,32)==(f/'bin/DISKBEN.i80').read_bytes()[:32]
   if controller and at==pc:return
   if controller and at in (s['error'],s['bank_module_error']):raise RuntimeError(('DiskBen',st,m.read(s['bankModuleRegs'],8).hex()))
   if st.get('breaked') and not controller:m.call('debug_step',mode='into')
   m.call('emu_run_for',ms=1000)
  raise RuntimeError(('timeout',pc,st))
 def command(text):
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
  keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}
  for c in text:m.tap(keys.get(c,c.lower()))
  assert m.read(nc['CMD_LINE'],len(text)).lower()==text.encode().lower(),text
 def start(key,system,locked=False):
  x.start(key,system=system,disk=f/'h.bin',locked=locked,dos=f/'bin/DOS.sys',bios=f/'bin/BIOS.sys',nc_binary=f/'bin/NC.i80',nc_symbols=f/'bin/NC.sym')
 def install(name):
  command('a:/drivers/'+name.lower()+'.com');trap(nc['mainLoop']);m.tap('enter');m.until(nc['mainLoop'],120,page=0);m.call('bp_clear');m.call('emu_run_for',ms=500)
 def bench(drive,driver,align,frag,check,readonly=False):
  key=f'{drive}-{driver}-b{align}-f{frag}-v{check}'
  if sum(r['key']==key for r in rows)==3:print('SKIP',key,flush=True);return
  print('RUN',key,flush=True)
  command(f'a:/bench/diskben/diskben.com {drive.lower()}: -c:all -m:all -b:'+('unaligned' if align else 'aligned'))
  trap(s['entry'] if readonly else s['menu_verify'],s['error'],s['bank_module_error']);m.tap('enter');wait(s['entry'] if readonly else s['menu_verify'])
  if readonly:
   trap(s['exit_wait'],s['error'],s['bank_module_error']);m.call('emu_run_for',ms=500);wait(s['exit_wait'])
  lbas=[]
  if not readonly:
   for target,value in [('menu_writes',str(check)),('menu_size','1'),('menu_fragment','8')]:
    trap(s[target],s['error'],s['bank_module_error']);m.line(value);wait(s[target])
   trap(ws['result'],s['error'],s['bank_module_error']);m.line(str(frag));wait(ws['result'])
   assert m.read(ws['config']+3,1)==b'\x20' and m.read(ws['config']+10,1)==bytes([align])
   lbas=list(struct.unpack('<32H',m.read(ws['sectors'],64)));assert len(set(lbas))==32
   trap(s['exit_wait'],s['error'],s['bank_module_error']);m.call('emu_run_for',ms=500);wait(s['exit_wait'])
  data=m.read(s['batchStats'],3*88);batch=[]
  for i,context in enumerate(('MAIN','DOS','BANK')):
   part=data[i*88:(i+1)*88];ticks={k:struct.unpack_from('<I',part,n)[0] for k,n in [('rawWrite',0),('rawRead',10),('fileWrite',20),('fileRead',30),('rawVerify',40),('fileVerify',44),('rawWriteHigh',48),('rawReadHigh',52),('rawVerifyHigh',56),('fileWriteHigh',60),('fileReadHigh',70),('fileVerifyHigh',84)]}
   assert all(ticks[k]>0 for k in (('rawRead','fileRead') if readonly else ('rawRead','rawWrite','fileRead','fileWrite'))),(key,context,ticks)
   if readonly:assert ticks['rawWrite']==ticks['fileWrite']==0
   elif check==1:assert ticks['rawVerify'] and ticks['fileVerify']
   else:assert ticks['rawVerify']==ticks['fileVerify']==0
   if drive=='A' and context=='MAIN':assert ticks['rawReadHigh'] and ticks['fileReadHigh'] and (readonly or ticks['rawWriteHigh'] and ticks['fileWriteHigh'])
   effective={k:v for k,v in ticks.items() if 'Verify' not in k}
   if check==1:
    for k,v in [('rawWrite','rawVerify'),('fileWrite','fileVerify'),('rawWriteHigh','rawVerifyHigh'),('fileWriteHigh','fileVerifyHigh')]:effective[k]+=ticks[v]
   numerator=8*7812.5*(3 if readonly else 1)
   rates={k:numerator/v for k,v in effective.items() if v}
   if check==1:
    rates.update({k+'Plain':numerator/ticks[k] for k in ('rawWrite','fileWrite','rawWriteHigh','fileWriteHigh') if ticks[k]})
   flush={k:dict(ticks=struct.unpack_from('<I',part,o+4)[0],times=struct.unpack_from('<H',part,o+8)[0]) for k,o in [('rawWrite',0),('fileWrite',20),('fileWriteHigh',60)]}
   batch.append(dict(key=key,drive=drive,driver=driver,alignment=align,fragment=frag,check=check,context=context,readonly=readonly,ticks=ticks,rates=rates,flush=flush,mainSampleLbas=lbas))
  m.screenshot(out/(key+'.png'));trap(s['finished']);m.call('emu_run_for',ms=500);m.tap('x');wait(s['finished']);trap(nc['mainLoop']);m.call('debug_step',mode='into');m.until(nc['mainLoop'],120,page=0)
  for row in batch:row['returnNC']=True
  rows.extend(batch);save();print('DONE',key,flush=True)
 complete=False
 try:
  start('full-flash',f/'system.bin')
  for drive,driver in [('A','AT512KS'),('B','CPM32'),('H','AT2MXF'),('H','AT2MX')]:
   if a.only_a and drive!='A':continue
   if driver=='AT2MX':install(driver)
   for align in ((0,1) if drive=='H' else (0,)):
    for frag in (0,1):
     for check in (1,2,0):bench(drive,driver,align,frag,check)
  x.stop()
  # DiskBen has deleted all its own files; compare every FAT byte to the seed.
  for name,original in [('system-flash.bin','system.bin'),('flash-disk.bin','h.bin')]:
   assert fat_bytes((a.runtime/'specialist-os'/name).read_bytes())==fat_bytes((f/original).read_bytes()),('FAT not restored',name)
  for frag in (0,1):
   start(f'ROM-{frag}',f/f'rom-f{frag}.bin',True)
   bench('A','ROM-SYS',0,frag,0,True);x.stop()
   assert (a.runtime/'specialist-os/system-flash.bin').read_bytes()==(f/f'rom-f{frag}.bin').read_bytes(),'Read-only ROM changed'
  assert len(rows)==expected_rows,len(rows)
  complete=True;save(True);print('COMPLETE:',len(rows),'rows; all requested contexts/allocation/check modes; A/H FAT restored',flush=True)
 except Exception:
  if x.pid:
   try:m.screenshot(out/'failure.png')
   except Exception:pass
  raise
 finally:x.stop();save(complete)

def fat_bytes(b):
 w=lambda n:struct.unpack_from('<H',b,n)[0];start=w(14)*w(11);return b[start:start+b[16]*w(22)*w(11)]

def make_rom(f):
 for scatter in (0,1):
  b=bytearray((f/'system.bin').read_bytes());w=lambda n:struct.unpack_from('<H',b,n)[0];ss=w(11);fat=w(14)*ss;fs=w(22)*ss;fc=b[16];root=fat+fc*fs;rootn=w(17);data=(root+rootn*32+ss-1)//ss;spc=b[13]
  entry=next(p for p in range(root,root+rootn*32,32) if b[p:p+11]==b'AUTOEXECBAT');payload=b'A:\\DRIVERS\\RAMDISK.COM -P 4\r';off=(data+(w(entry+26)-2)*spc)*ss
  assert len(payload)<=ss;b[off:off+ss]=payload+bytes(ss-len(payload));struct.pack_into('<I',b,entry+28,len(payload))
  count=((w(19) or struct.unpack_from('<I',b,32)[0])-data)//spc;free=[c for c in range(2,count+2) if w(fat+c*2)==0];n=8192//(ss*spc)
  chain=random.Random(0xACE1).sample(free,n) if scatter else next(free[i:i+n] for i in range(len(free)-n+1) if free[i+n-1]-free[i]==n-1)
  pattern=bytes((i*37+i//256)&255 for i in range(8192))
  for i,c in enumerate(chain):
   for j in range(fc):struct.pack_into('<H',b,fat+j*fs+c*2,chain[i+1] if i+1<n else 0xffff)
   off=(data+(c-2)*spc)*ss;b[off:off+ss*spc]=pattern[i*ss*spc:(i+1)*ss*spc]
  slot=next(p for p in range(root,root+rootn*32,32) if b[p] in (0,0xe5));e=bytearray(32);e[:11]=b'DISKBEN TMP';struct.pack_into('<H',e,26,chain[0]);struct.pack_into('<I',e,28,8192);b[slot:slot+32]=e
  (f/f'rom-f{scatter}.bin').write_bytes(b)

if __name__=='__main__':main()
