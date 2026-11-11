"""8080: extended DOS arena and mutual exclusion with active BAT frames."""
from pathlib import Path
import json,re,struct,subprocess
R=Path(__file__).resolve().parents[2]
O=R/'docs/diskbench/dos-workspace-2026-11-10/cpu';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/banked/KERNEL.sym').read_text(),re.M)}
base=(R/'bin/DOS.sys').read_bytes();rows=[]
def word(x):return struct.pack('<H',x)
def mem():
 m=bytearray(65536);m[0x100:0x100+len(base)]=base
 struct.pack_into('<H',m,s['v_handle'],0xD000)
 return m
def patch(m,label,code):m[s[label]:s[label]+len(code)]=code
def run(m,label,a=0,hl=0xD000):
 stub=b'\x31'+word(0xF300)+b'\x21'+word(hl)+bytes([0x3E,a,0xCD])+word(s[label])+bytes.fromhex('F5 C1')
 # PUSH AF; POP BC; MOV H,B; MOV L,C; SHLD status; JMP end.
 stub=stub+bytes.fromhex('60 69 22 20 D9 C3 00 DA')
 m[0xD800:0xD800+len(stub)]=stub;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','D800','-end','DA00','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=15)
 b=(O/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0xDA00
 return bytearray(b[:65536]),b[0xD920]&1,b[0xD921]

for end,depth,denied,borrow in [(0xD700,1,False,0),(0xD800,1,False,0),(0xD801,0,False,1),(0xEA00,0,False,1),(0xF000,0,False,1),(0xF001,0,True,0),(0xEA00,1,True,0),(0xEA00,8,True,0)]:
 m=mem();m[s['v_batDepth']]=depth
 patch(m,'mdDosEndReady',bytes.fromhex('AF C9'));patch(m,'mdError',bytes.fromhex('37 C9'))
 b,cf,a=run(m,'mdDos',hl=end)
 assert cf==denied and b[s['mdBorrowRequest']]==borrow,(hex(end),depth,cf,a)
 rows.append(['range',end,depth,denied,borrow])
m=mem();m[s['moduleDosOwner']]=1;m[s['moduleDosBorrowed']]=1
patch(m,'mdFreeRecordMain',bytes.fromhex('C9'));b,cf,a=run(m,'mdFreeTransientDos')
assert b[s['moduleDosOwner']]==b[s['moduleDosBorrowed']]==0
rows.append(['unload-releases-BAT',True])
for borrowed in [0,1]:
 m=mem();m[s['moduleDosBorrowed']]=borrowed;patch(m,'batArenaFree',bytes.fromhex('AF C9'))
 # execBat consumes one arguments word before it returns to its caller.
 struct.pack_into('<HH',m,0xF2FC,0x1234,0xD930)
 capture=bytes.fromhex('F5 C1 60 69 22 20 D9 C3 00 DA');m[0xD930:0xD930+len(capture)]=capture
 stub=b'\x31'+word(0xF2FC)+b'\xC3'+word(s['execBat']);m[0xD800:0xD800+len(stub)]=stub
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','D800','-end','DA00','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=15)
 b=(O/'out.bin').read_bytes();assert b[0xD920]&1==borrowed
 if borrowed:assert b[s['v_dosError']]==s['DOS_ERR_ACCESS']
 rows.append(['BAT-excludes-large-module',borrowed])
assert s['BIOS_DISK_BUFFER']==s['BIOS_BANK_IO_BUFFER']==s['BIOS_TEMP_BUFFER']
assert s['FONT_ADDR']==0xE200 and s['FONT_ADDR']+s['FONT_BYTES']==0xEA00
assert s['BIOS_BANK_STACK_BOTTOM']-0xEA00==640
assert s['BIOS_TEMP_BUFFER_END']==s['bios_upper'] and s['BIOS_TEMP_BUFFER_SIZE']==272
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print('PASS',len(rows),'DOS arena/BAT guards; shared upper buffer and 640-byte reserve')
