"""Счётчики путей DOS READ: горячий клиентский путь и банковый A:."""
from pathlib import Path
import runpy,json
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'docs/font-arena-044/profile';OUT.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=OUT,RUNTIME=ROOT/'.local/rom-pio-19782')
g=h['Matrix'].__init__.__globals__;g['mcp_owner']=lambda:h['shell']('$p=Get-NetTCPConnection -State Listen -LocalPort 19782 -ErrorAction SilentlyContinue; if($p){$p[0].OwningProcess};exit 0')
x=h['Matrix']();m=x.m;m.url='http://127.0.0.1:19782/mcp';k=h['symbols'](ROOT/'bin/banked/KERNEL.sym');b=h['symbols'](ROOT/'bin/BIOS.sym');rows=[]
def profile(entry,**regs):
 labels={k['ksBuffered']:'buffered',k['klBlock']:'localBlock',b['BIOS_BANK_IO_ADDR']:'ROMbridge'}
 m.stop_at(0x8E10);m.write(0x8E00,b'\xcd'+entry.to_bytes(2,'little')+b'\xc3\x10\x8e')
 for addr in labels:m.call('bp_set',addr=addr,type='exec')
 m.call('regs_set',pc=0x8E00,sp=0x8FDF,**regs)
 counts={name:0 for name in labels.values()};samples=[];frames=[];costs={name:0 for name in labels.values()};start=m.call('cpu_state')['cpu_ticks']
 for _ in range(500):
  m.call('emu_run_for',ms=1000);st=m.call('cpu_state');pc=st['registers']['pc']['dec']
  if pc==0x8E10:break
  page=m.read(k['bankPage'],1)[0]
  if frames and pc==frames[-1][0] and page==frames[-1][1]:
   _,_,name,begin=frames.pop();costs[name]+=st['cpu_ticks']-begin
   m.call('debug_break');m.call('debug_step',mode='into');continue
  if pc not in labels:
   m.call('debug_break');m.call('debug_step',mode='into');continue
  counts[labels[pc]]+=1
  if (pc==k['klBlock'] and page==1) or (pc==b['BIOS_BANK_IO_ADDR'] and page==0):
   sp=st['registers']['sp']['dec'];ret=int.from_bytes(m.read(sp,2),'little');frames.append((ret,page,labels[pc],st['cpu_ticks']));m.call('bp_set',addr=ret,type='exec')
  if pc==k['ksBuffered'] and len(samples)<4:samples.append({name:m.read(k[name],1).hex() for name in ('kdPage','ksDirect','kdFarActive','v_drive','v_diskInfo.blockShift')})
  m.call('debug_break');m.call('debug_step',mode='into')
 else:raise AssertionError('profile timeout')
 return dict(counts=counts,samples=samples,cycles=costs,totalCycles=st['cpu_ticks']-start)
api=runpy.run_path(str(ROOT/'scripts/emu80/file-api.py'))
call=lambda op,hl=0,a=0:api['main_call'](m,k,op,hl,a)
w=lambda n:n.to_bytes(2,'little')
try:
 x.start('profile-A')
 h['main_bios'](m,k['mx_printChar'],bc=31)
 m.write(0x3400,b'A:/APROFILE.BIN'+bytes(113));data=bytes((i*31+7)&255 for i in range(8192));m.write(0x4000,data)
 m.write(0x3200,w(0x3400)+w(0x4000)+w(len(data))+bytes(34));api['path_io'](m,call,k,0x3200,True)
 f=bytearray(64);f[41]=1;f[49:51]=w(0x3400);m.write(0x3100,f);call(k['MXOS_DOS_FILEHANDLE'],hl=0x3100,a=7)
 m.write(0x7e00,b'\xcd\x66\xf8'+w(k['MXOS_DOS_FILEHANDLE'])+b'\xc9')
 for buffer in (0x4000,0x9000):
  m.write(0x3100+53,bytes(5));call(k['MXOS_DOS_FILEHANDLE'],hl=0x3100,a=3)
  m.write(0x3100+49,w(buffer)+w(len(data)))
  row=profile(0x7e00,hl=0x3100,af=0x102);row['buffer']=buffer;rows.append(row)
  assert m.read(buffer,len(data))==data
 call(k['MXOS_DOS_FILEHANDLE'],hl=0x3100,a=5)
 print(rows,flush=True);(OUT/'result.json').write_text(json.dumps(rows,indent=2))
finally:x.stop()
