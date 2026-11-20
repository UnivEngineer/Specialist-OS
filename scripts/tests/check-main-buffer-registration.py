"""8080: exact owned MAIN buffers, protection, FREE and executable handoff."""
from pathlib import Path
import re,subprocess
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'.local/main-buffer-registration';OUT.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/banked/KERNEL.sym').read_text(encoding='utf-8'),re.M)}
W=lambda n:int(n).to_bytes(2,'little')
def run(mem,entry,hl=0,de=0,bc=0,a=0):
 code=b'\x31'+W(0xef70)+b'\x21'+W(hl)+b'\x11'+W(de)+b'\x01'+W(bc)+b'\x3e'+bytes([a])+b'\xcd'+W(S[entry])+b'\xc3'+W(0xe400)
 mem[0xe000:0xe000+len(code)]=code;(OUT/'input.bin').write_bytes(mem)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','e000','-end','e400','-counter','30000000','-output',str(OUT/'output.bin'),str(OUT/'input.bin')],check=True,capture_output=True,timeout=20)
 data=(OUT/'output.bin').read_bytes();assert data[65542:65544]==W(0xe400),'PC';assert data[65544:65546]==W(0xef70),'SP'
 return bytearray(data[:65536]),data[65536],data[65537],int.from_bytes(data[65540:65542],'little')
def seed():
 m=bytearray(65536);b=(ROOT/'bin/DOS.sys').read_bytes();m[0x100:0x100+len(b)]=b
 m[S['memoryPublishTop']]=0xc9
 m[S['MEM_ALLOC_TABLE']:S['MEM_ALLOC_TABLE']+32*6]=bytes([255])*192
 m[S['MODULE_TABLE']:S['MODULE_TABLE']+S['MODULE_RECORD_SIZE']*8]=bytes(S['MODULE_RECORD_SIZE']*8)
 m[S['appOwner']]=S['APP_OWNER_BASE'];m[S['memoryMainOwner']]=S['APP_OWNER_BASE']
 for key,value in [('memorySystemEnd',0x8c00),('memoryMainStart',0x1000),('memoryMainEnd',0x8c00),('appImage',S['MEM_ALLOC_TABLE'])]:m[S[key]:S[key]+2]=W(value)
 m[S['MEM_ALLOC_TABLE']:S['MEM_ALLOC_TABLE']+6]=b'\0'+W(0)+W(0x1000)+bytes([S['APP_OWNER_BASE']])
 m[0x8401:0x8604]=bytes([0xa5])*0x203
 return m
m=seed();table=S['MEM_ALLOC_TABLE']
m,flags,a,_=run(m,'dosMemoryService',hl=0x8401,de=0x203,a=S['MEM_OP_MAINREGISTER'])
assert not flags&1 and a==0
assert m[table+6:table+12]==b'\0'+W(0x8401)+W(0x203)+bytes([S['APP_OWNER_BASE']])
assert m[0x8401:0x8604]==bytes([0xa5])*0x203,'buffer was written'
registered=bytearray(m)
for name,hl,de,bc,error in [('duplicate',0x8401,0x203,0,S['MEM_ERROR_ACTIVE']),('partial overlap',0x8500,0x200,0,S['MEM_ERROR_ACTIVE']),('null base',0,1,0,S['MEM_ERROR_RANGE']),('empty',0x8700,0,0,S['MEM_ERROR_RANGE']),('overflow',0xff00,0x200,0,S['MEM_ERROR_RANGE']),('system',0x8bff,2,0,S['MEM_ERROR_RANGE']),('bank',0x8700,1,0x100,S['MEM_ERROR_RANGE'])]:
 result,flags,a,_=run(bytearray(registered),'dosMemoryService',hl=hl,de=de,bc=bc,a=S['MEM_OP_MAINREGISTER'])
 assert flags&1 and a==error,(name,a)
 assert result[table:table+192]==registered[table:table+192],name
