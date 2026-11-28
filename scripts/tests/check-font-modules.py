from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'.local/font-module-cpu';O.mkdir(exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
W=lambda x:x.to_bytes(2,'little')
def call(m,entry,a=0,bc=0,hl=0,de=0):
 m=bytearray(m);code=b'\x31'+W(0x8fe0)+b'\x3e'+bytes([a])+b'\x01'+W(bc)+b'\x21'+W(hl)+b'\x11'+W(de)+b'\xcd'+W(entry)+b'\xc3'+W(0x2000);m[0x100:0x100+len(code)]=code
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','100','-end','2000','-counter','30000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True,timeout=20)
 b=(O/'out.bin').read_bytes();assert b[65542:65546]==W(0x2000)+W(0x8fe0);return bytearray(b[:65536])
font=(R/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes();checks=0;reference={}
for name in ('Rotate','Packed6K','Fast12K'):
 m=bytearray(65536);data=(R/'bin/BIOS.sys').read_bytes();m[0xc000:0xc000+len(data)]=data
 data=(R/f'bin/fonts/{name}.i80').read_bytes();m[0xd800:0xd800+len(data)]=data
 m[S['mon2_vars']:S['mon2_vars']+S['MON2_VARIABLES']]=(R/'bin/BIOS.defaults.bin').read_bytes()[:S['MON2_VARIABLES']]
 m[S['bios_upper.escMode']]=255
 p=bytes(font[c*8+y]&63 for y in range(8) for c in range(256))
 m[0x3000:0x3800]=bytes(((b<<2)|(b>>4))&255 for b in p)
 m[0x3800:0x4000]=bytes(((b<<4)|(b>>2))&255 for b in p)
 if name=='Fast12K':m[0x3000:0x5800]=(R/'bin/fonts/Fast12K.pre').read_bytes()
 m=call(m,S['FONT_ENGINE_INIT'],de=0x3000)
 # Physical KOI switching changes every row page, including all lower
 # planes; repeated requests are no-ops. Attach normalizes an existing image.
 original=bytearray(m)
 for mode in (1,1,0,0):
  m=call(m,S['FONT_ENGINE_ENCODING'],a=mode)
  for base,size in [(0xe400,2048)]+([(0x3000,4096 if name=='Packed6K' else 10240)] if name!='Rotate' else []):
   expected=bytearray(original[base:base+size])
   if mode:
    for row in range(0,size,256):
     expected[row+0x60:row+0x7f],expected[row+0xe0:row+0xff]=expected[row+0xe0:row+0xff],expected[row+0x60:row+0x7f]
     expected[row+0x7f]=0
   assert m[base:base+size]==expected,(name,'KOI',mode,base)
 m=call(m,S['FONT_ENGINE_ENCODING'],a=1)
 m=call(m,S['FONT_ENGINE_INIT'],de=0x3000)
 assert m[0xe400:0xec00]==original[0xe400:0xec00],(name,'reattach')
 for inv in (0,255):
  for x in range(4):
   for n in (1,3,4,5,64):
    b=bytearray(m);text=bytes(range(256))[:n];b[0x1000:0x1000+n]=text
    b[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]=bytes([8,x]);b[S['mon2_vars.inverse']]=inv
    got=call(b,S['FONT_ENGINE_LIM'],hl=0x1000,bc=n)
    ref=bytearray(b)
    for c in text:ref=call(ref,S['FONT_ENGINE_RAW'],a=c)
    assert got[0x9000:0xc000]==ref[0x9000:0xc000],(name,inv,x,n,'batch vs scalar')
    assert got[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]==ref[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2],(name,inv,x,n,'cursor')
    key=inv,x,n
    if name=='Rotate':reference[key]=got[0x9000:0xc000]
    else:assert got[0x9000:0xc000]==reference[key],(name,key,'rotate reference')
    checks+=1
 # Stop before and after a successful quartet, at each possible slot.
 for n in range(8):
  for entry,char_entry,separator in [('mon2_printStringOld','mon2_printCharOld',b'\t'),
                                     ('FONT_ENGINE_RAWZ','FONT_ENGINE_RAW',b'\x01')]:
   text=b'ABCDEFG'[:n]+separator+b'wxyz'
   b=bytearray(m)
   b[0x1000:0x1000+len(text)+1]=text+b'\0'
   b[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]=b'\x08\0'
   actual=call(b,S[entry],hl=0x1000)
   expected=b
   for glyph in text:
    expected=call(expected,S[char_entry],a=glyph,bc=glyph)
   assert actual[0x9000:0xc000]==expected[0x9000:0xc000],(name,entry,n)
   cursor=S['mon2_vars.cursorY']
   assert actual[cursor:cursor+2]==expected[cursor:cursor+2],(name,entry,n)
   checks+=1
 print(name,'PASS',flush=True)
print(checks,'checks')

# Both public interpreted entries use the next four-cell stop, even from
# an X position that falls between characters.
for name,width in [('Cpm4',4),('Rotate',6),('Fast8',8)]:
 m=bytearray(65536)
 for binary,address in [(R/'bin/BIOS.sys',0xc000),(R/f'bin/fonts/{name}.i80',0xd800)]:
  data=binary.read_bytes()
  m[address:address+len(data)]=data
 m[S['mon2_vars']:S['mon2_vars']+S['MON2_VARIABLES']]=(R/'bin/BIOS.defaults.bin').read_bytes()[:S['MON2_VARIABLES']]
 m[S['bios_upper.escMode']]=255
 m=call(m,S['FONT_ENGINE_INIT'])
 m[0x1000:0x1002]=b'\t\0'
 screen=bytes(m[0x9000:0xc000])
 stop=2*width
 for x in (0,1,width//2,stop-1,stop,stop+1,192-stop-1,192-stop,191,192,255):
  at=S['mon2_vars.cursorY']
  m[at:at+2]=bytes([8,x])
  target=(x//stop+1)*stop
  expected=bytes([8,target]) if target<192 else b'\x12\0'
  for entry in ('mon2_printCharOld','mon2_printStringOld'):
   result=call(m,S[entry],bc=9,hl=0x1000)
   assert result[at:at+2]==expected,(name,entry,x,result[at:at+2],expected)
   assert result[0x9000:0xc000]==screen
 print(name,'Tab stops and right-edge wrap PASS',flush=True)
