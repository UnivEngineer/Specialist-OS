"""PIO sector loops: actual 8080 opcodes/timing and register/buffer contract.
Port A is constant RAM here; physical address selection is checked in Emu80.
"""
import argparse,re,struct,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/pio-unroll-2026-11-12/cpu';O.mkdir(parents=True,exist_ok=True)
W=lambda v:struct.pack('<H',v)
def symbols(path):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',path.read_text(),re.M)}
def check(i,path,factor=None):
 s=symbols(path.with_suffix('.sym'));raw=path.read_bytes();origin=s['Driver']
 if factor is None:factor=s.get('PIO_WRITE_FACTOR',2)
 if i:assert factor==s['PIO_WRITE_FACTOR']
 mode=s.get('PIO_UNALIGNED_KIND',0)
 repeat=s.get('PIO_UNALIGNED_FACTOR',8) if mode in (1,3) else 1
 writeRepeat=4 if mode==3 else s.get('PIO_WRITE_UNALIGNED_FACTOR',1) if mode==0 else repeat
 read=bytes.fromhex('79 32')+W(s['IO_PROG_B'])+bytes.fromhex('7e 12 13 0c c2')
 labels=['ReadLoop','FlashReadLoop'] if i==0 else ['ReadLoop']
 for name in labels:
  expected=(read[:-1]*repeat+bytes([0xc2])+W(s[name])) if mode!=2 else bytes.fromhex('7b c6 00 02 7e 12 13')*8+bytes.fromhex('7b fe 00 c2')+W(s[name])
  code=raw[s[name]-origin:s[name]-origin+len(expected)];assert code==expected,(i,name,code.hex())
  if mode!=3:
   aligned=name.replace('ReadLoop','ReadAlignedLoop');at=s[aligned]-origin
   expected=bytes.fromhex('7b 02 7e 12 1c')*8+bytes.fromhex('c2')+W(s[aligned])
   assert raw[at:at+len(expected)]==expected,(i,aligned)
 if i:
  write=bytes.fromhex('78 32')+W(s['IO_PROG_B'])+bytes.fromhex('1a 32')+W(s['IO_PROG_A'])+bytes.fromhex('36 0c 36 0d 13 04 c2')+W(s['WriteLoop'])
  expected=(write[:-3]*writeRepeat+bytes([0xc2])+W(s['WriteLoop'])) if mode!=2 else (bytes.fromhex('7b c6 00 02 1a 32')+W(s['IO_PROG_A'])+bytes.fromhex('36 0c 36 0d 13'))*8+bytes.fromhex('7b fe 00 c2')+W(s['WriteLoop'])
  assert raw[s['WriteLoop']-origin:s['WriteLoop']-origin+len(expected)]==expected,(i,'unaligned-write')
  if mode!=3:
   at=s['WriteAlignedLoop']-origin
   expected=(bytes.fromhex('7b 02 1a 32')+W(s['IO_PROG_A'])+bytes.fromhex('36 0c 36 0d 1c'))*factor+bytes.fromhex('c2')+W(s['WriteAlignedLoop'])
   assert raw[at:at+len(expected)]==expected,(i,'write-aligned')
 base=bytearray(65536);base[origin:origin+len(raw)]=raw;base[s['IO_PROG_A']]=0xa5
 if i:
  base[s['SectorLimit']+1]=32;base[s['v_chipMap']:s['v_chipMap']+4]=bytes((0,8,16,24))
 readMemory=base
 for dest in (0x4000,0x4001,0x40f0,0x40fc,0x40ff,0x8200,0x8201,0x82f0,0x82fc,0x82ff,0x4001,0x4001):
  for entry in (('Driver','FlashRead') if i==0 else ('Driver',)):
   m=bytearray(readMemory);m[dest-1]=0x12;m[dest+256]=0x34
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
   readMemory=out[:65536]
 if i:
  writeMemory=base
  for source in (0x4000,0x4001,0x40f0,0x40fc,0x40ff,0x8200,0x8201,0x82f0,0x82fc,0x82ff,0x4001,0x4001):
   m=bytearray(writeMemory);payload=bytes((j*73+19)&255 for j in range(256));m[source:source+256]=payload
   m[s['pioWrite.WriteDone']]=0xc9
   stub=b'\x31'+W(0x8f00)+b'\x01'+W(0x0037)+b'\x11'+W(source)
   if mode==2:stub+=b'\x21'+W(source)+b'\xcd'+W(s['pioWrite.Prepare'])
   stub+=b'\x21'+W(s['IO_PROG_MODE'])+b'\xcd'+W(s['pioWrite.WriteStart'])
   stub+=b'\x22'+W(0x8702)+b'\xeb\x22'+W(0x8704)+b'\x60\x69\x22'+W(0x8706)+b'\xc3'+W(0x88ff)
   m[0x8800:0x8800+len(stub)]=stub;(O/'in.bin').write_bytes(m)
   subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8800','-end','88ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
   out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x88ff
   assert struct.unpack_from('<HHH',out,0x8702)==(s['IO_PROG_MODE'],source+256,0x0037)
   assert out[source:source+256]==payload
   assert (out[s['IO_PROG_B']],out[s['IO_PROG_A']],out[s['IO_PROG_MODE']])==(255,payload[-1],13)
   writeMemory=out[:65536]
 print('PIO PASS:',i,'resident',len(raw),'bytes; unaligned mode',mode,'READ',38+22/8 if mode==2 else 42+10/repeat,'WRITE',64+22/8 if mode==2 else 68+10/writeRepeat)

if __name__ == '__main__':
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--variants',type=Path,help='Directory with AT2MX/AT2MXF-xN.bin experimental builds')
 args=parser.parse_args()
 if args.variants:
  O=args.variants.parent/'cpu-variants';O.mkdir(parents=True,exist_ok=True)
  files=sorted(args.variants.glob('AT2MX*-x*.bin'))
  assert files,'No variant binaries'
  for path in files:
   match=re.fullmatch(r'(AT2MXF?)-x(\d+)(?:-w\d+)?(?:-u[0123])?',path.stem);assert match,path
   check(2 if match[1]=='AT2MXF' else 1,path,int(match[2]))
 else:
  for i in range(3):check(i,R/f'bin/banked/LOCALDRV{i}.bin')
