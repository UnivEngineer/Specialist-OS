"""8080: PIO installer ID probing, detected geometry and DOS-owned descriptions."""
from pathlib import Path
import re,struct,subprocess
R=Path(__file__).resolve().parents[2];O=R/'docs/flash/description-2026-11-18';O.mkdir(parents=True,exist_ok=True)
w=lambda n:struct.pack('<H',n)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',p.read_text(encoding='utf-8'),re.M)}
asm=''' ORG 0E000h
probe:
 ld a,b
 rrca
 rrca
 rrca
 and 3
 ld e,a
 ld d,0
 ld hl,0E100h
 add hl,de
 ld b,(hl)
 ld hl,0E110h
 add hl,de
 ld c,(hl)
 ret
install:
 pop de
 inc de
 inc de
 push de
 ld hl,0E120h
 inc (hl)
 xor a
 ret
 END
'''
(O/'hooks.asm').write_text(asm);subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080',f'--raw={O/"hooks.bin"}',f'--sym={O/"hooks.sym"}',str(O/'hooks.asm')],capture_output=True,check=True)
h=sy(O/'hooks.sym');hooks=(O/'hooks.bin').read_bytes();cases=0
for binary,name,feature in [('Flash','AT2MX','SEQUENTIAL'),('FlashFst','AT2MXF','PIPELINE')]:
 s=sy(R/f'bin/{binary}.sym');origin=s['DRIVER_LOAD_ADDR'];code=(R/f'bin/{binary}.i80').read_bytes()
 for mask,bad in [(m,None) for m in range(16)]+[(15,'manufacturer'),(15,'device')]:
  m=bytearray(65536);m[origin:origin+len(code)]=code;m[0xe000:0xe000+len(hooks)]=hooks
  for label,hook in [('initProbe.ParallelProbe','probe'),('mx_callRegs','install')]:m[s[label]:s[label]+3]=b'\xc3'+w(h[hook])
  m[0xe100:0xe104]=bytes(0x1f if mask&(1<<i) else 0xff for i in range(4));m[0xe110:0xe114]=bytes([0xa4])*4
  if bad=='manufacturer':m[0xe100]=0x20
  if bad=='device':m[0xe110]=0xa7
  m[0xf9c0:0xfc00]=bytes([0xa5])*576
  stub=b'\x31'+w(0xe800)+b'\xcd'+w(origin)+b'\xf5\xe1\x22'+w(0xe122)+b'\xc3'+w(0xe2ff);m[0xe200:0xe200+len(stub)]=stub;(O/'in.bin').write_bytes(m)
  subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','e200','-end','e2ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True)
  out=(O/'out.bin').read_bytes();assert out[65542:65544]==w(0xe2ff)
  success=bool(mask) and bad is None
  assert bool(out[0xe122]&1)!=success and out[0xe120]==int(success)
  if success:
   count=mask.bit_count();desc=out[s['description']:s['description']+64].split(b'\0')[:2]
   assert desc==[name.encode(),f'{count}X512K {feature}'.encode()],desc
   at=s['registration']+s['DRIVER_INSTALL.blocks'];assert out[at:at+2]==w(count*2048)
   mapping=s['initProbe.v_chipMap'];assert out[mapping:mapping+count]==bytes(i*8 for i in range(4) if mask&(1<<i))
  else:assert out[0xf9c0:0xfc00]==bytes([0xa5])*576
  cases+=1
print('PIO DRIVER DESCRIPTION / GEOMETRY PASS',cases,'cases; 1-4 chips, gaps, unsupported IDs, refusal before install')
