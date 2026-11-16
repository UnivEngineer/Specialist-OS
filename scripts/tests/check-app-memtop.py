"""Boundary tests: NC tail capacity and E refusing before initial text write."""
from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'.local/app-memtop';O.mkdir(exist_ok=True)
sy=lambda f:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',f.read_text(),re.M)}
B=sy(R/'bin/BIOS.sym');N=sy(R/'bin/NC.sym');E=sy(R/'bin/E.sym');W=lambda n:n.to_bytes(2,'little')
def execute(m,entry,end=0xeff0):
 code=b'\x31'+W(0x8fe0)+b'\xcd'+W(entry)+b'\xc3'+W(end);m[0xee00:0xee00+len(code)]=code
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','ee00','-end',f'{end:x}','-counter','3000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=10)
 b=(O/'out.bin').read_bytes();assert b[65542:65544]==W(end);return b[:65536],b[65536]&1
def seed(app,top):
 m=bytearray([0xa5])*65536
 for addr,file in [(0xc000,'BIOS.sys'),(0,app+'.i80')]:
  data=(R/'bin'/file).read_bytes();m[addr:addr+len(data)]=data
 m[B['v_memTop']:B['v_memTop']+2]=W(top)
 return m
for top in [N['NC_MIN_END']-2,N['NC_MIN_END']-1,0x63ff,0x7bff,0x8bff]:
 m,cf=execute(seed('NC',top),N['ncMemoryInit'])
 assert cf==int(top<N['NC_MIN_END']-1)
 if cf:continue
 capacity=min(255,(top+1-N['FILE_CACHE_A'])//48-1)
 assert m[N['ncCatalogCapacity']]==capacity
 stride=(capacity+1)*16
 assert m[N['ncCatalogBases']:N['ncCatalogBases']+6]==b''.join(W(N['FILE_CACHE_A']+stride*i) for i in range(3))
 assert int.from_bytes(m[N['ncMemoryEnd']:N['ncMemoryEnd']+2],'little')<=top+1
 print(hex(top),'NC capacity',capacity)
for top in [0x1800,0x1801,0x63ff,0x7bff,0x8bff]:
 m=seed('E',top)
 # Stop before file argument parsing or before the explicit low-memory message.
 target=E['editorMemoryTooSmall'] if top<0x1801 else B['bios_argNext']
 if top>=0x1801:
  # bios_argNext is reached only after writing the empty text marker.
  target=B['bios_argNext'];m[B['bios_setFontEncoding']]=0xc9
 out,cf=execute(m,E['openCommandLineFile'],target)
 if top<0x1801:assert out[0x1800]==0xa5
 else:
  assert int.from_bytes(out[E['v_textLimitLo']:E['v_textLimitLo']+2],'little')==top+1
  assert out[0x1800]==0
print('Editor text boundary and early refusal PASS')

for cap in (64,185,255):
 for count in (0,63,64,184,185,255,256,300):
  m=seed('NC',0x8bff);m[N['ncCatalogCapacity']]=cap
  # Enter the count clamp after the directory API; capture its A result.
  at=N['lf_StoreCount'];m[at:at+4]=b'\x32'+W(0x8e00)+b'\xc9'
  shim=0xed00;m[shim:shim+6]=b'\x11'+W(count)+b'\xc3'+W(N['lf_ClampCount'])
  out,_=execute(m,shim);assert out[0x8e00]==min(cap,count),(cap,count)
print('NC directory total clamped to dynamic capacity PASS')
