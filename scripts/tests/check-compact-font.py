"""Проверка компактного и линейного шрифта на CPU 8080, включая KOI-7 и инверсию."""
from pathlib import Path
import re, subprocess
ROOT=Path(__file__).resolve().parents[2]
sy={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/DOS.sym').read_text(),re.M)}
out=ROOT/'docs/memory-map-004/font';out.mkdir(parents=True,exist_ok=True)
font=(ROOT/'assets/system-rom/FONT.FNT').read_bytes()
compact=(ROOT/'assets/system-rom/FONTKOI.FNT').read_bytes()
assert compact==font[256:1024]+font[1536:2048]
dos=(ROOT/'bin/DOS.sys').read_bytes()
ticks=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')
def run(c,phase,koi,inverse,linear):
 m=bytearray(65536);m[0xc000:0xc000+len(dos)]=dos
 m[sy['FONT_ADDR']:sy['FONT_ADDR']+1280]=compact;m[0x6000:0x6800]=font
 m[0x9000:0xc000]=bytes((i*37)&255 for i in range(12288))
 m[sy['bios_vars.charGen']:sy['bios_vars.charGen']+2]=(0x6000//8 if linear else sy['FONT_ADDR_DIV_8']).to_bytes(2,'little')
 m[sy['bios_vars.koi7']]=koi;m[sy['bios_vars.escMode']]=255
 m[sy['bios_vars.inverse']:sy['bios_vars.inverse']+2]=bytes([inverse])*2
 m[sy['bios_vars.cursorY']]=24;m[sy['bios_vars.cursorX']]=phase
 code=bytes.fromhex('31 DF 8F 01')+bytes([c,0x12])+bytes.fromhex('11 78 56 21 34 12 CD 09 C8 C3 00 02')
 m[0x100:0x100+len(code)]=code
 (out/'input.bin').write_bytes(m)
 subprocess.run([str(ticks),'-m8080','-pc','100','-end','200','-counter','100000','-output',str(out/'output.bin'),str(out/'input.bin')],check=True,capture_output=True)
 b=(out/'output.bin').read_bytes();assert b[65542:65546]==bytes.fromhex('00 02 DF 8F')
 assert b[65538:65542]==bytes([c,0x12,0x34,0x12])
 return b[0x9000:0xc000],b[sy['bios_vars.cursorY']:sy['bios_vars.cursorY']+2]
n=0
for koi in (0,255):
 for inverse in (0,255):
  for c in range(32,256):
   mapped=c+128 if koi and 96<=c<128 else c
   ref=32 if (128<=mapped<192 and mapped not in (0xa3,0xb3) and not (koi and c>=192)) else c
   for phase in (c%4,):
    assert run(c,phase,koi,inverse,False)==run(ref,phase,koi,inverse,True),(c,koi,inverse,phase)
    n+=1
print(f'PASS: {n} compact/linear comparisons, full font unchanged')
