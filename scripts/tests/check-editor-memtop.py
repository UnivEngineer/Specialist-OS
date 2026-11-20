"""8080: редактор соблюдает memtop при запуске, сканировании и открытии файла."""
from pathlib import Path
import re,struct,subprocess,json
R=Path(__file__).resolve().parents[2];O=R/'.local/editor-memtop-check';O.mkdir(parents=True,exist_ok=True)
def symbols(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(encoding='utf-8'),re.M)}
d,e=symbols('DOS.sym'),symbols('EDIT.sym');word=lambda n:struct.pack('<H',n)
base=bytearray(65536)
for name,at in [('BIOS.sys',0xC000),('IO.main.bin',0xF000),('EDIT.i80',0)]:
 code=(R/'bin'/name).read_bytes();base[at:at+len(code)]=code
base[e['FONT_ENGINE_HEADER']+5]=6
base[0x8D00:0x8E00]=bytes([0xA5])*256
rows=[]
def run(memory,code,end):
 memory[0x6200:0x6200+len(code)]=code;(O/'in.bin').write_bytes(memory)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6200','-end',f'{end:x}','-counter','50000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 got=(O/'out.bin').read_bytes();assert int.from_bytes(got[65542:65544],'little')==end
 assert got[0x8D00:0x8E00]==memory[0x8D00:0x8E00]
 return got
for top in [0x5FFF,0x6FFF,0x7FFF,0x8BFF]:
 # Stop at the first memory-registration call with the chosen BIOS limit.
 m=bytearray(base)
 getter=e['bios_getMemTop']
 m[getter:getter+4]=b'\x21'+word(top)+b'\xc9'
 code=b'\x31'+word(d['STACK_ADDR'])+b'\x11\x00\x60\xCD'+word(e['editorLoadingDone'])
 got=run(m,code,e['mx_callRegs'])
 limit=int.from_bytes(got[e['v_textLimitLo']:e['v_textLimitLo']+2],'little')
 assert limit==min(top+1,e['APP_MEMORY_END'])
 # A full document can contain zero bytes right up to its exclusive end.
 m=bytearray(base)
 begin=e['EDITOR_TEXT_BEGIN']
 m[begin:limit]=bytes((i%256 for i in range(limit-begin)))
 m[e['v_textEnd']:e['v_textEnd']+2]=word(limit-1)
 m[e['v_textLimitLo']:e['v_textLimitLo']+2]=word(limit)
 # Код стенда вне проверяемого текста: запуск из непереключаемой области.
 code=b'\x31'+word(d['STACK_ADDR'])+b'\xCD'+word(e['scanDocument'])
 m[0x8C00:0x8C00+len(code)]=code;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8c00','-end',f'{e["clearAndRedrawDocument"]:x}','-counter','50000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 got=(O/'out.bin').read_bytes();assert int.from_bytes(got[65542:65544],'little')==e['clearAndRedrawDocument']
 assert int.from_bytes(got[e['v_textEnd']:e['v_textEnd']+2],'little')==limit-1
 assert got[begin:limit-1]==m[begin:limit-1]
 assert got[limit-1]==0 and got[0x8D00:0x8E00]==base[0x8D00:0x8E00]
 rows.append(dict(memtop=top,textLimit=limit,modulePreserved=True))
(O/'result.json').write_text(json.dumps(rows,indent=2),encoding='utf-8');print('EDITOR MEMTOP PASS',len(rows))
