"""8080: редактор соблюдает memtop при запуске, сканировании и открытии файла."""
from pathlib import Path
import re,struct,subprocess,json
R=Path(__file__).resolve().parents[2];O=R/'docs/file-api-001/editor';O.mkdir(parents=True,exist_ok=True)
def symbols(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(encoding='utf-8'),re.M)}
d,e=symbols('DOS.sym'),symbols('E.sym');word=lambda n:struct.pack('<H',n)
base=bytearray(65536);code=(R/'bin/DOS.sys').read_bytes();base[0xC000:0xC000+len(code)]=code
code=(R/'bin/E.i80').read_bytes();base[:len(code)]=code
module=(R/'bin/DOSMOD.i80').read_bytes();base[0x8000:0x8000+len(module)]=module
rows=[]
def run(memory,code,end):
 memory[0x6200:0x6200+len(code)]=code;(O/'in.bin').write_bytes(memory)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6200','-end',f'{end:x}','-counter','50000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 got=(O/'out.bin').read_bytes();assert int.from_bytes(got[65542:65544],'little')==end
 assert got[0x8000:0x8C00]==memory[0x8000:0x8C00]
 return got
for top in [0x5FFF,0x6FFF,0x7FFF,0x8BFF]:
 m=bytearray(base);m[d['v_memTop']:d['v_memTop']+2]=word(top);m[0x6000:0x6002]=b'\r\0'
 code=b'\x31'+word(d['STACK_ADDR'])+b'\x11\x00\x60\xCD'+word(e['openCommandLineFile'])
 got=run(m,code,e['startupBannerAndLineBuffer']);limit=int.from_bytes(got[e['v_textLimitLo']:e['v_textLimitLo']+2],'little');assert limit==min(top+1,0x8000)
 m=bytearray(base);m[0x1800:limit]=b'X'*(limit-0x1800);m[e['v_textLimitLo']:e['v_textLimitLo']+2]=word(limit)
 # Код стенда вне проверяемого текста: запуск из непереключаемой области.
 code=b'\x31'+word(d['STACK_ADDR'])+b'\xCD'+word(e['scanDocument'])
 m[0x8C00:0x8C00+len(code)]=code;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8c00','-end',f'{e["clearAndRedrawDocument"]:x}','-counter','50000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 got=(O/'out.bin').read_bytes();assert int.from_bytes(got[65542:65544],'little')==e['clearAndRedrawDocument']
 assert int.from_bytes(got[e['v_textEnd']:e['v_textEnd']+2],'little')==limit-1
 assert got[limit-1]==255 and got[0x8000:0x8C00]==base[0x8000:0x8C00]
 rows.append(dict(memtop=top,textLimit=limit,modulePreserved=True))
(O/'result.json').write_text(json.dumps(rows,indent=2),encoding='utf-8');print('EDITOR MEMTOP PASS',len(rows))
