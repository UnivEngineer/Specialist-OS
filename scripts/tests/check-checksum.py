"""bios_calcCS: 8080, сравнение суммы/регистров/флагов с Monitor-2."""
from pathlib import Path
import json,random,re,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/compat/sector-load-001/checksum-cpu';OUT.mkdir(parents=True,exist_ok=True)
sy={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/DOS.sym').read_text(),re.M)}
def run(reference,start,payload):
 m=bytearray(65536)
 for i,value in enumerate(payload):m[(start+i)&65535]=value
 code=(ROOT/('bin/Monitor2.mon' if reference else 'bin/DOS.sys')).read_bytes();m[0xc000:0xc000+len(code)]=code
 end=(start+len(payload)-1)&65535
 wrapper=bytes.fromhex('31 00 8f 21')+start.to_bytes(2,'little')+b'\x11'+end.to_bytes(2,'little')+bytes.fromhex('CD 2A C8 C3 00 81')
 m[0x8000:0x8000+len(wrapper)]=wrapper
 inp=OUT/'input.bin';target=OUT/'output.bin';inp.write_bytes(m)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8100','-counter','10000000','-output',str(target),str(inp)],check=True,capture_output=True,text=True,timeout=15)
 b=target.read_bytes();state=b[65536:65546]+b[65548:65550]
 assert state[6:8]==bytes.fromhex('00 81') and state[8:10]==bytes.fromhex('00 8f')
 prefix=sum(payload[:-1]);expected=bytes((sum(payload)&255,(prefix+(prefix>>8))&255));assert state[2:4]==expected
 return state,int(p.stdout.strip())
rng=random.Random(8080);results=[]
ranges=[(start,size) for start in (0,1,0xf3,0xff,0x1000) for size in (1,2,3,4,5,7,8,9,15,16,17,31,32,33,239,240,255,256,257,511,512,513)]+[(0,8192),(0,24576),(0x10f3,8192),(0xfffd,24),(0xff00,512),(0xfdf0,1025)]
for start,size in ranges:
 for pattern in (0,255,'random'):
  payload=bytes(rng.randrange(256) for _ in range(size)) if pattern=='random' else bytes([pattern])*size
  old,old_ticks=run(True,start,payload);new,new_ticks=run(False,start,payload);assert old==new,(start,size,pattern,old.hex(),new.hex())
  results.append(dict(start=start,size=size,pattern=pattern,monitorTicks=old_ticks,dosTicks=new_ticks))
report=dict(bytes=sy['dos_piece_calcCS_0_end']-sy['dos_piece_calcCS_0_start'],cases=len(results),allRegistersAndFlagsEqual=True,samples=[r for r in results if r['size']>=8192 and r['start']==0 and r['pattern']==0])
(OUT/'results.json').write_text(json.dumps(report,indent=2)+'\n');print(report)
