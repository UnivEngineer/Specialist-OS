"""8080: проверка заголовков, выбора места и транзакций в аренах MAIN/DOS."""
from pathlib import Path
from dos_state import initialize_dos_state
import argparse,re,subprocess,sys,struct
R=Path(__file__).resolve().parents[2];O=R/'.local/image-memory';O.mkdir(exist_ok=True)
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--kernel',type=Path,default=R/'bin/banked/KERNEL.bin')
binary=parser.parse_args().kernel
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',binary.with_suffix('.sym').read_text('utf-8'),re.M)}
sys.path.insert(0,str(R/'scripts/images'));from executable import Image,encode
W=lambda v:struct.pack('<H',v)
def seed():
 m=bytearray(65536);p=binary.read_bytes();m[0x100:0x100+len(p)]=p
 initialize_dos_state(m, s)
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
  m=seed();lo=0x3000 if kind=='main' else s['DOS_ARENA_START'];hi=lo+0x200
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
 m=seed();lo=0x3000 if kind=='main' else s['DOS_ARENA_START'];record(m,0,kind=='dos',lo,lo+0x100);record(m,1,kind=='dos',lo+0x100,lo+0x200,active=1)
 h=encode(Image(b'\xc9',lo,lo,4,memory_end=lo+0x300,policy=2),'dll')[:32];m[s['v_imageHeader']:s['v_imageHeader']+32]=h
 q,e,_=run(m,'moduleMainClaimStrict' if kind=='main' else 'dosArenaImage',hl=lo,de=lo+0x300);assert e and q[s['MODULE_TABLE']:s['MODULE_TABLE']+2]==W(1),(kind,'atomic refusal');count+=1
# A cached inactive parent cannot evict an active child, even in another arena.
for kind in ('main','dos'):
 for depth in (1,2):
  m=seed();lo=0x3000 if kind=='main' else s['DOS_ARENA_START'];hi=lo+0x100
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
for at,value in [(27,16),(29,1),(22,4),(26,5),(23,1),(28,2)]:
 m=seed();h=bytearray(good);h[at]=value;h[30:32]=W(checksum(h[:30]));m[s['v_imageHeader']:s['v_imageHeader']+32]=h
 q,e,_=run(m,'imageValidateHeader');assert e,(at,value);count+=1
# TRANSIENT still validates the fixed MAIN header and bypasses the allocation table.
m=seed()
h=encode(Image(b'\xc9',0,0,4,bss=128,policy=3),'com')[:32]
m[s['v_imageHeader']:s['v_imageHeader']+32]=h
q,e,_=run(m,'imageValidateHeader');assert not e
q,e,_=run(m,'keReserveImage');assert not e and q[s['keImageToken']:s['keImageToken']+2]==W(0)
assert q[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+192]==m[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+192]
count+=1
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
m=seed();m[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+6]=b'\6'+W(0)+W(s['MEM_DATA_LIMIT'])+b'\xff'
q,e,_=run(m,'moduleWarm');assert q[s['MEM_ALLOC_TABLE']]==255;count+=1
# Application frames restore the caller's arena/image/owner after a nested RET.
m=seed();m[s['appDepth']]=1;m[s['appOwner']]=0x40;m[s['moduleActor']]=0x83;m[s['memoryMainOwner']]=0x40
for field,value in [('memoryMainStart',0x3000),('memoryMainEnd',0x8c00),('appImage',s['MEM_ALLOC_TABLE'])]:m[s[field]:s[field]+2]=W(value)
q,e,_=run(m,'appEnter');assert not e and q[s['appDepth']]==2
q,e,_=run(bytearray(q),'appLeave');assert q[s['appOwner']]==0x40 and q[s['moduleActor']]==0x83 and q[s['appDepth']]==1
for field,value in [('memoryMainStart',0x3000),('memoryMainEnd',0x8c00),('appImage',s['MEM_ALLOC_TABLE'])]:assert q[s[field]:s[field]+2]==W(value),field
count+=1

