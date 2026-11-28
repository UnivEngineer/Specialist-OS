from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'.local/aligned-font-cpu';O.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
W=lambda x:x.to_bytes(2,'little')
def call(m,entry,a=0,bc=0,hl=0,de=0):
 m=bytearray(m);code=b'\x31'+W(0x8fe0)+b'\x3e'+bytes([a])+b'\x01'+W(bc)+b'\x21'+W(hl)+b'\x11'+W(de)+b'\xcd'+W(entry)+b'\xc3'+W(0x2000);m[0x100:0x100+len(code)]=code
 (O/'in.bin').write_bytes(m)
 result=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','100','-end','2000','-counter','30000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True,timeout=20)
 b=(O/'out.bin').read_bytes();assert b[65542:65544]==W(0x2000);call.cycles=int(result.stdout.strip());return bytearray(b[:65536])

def seed(name):
 m=bytearray(65536)
 for at,file in [(0xc000,'bin/BIOS.sys'),(0xd800,f'bin/fonts/{name}.i80')]:
  data=(R/file).read_bytes();m[at:at+len(data)]=data
 m[S['mon2_vars']:S['mon2_vars']+S['MON2_VARIABLES']]=(R/'bin/BIOS.defaults.bin').read_bytes()[:S['MON2_VARIABLES']]
 m[S['bios_upper.escMode']]=255
 lower=0x3000 if name in ('Packed6K','Fast12K','Cpm4Fast') else 0
 if lower:
  data=(R/f'bin/fonts/{name}.pre').read_bytes();m[lower:lower+len(data)]=data
 return call(m,S['FONT_ENGINE_INIT'],de=lower)

