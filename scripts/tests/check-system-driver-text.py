"""A: DOS-owned descriptions, bounded input and rejection before registry writes."""
from pathlib import Path
import re,struct,subprocess
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/dos-driver-text-2026-11-11/cpu';O.mkdir(exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/banked/KERNEL.sym').read_text(encoding='utf-8'),re.M)}
w=lambda n:struct.pack('<H',n)
asm=""" ORG 08400h
fetch:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec bc
 ld a,b
 or c
 jp nz,fetch
 ret
physical:
 ld a,b
 cp 3
 jp nz,info
 ld de,0800h
 ld bc,0
 or a
 ret
info:
 ld bc,0307h
 ld hl,0
 or a
 ret
"""
(O/'stubs.asm').write_text(asm);subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'stubs.bin'),'--sym='+str(O/'stubs.sym'),str(O/'stubs.asm')],check=True,capture_output=True)
st={k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([\da-fA-F]+)',(O/'stubs.sym').read_text(),re.M)}
for payload,pointer,arg,error in [(b'AT512KS\0' + b'512K ONBOARD FLASH\0',0xD000,0,False),(b'A'*62+b'\0\0',0xD000,0,False),(b'A'*63+b'\0',0xD000,0,True),(b'A'*64,0xD000,0,True),(b'',0,0,False),(b'A\0B\0',0xD000,1,True)]:
 m=bytearray(65536);code=(R/'bin/DOS.sys').read_bytes();m[256:256+len(code)]=code;code=(O/'stubs.bin').read_bytes();m[0x8400:0x8400+len(code)]=code
 for label,target in [('kdFetch','fetch'),('dosSystemDriver','physical')]:m[s[label]:s[label]+3]=b'\xc3'+w(st[target])
 m[s['invalidateDriveCache']]=0xc9
 m[0xD000:0xD040]=payload.ljust(64,b'X');m[s['DOS_DRIVER_REGISTRY']:s['DOS_DRIVER_REGISTRY']+80]=b'Z'*80
 stub=b'\x31'+w(0xf300)+b'\x21'+w(pointer)+bytes([0x3e,arg,0xcd])+w(s['dosDriverRefresh'])+b'\xf5\xe1\x22\x00\x85\xc3\xff\x82';m[0x8200:0x8200+len(stub)]=stub;(O/'in.bin').write_bytes(m)
 subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','82ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 z=(O/'out.bin').read_bytes();assert int.from_bytes(z[65542:65544],'little')==0x82ff
 assert bool(z[0x8500]&1)==error
 record=z[s['DOS_DRIVER_REGISTRY']:s['DOS_DRIVER_REGISTRY']+80]
 if error:assert record==b'Z'*80
 else:
  want=payload if pointer else b'ROM-SYS\0BUILT-IN, READ ONLY\0'
  assert record[8:72]==want.ljust(64,b'\0');assert record[:2]==w(2048)
print('SYSTEM DRIVER TEXT PASS: 6 descriptor/default/boundary/error cases')
