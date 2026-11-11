"""Аллокатор DOS: дробление, повторное использование, realloc и ошибки."""
from pathlib import Path
import runpy,re,subprocess,json
R=Path(__file__).resolve().parents[2];O=R/'docs/banked/memory-054/allocator';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
s=sy(R/'bin/BIOS.sym');k=sy(R/'bin/banked/KERNEL.sym');m=bytearray(65536);runpy.run_path(str(R/'scripts/tests/mem-image.py'))['install'](m,s);b=(R/'bin/DOS.sys').read_bytes();m[0x100:0x100+len(b)]=b
m[0x3040:0x3080]=bytes(range(64))
imports=''.join(f'{n} = 0{k[n]:04X}h\n' for n in ('memoryInit','dosMemoryService','kdPage','memoryRamPages'))
code=imports+f' INCLUDE "{(R/"source/include/mxos.inc").as_posix()}"\n'+"""
 ORG 08000h
 ld sp,08FDFh
 call memoryInit
 xor a
 ld (kdPage),a
 ld hl,03000h
 ld de,01000h
 ld b,0
 ld a,MEM_OP_ARENA
 call dosMemoryService
 call result
 ld hl,64
 ld b,0
 ld a,MEM_OP_ALLOC
 call dosMemoryService
 call result
 ld hl,64
 ld b,0
 ld a,MEM_OP_ALLOC
 call dosMemoryService
 call result
 ld hl,03000h
 ld b,0
 ld a,MEM_OP_FREE
 call dosMemoryService
 call result
 ld hl,16
 ld b,0
 ld a,MEM_OP_ALLOC
 call dosMemoryService
 call result
 ld hl,03040h
 ld de,128
 ld b,0
 ld a,MEM_OP_REALLOC
 call dosMemoryService
 call result
 ld hl,03080h
 ld de,32
 ld b,0
 ld a,MEM_OP_REALLOC
 call dosMemoryService
 call result
 ld hl,03080h
 ld de,02000h
 ld b,0
 ld a,MEM_OP_REALLOC
 call dosMemoryService
 call result
 ld hl,03080h
 ld b,0
 ld a,MEM_OP_FREE
 call dosMemoryService
 call result
 ld hl,03080h
 ld b,0
 ld a,MEM_OP_FREE
 call dosMemoryService
 call result
 ld hl,64
 ld b,MEM_ALLOC_ANY
 ld a,MEM_OP_ALLOC
 call dosMemoryService
 call result
 jp 08400h
result:
 push af
 push bc
 push de
 push hl
 ld hl,0
 add hl,sp
 ex de,hl
 ld hl,(pointer)
 ld b,8
save:
 ld a,(de)
 ld (hl),a
 inc de
 inc hl
 dec b
 jp nz,save
 ld (pointer),hl
 pop hl
 pop de
 pop bc
 pop af
 ret
pointer: DW 08500h
 END
"""
(O/'case.asm').write_text(code,encoding='utf-8');subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080',f'--raw={O/"case.bin"}',str(O/'case.asm')],check=True,capture_output=True)
c=(O/'case.bin').read_bytes();m[0x8000:0x8000+len(c)]=c;(O/'in.bin').write_bytes(m)
p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8400','-counter','10000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,text=True)
b=(O/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x8400
rows=[b[0x8500+i*8:0x8508+i*8] for i in range(11)]
for i,r in enumerate(rows): print(i,r.hex())
assert [int.from_bytes(rows[i][:2],'little') for i in (1,2,4,5,6)]==[0x3000,0x3040,0x3000,0x3080,0x3080]
assert b[0x3080:0x30a0]==bytes(range(32))
assert all(not rows[i][6]&1 for i in range(7))
assert rows[7][6]&1 and rows[7][7]==0xe3
assert not rows[8][6]&1
assert rows[9][6]&1 and rows[9][7]==0xe4
assert rows[10][6]&1 and rows[10][7]==0xe3
(O/'result.json').write_text(json.dumps([r.hex() for r in rows],indent=2)+'\n');print('PASS: 11 operations, data preserved')
