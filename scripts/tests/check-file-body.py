"""8080: общий загрузчик тела, размеры контейнера, хвост и защита BIOS/запроса."""
from pathlib import Path
import json,re,subprocess,struct
ROOT=Path(__file__).resolve().parents[2];OUT=ROOT/'docs/run-body-041/cpu';OUT.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/BIOS.sym').read_text(),re.M)}
asm=OUT/'stream.asm';asm.write_text(''' ORG 07000h
stream:
 ex de,hl
 ld (cursor+1),hl
 ex de,hl
 push bc
 push hl
 ex de,hl
 inc hl
 inc hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 ld hl,06000h
 add hl,de
 pop de
 ld a,b
 or c
 jp z,done
loop:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec bc
 ld a,b
 or c
 jp nz,loop
done:
 ex de,hl
 pop bc
 push hl
cursor:
 ld hl,0
 inc hl
 inc hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 ex de,hl
 add hl,bc
 ex de,hl
 ld (hl),d
 dec hl
 ld (hl),e
 pop hl
 xor a
 ret
''',encoding='utf-8')
subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(OUT/'stream.bin'),str(asm)],capture_output=True,check=True)
base=bytearray(65536);bios=(ROOT/'bin/BIOS.sys').read_bytes();base[0xc000:0xc000+len(bios)]=bios
io=(ROOT/'bin/IO.sys').read_bytes();base[0xf000:0xf000+len(io)]=io
mock=(OUT/'stream.bin').read_bytes();base[0x7000:0x7000+len(mock)]=mock;base[s['bios_fileStream']:s['bios_fileStream']+3]=bytes.fromhex('C3 00 70')
code=bytes.fromhex('31 DF 8F 21 00 30 CD')+struct.pack('<H',s['fileBodyLoad'])+bytes.fromhex('C3 00 21')
base[0x2000:0x2000+len(code)]=code;rows=[]
for header,tail,count,dest,error,trailer in [(h,t,n,0x1000,False,0x5000) for h,t in [(0,0),(16,0),(4,2)] for n in (0,1,255,256,257,1025)]+[(4,2,257,0x1000,False,0),(0,0,1025,0xe600,False,0),(16,2,-1,0x1000,True,0),(0,0,2,0xbfff,True,0),(0,0,1,0x3000,True,0)]:
 m=bytearray(base);size=header+tail+count;body=bytes((i*19+3)&255 for i in range(max(count,0)));source=bytes([0xcc])*header+body+bytes([0xab])*tail
 m[0x6000:0x6000+len(source)]=source;req=bytearray(40);struct.pack_into('<H',req,26,2);struct.pack_into('<I',req,28,size);struct.pack_into('<HBBHH',req,32,dest,header,tail,trailer,0xffff);m[0x3000:0x3028]=req
 (OUT/'input.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','2000','-end','2100','-counter','2000000','-output',str(OUT/'output.bin'),str(OUT/'input.bin')],capture_output=True,check=True)
 got=(OUT/'output.bin').read_bytes();assert int.from_bytes(got[65542:65544],'little')==0x2100
 assert bool(got[65536]&1)==error,(header,tail,count,hex(dest),got[65536:65538].hex())
 if error:assert got[65537]==7 and got[0x3026:0x3028]==b'\0\0'
 else:
  assert got[dest:dest+count]==body
  assert int.from_bytes(got[0x3026:0x3028],'little')==count
  if tail and trailer:assert got[trailer:trailer+tail]==bytes([0xab])*tail
 rows.append(dict(header=header,tail=tail,body=count,destination=dest,rejected=error))
(OUT/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print('FILE BODY PASS',len(rows),'cases')
