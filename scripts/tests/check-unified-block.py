"""Единый вход B=5: MAIN/DOS/BANK, A:/B:/H:, ошибки и сохранение регистров."""
import json,re,struct,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/unified-b5-2026-11-11/cpu';O.mkdir(parents=True,exist_ok=True)
W=lambda x:struct.pack('<H',x)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/banked/CLIENT.sym').read_text(),re.M)}
source='''    ORG 08000h
    push bc
    push de
    push hl
    ld a,b
    cp 1
    jp nz,read
    ld a,(08100h)
    cp 1
    jp z,failed
    ld de,07000h
    jp copy
read:
    ld a,(08100h)
    cp 2
    jp z,failed
    ex de,hl
    ld hl,07000h
copy:
    ld bc,256
loop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,loop
    ld a,(08100h)
    cp 3
    jp nz,success
    ld hl,070FFh
    inc (hl)
success:
    or a
    jp done
failed:
    scf
done:
    pop hl
    pop de
    pop bc
    ret
'''
(O/'driver.asm').write_text(source)
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'driver.bin'),str(O/'driver.asm')],check=True,capture_output=True)
base=bytearray(65536);code=(R/'bin/banked/CLIENT.bin').read_bytes();base[0xF800:0xF800+len(code)]=code
for name,origin in [('LOCALBLOCK',s['MEM_IMAGE_LOCAL_START']),('TRANSPORT',s['BANK_LANDING'])]:
 code=(R/f'bin/banked/{name}.bin').read_bytes();base[origin:origin+len(code)]=code
ls={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/banked/LOCALBLOCK.sym').read_text(),re.M)}
# Подменены только физические операции. Диспетчер B=5 и сравнение настоящие.
for addr in (ls['localRamBlock'],s['LOCAL_DRIVER_ADDR'],s['MEM_IO_SYSTEM']):base[addr:addr+3]=b'\xc3'+W(0x8000)
struct.pack_into('<H',base,s['MEM_LOCAL_A'],0x8000)
struct.pack_into('<H',base,s['MEM_LOCAL_CHECKED'],s['MEM_IO_CHECKED'])
driver=(O/'driver.bin').read_bytes();base[0x8000:0x8000+len(driver)]=driver
payload=bytes((n*37+11)&255 for n in range(256));base[0x3fff:0x4101]=b'X'+payload+b'Y'
# Искажение в storage после WRITE будет прочитано и обнаружено VERIFY.
rows=[]
for page,drive,source_addr,mode in __import__("itertools").product((0,1,2),(0,1,7),(0x4000,0xB400),range(4)):
 mem=bytearray(base);mem[0x8100]=mode;mem[s["bankPage"]]=page
 mem[source_addr-1:source_addr+257]=b"X"+payload+b"Y"
 stub=b'\x31'+W(0xE000)+b'\x21'+W(source_addr)+b'\x11'+W(0x1234)+b'\x01'+W(0x0500+drive)+b'\xcd'+W(s['LOCAL_BLOCK_ENTRY'])
 stub+=b'\x22'+W(0x8302)+b'\xeb\x22'+W(0x8304)+b'\x60\x69\x22'+W(0x8306)+b'\xf5\xe1\x22'+W(0x8300)+b'\x21'+W(0)+b'\x39\x22'+W(0x8308)+b'\xc3'+W(0x82ff)
 mem[0x8200:0x8200+len(stub)]=stub;(O/'in.bin').write_bytes(mem)
 subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','82ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 result=(O/'out.bin').read_bytes();assert int.from_bytes(result[65542:65544],'little')==0x82ff
 assert bool(result[0x8300]&1)==bool(mode),(mode,result[0x8300:0x8302].hex())
 assert result[source_addr-1:source_addr+256]==b'X'+payload,(page,source_addr,mode)
 if source_addr+256!=s['DOS_VERIFY_BUFFER']:assert result[source_addr+256]==ord('Y')
 assert int.from_bytes(result[0x8308:0x830a],'little')==57344
 assert struct.unpack_from('<HHH',result,0x8302)==(source_addr,0x1234,0x0500+drive)
 if page==1:assert result[0xDD00:0xDEC0]==base[0xDD00:0xDEC0]
 rows.append(dict(page=page,source=source_addr,mode=mode,error=bool(mode)))
(O/'result.json').write_text(json.dumps(rows,indent=2)+'\n');print('UNIFIED BLOCK PASS:',len(rows),'MAIN/DOS/BANK A/B/H cases: success, write/read failure, mismatch; BC/DE/HL/SP preserved')
