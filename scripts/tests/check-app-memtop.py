"""Boundary tests: NC tail capacity and E refusing before initial text write."""
from pathlib import Path
import re,subprocess,runpy
R=Path(__file__).resolve().parents[2];O=R/'.local/app-memtop';O.mkdir(exist_ok=True)
sy=lambda f:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',f.read_text(),re.M)}
B=sy(R/'bin/BIOS.sym');N=sy(R/'bin/NC.sym');E=sy(R/'bin/EDIT.sym');W=lambda n:n.to_bytes(2,'little')
def execute(m,entry,end=0xeff0):
 code=b'\x31'+W(0x8fe0)+b'\x11'+W(0x7800)+b'\xcd'+W(entry)+b'\xc3'+W(end);m[0xee00:0xee00+len(code)]=code
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','ee00','-end',f'{end:x}','-counter','3000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=10)
 b=(O/'out.bin').read_bytes();assert b[65542:65544]==W(end);return b[:65536],b[65536]&1
def seed(app,top):
 m=bytearray([0xa5])*65536
 for addr,file in [(0xc000,'BIOS.sys'),(0,app+'.i80')]:
  data=(R/'bin'/file).read_bytes();m[addr:addr+len(data)]=data
 m[B['v_memTop']:B['v_memTop']+2]=W(top)
 return m
# NC now uses the real DOS allocator. Its boundary check must include the
# loader's image/BSS reservation and ARENA/ALLOC rather than a bare BIOS image.
runpy.run_path(str(R/'scripts/tests/check-nc-main-memory.py'))
text=E['EDITOR_TEXT_BEGIN']
for top in [text,text+1,0x63ff,0x7bff,0x8bff,N['APP_MEMORY_END']-1]:
 m=seed('EDIT',top)
 # BIOS/DOS tests exercise the public routes independently. This fixture
 # checks writes and bounds in EDIT with one successful registration.
 m[0x7800:0x7804]=b'-NC\0'
 m[E['v_textBeginLo']:E['v_textBeginLo']+2]=W(text)
 at=B['mx_getMemTop'];m[at:at+4]=b'\x21'+W(top)+b'\xc9'
 at=B['mx_callRegs'];m[at:at+6]=bytes.fromhex('e3 23 23 e3 af c9')
 m[B['FONT_ENGINE_ENCODING']]=0xc9
 target=E['editorMemoryTooSmall'] if top<text+1 else E['editorStart']
 out,cf=execute(m,E['openCommandLineFile'],target)
 if top<text+1:assert out[text]==0xa5
 else:
  assert int.from_bytes(out[E['v_textLimitLo']:E['v_textLimitLo']+2],'little')==top+1
  assert out[text]==0
print('Editor text boundary and early refusal PASS')

for cap in (64,185,255):
 for count in (0,63,64,184,185,255,256,300):
  m=seed('NC',0x8bff);m[N['ncCatalogCapacity']]=cap
  # Enter the count clamp after the directory API; capture its A result.
  at=N['lf_StoreCount'];m[at:at+4]=b'\x32'+W(0x8e00)+b'\xc9'
  shim=0xed00;m[shim:shim+6]=b'\x11'+W(count)+b'\xc3'+W(N['lf_ClampCount'])
  out,_=execute(m,shim);assert out[0x8e00]==min(cap,count),(cap,count)
print('NC directory total clamped to dynamic capacity PASS')
