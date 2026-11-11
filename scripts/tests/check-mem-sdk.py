"""sccz80 и реальный публичный memcpy_far: ABI и структура из C SDK."""
from pathlib import Path
import os,runpy,subprocess,re,json
R=Path(__file__).resolve().parents[2];O=R/'docs/banked/lazy-053/c-sdk';O.mkdir(parents=True,exist_ok=True)
z=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk');env=os.environ.copy();env['ZCCCFG']=str(z/'lib/config');env['PATH']=str(z/'bin')+os.pathsep+env['PATH']
subprocess.run([str(z/'bin/zcc.exe'),'+cpm','-clib=8080','-compiler=sccz80','-crt0='+str(R/'sdk/tests/file-crt'),'-zorg=8192','-I'+str(R/'sdk/include'),str(R/'sdk/src/mxos_z88dk.asm'),str(R/'sdk/tests/mem-copy.c'),'-o',str(O/'mem.bin'),'-m'],env=env,check=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
m=bytearray(65536);runpy.run_path(str(R/'scripts/tests/mem-image.py'))['install'](m,s)
code=(O/'mem_code_crt_init.bin').read_bytes();assert 0x2000+len(code)<0x3000
m[0x2000:0x2000+len(code)]=code;(O/'input.bin').write_bytes(m)
p=subprocess.run([str(z/'bin/z88dk-ticks.exe'),'-m8080','-pc','2000','-end','6100','-counter','10000000','-output',str(O/'out.bin'),str(O/'input.bin')],capture_output=True,text=True,check=True)
b=(O/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x6100
result=int.from_bytes(b[0x5e80:0x5e82],'little');assert result==0,result
(O/'checks.json').write_text(json.dumps(dict(compiler='sccz80 -clib=8080',programBytes=len(code),exitCode=result))+'\n');print('C MEM COPY PASS',len(code),'bytes')
