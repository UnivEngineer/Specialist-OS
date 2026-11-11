"""8080: FILEHANDLE options, scatter allocation and checked sector routing."""
from pathlib import Path
import json,re,struct,subprocess
R=Path(__file__).resolve().parents[2]
O=R/'docs/diskbench/fragment-2026-11-10/cpu';O.mkdir(parents=True,exist_ok=True)
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
# Public OPTIONS must not mount/select any disk, and exchanges return previous flags.
for origin,new,old,want_error in [(0,65535,1,False),(1,1,0,False),(1,0,1,False),(1,2,1,True),(1,256,1,True),(2,0,1,True)]:
 m=mem();patch(m,'streamBufferRange',bytes.fromhex('B7 C9'));patch(m,'fileChangeDrive',bytes.fromhex('37 C9'))
 m[s['v_fileOptions']]=old;m[0xD000+57]=origin;struct.pack_into('<H',m,0xD000+51,new)
 b,cf,a=run(m,'fileHandle',6);assert cf==want_error,(origin,new,cf,a)
 if not want_error:
  assert int.from_bytes(b[0xD033:0xD035],'little')==old
  assert b[s['v_fileOptions']]==(new if origin else old)
 else:assert b[s['v_fileOptions']]==old
 rows.append(['options',origin,new,cf])
# Read must clear carry even though READ < WRITE, including when global verify is on.
for op,mode,global_flags,caps,want in [(1,1,1,7,0),(2,2,0,7,0),(2,2,1,7,4),(2,34,0,7,4),(2,2,1,3,0)]:
 m=mem();m[s['v_handleOp']]=op;m[0xD029]=mode;m[s['v_fileOptions']]=global_flags
 patch(m,'j_diskDriver',bytes([0x0E,caps,0xB7,0xC9]))
 b,cf,a=run(m,'handleWritePolicy');assert cf==0 and b[s['v_handleChecked']]==want,(op,mode,global_flags,cf)
 rows.append(['policy',op,mode,global_flags,caps,want])
# Stable seed produces bounded, dispersed candidates; real common search only uses free FAT entries.
seed=0xACE1;clusters=[]
for i in range(64):
 m=mem();struct.pack_into('<H',m,0xD000+60,seed);struct.pack_into('<H',m,s['v_diskInfo.dataClusters'],2046)
 patch(m,'fatReadCluster',bytes.fromhex('11 00 00 B7 C9'));patch(m,'handleAllocateFound',bytes.fromhex('C9'))
 b,cf,a=run(m,'handleAllocateScatter');assert cf==0
 seed=int.from_bytes(b[0xD03C:0xD03E],'little');candidate=int.from_bytes(b[s['v_findCluster']:s['v_findCluster']+2],'little')
 assert 2<=candidate<2048;clusters.append(candidate)
assert max(clusters)-min(clusters)>1800 and len(set(clusters))>56
rows.append(['scatter',clusters])
# Full and partial I/O use B=5 only for checked writes. Failed partial writes remain dirty.
for mode,checked,fail in [(1,0,False),(1,4,False),(2,4,False),(1,4,True)]:
 m=mem();m[s['v_streamMode']]=mode;m[s['v_streamChecked']]=checked
 struct.pack_into('<H',m,s['v_cacheLastSlot'],0xD100);struct.pack_into('<H',m,0xD102,0xD200);m[0xD106]=1
 code=bytes.fromhex('78 32 10 D9 22 12 D9 3E 01 32 11 D9')+(bytes.fromhex('37 C9') if fail else bytes.fromhex('B7 C9'))
 patch(m,'logicalSectorPrepared',code)
 b,cf,a=run(m,'streamPartialVerify');called=mode==1 and checked!=0
 assert b[0xD911]==called and cf==int(called and fail)
 assert b[0xD106]==(0 if called and not fail else 1)
 if called:assert b[0xD910]==5 and int.from_bytes(b[0xD912:0xD914],'little')==0xD200
 rows.append(['partial',mode,checked,fail,cf])
# Full-sector routing is local and switches only writes to checked entry.
for mode,checked,want in [(1,0,1),(1,4,5),(2,4,2)]:
 m=mem();m[s['v_streamMode']]=mode;m[s['v_streamChecked']]=checked
 patch(m,'ksSector',bytes.fromhex('78 32 10 D9 B7 C9'));patch(m,'streamCopyDone',bytes.fromhex('C9'))
 b,cf,a=run(m,'streamDirectIO');assert cf==0 and b[0xD910]==want
 rows.append(['full',mode,checked,want])
for caps in [0,4]:
 m=mem();m[0xD029]=34;patch(m,'j_diskDriver',bytes([0x0E,caps,0xB7,0xC9]))
 b,cf,a=run(m,'handleCheckMode');assert cf==int(caps==0)
 if not caps:assert b[s['v_dosError']]==10
 rows.append(['explicit-capability',caps,cf])
# Even scattered allocation uses a bounded full-volume search.
m=mem();struct.pack_into('<H',m,0xD03C,0xACE1);struct.pack_into('<H',m,s['v_diskInfo.dataClusters'],3)
patch(m,'fatReadCluster',bytes.fromhex('11 FF FF B7 C9'));patch(m,'handleAllocateFound',bytes.fromhex('C9'))
b,cf,a=run(m,'handleAllocateScatter');assert cf==1 and b[s['v_dosError']]==4
rows.append(['scatter-full',cf])
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print('PASS',len(rows),'FILEHANDLE options/policy/scatter/partial cases')
