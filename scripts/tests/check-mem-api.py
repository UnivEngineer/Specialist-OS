"""Публичный MEM ABI и цена повторных копий в CPU 8080 (без маппера)."""
from pathlib import Path
import runpy,re,struct,subprocess,json
R=Path(__file__).resolve().parents[2];O=R/'docs/banked/memory-054/copy';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
s=sy(R/'bin/BIOS.sym');t=sy(R/'bin/banked/TRANSPORT.sym');base=bytearray(65536)
runpy.run_path(str(R/'scripts/tests/mem-image.py'))['install'](base,s)
payload=bytes((i*17+11)&255 for i in range(256));base[0x3000:0x3100]=payload
rows=[]
for label,size,bank,force,copies,damage in [('lazy',256,0,False,32,False),('restore-each',256,0,True,32,False),('zero',0,255,False,1,False),('bad-bank',1,17,False,1,False),('bad-range',257,0,False,1,False),('damaged-guard',256,0,False,1,True)]:
 m=bytearray(base);m[0x6000:0x6008]=struct.pack('<BHBHH',0,0xfe00 if label=='bad-range' else 0x3000,bank,0x4000,size)
 if damage:m[0xff00:0xff04]=b'\0'*4
 body=f''' INCLUDE "{(R/'source/include/mxos.inc').as_posix()}"
 ORG 08000h
 ld sp,08FDFh
 call mx_memInfo
 ld (08500h),hl
 ld (08502h),a
 ld a,b
 ld (08503h),a
 ld a,c
 ld (08504h),a
 ld hl,06000h
 ld bc,{copies}
 ld de,05678h
loop:
 call mx_memcpyFar
 push af
 pop hl
 ld (08506h),hl
 ld hl,06000h
 {'call 0%04Xh'%t['bankCommandWindow'] if force else ''}
 dec c
 jp nz,loop
 ex de,hl
 ld (08508h),hl
 jp 08100h
 END
'''
 (O/'case.asm').write_text(body,encoding='utf-8');subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080',f'--raw={O/"case.bin"}',str(O/'case.asm')],check=True,capture_output=True)
 code=(O/'case.bin').read_bytes();m[0x8000:0x8000+len(code)]=code;(O/'input.bin').write_bytes(m)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8100','-counter','10000000','-output',str(O/'out.bin'),str(O/'input.bin')],check=True,capture_output=True,text=True)
 b=(O/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x8100,label
 assert b[0x8500:0x8505]==bytes.fromhex('0001001001'),(label,b[0x8500:0x8505].hex())
 error=0xe0 if label=='bad-bank' else 0xe2 if label=='bad-range' else 0
 assert b[0x8507]==error and bool(b[0x8506]&1)==bool(error),label
 assert b[0x8508:0x850a]==bytes.fromhex('7856'),label
 if size and not error:assert b[0x4000:0x4100]==payload,label
 assert b[0xff00:0xff04]==(b'\0'*4 if damage else b'\xff'*4),label
 assert b[0xff04:0xff9c]==base[0xff04:0xff9c],label
 assert b[0xffdf]==(0 if force or not size or error else 0),(label,b[0xffdf])
 cycles=int(p.stdout.strip());rows.append(dict(case=label,copies=copies,cycles=cycles,ms=cycles/2000))
(O/'checks.json').write_text(json.dumps(rows,indent=2)+'\n');print(json.dumps(rows))