def place(m, routine, span, mask, expected, **regs):
 before=bytes(m[s['MEM_ALLOC_TABLE']:s['MODULE_TABLE']+8*16])
 q,failed,code=run(m,routine,bc=span,hl=mask,**regs)
 assert q[s['MEM_ALLOC_TABLE']:s['MODULE_TABLE']+8*16]==before,(routine,'изменена таблица на пробе')
 if expected is None:
  assert failed and code==s['MEM_ERROR_NOMEM'],(routine,span,mask,failed,code)
 else:
  assert not failed and q[0xe700:0xe702]==W(expected),(routine,hex(expected),q[0xe700:0xe702].hex())
 return q

def main_block(m, index, begin, end, owner):
 offset=s['MEM_ALLOC_TABLE']+index*6
 m[offset:offset+6]=b'\0'+W(begin)+W(end-begin)+bytes([owner])

def put(m, name, value):
 start=s[name]
 m[start:start+2]=W(value)

# Несмежные интервалы при верхнем поиске, две смежные занятые области
# и выравнивание со свободным хвостом под минимальную кучу.
m=seed();header(m);put(m,'memorySystemEnd',0x8013)
record(m,0,0,0x7d80,0x7e80,active=1,cache=0)
main_block(m,0,0x7e80,0x8013,0x40)
place(m,'relocMainHole',0x240,0xff,0x7b00);count+=1

# Не занятый записью FAR минимум активных родителей защищён и во вложенном вызове.
m=seed();header(m);put(m,'memorySystemEnd',0x8000);m[s['appDepth']]=2
m[s['appHeapBounds']:s['appHeapBounds']+8]=W(0x8000)+W(0x7f00)+W(0x7f00)+W(0x7e80)
place(m,'relocMainHole',0x100,0xff,0x7d00);count+=1

# Суммарно свободно 0440h, но нет выровненного интервала 0201h.
m=seed();header(m);put(m,'memorySystemEnd',0x650)
for i,(lo,hi) in enumerate([(0,0x50),(0x210,0x310),(0x510,0x5d0)]):
 main_block(m,i,lo,hi,0x40)
place(m,'relocMainHole',0x201,0xff,None);count+=1

# Без вытеснения сначала ищется место ниже кэша. Только при его отсутствии
# разрешается наложение на неактивный CACHEABLE; сама проба ничего не выгружает.
for target in ('main','dos'):
 m=seed();header(m)
 if target=='main':
  low,high=0,0x500;put(m,'memorySystemEnd',high);routine='relocMainHole';bank=0
 else:
  low,high=s['DOS_ARENA_START'],s['DOS_BANK_MODULE_END'];routine='relocDosHole';bank=1
 record(m,0,bank,high-0x200,high,cache=1)
 place(m,routine,0x100,0,high-0x300);count+=1
 record(m,1,bank,low,high-0x200,active=1,cache=0)
 place(m,routine,0x100,0,high-0x100);count+=1
 m[s['v_imageHeader']+s['IMAGE_POLICY']]=s['IMAGE_POLICY_PRESERVE']
 place(m,routine,0x100,0,None);count+=1

# Буфер BAT выбирают только после поиска свободного места за его пределами.
m=seed();header(m);top=s['DOS_BANK_MODULE_END'];bottom=s['DOS_ARENA_START']
put(m,'batWindowEnd',top)
at=s['BAT_HANDLE']+s['FH_BUFFER'];m[at:at+2]=W(top-0x200)
place(m,'relocDosHole',0x100,0,top-0x300)
record(m,0,1,bottom,top-0x200,active=1,cache=0)
q=place(m,'relocDosHole',0x100,0,top-0x100)
assert q[s['batWindowEnd']:s['batWindowEnd']+2]==W(top);count+=2

# Разрешённое при пробе вытеснение действительно освобождает minHeap,
# даже если с кэшем пересекается только продолжение за концом тела.
m=seed();header(m);m[s['v_imageHeader']+4]=3
at=s['v_imageHeader']+s['IMAGE_MIN_HEAP'];m[at:at+2]=W(0x80)
record(m,0,1,top-0x80,top,cache=1)
q,error,_=run(m,'imageClaimDos',hl=top-0x100,de=top-0x80)
assert not error and q[s['MODULE_TABLE']:s['MODULE_TABLE']+2]==W(0)
count+=1

