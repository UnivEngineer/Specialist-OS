"""Границы и перекрытия публичных операций MEM в CPU 8080."""
from pathlib import Path
import runpy,re,subprocess,struct,json
R=Path(__file__).resolve().parents[2];O=R/'docs/banked/memory-054/cpu';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
base=bytearray(65536);runpy.run_path(str(R/'scripts/tests/mem-image.py'))['install'](base,s)
def case(name,code,prepare,check):
 m=bytearray(base);prepare(m)
 asm=f' INCLUDE "{(R/"source/include/mxos.inc").as_posix()}"\n ORG 08000h\n ld sp,08FDFh\n'+code+'\n push af\n pop hl\n ld (08500h),hl\n jp 08100h\n END\n'
 (O/'case.asm').write_text(asm,encoding='utf-8');subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080',f'--raw={O/"case.bin"}',str(O/'case.asm')],check=True,capture_output=True)
 c=(O/'case.bin').read_bytes();m[0x8000:0x8000+len(c)]=c;(O/'in.bin').write_bytes(m)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8100','-counter','10000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,text=True,check=True)
 out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x8100,name
 check(out);return dict(case=name,cycles=int(p.stdout.strip()))
rows=[]
for src,dst,n in [(0x3000,0x3001,257),(0x3001,0x3000,257),(0x3000,0x3000,256),(0x3000,0x3400,1),(0x3000,0x3400,0)]:
 initial=bytes((i*37)&255 for i in range(2048));expected=bytearray(initial);expected[dst-0x3000:dst-0x3000+n]=initial[src-0x3000:src-0x3000+n]
 def prep(m):m[0x3000:0x3800]=initial;m[0x6000:0x6008]=struct.pack('<BHBHH',0,src,0,dst,n)
 def check(m):assert m[0x3000:0x3800]==expected;assert not m[0x8500]&1
 rows.append(case(f'move-{src:x}-{dst:x}-{n}', ' ld hl,06000h\n ld a,MEM_OP_MOVE\n call mx_memory',prep,check))
for op in ('GET8','GET16','PUT8','PUT16'):
 def prep(m):m[0x3000:0x3003]=bytes.fromhex('123456')
 def check(m):
  assert not m[0x8500]&1
  if op=='GET8':assert m[0x8510:0x8512]==bytes.fromhex('1200')
  if op=='GET16':assert m[0x8510:0x8512]==bytes.fromhex('1234')
  if op=='PUT8':assert m[0x3000:0x3003]==bytes.fromhex('cd3456')
  if op=='PUT16':assert m[0x3000:0x3003]==bytes.fromhex('cdab56')
 rows.append(case(op, f' ld hl,03000h\n ld b,0\n ld de,0ABCDh\n ld a,MEM_OP_{op}\n call mx_memory\n push af\n ex de,hl\n ld (08510h),hl\n pop af',prep,check))
for op in ('GET16','PUT16'):
 def check(m):assert m[0x8500]&1 and m[0x8501]==0xe2
 rows.append(case(op+'-boundary',f' ld hl,0FEFFh\n ld b,0\n ld a,MEM_OP_{op}\n call mx_memory',lambda m:None,check))
(O/'result.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows))
