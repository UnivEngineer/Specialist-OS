"""Штатный bootstrap: MEM из ROM, сохранение B: при Reset и скалярный API."""
from pathlib import Path
import runpy,re,struct,json
R=Path(__file__).resolve().parents[2];O=R/'docs/mem/get-put-profile-work/boot-check';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text('utf-8'),re.M)}
b=sy(R/'bin/BIOS.sym');k=sy(R/'bin/banked/KERNEL.sym');s=sy(R/'bin/bootstrap/BOOT.sym');n=sy(R/'bin/NC.sym')
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001');x=h['Matrix']();m=x.m
page=lambda v:m.write(0xfffd if v else 0xfffc,bytes([v-1 if v else 0]))
rows=[]
try:
 x.start('bootstrap MEM tail')
 m.call('debug_break');m.call('bp_clear');page(0)
 page(2);boot=m.read(0,256);assert boot[3:11]==b'MXOS256 '
 assert int.from_bytes(boot[11:13],'little')==256
 total=int.from_bytes(boot[19:21],'little');fat=int.from_bytes(boot[22:24],'little');start=1+fat+8
 entry=256+2*(254-start+2)
 assert m.read(entry,4)==bytes.fromhex('f7fff7ff')
 page(1);assert m.read(k['memoryRamOwner'],1)==bytes([1]);page(0)
 rows.append('cold NC, fixed B: and two BAD sectors')
 # ABI-вызов установщика: текущая раскладка/битмап шрифта сохраняются.
 font=m.read(b['FONT_ADDR'],2048);encoding=m.read(b['bios_upper.koi7'],1)
 for args in ('-P 4','-P 0'):
  page(0);command=('C:\\DRIVERS\\RAMDISK.COM '+args).encode()+b'\0';m.write(0x5000,command+bytes(128-len(command)))
  m.write(0x5100,struct.pack('<4H',0x5000,0,0,0));m.write(0x5200,bytes.fromhex('cd60f8c30053'));m.stop_at(0x5300)
  m.call('regs_set',pc=0x5200,sp=0x8f00,bc=k['MXOS_DOS_FILE_EXEC_NATIVE'],de=0x5100);state=m.until(0x5300,40)
  if args=='-P 4':assert not state['flags']['C'],state
  assert m.read(b['FONT_ADDR'],2048)==font and m.read(b['bios_upper.koi7'],1)==encoding
 rows.append('RAMDISK reinstall/error preserve font and encoding')
 page(2);payload=b'WARM RAM DISK DATA';m.write(0xe000,payload);page(0)
 m.call('bp_clear');m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120,page=0)
 page(2);assert m.read(0xe000,len(payload))==payload;page(0)
 rows.append('warm Reset reaches NC and preserves B:')
 # GET/PUT8/16 из MAIN/DOS/EXEC к DATA/MAIN с разным наполнением окна.
 image=(R/'bin/MEM.sys').read_bytes();parts=[struct.unpack_from('<3H',image,6+6*i) for i in range(image[5])];window=image[parts[-1][2]:parts[-1][2]+32]
 page(6)
 for address,size,offset in parts[:-1]:m.write(address,image[offset:offset+size])
 m.write(b['bankPage'],bytes([6]));m.write(b['bankPageCount'],bytes([16]));m.write(b['MEM_BANK_KIND'],bytes([1]))
 scalar=[]
 for caller,target in ((0,2),(1,7),(6,2),(6,0),(6,7)):
  for seed,preceding in ((0,None),(41,None),(82,32),(123,1),(164,None),(205,None)):
   for op in (1,2,3,4):
    page(target);m.write(0x4000,bytes.fromhex('aa123456bb'))
    page(caller);m.write(0xffc0,bytes((seed+31*i)&255 for i in range(32)))
    if preceding is not None:
     # Настоящее содержимое COPY: полная порция либо точный остаток.
     m.write(0x8200,struct.pack('<BHBHH',caller,0x4100,target,0x4200,preceding))
     m.write(0x8300,bytes.fromhex('cd6ff8c30084'));m.stop_at(0x8400)
     m.call('regs_set',pc=0x8300,sp=0x8f00,hl=0x8200)
     copied=m.until(0x8400,20);assert not copied['flags']['C']
    last=m.read(0xffde,2)
    m.write(0x8000,bytes.fromhex('cd75f8c30081'));m.stop_at(0x8100)
    m.call('regs_set',pc=0x8000,sp=0x8f00,af=op*256+2,bc=target*256+0x5a,de=0xabcd,hl=0x4001)
    ticks=m.call('cpu_state')['cpu_ticks'];state=m.until(0x8100,20);ticks=state['cpu_ticks']-ticks
    assert not state['flags']['C'] and state['registers']['sp']['dec']==0x8f00
    assert state['registers']['hl']['dec']==0x4001 and state['registers']['bc']['dec']==target*256+0x5a
    if op in (1,2):assert state['registers']['de']['dec']==(0x12 if op==1 else 0x3412)
    assert m.read(0xffde,2)==last
    page(target);expected={1:'aa123456bb',2:'aa123456bb',3:'aacd3456bb',4:'aacdab56bb'}[op];assert m.read(0x4000,5)==bytes.fromhex(expected)
    scalar.append(dict(caller=caller,target=target,seed=seed,preceding=preceding,op=op,ticks=ticks))
 page(0);m.write(0xffc0,window)
 rows.append('120 physical GET/PUT8/16 cases preserve registers, guards and SP with different initial window contents and after CPM32')
 (O/'scalar.json').write_text(json.dumps(scalar,indent=2)+'\n')
 print('BOOTSTRAP TAIL PASS',rows,flush=True);(O/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
finally:x.stop()
