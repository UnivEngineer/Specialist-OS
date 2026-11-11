"""PIO sector loops: actual 8080 opcodes/timing and register/buffer contract.
Port A is constant RAM here; physical address selection is checked in Emu80.
"""
import re,struct,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/h-pio-2026-11-10/cpu';O.mkdir(parents=True,exist_ok=True)
W=lambda v:struct.pack('<H',v)
def symbols(i):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/f'bin/banked/LOCALDRV{i}.sym').read_text(),re.M)}
for i in range(3):
 s=symbols(i);raw=(R/f'bin/banked/LOCALDRV{i}.bin').read_bytes()
 read=bytes.fromhex('79 32')+W(s['IO_PROG_B'])+bytes.fromhex('7e 12 13 0c c2')
 labels=['ReadLoop','FlashReadLoop'] if i==0 else ['ReadLoop']
 for name in labels:
  code=raw[s[name]-0xfa00:s[name]-0xfa00+11];assert code==read+W(s[name]),(i,name,code.hex())
 if i:
  write=bytes.fromhex('78 32')+W(s['IO_PROG_B'])+bytes.fromhex('1a 32')+W(s['IO_PROG_A'])+bytes.fromhex('36 0c 36 0d 13 04 c2')+W(s['WriteLoop'])
  assert raw[s['WriteLoop']-0xfa00:s['WriteLoop']-0xfa00+17]==write
 base=bytearray(65536);base[0xfa00:0xfa00+len(raw)]=raw;base[s['IO_PROG_A']]=0xa5
 if i:
  base[s['SectorLimit']+1]=32;base[s['v_chipMap']:s['v_chipMap']+4]=bytes((0,8,16,24))
 for dest in (0x4001,0x8201):
  for entry in (('Driver','FlashRead') if i==0 else ('Driver',)):
   m=bytearray(base);m[dest-1]=0x12;m[dest+256]=0x34
   sector=127 if entry=='Driver' and i==0 else 0x1fff
   stub=b'\x31'+W(0x8f00)+b'\x01'+W(0x0277)+b'\x11'+W(sector)+b'\x21'+W(dest)+b'\xcd'+W(s[entry])
   stub+=b'\x22'+W(0x8702)+b'\xeb\x22'+W(0x8704)+b'\x60\x69\x22'+W(0x8706)+b'\xf5\xe1\x22'+W(0x8700)+b'\xc3'+W(0x88ff)
   m[0x8800:0x8800+len(stub)]=stub;(O/'in.bin').write_bytes(m)
   subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8800','-end','88ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
   out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x88ff
   assert not(out[0x8700]&1),(i,entry,'CF')
   assert out[dest:dest+256]==bytes([0xa5])*256,(i,entry,'data')
   assert (out[dest-1],out[dest+256])==(0x12,0x34)
   assert struct.unpack_from('<HHH',out,0x8702)==(dest,sector,0x0277)
 print('PIO PASS:',i,'resident',len(raw),'bytes; READ 52 T/byte'+('; WRITE 78 T/byte' if i else '; legacy + latched ROM'))
