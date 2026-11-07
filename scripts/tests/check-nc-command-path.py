"""8080: вставка пути при пересечении командной строкой границы 256 байт."""
from pathlib import Path
import re,subprocess
R=Path(__file__).resolve().parents[2];O=R/'docs/nc-fixes-048/cpu';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text(),re.M)}
W=lambda x:x.to_bytes(2,'little');base=bytearray(65536)
for name,addr in [('NC.i80',0),('BIOS.sys',0xc000)]:
 b=(R/'bin'/name).read_bytes();base[addr:addr+len(b)]=b
rows=0
for used,path,ok in [(0,b'A:\\README.TXT',True),(90,b'A:\\README.TXT',True),(114,b'A:\\README.TXT',True),(115,b'A:\\README.TXT',False),(127,b'X',False),(0,b'X'*127,True),(0,b'X'*128,False)]:
 m=bytearray(base);addr=s['vars.cmdLine'];m[addr:addr+130]=b'Q'*used+bytes(130-used)
 m[s['vars.cmdLineEnd']:s['vars.cmdLineEnd']+2]=W(addr+used);m[s['vars.pathArg']:s['vars.pathArg']+len(path)+1]=path+b'\0'
 m[0x6200:0x620c]=b'\x31\x00\x8f\xcd'+W(s['appendPathToCommand'])+b'\xf5\xe1\x22\x00\x61\x76'
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6200','-end','620b','-counter','100000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True)
 b=(O/'out.bin').read_bytes();assert bool(b[0x6100]&1)==(not ok),(used,len(path),b[0x6100])
 expect=b'Q'*used+(path if ok else b'')+b'\0';assert b[addr:addr+len(expect)]==expect,(used,path)
 rows+=1
print('NC command path PASS',rows)
