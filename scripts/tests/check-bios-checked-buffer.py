"""Локальная проверяющая запись MEM: Intel 8080, все байты и ошибки драйвера."""
import json,re,struct,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/shared-buffer-2026-11-10/cpu-checked';O.mkdir(parents=True,exist_ok=True)
W=lambda x:struct.pack('<H',x)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/banked/CLIENT.sym').read_text(),re.M)}
source='''    ORG 08000h
    push bc
    push de
    push hl
    ld a,c
    cp 7
    jp nz,failed
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
base[0xF740:0xF743]=b'\xc3'+W(0x8000);driver=(O/'driver.bin').read_bytes();base[0x8000:0x8000+len(driver)]=driver
payload=bytes((n*37+11)&255 for n in range(256));base[0x3fff:0x4101]=b'X'+payload+b'Y'
# Искажение в storage после WRITE будет прочитано и обнаружено VERIFY.
bios=(R/'bin/BIOS.sys').read_bytes();base[0xc000:0xc000+len(bios)]=bios
base[0x80f0:0x80f5]=b'\x0e\x07\xc3'+W(0x8000)
base[0xED7F]=0xAB;base[0xEE80]=0xCD
rows=[]
for mode in range(4):
 mem=bytearray(base);mem[0x8100]=mode
 stub=b'\x31'+W(0xED70)+b'\x21'+W(0x4000)+b'\x11'+W(0x1234)+b'\x01'+W(0x80f0)+b'\xcd'+W(0xC89F)
 stub+=b'\x22'+W(0x8302)+b'\xeb\x22'+W(0x8304)+b'\x60\x69\x22'+W(0x8306)+b'\xf5\xe1\x22'+W(0x8300)+b'\x21'+W(0)+b'\x39\x22'+W(0x8308)+b'\xc3'+W(0x82ff)
 mem[0x8200:0x8200+len(stub)]=stub;(O/'in.bin').write_bytes(mem)
 subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','82ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 result=(O/'out.bin').read_bytes();assert int.from_bytes(result[65542:65544],'little')==0x82ff
 assert bool(result[0x8300]&1)==bool(mode),(mode,result[0x8300:0x8302].hex())
 assert result[0x3fff:0x4101]==b'X'+payload+b'Y'
 assert int.from_bytes(result[0x8308:0x830a],'little')==60784
 assert struct.unpack_from('<HHH',result,0x8302)==(0x4000,0x1234,0x80f0)
 assert result[0xED7F]==0xAB and result[0xEE80]==0xCD
 # No 256-byte allocation below the BIOS call stack.
 assert result[0xEC80:0xED20]==base[0xEC80:0xED20]
 rows.append(dict(mode=mode,error=bool(mode)))
(O/'result.json').write_text(json.dumps(rows,indent=2)+'\n');print('BIOS SHARED BUFFER PASS: success, write failure, read failure, last-byte mismatch')
