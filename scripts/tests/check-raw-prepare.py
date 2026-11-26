"""RAW PREPARE: адреса своего банка, MAIN BIOS и отказы на CPU 8080."""
import re,struct,subprocess
from pathlib import Path
from dos_state import initialize_dos_state
R=Path(__file__).resolve().parents[2]
O=R/'docs/diskbench/raw-local-2026-11-09/cpu-prepare';O.mkdir(parents=True,exist_ok=True)
W=lambda n:struct.pack('<H',n)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/banked/KERNEL.sym').read_text(encoding='utf-8'),re.M)}
# Подменяем только транспорт/flush/установку; исполняется настоящий PREPARE.
asm=''' ORG 08400h
fetch:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec bc
 ld a,b
 or c
 jp nz,fetch
 ret
flush:
 ld a,1
 ld (08700h),a
 ld a,(08702h)
 or a
 ret z
 scf
 ret
install:
 ld a,1
 ld (08701h),a
 ld a,(08703h)
 or a
 ret z
 scf
 ret
 END
'''
(O/'stubs.asm').write_text(asm,encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'stubs.bin'),'--sym='+str(O/'stubs.sym'),str(O/'stubs.asm')],check=True,capture_output=True)
st={k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([\da-fA-F]+)',(O/'stubs.sym').read_text(),re.M)}
base=bytearray(65536);code=(R/'bin/DOS.sys').read_bytes();base[0x100:0x100+len(code)]=code
initialize_dos_state(base, s)
raw=(O/'stubs.bin').read_bytes();base[0x8400:0x8400+len(raw)]=raw
for name,target in [('kdFetch','fetch'),('flushVolume','flush'),('klInstall','install')]:base[s[name]:s[name]+3]=b'\xc3'+W(st[target])
# Реальный MAIN ABI намеренно отличается от одноимённого blockIO в DOS.
struct.pack_into('<H',base,s['MEM_LOCAL_SYSTEM'],s['bios_blockIO'])
struct.pack_into('<H',base,s['MEM_LOCAL_CHECKED'],0xC195)
stub=b'\x31'+W(0xE000)+b'\xcd'+W(s['dosRawPrepare'])
stub+=b'\x22'+W(0x8302)+b'\xeb\x22'+W(0x8304)+b'\x60\x69\x22'+W(0x8306)+b'\xf5\xe1\x22'+W(0x8300)+b'\xc3'+W(0x82ff)
base[0x8200:0x8200+len(stub)]=stub
system, ram, external = (s[name] for name in ('SYSTEM_DRIVE','RAM_DRIVE','EXTERNAL_DRIVE'))
cases=[(drive,page,8,0,0) for drive in (system,ram,external) for page in (0,1,5)]
cases += [(0,5,8,0,0),(system,0,9,0,0),(external,5,8,1,0),(external,5,8,0,1)]
for drive,page,shift,flushError,installError in cases:
 mem=bytearray(base);mem[s['v_drive']]=drive;mem[s['kdPage']]=page;mem[s['v_diskInfo.blockShift']]=shift
 mem[s['kernelRamDrive']]=ram
 mem[s['kernelExternalDrive']]=external
 for mounted in (system,ram,external):
  struct.pack_into('<H',mem,s['DOS_DRIVER_REGISTRY']+mounted*s['DRIVER_RECORD_SIZE'],128)
 mem[0x8702:0x8704]=bytes([flushError,installError]);(O/'in.bin').write_bytes(mem)
 subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','82ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x82ff
 error=drive==0 or shift!=8 or flushError or installError
 assert bool(out[0x8300]&1)==bool(error),(drive,page,shift,out[0x8300:0x8308].hex())
 if not error:
  entry=s['LOCAL_BLOCK_ENTRY']
  checked=0
  assert struct.unpack_from('<HHH',out,0x8302)==(entry,checked,0x0800+drive),(drive,page,out[0x8302:0x8308].hex())
  assert out[0x8700]==1 and out[0x8701]==(drive==7)
print('RAW PREPARE PASS:',len(cases),'cases; MAIN BIOS, DOS/EXEC, geometry, flush/install failures')
