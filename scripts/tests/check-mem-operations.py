"""Границы и перекрытия публичных операций MEM в CPU 8080."""
from pathlib import Path
import runpy,re,subprocess,struct,json
R=Path(__file__).resolve().parents[2];O=R/'docs/banked/memory-054/cpu';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
base=bytearray(65536);runpy.run_path(str(R/'scripts/tests/mem-image.py'))['install'](base,s)
def case(name,code,prepare,check):
 m=bytearray(base);prepare(m)
 asm=f' INCLUDE "{(R/"sdk/asm/mxos.inc").as_posix()}"\n ORG 08000h\n ld sp,08FDFh\n'+code+'\n ld (08502h),hl\n push af\n pop hl\n ld (08500h),hl\n ld h,b\n ld l,c\n ld (08504h),hl\n ex de,hl\n ld (08506h),hl\n jp 08100h\n END\n'
 (O/'case.asm').write_text(asm,encoding='utf-8');subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080',f'--raw={O/"case.bin"}',str(O/'case.asm')],check=True,capture_output=True)
 c=(O/'case.bin').read_bytes();m[0x8000:0x8000+len(c)]=c;(O/'in.bin').write_bytes(m)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8100','-counter','10000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,text=True,check=True)
 out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x8100,name
 assert int.from_bytes(out[65544:65546],'little')==0x8fdf,name
 check(out);return dict(case=name,cycles=int(p.stdout.strip()))
rows=[]
for src,dst,n in [(0x3000,0x3001,257),(0x3001,0x3000,257),(0x3000,0x3000,256),(0x3000,0x3400,1),(0x3000,0x3400,0)]:
 initial=bytes((i*37)&255 for i in range(2048));expected=bytearray(initial);expected[dst-0x3000:dst-0x3000+n]=initial[src-0x3000:src-0x3000+n]
 def prep(m):m[0x3000:0x3800]=initial;m[0x6000:0x6008]=struct.pack('<BHBHH',0,src,0,dst,n)
 def check(m):assert m[0x3000:0x3800]==expected;assert not m[0x8500]&1
 rows.append(case(f'move-{src:x}-{dst:x}-{n}', ' ld hl,06000h\n ld a,MEM_OP_MOVE\n call mx_memory',prep,check))
initial=bytes(((i*37)^(i>>5))&255 for i in range(2048))
move=' ld hl,06000h\n ld bc,01234h\n ld de,05678h\n ld a,MEM_OP_MOVE\n call mx_memory'
for src,dst in ((0,1),(1,0)):
 for length in (1,2,3,15,16,17,255,256,257,1019):
  expected=bytearray(initial)
  expected[dst:dst+length]=initial[src:src+length]
  def prep(m):
   m[0x3000:0x3800]=initial
   struct.pack_into('<BHBHH',m,0x6000,0,0x3000+src,0,0x3000+dst,length)
  def check(m):
   assert m[0x3000:0x3800]==expected,(src,dst,length)
   assert m[0x8500]&1==0 and m[0x8501]==0
   assert struct.unpack_from('<HHH',m,0x8502)==(0x6000,0x1234,0x5678)
  rows.append(case(f'exec-move-{src}-{dst}-{length}',move,prep,check))
for length in (0,17):
 def prep(m):
  m[0x3000:0x3800]=initial
  struct.pack_into('<BHBHH',m,0x6000,0,0x3000,0,0x3001,length)
 def check(m):
  expected=bytearray(initial)
  expected[1:1+length]=initial[:length]
  assert m[0x3000:0x3800]==expected
  assert not m[0x8500]&1 and m[0x8501]==0
  assert struct.unpack_from('<HHH',m,0x8502)==(0x6000,0x1234,0x5678)
 rows.append(case(f'far-overlap-{length}', ' ld hl,06000h\n ld bc,01234h\n ld de,05678h\n call mx_memcpyFar',prep,check))
for op in ('GET8','GET16','PUT8','PUT16'):
 def prep(m):m[0x3000:0x3003]=bytes.fromhex('123456')
 def check(m):
  assert not m[0x8500]&1
  assert struct.unpack_from('<HH',m,0x8502)==(0x3000,0x004e)
  if op=='GET8':assert m[0x8510:0x8512]==bytes.fromhex('1200')
  if op=='GET16':assert m[0x8510:0x8512]==bytes.fromhex('1234')
  if op=='PUT8':assert m[0x3000:0x3003]==bytes.fromhex('cd3456')
  if op=='PUT16':assert m[0x3000:0x3003]==bytes.fromhex('cdab56')
 rows.append(case(op, f' ld hl,03000h\n ld bc,04Eh\n ld de,0ABCDh\n ld a,MEM_OP_{op}\n call mx_memory\n ex de,hl\n ld (08510h),hl\n ex de,hl',prep,check))
# Банк и безопасный диапазон GET/PUT обеспечивает клиент. Ошибки границ
# проверяем через MOVE, где они по-прежнему входят в публичный контракт.
for src_page,src,dst,size,error in [(17,0x3000,0x3400,1,0xe0),
                                    (0,0xfeff,0x3400,2,0xe2),
                                    (0,0x3000,0xfeff,2,0xe2),
                                    (0,0xfff0,0x3400,32,0xe2)]:
 def prep(m):
  m[0x6000:0x6008]=struct.pack('<BHBHH',src_page,src,0,dst,size)
  m[0x3000:0x3020]=b'\xa5'*32
  if dst==0x3400:m[dst:dst+size]=b'\x5a'*size
 def check(m):
  assert m[0x8500]&1 and m[0x8501]==error
  expected=b'\x5a'*size if dst==0x3400 else base[dst:dst+size]
  assert m[dst:dst+size]==expected
 rows.append(case(f'move-invalid-{src_page}-{src:x}-{dst:x}-{size}',
                  ' ld hl,06000h\n ld a,MEM_OP_MOVE\n call mx_memory',prep,check))
(O/'result.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows))
