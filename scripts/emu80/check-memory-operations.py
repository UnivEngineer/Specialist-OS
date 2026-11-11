"""Реальные банки: scalar, перекрытие, локальный движок DOS и FAR-куча."""
from pathlib import Path
import runpy,re,struct,json
R=Path(__file__).resolve().parents[2];O=R/'docs/banked/memory-054/banks';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001');x=h['Matrix']();m=x.m;rows=[]
def page(n):m.write(0xfffd if n else 0xfffc,bytes([n-1 if n else 0]))
def invoke(entry,**regs):
 m.write(0x8000,b'\xcd'+struct.pack('<H',entry)+b'\xc3\x00\x70');m.stop_at(0x7000);m.call('regs_set',pc=0x8000,sp=0x8fdf,**regs);before=m.call('cpu_state')['cpu_ticks'];z=m.until(0x7000,10);assert z['registers']['sp']['dec']==0x8fdf
 return {k:v['dec'] for k,v in z['registers'].items()},z['cpu_ticks']-before
try:
 x.start('FAR memory banks');m.call('debug_break');m.call('bp_clear')
 page(3);m.write(0x4000,bytes.fromhex('123456'));page(0)
 for op in (1,2,3,4):
  r,t=invoke(0xf815,af=op*256+2,bc=0x300,hl=0x4000,de=0xabcd);assert not r['af']&1
  if op==1:assert r['de']==0x12
  if op==2:assert r['de']==0x3412
  rows.append(dict(case='scalar',operation=op,ticks=t));print(rows[-1],flush=True)
 page(3);assert m.read(0x4000,3)==bytes.fromhex('cdab56')
 initial=bytes((i*37)&255 for i in range(512));m.write(0x4000,initial);page(0)
 m.write(0x6000,struct.pack('<BHBHH',3,0x4000,3,0x4001,257));r,t=invoke(0xf815,af=2,bc=0x1234,de=0x5678,hl=0x6000);assert not r['af']&1
 page(3);expected=bytearray(initial);expected[1:258]=initial[:257];assert m.read(0x4000,512)==expected;page(0);rows.append(dict(case='remote-overlap',ticks=t))
 page(1);m.write(0xa000,initial[:256]);m.write(0x800,struct.pack('<BHBHH',1,0xa000,1,0xa200,256));r,t=invoke(0xf80f,hl=0x800,bc=0x1234,de=0x5678);assert not r['af']&1 and m.read(0xa200,256)==initial[:256];page(0);rows.append(dict(case='DOS-near-copy',ticks=t))
 # COPY -> TEXT -> COPY и повторные операции без промежуточного COMMAND.
 # Вызовы ABI тестовые; самостоятельное банковое приложение не запускается.
 for length in (0,1,19,20,21,40,41,256):
  payload=b'X'*length+b'\0'
  page(1);m.write(0xa600,payload)
  m.write(0xa400,struct.pack('<BHBHH',1,0xa600,0,0x6000,len(payload)))
  r,t=invoke(0xf80f,hl=0xa400,bc=0x1234,de=0x5678);assert not r['af']&1
  assert m.read(0xffdf,1)==b'\x01'
  for repeat in range(2):
   r,t=invoke(0xf815,af=0x502,hl=0xa600,bc=0x1234,de=0x5678)
   assert r['hl']==0xa600+length and r['bc']==0x1234 and r['de']==0x5678 and not r['af']&1,r
   assert m.read(0xffdf,1)==b'\x02'
  r,t=invoke(0xf80f,hl=0xa400);assert not r['af']&1
  assert m.read(0xffdf,1)==b'\x01'
  # Вызов BIOS после COPY восстанавливает COMMAND и печатает main-адрес.
  m.write(0xa500,struct.pack('<HHHH',0x6000,0x4567,0x89ab,0x5502))
  r,t=invoke(0xf800,bc=0x129,de=0xa500)
  assert r['hl']==0x6000+length and r['de']==0x4567 and r['bc']==0x89ab,r
  assert m.read(0xffdf,1)==b'\0'
  page(0);assert m.read(0x6000,len(payload))==payload
  rows.append(dict(case='COPY-TEXT-TEXT-COPY-BIOS',bytes=length))
 # Повторный SCALAR сохраняет установленный профиль без возврата COMMAND.
 for value in (0x1234,0xabcd):
  r,t=invoke(0xf815,af=0x402,bc=0x300,hl=0x4200,de=value);assert not r['af']&1
  assert m.read(0xffdf,1)==b'\x03'
  r,t=invoke(0xf815,af=0x202,bc=0x300,hl=0x4200);assert r['de']==value and not r['af']&1
  assert m.read(0xffdf,1)==b'\x03'
 rows.append(dict(case='SCALAR-repeat',count=4))
 # Изолированный стенд удаляет разметку только собственного B: для разделения пула.
 page(2);m.write(0,bytes(256));page(0)
 def dos(op,bank=0,address=0,value=0):
  m.write(0x3000,struct.pack('<HHHH',address,value,bank*256,op*256+2));return invoke(0xf800,bc=0x226,de=0x3000)
 r,t=dos(10,4);assert not r['af']&1,r
 r,t=dos(6,255,128);assert not r['af']&1 and r['bc']>>8==6 and r['hl']==0,r;rows.append(dict(case='allocate-bank',bank=6,ticks=t))
 page(6);m.write(0,initial[:128]);page(0)
 r,t=dos(8,6,0,512);assert not r['af']&1,r;bank=r['bc']>>8;address=r['hl'];page(bank);assert m.read(address,128)==initial[:128];page(0);rows.append(dict(case='realloc-bank',bank=bank,address=address,ticks=t))
 r,t=dos(7,bank,address);assert not r['af']&1,r
 (O/'result.json').write_text(json.dumps(rows,indent=2)+'\n');print('FAR BANK OPERATIONS PASS',rows,flush=True)
finally:x.stop()
