"""8080: частное/остаток для секторных сдвигов 6..9, включая большие номера FAT."""
from pathlib import Path
import random,re,struct,subprocess
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/regression-004/math';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOS.sym').read_text(),re.M)}
rng=random.Random(8080);values=[0,1,63,64,127,128,255,256,511,512,0x7FFF,0x8000,0xFF00,0xFFEF,0xFFFF]+[rng.randrange(65536) for _ in range(241)]
cases=[(n,a) for a in range(6,10) for n in values]
m=bytearray(65536);dos=(R/'bin/DOS.sys').read_bytes();m[0xC000:0xC000+len(dos)]=dos
w=lambda n:struct.pack('<H',n)
code=b'\x31'+w(s['STACK_ADDR'])
for i,(n,a) in enumerate(cases):
 code+=b'\x21'+w(n)+bytes([0x3E,a,0xCD])+w(s['splitSectorOffset'])+b'\x22'+w(0x6000+i*4)+b'\xEB\x22'+w(0x6002+i*4)
code+=b'\xC3'+w(0x8000)
assert 0x2000+len(code)<0x6000
m[0x2000:0x2000+len(code)]=code;(O/'in.bin').write_bytes(m)
subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','2000','-end','8000','-counter','10000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
got=(O/'out.bin').read_bytes()
for i,(n,a) in enumerate(cases):assert struct.unpack_from('<HH',got,0x6000+i*4)==(n>>a,n&((1<<a)-1)),(n,a)
print('SECTOR MATH PASS',len(cases))