source4='assets/system-rom/fonts/4x8/Cpm4.pf'
for name,width,source in [('Fast8',8,'assets/system-rom/fonts/8x8/Win8.pf'),('Cpm4',4,source4),
                          ('Cpm4Compact',4,source4),('Cpm4Fast',4,source4)]:
 m=seed(name);font=(R/source).read_bytes()
 for inv in (0,1,255):
  for x in (0,2,4,188,190):
   b=bytearray(m);b[0x9000:0xc000]=bytes([0x55])*12288
   b[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]=bytes([7,x]);b[S['mon2_vars.inverse']]=inv
   # Все коды и все четыре сочетания чётности в паре.
   text=bytes(range(256))+bytes([0,2,1,3,2,1,3,0,255])
   b[0x1000:0x1000+len(text)]=text
   got=call(b,S['FONT_ENGINE_LIM'],hl=0x1000,bc=len(text))
   expect=bytearray(b[0x9000:0xc000]);xp=x*2;y=0
   for code in text:
    xp=xp//width*width
    for row in range(8):
     bits=font[code*8+row]^(255 if inv else 0)
     for bit in range(width):
      px=xp+bit;offset=px//8*256+y+row;mask=128>>(px%8)
      expect[offset]=(expect[offset]&~mask)|(mask if bits&(1<<(width-1-bit)) else 0)
    xp+=width
    if xp>=384:xp=0;y+=10
   assert got[0x9000:0xc000]==expect,(name,inv,x,'pixels')
   assert got[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]==bytes([y+7,xp//2]),(name,inv,x,'cursor')
 # All frontends, same codes and CRLF; stop before NUL in Z variants.
 text=b'ABCD EFGH'+bytes([13,10])+b'IJK';m[0x1000:0x1000+len(text)+1]=text+b'\0'
 for string,char in [('mon2_printStringOld','mon2_printCharOld'),('FONT_ENGINE_RAWZ','FONT_ENGINE_RAW')]:
  got=call(m,S[string],hl=0x1000);ref=bytearray(m)
  for ch in text:ref=call(ref,S[char],a=ch,bc=ch)
  assert got[0x9000:0xc000]==ref[0x9000:0xc000],(name,string)
 if name in ('Cpm4','Cpm4Fast'):
  # Один и два прохода в пакете, включая переход E через FFh и назад.
  for y,x in ((0,0),(6,188),(255,184),(7,190)):
   for length in (2,3,4,5,7):
    for inverse in (0,255):
     b=bytearray(m);b[0x9000:0xc000]=bytes([0x55])*12288
     b[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]=bytes([y,x]);b[S['mon2_vars.inverse']]=inverse
     text=b'!QAmv#R'[:length]
     b[0x1000:0x1000+length]=text
     actual=call(b,S['FONT_ENGINE_LIM'],hl=0x1000,bc=length)
     for code in text:
      b=call(b,S['FONT_ENGINE_RAW'],a=code)
     assert actual[0x9000:0xc000]==b[0x9000:0xc000],(name,y,x,length,inverse)
     cursor=S['mon2_vars.cursorY']
     assert actual[cursor:cursor+2]==b[cursor:cursor+2],(name,y,x,length,inverse,'cursor')
 # Controls use the active width, including ESC Y and left/right.
 tab=width*2
 for code,expected in [(8,20-width//2),(24,20+width//2),(9,(20//tab+1)*tab)]:
  b=bytearray(m);b[S['mon2_vars.cursorX']]=20
  got=call(b,S['mon2_printCharOld'],bc=code)
  assert got[S['mon2_vars.cursorX']]==expected,(name,code)
 b=bytearray(m);b[0x1000:0x1006]=bytes([27,89,32,32+40,0,0])
 got=call(b,S['mon2_printStringOld'],hl=0x1000);assert got[S['mon2_vars.cursorX']]==40*width//2
 # Empty strings do not move an unaligned cursor.
 b=bytearray(m);b[S['mon2_vars.cursorX']]=1;b[0x1000]=0
 for api in ('mon2_printStringOld','FONT_ENGINE_RAWZ','FONT_ENGINE_LIM'):
  got=call(b,S[api],hl=0x1000,bc=0)
  assert got[S['mon2_vars.cursorX']]==1,(name,api,'empty')
 # KOI переставляет 31 пару, очищает DEL и сохраняет FFh.
 b=call(m,S['FONT_ENGINE_ENCODING'],a=1)
 if name=='Cpm4Compact':
  assert b[0xe580:0xe5f8]==m[0xe780:0xe7f8]
  for left,right in zip(b[0xe5f8:0xe600],m[0xe7f8:0xe800]):
   assert left==right&0xf0
  for row in range(8):
   assert b[0xe7f8+row]==(m[0xe5f8+row]&0xf0)|(m[0xe7f8+row]&15)
 else:
  for first in ([0xe400,0x3000] if name=='Cpm4Fast' else [0xe400]):
   for row in range(8):
    at=first+row*256
    assert b[at+0x60:at+0x7f]==m[at+0xe0:at+0xff]
    assert b[at+0x7f]==0 and b[at+0xff]==m[at+0xff]
 b=call(b,S['FONT_ENGINE_ENCODING'],a=0)
 assert b[0xe400:0xec00]==m[0xe400:0xec00]
 assert b[0x3000:0x3800]==m[0x3000:0x3800]
 # Flat import is performed by the transient module, never the resident.
 data=(R/'bin/fonts/InitA.i80').read_bytes();m[0x5000:0x5000+len(data)]=data
 raw=bytes((i*37+i//8)&255 for i in range(2048));m[0x4000:0x4800]=raw
 m[0x1100:0x1106]=W(0xd800)+W(0x4000)+W(0x3000)
 got=call(m,0x5000,hl=0x1100)
 if name=='Cpm4Compact':
  expected=bytes((raw[c*8+y]&15)*16+(raw[(c+1)*8+y]&15) for c in range(0,256,2) for y in range(8))
 else:
  multiplier=16 if name=='Cpm4Fast' else 17
  expected=bytes((raw[c*8+y] if width==8 else (raw[c*8+y]&15)*multiplier) for y in range(8) for c in range(256))
 assert got[0xe400:0xe400+len(expected)]==expected,(name,'import')
 if name=='Cpm4Fast':
  built=call(got,0x5000,a=1,hl=0x1100)
  assert built[0x3000:0x3800]==bytes(b>>4 for b in expected),(name,'generate')
 print(name,'pixels, cursor, controls, frontends, import PASS',flush=True)

# Reproducible model timings, all resident engines on one corpus, no disk I/O.
import json,hashlib
out=R/'docs/bench/font-modules-aligned';out.mkdir(parents=True,exist_ok=True)
raw=(R/'assets/system-rom/README.TXT').read_bytes().replace(b'\r\n',b' ').replace(b'\r',b' ').replace(b'\n',b' ')
raw=(raw*10)[:1104];text=b''.join(raw[i:i+46]+b'\r\n' for i in range(0,1104,46))
(out/'corpus.bin').write_bytes(text)
rows=[]
for name in ('Rotate','Compact','Packed6K','Fast12K','Fast8','Cpm4','Cpm4Compact','Cpm4Fast'):
 m=seed(name);m[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]=bytes([8,0]);m[S['mon2_vars.inverse']]=0;m[0x1000:0x1000+len(text)+1]=text+b'\0'
 row={'engine':name,'sha256':hashlib.sha256((R/f'bin/fonts/{name}.i80').read_bytes()).hexdigest()}
 for api in ('mon2_printStringOld','FONT_ENGINE_LIM'):
  call(m,S[api],hl=0x1000,bc=len(text));row[api]=call.cycles/2000
 rows.append(row);print(row,flush=True)
(out/'results.json').write_text(json.dumps({'method':'z88dk-ticks 8080 2 MHz; MAIN direct public ABI, 1152 bytes, includes call harness', 'rows':rows},indent=2))
