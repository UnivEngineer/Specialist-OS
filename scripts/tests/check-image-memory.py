"""8080: v2 image validation and transactional MAIN/DOS arena policies."""
from pathlib import Path
import re,subprocess,sys,struct
R=Path(__file__).resolve().parents[2];O=R/'.local/image-memory';O.mkdir(exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/banked/KERNEL.sym').read_text('utf-8'),re.M)}
sys.path.insert(0,str(R/'scripts/images'));from executable import Image,encode
W=lambda v:struct.pack('<H',v)
def seed():
 m=bytearray(65536);p=(R/'bin/DOS.sys').read_bytes();m[0x100:0x100+len(p)]=p
 m[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+32*6]=b'\xff'*192
 m[s['MODULE_TABLE']:s['MODULE_TABLE']+8*16]=bytes(128)
 for name in ('klReclaim','fileopsReleaseOwner','enumReleaseOwner','memoryPublishTop'):m[s[name]:s[name]+2]=b'\xaf\xc9'
 # A flat 8080 image has no bank window to clear MEM route state through.
 m[s['mdForgetRoute']]=0xc9
 m[s['residentKeys']:s['residentKeys']+16]=bytes(16)
 return m
def run(m,label,hl=0,bc=0,de=0,a=0):
 code=b'\x31'+W(0xef70)+b'\x21'+W(hl)+b'\x01'+W(bc)+b'\x11'+W(de)+b'\x3e'+bytes([a])+b'\xcd'+W(s[label])+b'\x22'+W(0xe700)+b'\xc3'+W(0xe400)
 m[0xe000:0xe000+len(code)]=code;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','e000','-end','e400','-counter','30000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=15)
 q=(O/'out.bin').read_bytes();assert q[65542:65546]==W(0xe400)+W(0xef70),(label,'PC/SP',q[65542:65546].hex())
 return q[:65536],bool(q[65536]&1),q[65537]
def record(m,i,bank=0,lo=0x3000,hi=0x5000,active=0,cache=1):
 p=s['MODULE_TABLE']+i*16;m[p:p+16]=W(i+1)+bytes([active,bank])+W(lo)+W(0)+bytes([0,0x80+i])+W(lo)+W(hi)+bytes([cache,2])
def header(m,policy=2,lo=0x4000,hi=0x4100):
 h=encode(Image(b'\xc9',0x4000,0x4000,4,bss=0,memory_begin=lo,memory_end=hi,policy=policy),'dll')[:32]
 m[s['v_imageHeader']:s['v_imageHeader']+32]=h
 return h
count=0
for kind in ('main','dos'):
 for name,policy,active,cache,overlap,error in [('managed-cache',2,0,1,True,False),('managed-pinned',2,0,0,True,True),('preserve',1,0,1,True,True),('active',2,1,1,True,True),('adjacent',2,1,1,False,False)]:
  m=seed();lo=0x3000 if kind=='main' else s['DOS_SERVICE_ADDR'];hi=lo+0x200
  record(m,0,0 if kind=='main' else 1,lo,hi,active,cache)
  h=header(m,policy,0x4000,0x4100)
  if kind=='main':
   # Header stores the claim; construct it for this exact region.
   h=encode(Image(b'\xc9',lo,lo,4,bss=0,memory_end=hi if overlap else lo+0x300,policy=policy),'dll')[:32]
   if not overlap:h=encode(Image(b'\xc9',hi,hi,4,bss=0,policy=policy),'dll')[:32]
   m[s['v_imageHeader']:s['v_imageHeader']+32]=h;entry='moduleMainClaimStrict';kw={}
  else:entry='dosArenaImage';kw=dict(hl=lo+0x100 if overlap else hi,de=hi+0x100)
  q,e,_=run(m,entry,**kw);assert e==error,(kind,name,e)
  assert q[s['v_imageHeader']:s['v_imageHeader']+32]==h,(kind,name,'header changed')
  removed=not error and overlap;assert (q[s['MODULE_TABLE']:s['MODULE_TABLE']+2]==W(0))==removed,(kind,name,'record')
  count+=1
 # A later active conflict must leave the first evictable object untouched.
 m=seed();lo=0x3000 if kind=='main' else s['DOS_SERVICE_ADDR'];record(m,0,kind=='dos',lo,lo+0x100);record(m,1,kind=='dos',lo+0x100,lo+0x200,active=1)
 h=encode(Image(b'\xc9',lo,lo,4,memory_end=lo+0x300,policy=2),'dll')[:32];m[s['v_imageHeader']:s['v_imageHeader']+32]=h
 q,e,_=run(m,'moduleMainClaimStrict' if kind=='main' else 'dosArenaImage',hl=lo,de=lo+0x300);assert e and q[s['MODULE_TABLE']:s['MODULE_TABLE']+2]==W(1),(kind,'atomic refusal');count+=1
