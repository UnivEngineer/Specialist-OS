"""8080: KOI8 language/case are independent; KOI7 character mapping retained."""
from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'.local/keyboard-koi-check';O.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
W=lambda n:n.to_bytes(2,'little');base=bytearray(65536)
for name,at in [('BIOS.sys',0xc000),('IO.main.bin',0xf000)]:
 b=(R/'bin'/name).read_bytes();base[at:at+len(b)]=b
count=0
def run(koi,locks,shift,raw):
 global count
 m=bytearray(base);m[S['bios_upper.koi7']]=koi;m[S['mon2_vars.keyLocks']]=locks;m[0xffe1]=0 if shift else 2
 code=(b'\x31\x00\x8f\x01'+W(raw<<8)+b'\x11'+W(koi<<8|locks)+b'\xcd'+W(S['keyDecode'])+
       b'\xf5\xe1\x22\x00\x8d\xeb\x22\x02\x8d\xc3\x40\x8c')
 m[0x8c00:0x8c00+len(code)]=code
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8c00','-end','8c40','-counter','100000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 out=(O/'out.bin').read_bytes();assert out[65542:65544]==W(0x8c40),(raw,out[65536:].hex())
 assert out[S['bios_upper.koi7']]==koi
 count+=1;return out
for koi in (0,255):
 for locks in (0x3a,0xba,0x3b,0xbb):
  for shift in (False,True):
   for raw in range(0x60,0x7f):
    out=run(koi,locks,shift,raw)
    lowercase=bool(locks&128)!=shift
    offset=(0 if lowercase else 32) if koi or not locks&1 else (32 if lowercase else 0)
    expected=(raw^offset)+(128 if locks&1 else 0)
    assert out[0x8d01]==expected and not out[0x8d00]&0x40,(koi,locks,shift,raw,out[0x8d01],expected)
   out=run(koi,locks,shift,0x81)
   mask=0x80 if koi or shift else 1
   assert out[0x8d02]==locks^mask and out[0x8d00]&0x40,(koi,locks,shift)
 for shift in (False,True):
  for raw in range(0x21,0x40):
   out=run(koi,0xba,shift,raw)
   expected=(0x5f if raw==0x30 else raw^0x10) if shift else raw
   assert out[0x8d01]==expected and not out[0x8d00]&0x40,(koi,shift,raw,out[0x8d01],expected)
print('KEYBOARD KOI/CAPS PASS',count)
