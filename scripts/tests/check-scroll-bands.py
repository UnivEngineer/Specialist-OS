"""CPU 8080: прокрутка очищает отбрасываемые строки до общего memmove."""
from pathlib import Path
import re, subprocess
R=Path(__file__).resolve().parents[2]
O=R/'docs/cli/cli-fixes-2026-11-05/scroll';O.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOS.sym').read_text(encoding='utf-8'),re.M)}
W=lambda x:x.to_bytes(2,'little')
def run(m,stop):
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6000','-end',f'{stop:x}','-counter','3000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 b=(O/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==stop
 return b[:65536]
for down in (False,True):
 for fill in (0,255):
  m=bytearray(65536);dos=(R/'bin/DOS.sys').read_bytes();m[0xC000:0xC000+len(dos)]=dos
  screen=bytes((x*31+y*7+1)&255 for x in range(48) for y in range(256));m[0x9000:0xC000]=screen
  at=S['bios_vars.inverse'];m[at:at+2]=bytes([fill,fill])
  entry=S['dos_piece_scrollDown_1_start' if down else 'dos_piece_scroll_1_start']
  code=b'\x31'+W(S['STACK_ADDR'])+b'\xCD'+W(entry)+b'\xC3'+W(0x6100);m[0x6000:0x6000+len(code)]=code
  pre=run(m,S['bios_memcpy_bc_hl']);out=run(m,0x6100)
  for x in range(48):
   old=screen[x*256:(x+1)*256];base=0x9000+x*256
   want=(bytes([fill])*10+old[:246]) if down else (old[10:]+bytes([fill])*10)
   cleared=(old[:246]+bytes([fill])*10) if down else (bytes([fill])*10+old[10:])
   assert pre[base:base+256]==cleared,(down,fill,x,'before memcpy')
   assert out[base:base+256]==want,(down,fill,x,'final')
print('SCROLL BANDS PASS: both directions and inverse; pre-copy and final bitmap')
