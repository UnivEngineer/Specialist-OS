"""DISKBEN: расчёт КиБ/с и вывод дробной скорости на CPU 8080."""
from pathlib import Path
import argparse,json,re,struct,subprocess,random
R=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--report',type=Path,default=R/'docs/diskbench/kib-038/model')
args=parser.parse_args();O=args.report.resolve();O.mkdir(parents=True,exist_ok=True)
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
st=symbols(stub.with_suffix('.sym'));count=0;rates=[]
for hz in [2000000,2500000]:
 base=O/f'DISKBEN-{hz}'
 subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',f'-DPIT_HZ={hz}',f'--raw={base}.bin',f'--sym={base}.sym','DISKBEN.asm'],cwd=R/'source-apps/diskbench',check=True,capture_output=True)
 s=symbols(base.with_suffix('.sym'));code=base.with_suffix('.bin').read_bytes()
 def run(label,values):
  m=bytearray(65536);m[:len(code)]=code;bios=(R/'bin/BIOS.sys').read_bytes();m[0xC000:0xC000+len(bios)]=bios
  raw=stub.with_suffix('.bin').read_bytes();m[0x8800:0x8800+len(raw)]=raw
  for name,target in [('timing_print',st['string']),('timing_printChar',st['emit'])]:m[s[name]:s[name]+3]=b'\xC3'+struct.pack('<H',target)
  struct.pack_into('<H',m,0x8A00,0x9000)
  for name,fmt,value in values:struct.pack_into(fmt,m,s[name],value)
  wrapper=b'\x31\xDF\x8F\xCD'+struct.pack('<H',s[label])+bytes.fromhex('C3 00 83');m[0x8200:0x8200+len(wrapper)]=wrapper
  (O/'in.bin').write_bytes(m)
  subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','8300','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=15)
  b=(O/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x8300
  end=int.from_bytes(b[0x8A00:0x8A02],'little');return b[0x9000:end].decode().strip()
 maximum=0xFFFFFFFF>>(4 if hz==2000000 else 6)
 rng=random.Random(hz)
 cases=[(0,1),(1,3),(2,3),(1,10),(64000,831),(64000,1674),(2500000,28244),(1234567,12345),(maximum-1,maximum),(maximum,maximum),(1,maximum)]
 cases += [(rng.randrange(2500000),rng.randrange(3000,maximum)) for _ in range(15)]
 for numerator,denominator in cases:
  actual=run('print_rate_value',[('v_dividend','<I',numerator),('v_elapsed','<I',denominator)])
  hundredths=numerator*100//denominator;expected=f'{hundredths//100}.{hundredths%100:02d}'
  assert actual==expected,(hz,numerator,denominator,actual,expected)
  count+=1
 # Полный путь проверяет не только десятичную печать, но и масштаб КиБ/с,
 # число операций, размер сектора, миллисекунды и подпись единиц.
 for operations,sector,ticks in [(4,256,625),(32,256,928),(32,256,1900),(96,256,2500),(864,256,28244),(1024,512,65536)]:
  actual=run('print_result_value',[('v_operations','<H',operations),('v_sectorSize','<H',sector),('v_elapsed','<I',ticks)])
  numerator=operations*(sector//256)*(hz//1024)
  hundredths=numerator*100//ticks
  millis=ticks*256000000//hz
  expected=f'{millis//1000}.{millis%1000:03d} ms\t{hundredths//100:8d}.{hundredths%100:02d} KiB/s'
  assert actual==expected,(hz,operations,sector,ticks,actual,expected)
  rates.append(dict(hz=hz,operations=operations,sector=sector,ticks=ticks,text=actual))
(O/'result.json').write_text(json.dumps(dict(formatCases=count,rateCases=rates),indent=2)+'\n',encoding='utf-8')
print('DISKBEN KiB/s PASS',count,'format cases,',len(rates),'full calculations, 2/2.5 MHz')
