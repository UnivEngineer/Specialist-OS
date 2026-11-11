"""Полный MXOS_CALL и этапы скалярного BIOS-маршрута, такты CPU Emu80."""
from pathlib import Path
import argparse,json,re,runpy,struct
R=Path(__file__).resolve().parents[2]
def symbols(path):
 return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',path.read_text('utf-8'),re.M)}
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--label',required=True);p.add_argument('--out',type=Path,default=R/'docs/bench/mxcall-2026-11-09');a=p.parse_args()
 out=a.out.resolve();out.mkdir(parents=True,exist_ok=True)
 h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=out,RUNTIME=R/'.local/monitor2-001')
 x=h['Matrix']();m=x.m;b=R/'bin/banked';cs=symbols(b/'CLIENT.sym');ts=symbols(b/'TRANSPORT.sym');bs=symbols(b/'MAINBIOS.sym');s=symbols(R/'bin/BIOS.sym')
 word=lambda n:struct.pack('<H',n)
 packet=struct.pack('<4H',0x1234,0x2345,0x3456,0x7103)
 def page(n):m.write(0xfffd if n else 0xfffc,bytes((n-1 if n else 0,)))
 def state():return m.call('cpu_state')['cpu_ticks']
 rows=[]
 try:
  (out/a.label).mkdir(parents=True,exist_ok=True)
  fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,out/a.label)
  x.start('MXOS_CALL '+a.label,**fixture);m.call('debug_break');m.call('bp_clear')
  font=(R/'assets/system-rom/FONT.FNT').read_bytes()
  expected_font=bytes(font[char*8+row] for row in range(8) for char in range(256))
  assert m.read(s['FONT_ADDR'],s['FONT_BYTES'])==expected_font,'Boot font differs from FONT.FNT'
  page(6)
  for address,name in ((cs['MEM_IMAGE_LOCAL_START'],'LOCALBLOCK'),(cs['mx_call'],'CLIENT'),(ts['BANK_LANDING'],'TRANSPORT')):m.write(address,(b/(name+'.bin')).read_bytes())
  m.write(cs['MEM_BANK_KIND'],bytes((cs['MEM_BANK_EXEC'],)))
  for n in (0,1,6):
   page(n);m.write(ts['bankPage'],bytes((n,)));m.write(ts['bankOwner'],bytes((7+n,n)));m.write(ts['bankDepth'],b'\0')
   top=s['BIOS_STACK_ADDR'] if 'BIOS_STACK_ADDR' in s else bs['BANK_BIOS_STACK_TOP']
   low=bs['BANK_BIOS_STACK_LOW']
   if n:top=cs['DOS_BANK_STACK_TOP'];low=cs['DOS_BANK_STACK_BOTTOM']
   m.write(ts['bankServiceSP'],word(top));m.write(ts['bankStackFloor'],word(low+64))
  page(0);m.write(bs['bbBusy'],b'\0');m.write(0xffc0,(b/'WINDOW.bin').read_bytes())
  for n in (0,1,6):
   for inline in (False,True):
    for repeat in range(2):
     page(n);m.write(0x3000,packet)
     stub=b'\xcd'+word(cs['mx_callRegs'])+word(cs['MXOS_BIOS_UPPERCASE']) if inline else b'\xcd'+word(cs['mx_call'])
     m.write(0x2000,stub+b'\xc3\x00\x70');m.call('bp_clear')
     regs=dict(hl=0x1234,de=0x2345,bc=0x3456,af=0x7103) if inline else dict(de=0x3000,bc=cs['MXOS_BIOS_UPPERCASE'])
     m.call('regs_set',pc=0x2000,sp=0x7000,**regs)
     checkpoints=[]
     if inline:checkpoints.append((n,cs['bankMxCallRegs'],'inline-entry'))
     if n:
      checkpoints += [(n,cs['bankMxCall'],'client'),(n,ts['bankCallPacket'],'transport-call'),(0,ts['bankLanding'],'main-landing'),(0,bs['bankBiosPacketDispatch'],'bios-adapter'),(0,bs['bbInvoke'],'bios-invoke')]
     checkpoints += [(0,s['sysCall'],'main-dispatch'),(0,s['sysReturned'],'main-result')]
     if n:checkpoints += [(0,bs['bbFinish'],'bios-result'),(0,ts['bankServiceReturn'],'transport-result'),(n,ts['bankResume'],'caller-resume'),(n,cs['bankMxResult'],'client-result')]
     checkpoints.append((n,0x7000,'return'))
     start=last=state();previous='stub';stages=[]
     for target,address,name in checkpoints:
      m.stop_at(address);result=m.until(address,10,page=target);ticks=state();stages.append(dict(stage=previous+' -> '+name,ticks=ticks-last));last=ticks;previous=name
     r={k:result['registers'][k]['dec'] for k in ('af','bc','de','hl','sp')}
     assert r['sp']==0x7000 and r['af']>>8==ord('Q'),r
     assert (r['hl'],r['de'],r['bc'])==(0x1234,0x2345,0x3456),r
     assert m.read(ts['bankPage'],1)==bytes((n,)) and m.read(ts['bankDepth'],1)==b'\0'
     assert m.read(ts['bankOwner'],2)==bytes((7+n,n))
     if not inline:assert m.read(0x3000,8)==struct.pack('<4H',r['hl'],r['de'],r['bc'],r['af'])
     row=dict(page=n,inline=inline,repeat=repeat,ticks=last-start,stages=stages);rows.append(row)
     print(a.label,n,'MXOS_CALL' if inline else 'MX_REGS',repeat,last-start,flush=True)
  (out/(a.label+'.json')).write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
 finally:x.stop()
if __name__=='__main__':main()
