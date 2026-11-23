from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'.local/compact-font-cpu';O.mkdir(exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
W=lambda x:x.to_bytes(2,'little')
def call(m,entry,a=0,bc=0,hl=0,de=0):
 m=bytearray(m);code=b'\x31'+W(0x8fe0)+b'\x3e'+bytes([a])+b'\x01'+W(bc)+b'\x21'+W(hl)+b'\x11'+W(de)+b'\xcd'+W(entry)+b'\xc3'+W(0x2000);m[0x100:0x100+len(code)]=code
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','100','-end','2000','-counter','30000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True,timeout=20)
 b=(O/'out.bin').read_bytes();assert b[65542:65544]==W(0x2000);return bytearray(b[:65536])

C={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/fonts/Compact.sym').read_text(),re.M)}
m=bytearray(65536)
for at,file in [(0xc000,'bin/BIOS.sys'),(0xd800,'bin/fonts/Compact.i80')]:
 data=(R/file).read_bytes();m[at:at+len(data)]=data
m[S['bios_vars']:S['bios_vars']+S['BIOS_VARIABLES']]=m[S['initVars']:S['initVarsEnd']]
m[S['bios_upper.escMode']]=255
m=call(m,S['FONT_ENGINE_INIT'])
font=(R/'bin/fonts/compact-font.bin').read_bytes();lookup=(R/'bin/fonts/compact-map.bin').read_bytes()
blank=len(font)//8
font+=bytes(8)
present=set(range(0x20,0x80))|set(range(0xc0,0x100))|{0xa3,0xb3}
assert all(lookup[c]==lookup[0x7f] for c in range(256) if c not in present)
assert font[lookup[0x7f]*8:lookup[0x7f]*8+8]==bytes([63,33,33,33,33,33,63,0])
source=(R/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
assert all(font[lookup[c]*8:lookup[c]*8+8]==source[c*8:c*8+8] for c in present if c!=0x7f)
text=bytes(range(256));m[0x1000:0x1100]=text
for koi in (0,1,0):
 m=call(m,S['bios_setFontEncoding'],a=koi)
 map_=bytearray(lookup)
 if koi:
  map_[0x60:0x7f],map_[0xe0:0xff]=map_[0xe0:0xff],map_[0x60:0x7f]
  map_[0x7f]=blank
 for inv in (0,255):
  for x in range(4):
   seed=bytearray(m);seed[0x9000:0xc000]=bytes([0x55])*12288
   seed[S['bios_vars.cursorY']:S['bios_vars.cursorY']+2]=bytes([7,x]);seed[S['bios_vars.inverse']]=inv
   got=call(seed,S['bios_printStringRawLim'],hl=0x1000,bc=256)
   expected=bytearray(seed[0x9000:0xc000]);xp=x*2;y=0
   for code in text:
    if xp>=380:xp=0;y+=10
    rows=font[map_[code]*8:map_[code]*8+8]
    for row,bits in enumerate(rows):
     bits^=63 if inv else 0
     for bit in range(6):
      px=xp+bit;offset=(px//8)*256+y+row;mask=128>>(px%8)
      expected[offset]=(expected[offset]&~mask)|(mask if bits&(32>>bit) else 0)
    xp+=6
    if xp>=384:xp=0;y+=10
   assert got[0x9000:0xc000]==expected,(koi,inv,x,'pixels')
   assert got[S['bios_vars.cursorY']:S['bios_vars.cursorY']+2]==bytes([y+7,xp//2]),(koi,inv,x,'cursor')
print('Compact: all 256 codes, four phases, inversion and KOI switches PASS')
# Import a deliberately different full font into a staged compact module.
i=(R/'bin/fonts/InitKoi.i80').read_bytes();m[0x5000:0x5000+len(i)]=i
raw=bytes((n*37+n//8)&255 for n in range(2048));m[0x4000:0x4800]=raw
m[0x1100:0x1106]=W(0xd800)+W(0x4000)+W(0)
m=call(m,0x5000,hl=0x1100)
codes=(R/'bin/fonts/compact-codes.bin').read_bytes()
assert m[C['fontData']:C['fontData']+len(codes)*8]==bytes(raw[c*8+y]&63 for c in codes for y in range(8))
print('Compact flat import PASS')
