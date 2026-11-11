"""DISKBEN: вывод миллисекунд с тремя десятичными знаками на CPU 8080."""
from pathlib import Path
import re,struct,subprocess,random
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/high-005/formatting';O.mkdir(parents=True,exist_ok=True)
def symbols(p):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(encoding='utf-8'),re.M)}
stub=O/'capture.asm';stub.write_text(''' ORG 08800h
emit:
 push hl
 ld hl,(08A00h)
 ld (hl),a
 inc hl
 ld (08A00h),hl
 pop hl
 ret
string:
 push hl
 push af
loop:
 ld a,(hl)
 or a
 jp z,done
 call emit
 inc hl
 jp loop
done:
 pop af
 pop hl
 ret
 END
''',encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(stub.with_suffix('.bin')),'--sym='+str(stub.with_suffix('.sym')),str(stub)],check=True,capture_output=True)
st=symbols(stub.with_suffix('.sym'));count=0
for hz in [2000000,2500000]:
 base=O/f'DISKBEN-{hz}'
 subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',f'-DPIT_HZ={hz}',f'--raw={base}.bin',f'--sym={base}.sym','DISKBEN.asm'],cwd=R/'source-apps/diskbench',check=True,capture_output=True)
 s=symbols(base.with_suffix('.sym'));code=base.with_suffix('.bin').read_bytes()
 max_ticks=0xFFFFFFFF>>(4 if hz==2000000 else 6)
 rng=random.Random(hz)
 for ticks in [0,1,2,7,8,124,125,624,625,7812,7813,65535,65536,max_ticks]+[rng.randrange(max_ticks) for _ in range(12)]:
  m=bytearray(65536);m[:len(code)]=code;bios=(R/'bin/BIOS.sys').read_bytes();m[0xC000:0xC000+len(bios)]=bios
  raw=stub.with_suffix('.bin').read_bytes();m[0x8800:0x8800+len(raw)]=raw
  for label,target in [('timing_print',st['string']),('timing_printChar',st['emit'])]:m[s[label]:s[label]+3]=b'\xC3'+struct.pack('<H',target)
  struct.pack_into('<H',m,0x8A00,0x9000);struct.pack_into('<I',m,0x8000,ticks)
  wrapper=b'\x31\xDF\x8F\x21\x00\x80\xCD'+struct.pack('<H',s['print_time_ms'])+bytes.fromhex('C3 00 83');m[0x8200:0x8200+len(wrapper)]=wrapper
  (O/'in.bin').write_bytes(m)
  subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','8300','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
  b=(O/'out.bin').read_bytes();end=int.from_bytes(b[0x8A00:0x8A02],'little');actual=b[0x9000:end].decode().strip()
  micros=ticks*256000000//hz;expected=f'{micros//1000}.{micros%1000:03d}'
  assert actual==expected,(hz,ticks,actual,expected)
  count+=1
print('DISKBEN fractional milliseconds PASS',count,'cases, 2/2.5 MHz')
