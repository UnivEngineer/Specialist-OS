"""DISKBEN: вывод скорости с двумя десятичными знаками на CPU 8080."""
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
 base=R/f'docs/timing/shared-001/DISKBEN-{hz}'
 s=symbols(base.with_suffix('.sym'));code=base.with_suffix('.bin').read_bytes()
 maximum=0xFFFFFFFF>>(4 if hz==2000000 else 6)
 rng=random.Random(hz)
 cases=[(0,1),(1,3),(2,3),(1,10),(64000,831),(64000,1674),(2500000,28244),(1234567,12345),(maximum-1,maximum),(maximum,maximum),(1,maximum)]
 cases += [(rng.randrange(2500000),rng.randrange(3000,maximum)) for _ in range(15)]
 for numerator,denominator in cases:
  m=bytearray(65536);m[:len(code)]=code;dos=(R/'bin/DOS.sys').read_bytes();m[0xC000:0xC000+len(dos)]=dos
  raw=stub.with_suffix('.bin').read_bytes();m[0x8800:0x8800+len(raw)]=raw
  for label,target in [('timing_print',st['string']),('timing_printChar',st['emit'])]:m[s[label]:s[label]+3]=b'\xC3'+struct.pack('<H',target)
  struct.pack_into('<H',m,0x8A00,0x9000)
  struct.pack_into('<I',m,s['v_dividend'],numerator);struct.pack_into('<I',m,s['v_elapsed'],denominator)
  wrapper=b'\x31\xDF\x8F\xCD'+struct.pack('<H',s['print_rate_value'])+bytes.fromhex('C3 00 83');m[0x8200:0x8200+len(wrapper)]=wrapper
  (O/'in.bin').write_bytes(m)
  subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','8300','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
  b=(O/'out.bin').read_bytes();end=int.from_bytes(b[0x8A00:0x8A02],'little');actual=b[0x9000:end].decode().strip()
  hundredths=numerator*100//denominator;expected=f'{hundredths//100}.{hundredths%100:02d}'
  assert actual==expected,(hz,numerator,denominator,actual,expected)
  count+=1
print('DISKBEN fractional speed PASS',count,'cases, 2/2.5 MHz')
