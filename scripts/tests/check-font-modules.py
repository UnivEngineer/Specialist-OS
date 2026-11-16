from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'.local/font-module-cpu';O.mkdir(exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
W=lambda x:x.to_bytes(2,'little')
def call(m,entry,a=0,bc=0,hl=0,de=0):
 m=bytearray(m);code=b'\x31'+W(0x8fe0)+b'\x3e'+bytes([a])+b'\x01'+W(bc)+b'\x21'+W(hl)+b'\x11'+W(de)+b'\xcd'+W(entry)+b'\xc3'+W(0x2000);m[0x100:0x100+len(code)]=code
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','100','-end','2000','-counter','30000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True,timeout=20)
 b=(O/'out.bin').read_bytes();assert b[65542:65544]==W(0x2000);return bytearray(b[:65536])
font=(R/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes();checks=0;reference={}
for name in ('Rotate','Packed6K','Fast12K'):
 m=bytearray(65536);data=(R/'bin/BIOS.sys').read_bytes();m[0xc000:0xc000+len(data)]=data
 data=(R/f'bin/fonts/{name}.i80').read_bytes();m[0xd800:0xd800+len(data)]=data
 m[S['bios_vars']:S['bios_vars']+S['BIOS_VARIABLES']]=m[S['initVars']:S['initVarsEnd']]
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
     expected[row+0x60:row+0x80],expected[row+0xe0:row+0x100]=expected[row+0xe0:row+0x100],expected[row+0x60:row+0x80]
   assert m[base:base+size]==expected,(name,'KOI',mode,base)
 m=call(m,S['FONT_ENGINE_ENCODING'],a=1)
 m=call(m,S['FONT_ENGINE_INIT'],de=0x3000)
 assert m[0xe400:0xec00]==original[0xe400:0xec00],(name,'reattach')
 for inv in (0,255):
  for x in range(4):
   for n in (1,3,4,5,64):
    b=bytearray(m);text=bytes(range(256))[:n];b[0x1000:0x1000+n]=text
    b[S['bios_vars.cursorY']:S['bios_vars.cursorY']+2]=bytes([8,x]);b[S['bios_vars.inverse']]=inv
    got=call(b,S['bios_printStringRawLim'],hl=0x1000,bc=n)
    ref=bytearray(b)
    for c in text:ref=call(ref,S['bios_printCharRaw'],a=c)
    assert got[0x9000:0xc000]==ref[0x9000:0xc000],(name,inv,x,n,'batch vs scalar')
    assert got[S['bios_vars.cursorY']:S['bios_vars.cursorY']+2]==ref[S['bios_vars.cursorY']:S['bios_vars.cursorY']+2],(name,inv,x,n,'cursor')
    key=inv,x,n
    if name=='Rotate':reference[key]=got[0x9000:0xc000]
    else:assert got[0x9000:0xc000]==reference[key],(name,key,'rotate reference')
    checks+=1
 print(name,'PASS',flush=True)
print(checks,'checks')
