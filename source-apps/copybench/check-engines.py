"""Проверка ABI копировщиков: только изолированный эмулятор, без UI-подмены команд."""
from pathlib import Path
import runpy,re,json,argparse
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--report',type=Path,default=ROOT/'docs/bench/copyengines-001');a=p.parse_args();a.report.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=a.report;g['RUNTIME']=ROOT/'.local/monitor2-001'
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',(ROOT/'bin/COPYBEN.sym').read_text(),re.M)}
x=h['Matrix']();rows=[]
try:
 x.start('copyengines-abi');m=x.m
 # Отдельный ABI-стенд: загрузка кода напрямую не считается UI-проверкой.
 m.write(0,(ROOT/'bin/COPYBEN.i80').read_bytes())
 pages=m.read(0x8ff8,1)[0];assert pages>=2
 m.write(s['targetPage'],bytes([pages-1]));m.write(s['sourcePhysical'],bytes([pages-1]));m.write(s['destinationPhysical'],bytes([pages-2]));m.write(s['engineBank'],b'\1');m.write(s['bankPageCount'],bytes([pages]))
 def select(page):m.write(0xfffc if page==0 else 0xfffd,bytes([0 if page==0 else page-1]))
 def write(page,addr,data):
  if not data:return
  select(page);m.write(addr,data);select(0)
 def read(page,addr,n):
  if not n:return b''
  select(page);v=m.read(addr,n);select(0);return v
 def invoke(entry,src,dst,start,end,n):
  packet=bytes([src])+start.to_bytes(2,'little')+bytes([dst])+end.to_bytes(2,'little')+n.to_bytes(2,'little')
  m.write(0x2400,packet)
  return x.func(s[entry],hl=0x2400,bc=0x1234,de=0x5678)
 for mode,entry in enumerate(('r4Block256','cpmBlock256','bankCopyFar','cpmUniversal')):
  m.write(s['engineMode'],bytes([mode]));x.func(s['engineInstall'])
  lengths=[256] if mode<2 else [0,1,3,4,15,16,17,255,256,257,8191,8192]
  for src,dst in [(0,pages),(pages,0),(pages,pages-1)]:
   for n in lengths:
    start=0x3000 if mode<2 else 0x3001
    end=0x5003
    data=bytes((i*37+(i>>8)+19)&255 for i in range(n))
    write(src,start,data)
    write(dst,end-1,b'\xA5'+bytes(v^255 for v in data)+b'\x5A')
    window=m.read(0xffc0,32)
    state=invoke(entry,src,dst,start,end,n)
    assert read(dst,end-1,n+2)==b'\xA5'+data+b'\x5A',(entry,src,dst,n)
    assert read(src,start,n)==data,(entry,'source',n)
    if mode>=2:
     assert m.read(0xffc0,32)==window,(entry,'window',n)
     regs=state['registers'];assert [regs[k]['dec'] for k in ('bc','de','hl','sp')]==[0x1234,0x5678,0x2400,0x8fdf],state
     assert regs['af']['dec']>>8==0 and not state['flags']['C'],state
    rows.append({'entry':entry,'src':src,'dst':dst,'bytes':n})
  if mode>=2:
   for src,start,n,error in [(pages+1,0x3000,1,0xE0),(pages,0xFEFF,2,0xE2),(pages,0xFFFE,4,0xE2)]:
    window=m.read(0xffc0,32);guard=read(0,0x5003,32)
    state=invoke(entry,src,0,start,0x5003,n)
    assert state['registers']['af']['dec']>>8==error and state['flags']['C'],state
    assert m.read(0xffc0,32)==window and read(0,0x5003,32)==guard
    rows.append({'entry':entry,'error':error})
  print(entry,'OK',flush=True)
 (a.report/'result.json').write_text(json.dumps(rows,indent=2));print('Passed',len(rows),'cases')
finally:x.stop()