m,flags,a,_=run(bytearray(registered),'dosMemoryService',hl=0x8604,de=0x20,a=S['MEM_OP_MAINREGISTER']);assert not flags&1,'adjacent'
m=bytearray(registered);m[S['memorySystemEnd']:S['memorySystemEnd']+2]=W(0x8000)
_,flags,a,_=run(m,'dosMemoryService',hl=0x8000,de=1,a=S['MEM_OP_MAINREGISTER']);assert flags&1 and a==S['MEM_ERROR_RANGE']
m=bytearray(registered);m[S['appOwner']]+=1
_,flags,a,_=run(m,'dosMemoryService',hl=0x8401,a=S['MEM_OP_FREE']);assert flags&1 and a==S['MEM_ERROR_OWNER']
m,flags,a,_=run(bytearray(registered),'dosMemoryService',hl=0x8401,a=S['MEM_OP_FREE']);assert not flags&1 and m[table+6]==255
# Normal owner teardown frees exact registrations through the same ledger.
m,_,_,_=run(bytearray(registered),'appReleaseOwner',bc=S['APP_OWNER_BASE'])
assert m[table]==255 and m[table+6]==255,'application cleanup'
# MAIN DLL unload also releases its extra buffers, preserving foreign records.
m=bytearray(registered);record=S['MODULE_TABLE'];owner=S['MODULE_OWNER_BASE']
m[table+11]=owner
m[record:record+2]=W(1);m[record+9]=owner
m,flags,a,_=run(m,'mdRelease',hl=record)
assert not flags&1 and m[table]!=255 and m[table+6]==255 and m[record:record+2]==W(0),'module cleanup'
# An ordinary exact buffer owned by EXEC/module also contributes to memTop.
m=seed();m[table:table+6]=bytes([255])*6;m[S['appOwner']]=0;m[S['moduleActor']]=S['MODULE_OWNER_BASE']
m,flags,a,_=run(m,'dosMemoryService',hl=0x8401,de=0x203,a=S['MEM_OP_MAINREGISTER']);assert not flags&1
m,_,_,top=run(m,'memoryMainCeiling',hl=0x8c00);assert top==0x8401,hex(top)
m,flags,a,_=run(m,'dosMemoryService',hl=0x8401,a=S['MEM_OP_FREE']);assert not flags&1
m,_,_,top=run(m,'memoryMainCeiling',hl=0x8c00);assert top==0x8c00
# Warm boot drops resident clients' MAIN buffers along with their stacks.
# The bank image, its other bank allocations and DOS-owned blocks survive.
m=seed();m[S['mdForgetRoute']]=0xc9
m[record:record+2]=W(1);m[record+2:record+4]=bytes([1,3])
m[record+6:record+8]=W(table+12);m[record+8:record+10]=bytes([S['DOS_IO_PAGE'],owner])
m[S['residentKeys']:S['residentKeys']+2]=W(S['NC_BANK_KEY'])
records = [(0,0x3380,0x3080,owner), (3,0x100,0x5000,255),
           (4,0x200,0x100,owner), (0,0x7800,0x100,S['DOS_IO_PAGE']),
           (0,0x7900,0x100,255)]
for index,(bank,base,size,tag) in enumerate(records,1):
 start=table+index*6
 m[start:start+6]=bytes([bank])+W(base)+W(size)+bytes([tag])
before=bytes(m[table:table+36])
m,_,_,_=run(m,'moduleWarm')
assert m[table]==m[table+6]==m[table+30]==255,'abandoned MAIN buffers'
assert m[table+12:table+30]==before[12:30],'live bank image and system allocation'
assert m[record:record+4]==W(1)+bytes([0,3]),'resident handle survives Reset'
# An overlap only with the registered tail still retires the MAIN parent.
for overlap in [False,True]:
 m=bytearray(registered);m[S['mmcStart']:S['mmcStart']+2]=W(0x8500 if overlap else 0x6000);m[S['mmcEnd']:S['mmcEnd']+2]=W(0x8550 if overlap else 0x6100)
 m,flags,a,_=run(m,'keRetireMain')
 assert (m[table]==255)==overlap and (m[table+6]==255)==overlap
 assert int.from_bytes(m[S['appImage']:S['appImage']+2],'little')==(0 if overlap else table)
# MODULE_LOAD is not a destructive handoff and must reject the live buffer.
m=bytearray(registered);h=S['v_imageHeader'];m[h+S['IMAGE_POLICY']]=S['IMAGE_POLICY_MANAGED'];m[h+S['IMAGE_MEMORY_BEGIN']:h+S['IMAGE_MEMORY_BEGIN']+2]=W(0x8500);m[h+S['IMAGE_MEMORY_END']:h+S['IMAGE_MEMORY_END']+2]=W(0x8550)
m,flags,a,_=run(m,'moduleMainClaimStrict');assert flags&1 and a==S['MEM_ERROR_ACTIVE'];assert m[table:table+12]==registered[table:table+12]
# Full registry cannot partially publish a reservation.
m=seed();m[table:table+192]=(b'\x02'+W(0x1000)+W(1)+b'\x30')*32
before=bytes(m[table:table+192]);m,flags,a,_=run(m,'dosMemoryService',hl=0x8400,de=10,a=S['MEM_OP_MAINREGISTER']);assert flags&1 and a==S['MEM_ERROR_NOMEM'];assert m[table:table+192]==before
print('MAINREGISTER PASS: exact/odd bounds, no writes, atomic errors, owner/FREE, memTop, tail handoff, strict MODULE_LOAD')
