"""Same public font source in resident slot and MAIN, including inversion."""
from pathlib import Path
import re
base=Path(__file__).with_name('check-aligned-font.py').read_text().split('for name,width,source')[0]
exec(base)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',p.read_text(),re.M)}
count=0
for native,shared in [('Rotate','Rot6Col4'),('Packed6K','FontPack'),('Fast12K','FontFull'),('Compact','FontKoi'),('Fast8','Font8'),('Cpm4','Font4'),('Cpm4Fast','Font4Fst')]:
 m=seed(native);ss=sy(R/f'bin/fonts/{shared}.sym')
 other=bytearray(m);data=(R/f'bin/fonts/{shared}.i80').read_bytes();other[0x2000:0x2000+len(data)]=data
 other=call(other,ss['benchInitialize'])
 for name in ('printChar','printCharRaw','printStringZ','printStringRawZ','printStringRawLim','setFontEncoding'):
  symbol='setEncoding' if name=='setFontEncoding' else name
  addr=S['bios_'+name];other[addr+1:addr+3]=W(ss[symbol])
 other[0xd816:0xd818]=W(ss['scalar'])
 for inv in (0,255):
  for mode in (0,1,0):
   m=call(m,S['FONT_ENGINE_ENCODING'],a=mode)
   other=call(other,S['FONT_ENGINE_ENCODING'],a=mode)
   for api in ('printChar','printCharRaw','printStringZ','printStringRawZ','printStringRawLim'):
    a=bytearray(m);b=bytearray(other)
    text=bytes(range(32,256))+b'\r\nABC\0'
    for v in (a,b):
     v[0x1000:0x1000+len(text)]=text
     v[0x9000:0xc000]=bytes([0x55])*12288
     v[S['mon2_vars.inverse']]=inv
     v[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]=bytes([7,2])
    args=dict(a=0xe1,bc=(len(text)-1 if api.startswith('printString') else 0xe1),hl=0x1000)
    a=call(a,S['bios_'+api],**args);b=call(b,S['bios_'+api],**args)
    assert a[0x9000:0xc000]==b[0x9000:0xc000],(native,api,inv,mode,'pixels')
    assert a[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]==b[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2],(native,api,'cursor')
    count+=1
 print(native,shared,'PASS',flush=True)
print(count,'shared frontend/KOI/inversion cases PASS')

# Public register contract on resident modules (same code is shared in MAIN).
register_cases=0
for native in ('Rotate','Packed6K','Fast12K','Compact','Fast8','Cpm4','Cpm4Fast'):
 m=seed(native);m[0x1000:0x1005]=b'ABCD\0'
 for inverse in (0,1,255):
  for api in ('printChar','printCharRaw','printStringZ','printStringRawZ','printStringRawLim','setFontEncoding'):
   b=bytearray(m);b[S['mon2_vars.inverse']]=inverse
   b[S['mon2_vars.cursorY']:S['mon2_vars.cursorY']+2]=bytes([7,0])
   af=0x4197;bc=4 if api.startswith('printString') else 0x1341;hl=0x1000;de=0x2468
   code=b'\x31'+W(0x8fe0)+b'\x21'+W(af)+b'\xe5\xf1\x01'+W(bc)+b'\x11'+W(de)+b'\x21'+W(hl)+b'\xcd'+W(S['bios_'+api])+b'\xc3'+W(0x2000)
   b[0x100:0x100+len(code)]=code;(O/'abi.bin').write_bytes(b)
   subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','100','-end','2000','-counter','30000000','-output',str(O/'abi-out.bin'),str(O/'abi.bin')],capture_output=True,check=True,timeout=20)
   regs=(O/'abi-out.bin').read_bytes()[65536:]
   assert regs[:4]==W(af)+W(bc) and regs[12:14]==W(de) and regs[8:10]==W(0x8fe0),(native,api,inverse,'registers',regs.hex())
   assert regs[4:6]==W(hl+4 if api.startswith('printString') else hl),(native,api,'HL')
   register_cases+=1
print(register_cases,'public register/stack cases PASS')