# A cached inactive parent cannot evict an active child, even in another arena.
for kind in ('main','dos'):
 for depth in (1,2):
  m=seed();lo=0x3000 if kind=='main' else s['DOS_SERVICE_ADDR'];hi=lo+0x100
  record(m,0,0 if kind=='main' else 1,lo,hi)
  for i in range(1,depth+1):
   record(m,i,6,0x4000,0x4100,active=int(i==depth))
   m[s['MODULE_TABLE']+i*16+8]=0x80+i-1
  h=encode(Image(b'\xc9',lo,lo,4,memory_end=hi,policy=2),'dll')[:32]
  m[s['v_imageHeader']:s['v_imageHeader']+32]=h
  before=bytes(m[s['MODULE_TABLE']:s['MODULE_TABLE']+128])
  q,e,_=run(m,'moduleMainClaimStrict' if kind=='main' else 'dosArenaImage',hl=lo,de=hi)
  assert e and q[s['MODULE_TABLE']:s['MODULE_TABLE']+128]==before,(kind,depth,'active descendant')
  q,e,_=run(m,'mdRelease',hl=s['MODULE_TABLE'])
  assert e and q[s['MODULE_TABLE']:s['MODULE_TABLE']+128]==before,(kind,depth,'direct unload')
  count+=2
# Header checksum covers all 32 bytes; reserved/mode/policy rejected even when checksum is recomputed.
m=seed();good=header(m);q,e,_=run(m,'imageValidateHeader');assert not e;count+=1
from executable import checksum
for at,value in [(27,16),(29,1),(22,3),(26,5),(23,1),(28,2)]:
 m=seed();h=bytearray(good);h[at]=value;h[30:32]=W(checksum(h[:30]));m[s['v_imageHeader']:s['v_imageHeader']+32]=h
 q,e,_=run(m,'imageValidateHeader');assert e,(at,value);count+=1
# Fixed internal MAIN reservations can begin at zero without public NULL ambiguity.
m=seed();q,e,_=run(m,'memoryReserveMain',hl=0,de=0x1234);assert not e and q[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+6]==b'\0\0\0\x34\x12\xff';count+=1
# No loader-side zero fill: BSS is an owned interval, initialized by startup.
m=seed();m[s['v_tmpFileDescr']+8:s['v_tmpFileDescr']+16]=W(0x4000)+W(1)+W(0xc9)+W(16)
m[0x4001:0x4011]=b'\xa5'*16
q,e,_=run(m,'imageCheckSum',bc=0xc9);assert not e and q[0x4001:0x4011]==b'\xa5'*16;count+=1
# Own static image and lower disjoint parents do not truncate the current heap.
for label,foreign,expected_top in [('lower',0x1000,0x8c00),('upper',0x6000,0x6000)]:
 m=seed();m[s['appOwner']]=0x40;m[s['memoryMainStart']:s['memoryMainStart']+2]=W(0x3000)
 m[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+6]=b'\0'+W(0)+W(0x2000)+b'\x40'
 m[s['MEM_ALLOC_TABLE']+6:s['MEM_ALLOC_TABLE']+12]=b'\0'+W(foreign)+W(0x100)+b'\x02'
 q,e,_=run(m,'memoryMainCeiling',hl=0x8c00);assert q[0xe700:0xe702]==W(expected_top),(label,q[0xe700:0xe702].hex());count+=1
# Reset releases a private reservation which never reached publication.
m=seed();m[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+6]=b'\6'+W(0)+W(0xfed0)+b'\xff'
q,e,_=run(m,'moduleWarm');assert q[s['MEM_ALLOC_TABLE']]==255;count+=1
# Application frames restore the caller's arena/image/owner after a nested RET.
m=seed();m[s['appDepth']]=1;m[s['appOwner']]=0x40;m[s['moduleActor']]=0x83;m[s['memoryMainOwner']]=0x40
for field,value in [('memoryMainStart',0x3000),('memoryMainEnd',0x8c00),('appImage',s['MEM_ALLOC_TABLE'])]:m[s[field]:s[field]+2]=W(value)
q,e,_=run(m,'appEnter');assert not e and q[s['appDepth']]==2
q,e,_=run(bytearray(q),'appLeave');assert q[s['appOwner']]==0x40 and q[s['moduleActor']]==0x83 and q[s['appDepth']]==1
for field,value in [('memoryMainStart',0x3000),('memoryMainEnd',0x8c00),('appImage',s['MEM_ALLOC_TABLE'])]:assert q[s[field]:s[field]+2]==W(value),field
count+=1
print('IMAGE MEMORY PASS:',count,'cases; policies, atomic refusal, full header checksum, fixed zero-origin allocation')
