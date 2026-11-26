"""8080: NC fixed image BSS and variable catalog heap using actual DOS allocator."""
from pathlib import Path
import re,runpy,subprocess
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'.local/nc-main-memory';OUT.mkdir(exist_ok=True)
def sy(path):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',path.read_text(encoding='utf-8'),re.M)}
K=sy(ROOT/'bin/banked/KERNEL.sym');N=sy(ROOT/'bin/NC.sym');W=lambda n:int(n).to_bytes(2,'little')
assert len((ROOT/'bin/NC.i80').read_bytes())==N['vars']
assert N['IMAGE_BSS_SIZE']==N['FILE_CACHE_A']-N['vars']
assert N['IMAGE_MIN_HEAP_SIZE']==N['NC_MIN_END']-N['FILE_CACHE_A']
# Execute the real included NC initialization beside the DOS bank, not on top
# of it. Only the bank gateway and three MAIN BIOS arithmetic calls are stubs.
source=f' INCLUDE "{(ROOT/"source/common/system.inc").as_posix()}"\n'
for name in ['FILE_CACHE_A','NC_MIN_END','FILE_INFO_SIZE','FILE_CACHE_CAPACITY','FILE_CACHE_BYTES','NC_END']:source+=f'{name} EQU {N[name]}\n'
source+=' ORG 0E600h\n'
source+=f' INCLUDE "{(ROOT/"source/nc/memory.inc").as_posix()}"\n END\n'
(OUT/'case.asm').write_text(source,encoding='utf-8')
subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--raw={OUT}/case.bin',f'--sym={OUT}/case.sym',str(OUT/'case.asm')],check=True,capture_output=True)
S=sy(OUT/'case.sym')
helper=runpy.run_path(str(ROOT/'scripts/tests/check-main-buffer-registration.py'))
for top in [N['NC_MIN_END']-2,N['NC_MIN_END']-1,0x63ff,0x7bff,0x8bff]:
 m=helper['seed']();table=K['MEM_ALLOC_TABLE'];owner=K['APP_OWNER_BASE']
 m[table:table+6]=b'\0'+W(0)+W(N['FILE_CACHE_A'])+bytes([owner])
 m[K['memorySystemEnd']:K['memorySystemEnd']+2]=W(top+1)
 body=(OUT/'case.bin').read_bytes();m[0xe600:0xe600+len(body)]=body
 m[K['bios_getMemTop']:K['bios_getMemTop']+4]=b'\x21'+W(top)+b'\xc9'
 for entry,target in [('bios_cmp_hl_de','cmp_hl_de'),('bios_sub_hl_de','sub_hl_de')]:m[K[entry]:K[entry]+3]=b'\xc3'+W(K[target])
 m[K['mx_callRegs']:K['mx_callRegs']+7]=bytes.fromhex('e32323e3c3')+W(K['dosMemoryService'])
 # The helper runs actual assembled DOS and checks CPU/SP completion.
 helper['S']['ncMemoryTest']=S['ncMemoryInit']
 result,flags,a,_=helper['run'](m,'ncMemoryTest')
 if top<N['NC_MIN_END']-1:
  assert flags&1 and result[table+6]==255
  continue
 assert not flags&1,(hex(top),a)
 size=min(8192,(top+1-N['FILE_CACHE_A'])&~15)
 assert result[S['ncCatalogPoolSize']:S['ncCatalogPoolSize']+2]==W(size)
 assert result[table+6:table+12]==b'\0'+W(N['FILE_CACHE_A'])+W(size)+bytes([owner])
 assert result[table:table+6]==m[table:table+6],'static image reservation changed'
 assert result[S['ncMemoryEnd']:S['ncMemoryEnd']+2]==W(N['FILE_CACHE_A']+size)
print('NC MAIN memory PASS: image BSS, raw file size, minimum heap, two-slot pool boundaries, actual ARENA/ALLOC records')
