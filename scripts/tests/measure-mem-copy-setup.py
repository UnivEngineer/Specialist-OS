"""8080: стоимость подготовки CPM32 отдельно от горячей передачи данных."""
from pathlib import Path
import argparse,json,re,struct,subprocess
R=Path(__file__).resolve().parents[2];W=lambda n:struct.pack('<H',n)
def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,default=R/'docs/bench/cpm32-system-2026-11-13/preparation-final');a=p.parse_args();a.report.mkdir(parents=True,exist_ok=True)
 s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/MEM.sym').read_text(),re.M)}
 image=(R/'bin/MEM.sys').read_bytes();base=bytearray(65536)
 for i in range(image[5]):
  addr,size,offset=struct.unpack_from('<HHH',image,6+6*i);base[addr:addr+size]=image[offset:offset+size]
 base[s['bankPage']]=0;base[s['bankPageCount']]=16;base[s['MEM_NEAR_COPY']:s['MEM_NEAR_COPY']+2]=W(0)
 base[s['MEM_COMMAND_SHADOW']:s['MEM_COMMAND_SHADOW']+32]=base[0xffc0:0xffe0]
 rows=[]
 for profile in (0,1):
  for count in (256,8192):
   for entry in ('bankCopyFar','bankCopySector'):
    memory=bytearray(base);memory[0xffdf]=profile
    memory[0x8500:0x8508]=b'\2'+W(0x2001)+b'\3'+W(0x6001)+W(count)
    stub=b'\x31'+W(0x8f00)+b'\x21'+W(0x8500)+b'\xcd'+W(s[entry])+b'\xc3'+W(0x88ff)
    memory[0x8800:0x8800+len(stub)]=stub
    rows.append(run(a.report,memory,s['bankCopyTail'],37,dict(profile=profile,bytes=count,entry=entry)))
  memory=bytearray(base);memory[0xffdf]=profile
  stub=b'\x31'+W(0x8f00)+b'\xcd'+W(s['bankCommandWindow'])+b'\xc3'+W(0x88ff)
  memory[0x8800:0x8800+len(stub)]=stub
  rows.append(run(a.report,memory,0x88ff,37,dict(profile=profile,entry='bankCommandWindow')))
 (a.report/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
 for row in rows:print(row)
def run(out,memory,stop,setup,meta):
 (out/'in.bin').write_bytes(memory)
 p=subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8800','-end',f'{stop:x}','-counter','1000000','-output',str(out/'out.bin'),str(out/'in.bin')],check=True,capture_output=True,text=True)
 b=(out/'out.bin').read_bytes();assert b[65542:65544]==W(stop)
 ticks=int((p.stdout+p.stderr).strip())-setup
 return dict(**meta,cycles=ticks,ms=ticks/2000,excludedCallerSetupCycles=setup)
if __name__=='__main__':main()
