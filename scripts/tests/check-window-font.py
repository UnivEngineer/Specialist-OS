"""WIN8 preserves the original row kernel and renders every flat-font code."""
from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'docs/bench/window8-grid/cpu';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/ENGINEW.sym').read_text(),re.M)}
raw=(R/'bin/ENGINEW.i80').read_bytes();rom=(R/'source/monitor_win/originals/monitor_win.rom').read_bytes()
original=bytearray(rom[0x1172:0x11a3]);adapted=bytearray(raw[s['glyph']-0x2000:s['glyph']-0x2000+len(original)])
# Only addresses change: block table, screen pointer, row operation and loop.
for offset in (1,0x118f-0x1172+1,0x1199-0x1172+1,0x11a0-0x1172+1):
 original[offset:offset+2]=b'\0\0';adapted[offset:offset+2]=b'\0\0'
assert original==adapted,'historical instructions changed'
font=rom[0x1800:0x2000]
# One call renders all 256 codes, repositioning between glyphs outside scalar.
m=bytearray(65536);m[0x2000:0x2000+len(raw)]=raw
w=lambda x:x.to_bytes(2,'little');code=bytearray(b'\x31\x00\x8f');expect=bytearray(12288)
for c in range(256):
 x=(c%48)*4;y=(c//48)*10+7
 code+=b'\x21'+bytes([y,x])+b'\x22'+w(s['mon2_vars.cursorY'])+b'\x0e'+bytes([c])+b'\xcd'+w(s['scalar'])
 for row in range(8):expect[(x//4)*256+y-7+row]=font[c*8+row]
code+=b'\xc3\x00\x18';m[0x100:0x100+len(code)]=code;(O/'in.bin').write_bytes(m)
subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','100','-end','1800','-counter','30000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=30)
got=(O/'out.bin').read_bytes();assert int.from_bytes(got[65542:65544],'little')==0x1800
assert got[0x9000:0xc000]==expect
print('WIN8: original instructions and 256 glyphs PASS')
