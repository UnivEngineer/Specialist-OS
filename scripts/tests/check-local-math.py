"""8080: selectable local arithmetic, register contracts, overflow and zero divisor."""
from pathlib import Path
import random,re,struct,subprocess
R=Path(__file__).resolve().parents[2]; O=R/'docs/bench/local-math'; O.mkdir(parents=True,exist_ok=True)
asm=O/'math.asm';asm.write_text(' ORG 04000h\n DEFINE MATH_SUB\n DEFINE MATH_DIV2\n DEFINE MATH_MUL\n DEFINE MATH_DIV\n INCLUDE "'+(R/'source/include/math.inc').as_posix()+'"\n END\n',encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'math.bin'),'--sym='+str(O/'math.sym'),str(asm)],check=True,capture_output=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(O/'math.sym').read_text(),re.M)}
rng=random.Random(8080);edges=[0,1,2,255,256,32767,32768,65534,65535];cases=[(x,y) for x in edges for y in edges]+[(rng.randrange(65536),rng.randrange(65536)) for _ in range(256)]
w=lambda n:struct.pack('<H',n&65535)
count=0
for start in range(0,len(cases),64):
 chunk=cases[start:start+64];m=bytearray(65536);binary=(O/'math.bin').read_bytes();m[0x4000:0x4000+len(binary)]=binary;code=b'\x31'+w(0x8fe0);expected=[]
 for x,y in chunk:
  for name in ['sub_hl_de','div_hl_2','mul_hl_de','div_hl_de']:
   at=0x7000+len(expected)*6
   code+=b'\x01'+w(0x1357)+b'\x21'+w(x)+b'\x11'+w(y)+b'\xcd'+w(S[name])+b'\x22'+w(at)+b'\xeb\x22'+w(at+2)+b'\x60\x69\x22'+w(at+4)
   if name=='sub_hl_de': v,d=(x-y)&65535,y
   elif name=='div_hl_2':v,d=x>>1,y
   elif name=='mul_hl_de':v,d=(x*y)&65535,None
   else:v,d=(divmod(x,y) if y else (0,x))
   expected.append((v,d,name,x,y))
 code+=b'\xc3'+w(0x3f00);assert len(code)+0x100<0x3f00;m[0x100:0x100+len(code)]=code;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','100','-end','3f00','-counter','10000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 got=(O/'out.bin').read_bytes()
 for i,(v,d,name,x,y) in enumerate(expected):
  hl,de,bc=struct.unpack_from('<HHH',got,0x7000+i*6);assert hl==v and (d is None or de==d) and bc==0x1357,(name,x,y,hl,de,bc,v,d)
 count+=len(expected)
print('LOCAL MATH PASS',count,'cases;',len(binary),'bytes; BC preserved')
