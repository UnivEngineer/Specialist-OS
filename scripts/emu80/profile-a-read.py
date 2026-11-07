"""Счётчики путей DOS READ: горячий клиентский путь и банковый A:."""
from pathlib import Path
import runpy,json
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'docs/font-arena-044/profile';OUT.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=OUT,RUNTIME=ROOT/'.local/monitor2-001')
x=h['Matrix']();m=x.m;k=h['symbols'](ROOT/'bin/banked/KERNEL.sym');b=h['symbols'](ROOT/'bin/BIOS.sym');rows=[]
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
   m.call('debug_step',mode='into');continue
  if pc not in labels:
   m.call('debug_step',mode='into');continue
  counts[labels[pc]]+=1
  if (pc==k['klBlock'] and page==1) or (pc==b['BIOS_BANK_IO_ADDR'] and page==0):
   sp=st['registers']['sp']['dec'];ret=int.from_bytes(m.read(sp,2),'little');frames.append((ret,page,labels[pc],st['cpu_ticks']));m.call('bp_set',addr=ret,type='exec')
  if pc==k['ksBuffered'] and len(samples)<4:samples.append({name:m.read(k[name],1).hex() for name in ('kdPage','ksDirect','kdFarActive','v_drive','v_diskInfo.blockShift')})
  m.call('debug_step',mode='into')
 else:raise AssertionError('profile timeout')
 return dict(counts=counts,samples=samples,cycles=costs,totalCycles=st['cpu_ticks']-start)
try:
 x.start('profile-A');x.console();x.init_app(0,1)
 m.write(x.s['v_pass'],b'\1');x.func(x.s['fill_payload'],af=0)
 assert not x.func(b['bios_fileCreate'],hl=x.s['tmpDescriptor'],de=x.s['sectorBuffer'])['flags']['C']
 rows.append(profile(b['bios_fileLoad2'],hl=x.s['tmpDescriptor'],de=0x4000))
 rows.append(profile(x.s['read_high'],bc=0))
 print(rows,flush=True);(OUT/'result.json').write_text(json.dumps(rows,indent=2))
finally:
 if x.pid:
  x.func(x.s['file_cleanup'])
 x.stop()
