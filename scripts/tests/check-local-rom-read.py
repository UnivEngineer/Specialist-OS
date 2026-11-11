"""Локальный ROM READ MEM: границы банков, регистры и отсутствие MAIN-вызова."""
import re,struct,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/rom-local-2026-11-10/cpu';O.mkdir(parents=True,exist_ok=True)
W=lambda n:struct.pack('<H',n)
def syms(name):
 return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/banked'/f'{name}.sym').read_text(encoding='utf-8'),re.M)}
s=syms('CLIENT');t=syms('TRANSPORT')
asm=''' ORG 08000h
capture:
 ld de,08310h
 ld b,8
loop:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec b
 jp nz,loop
 ld a,(08320h)
 or a
 ret z
 scf
 ret
forbidden:
 ld a,1
 ld (08321h),a
 scf
 ret
 END
'''
(O/'stub.asm').write_text(asm,encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'stub.bin'),'--sym='+str(O/'stub.sym'),str(O/'stub.asm')],check=True,capture_output=True)
st={k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([\da-fA-F]+)',(O/'stub.sym').read_text(),re.M)}
base=bytearray(65536);code=(R/'bin/banked/CLIENT.bin').read_bytes();base[0xF800:0xF800+len(code)]=code
raw=(O/'stub.bin').read_bytes();base[0x8000:0x8000+len(raw)]=raw
base[t['bankCopySector']:t['bankCopySector']+3]=b'\xc3'+W(st['capture'])
base[t['bankCall']:t['bankCall']+3]=b'\xc3'+W(st['forbidden'])
count=0
for page in (1,6):
 for sector in (0,127,128,511,512,1023,1024,2047):
  for error in (0,1):
   mem=bytearray(base);mem[t['bankPage']]=page;mem[0x8320]=error
   stub=b'\x31'+W(0xE000)+b'\x01'+W(0x0200)+b'\x11'+W(sector)+b'\x21'+W(0x40F1)+b'\xcd'+W(s['MEM_IO_SYSTEM'])
   stub+=b'\x22'+W(0x8302)+b'\xeb\x22'+W(0x8304)+b'\x60\x69\x22'+W(0x8306)+b'\xf5\xe1\x22'+W(0x8300)+b'\xc3'+W(0x82ff)
   mem[0x8200:0x8200+len(stub)]=stub;(O/'in.bin').write_bytes(mem)
   subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','82ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
   out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x82ff
   assert out[0x8321]==0,'READ entered MAIN'
   assert bool(out[0x8300]&1)==bool(error)
   assert struct.unpack_from('<HHH',out,0x8302)==(0x40F1,sector,0x0200)
   expected=bytes([0x80+(sector>>7)])+W((sector&127)<<8)+bytes([page])+W(0x40F1)+W(256)
   assert out[0x8310:0x8318]==expected,(page,sector,out[0x8310:0x8318].hex())
   count+=1
print('LOCAL ROM READ PASS:',count,'cases; ROM banks, unaligned buffer, preserved registers, copy errors, no MAIN')

# Публичные/внутренние DOS-обёртки проверяют границы до доверенного MEM.
k=syms('KERNEL');base=bytearray(65536);code=(R/'bin/DOS.sys').read_bytes();base[0x100:0x100+len(code)]=code
# Обмен подменён только на успешный обработчик с отметкой вызова.
hook=b'\x3e\x01\x32'+W(0x8321)+b'\xaf\xc9';base[0x8000:0x8000+len(hook)]=hook
base[k['MEM_IO_SYSTEM']:k['MEM_IO_SYSTEM']+3]=b'\xc3'+W(0x8000)
base[t['bankCallPacket']:t['bankCallPacket']+3]=b'\xc3'+W(0x8000)
count=0
for capacity in (512,1024,2048):
 for entry in ('dosSystemDriver','klBlock'):
  cases=[(capacity-1,0x4001,False),(capacity,0x4001,True),(0,0xFE00,False)]
  if entry=='dosSystemDriver':cases += [(0,0xFE01,True),(0,0xFFFF,True)]
  for sector,buffer,error in cases:
   mem=bytearray(base);struct.pack_into('<H',mem,k['DOS_DRIVER_REGISTRY'],capacity)
   mem[k['v_drive']]=0;mem[k['kdPage']]=1
   stub=b'\x31'+W(0xE000)+b'\x01'+W(0x0200)+b'\x11'+W(sector)+b'\x21'+W(buffer)+b'\xcd'+W(k[entry])
   stub+=b'\xf5\xe1\x22'+W(0x8300)+b'\xc3'+W(0x82ff)
   mem[0x8200:0x8200+len(stub)]=stub;(O/'in.bin').write_bytes(mem)
   subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','82ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
   out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x82ff
   assert bool(out[0x8300]&1)==error,(capacity,entry,sector,hex(buffer),out[0x8300:0x8302].hex())
   assert bool(out[0x8321])!=error
   count+=1
print('LOCAL ROM BOUNDS PASS:',count,'cases; last/outside sector, 128/256/512 KiB, buffer ends')
