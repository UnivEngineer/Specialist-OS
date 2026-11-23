"""8080 system reservation lifecycle; BIOS publication is tested in Emu80."""
from pathlib import Path
from dos_state import initialize_dos_state
import re,subprocess
R=Path(__file__).resolve().parents[2];S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/banked/KERNEL.sym').read_text(),re.M)}
O=R/'.local/resident-cpu';O.mkdir(exist_ok=True);W=lambda x:x.to_bytes(2,'little')
m=bytearray(65536);d=(R/'bin/DOS.sys').read_bytes();m[0x100:0x100+len(d)]=d
initialize_dos_state(m, S);m[S['memoryPublishTop']]=0xc9
# Flat CPU fixture: only bank 0 is queried; transport/page validation is external.
m[S['bankCopyPageCheck']:S['bankCopyPageCheck']+2]=bytes([0xaf,0xc9])

def call(entry,a=0,bc=0,hl=0,de=0):
 global m
 code=b'\x31'+W(0xef70)+b'\x3e'+bytes([a])+b'\x01'+W(bc)+b'\x21'+W(hl)+b'\x11'+W(de)+b'\xcd'+W(S[entry])+b'\xc3'+W(0xe400)
 m[0xe000:0xe000+len(code)]=code;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','e000','-end','e400','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=5)
 b=(O/'out.bin').read_bytes();assert b[65542:65544]==W(0xe400);m=bytearray(b[:65536]);return b[65536]&1,int.from_bytes(b[65540:65542],'little')
def end():return int.from_bytes(m[S['memorySystemEnd']:S['memorySystemEnd']+2],'little')
call('memoryInit');call('moduleInit')
assert call('dosMemoryService',15,0x100,0x1000,0x3000)==(0,0x7c00);assert end()==0x7c00
assert call('dosMemoryService',15,0x100,0x800,0x3000)==(0,0x8400)
assert call('dosMemoryService',15,0x100,0x1000,0x3000)==(0,0x7c00)
call('moduleWarm');assert end()==0x7c00
assert call('dosMemoryService',15,0x101,0x200,0x3000)==(0,0x7a00)
assert call('dosMemoryService',15,0x100,0,0)==(0,0);assert end()==0x7a00
assert call('dosMemoryService',15,0x101,0,0)==(0,0);assert end()==0x8c00
before=bytes(m[S['mrSlots']:S['mrSlots']+16]);assert call('dosMemoryService',15,0x100,0x1000,0x8000)[0]==1;assert end()==0x8c00;assert bytes(m[S['mrSlots']:S['mrSlots']+16])==before
assert call('dosMemoryService',15,0x100,1,0)[0]==1
m[S['MEM_ALLOC_TABLE']:S['MEM_ALLOC_TABLE']+6]=b'\0'+W(0x7b00)+W(0x200)+b'\x20'
assert call('dosMemoryService',15,0x100,0x1000,0)[0]==1;assert end()==0x8c00
m[S['MEM_ALLOC_TABLE']]=255
assert call('dosMemoryService',15,0x100,0x1000,0x3000)==(0,0x7c00)
assert call('dosMemoryService',9,0,0x7000,0xd00)[0]==1
m[S['memoryMainStart']:S['memoryMainStart']+2]=W(0x8000)
m[S['memoryMainEnd']:S['memoryMainEnd']+2]=W(0x8c00)
assert call('dosMemoryService',12,0)==(0,0),'restored arena above resident boundary is empty'
print('PASS: reserve/resize/release, other slot retained, warm lifetime, invalid alignment/floor and allocated-block collision, arena cap')
