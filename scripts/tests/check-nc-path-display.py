"""Проверка компактного пути на машинном коде NC для 8080."""
from pathlib import Path
import re,struct,subprocess
R=Path(__file__).resolve().parents[2]
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/NC.sym').read_text(),re.M)}
O=R/'docs/fat/directories-002/cpu';O.mkdir(parents=True,exist_ok=True)
W=lambda x:struct.pack('<H',x)
cases=[('A:\\',29,'A:\\'),('A:\\RAMFOS\\Almaz-2.exe',23,'A:\\RAMFOS\\Almaz-2.exe'),('A:\\GAMES\\COLLECT\\RAMFOS\\Almaz-2.exe',25,'A:\\...\\RAMFOS\\Almaz-2.exe'),('A:\\GAMES\\COLLECT\\RAMFOS\\Almaz-2.exe',23,'A:\\...\\Almaz-2.exe'),('H:\\LONGNAME.EXT\\ABCDEFGH.XYZ',19,'H:\\...\\ABCDEFGH.XYZ')]
for path,width,want in cases:
 m=bytearray(65536);code=(R/'bin/NC.i80').read_bytes();m[:len(code)]=code
 raw=path.encode()+b'\0';m[0x7000:0x7000+len(raw)]=raw
 stub=b'\x31'+W(0x7F00)+b'\x21'+W(0x7000)+bytes([6,width,0xCD])+W(S['compactPath'])+b'\xC3'+W(0x6100)
 m[0x6000:0x6000+len(stub)]=stub;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6000','-end','6100','-counter','1000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True)
 out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x6100
 got=out[S['vars.displayPath']:S['vars.displayPath']+128].split(b'\0')[0].decode();assert got==want,(path,width,got,want)
 assert out[0x7000:0x7000+len(raw)]==raw
print('NC compact path: 5 cases PASS')
