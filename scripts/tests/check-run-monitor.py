"""8080: final MON loader survives BIOS replacement and selects its entry."""
from pathlib import Path
import re, subprocess
R=Path(__file__).resolve().parents[2]
O=R/'docs/compat/monitor-launch/cpu';O.mkdir(parents=True,exist_ok=True)
sym=lambda n:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin'/n).with_suffix('.sym').read_text(),re.M)}
s=sym('RUN');b=sym('BIOS')
for kind,name in enumerate(('Monitor2','MONWIN','RAMFOS')):
 m=bytearray(65536)
 for addr,path in ((0,'RUN.i80'),(0xc000,'BIOS.sys'),(0xf000,'IO.sys')):
  data=(R/'bin'/path).read_bytes();m[addr:addr+len(data)]=data
 core=(R/'bin'/f'{name}.mon').read_bytes();m[0x9000:0x9000+len(core)]=core
 m[s['monitorKind']]=kind;m[s['monitorSize']:s['monitorSize']+2]=len(core).to_bytes(2,'little')
 m[b['bios_fileGetSetDrive']:b['bios_fileGetSetDrive']+2]=bytes.fromhex('b7 c9')
 (O/'in.bin').write_bytes(m)
 stop=0xf85d if kind==2 else 0xc800
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc',f'{s["monitorLoaded"]:x}','-end',f'{stop:x}','-counter','20000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=30)
 got=(O/'out.bin').read_bytes();assert int.from_bytes(got[65542:65544],'little')==stop,name
 if kind==2:
  assert got[0xc000:0xd300]==core[:0x1300] and got[0xf800:0xffe0]==core[0x1300:],name
 else:
  assert got[0xc000:0xc000+len(core)]==core,name
  assert got[s['IO_PAGE_STD']]==3,name
 print(name,'copy and entry PASS')