# Отказ до вытеснения при полной таблице образов или отсутствии записи для MAIN.
for routine in ('mdPlacementDispatch','mdMain'):
 m=seed();header(m)
 record(m,0,0,0x4000,0x4100,cache=1)
 if routine=='mdPlacementDispatch':
  for index in range(1,8):record(m,index,6)
 else:
  for index in range(32):
   main_block(m,index,0x2000+2*index,0x2001+2*index,0x40)
  put(m,'mdMemoryStart',0x4000);put(m,'mdMemoryEnd',0x4100)
 m[s['kdError']]=0xc9
 before=bytes(m[s['MEM_ALLOC_TABLE']:s['MODULE_TABLE']+8*16])
 q,failed,code=run(m,routine)
 assert failed and code==s['MEM_ERROR_NOMEM']
 assert q[s['MEM_ALLOC_TABLE']:s['MODULE_TABLE']+8*16]==before
 count+=1

# Прежний диапазон vacant должен быть целиком допустим. При конфликте той
# же DLL назначается другая база и снимается признак тёплой активации.
m=seed();descriptor=s['v_tmpFileDescr'];head=s['v_imageHeader']
raw=encode(Image(b'\xc9',4,4,4,bss=0,memory_begin=0,memory_end=0x100,
                 reloc_kind=0,align_log2=8,options=3),'dll')[:40]
m[head:head+40]=raw;m[descriptor:descriptor+16]=raw[:16]
put(m,'memorySystemEnd',0x8000)
record(m,0,0,0x7200,0x7300,active=1);record_at=s['MODULE_TABLE'];m[record_at:record_at+2]=W(0)
put(m,'mdRecord',record_at);m[s['mdReuseHit']]=1
for block,expected,warm in [(False,0x7200,1),(True,0x7f00,0)]:
 case=bytearray(m)
 if block:main_block(case,0,0x7280,0x7400,0x40)
 q,failed,_=run(case,'imagePlace',a=s['MODULE_TARGET_MAIN'])
 assert not failed and q[head+16:head+20]==W(expected)+W(expected+0x100)
 assert q[descriptor+6:descriptor+10]==W(expected+4)*2 and q[s['mdReuseHit']]==warm
 count+=1

# Выбор прежней базы проверяет перенос адреса и маску до обхода карты владельцев.
for old,end,span,mask in [(0x7180,0x8000,0x100,255),(0xff00,0xffff,0x200,255),
                           (0x7f00,0x7fff,0x100,255)]:
 case=bytearray(m);put(case,'memorySystemEnd',end)
 q,failed,_=run(case,'relocMainProbeReuse',de=old,bc=span,hl=mask)
 assert failed,(old,end,span,mask)
 count+=1

# При вложенном открытии сохраняются обе части v3, каталог и курсор FAT.
case=bytearray(m);hdr=s['v_imageHeader'];descriptor=s['v_tmpFileDescr']
case[descriptor:descriptor+32]=bytes(range(32))
case[s['fileSelectDrive']]=0xc9
q,_,_=run(case,'imageSaveContext')
modified=bytearray(q);modified[hdr:hdr+40]=bytes(40);modified[descriptor:descriptor+32]=bytes(32)
q,_,_=run(modified,'imageRestoreContext')
assert q[hdr:hdr+40]==case[hdr:hdr+40] and q[descriptor:descriptor+32]==case[descriptor:descriptor+32]
count+=1

# В момент Reset после установки EXEC, но до публикации handle, профиль
# страницы возвращается в DATA, резерв снимается, расчёт суммы отключается.
m=seed();put(m,'mdRecord',s['MODULE_TABLE']);m[s['mdLoadingBank']]=6;m[s['kiSumActive']]=1
at=s['mdRestoreData'];m[at:at+4]=b'\x32'+W(0xe710)+b'\xc9'
m[s['MEM_ALLOC_TABLE']:s['MEM_ALLOC_TABLE']+6]=b'\x06'+W(0)+W(s['MEM_DATA_LIMIT'])+b'\xff'
q,failed,_=run(m,'moduleWarm')
assert not failed and q[0xe710]==6 and q[s['mdLoadingBank']]==q[s['kiSumActive']]==0
assert q[s['MEM_ALLOC_TABLE']]==255;count+=1

print('IMAGE MEMORY PASS:',count,'cases; policies, fragmentation, alignment, repeat activation, Reset')
