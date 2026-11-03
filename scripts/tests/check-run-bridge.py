"""CPU 8080: установка/релокация переходника и завершение смены ядра."""
from pathlib import Path
import re,subprocess,json
root=Path(__file__).resolve().parents[2];out=root/'docs/compat/sector-load-001/bridge-cpu';out.mkdir(exist_ok=True)
def symbols(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(root/'bin'/name).with_suffix('.sym').read_text(),re.M)}
ds=symbols('DOS');rs=symbols('RUN');results=[]
for mode,staged,returning in ((0,False,False),(0,True,False),(1,False,False),(2,False,False),(3,False,False),(3,True,False),(0,False,True),(0,True,True),(1,False,True)):
 m=bytearray(65536);d=(root/'bin/DOS.sys').read_bytes();m[0xc000:0xc000+len(d)]=d;r=(root/'bin/RUN.i80').read_bytes();m[rs['start']:rs['start']+len(r)]=r
 m[ds['bios_fileGetSetDrive']:ds['bios_fileGetSetDrive']+2]=bytes.fromhex('B7 C9')
 load=0xd300 if staged and mode==3 else 0xf100 if staged else 0x1000
 size=513
 # Программа сразу возвращается для проверки native-return; ядра — до точки входа.
 payload=bytearray((i*19+7)&255 for i in range(size));payload[:3]=bytes.fromhex('C3 00 83')
 if returning:payload[0]=0xC9
 m[0xe1f0:0xe1f2]=bytes.fromhex('00 83')
 m[0 if staged else load:(0 if staged else load)+size]=payload
 for field,val in [('callerSP',0xE1F0),('appEntry',load),('appLoad',load),('appSize',size),('monSP',0x8FDF),('monInit',0xC444)]:m[rs[field]:rs[field]+2]=val.to_bytes(2,'little')
 m[rs['mode']]=mode;m[rs['staged']]=staged;m[rs['callerDrive']]=1;m[rs['callerKoi']]=255 if returning else 0;m[rs['appArgs']:rs['appArgs']+6]=b'HELLO\0'
 if mode>=2:
  kernel=(root/('bin/RAMFOS.mon' if mode==3 else 'bin/Monitor2.mon')).read_bytes();m[0x9000:0x9000+len(kernel)]=kernel
 inp=out/'input.bin';target=out/'output.bin';inp.write_bytes(m)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc',f'{rs["install"]:x}','-end','8300','-counter','10000000','-output',str(target),str(inp)],capture_output=True,text=True,check=True,timeout=15)
 got=target.read_bytes();pc=int.from_bytes(got[65542:65544],'little');assert pc==0x8300,(mode,staged,hex(pc),p.stdout)
 assert got[load:load+size]==payload,(mode,staged,'payload')
 args=0x8e00 if staged else ds['SYSTEM_PROGRAM_ADDR'];assert got[args:args+6]==b'HELLO\0',(mode,staged,got[args:args+6])
 if not returning:assert int.from_bytes(got[65548:65550],'little')==args,(mode,staged,'DE arguments')
 if mode==2:
  assert got[0xc000:0xd000]==kernel
  # Очистка экрана Monitor-2 уже уничтожила переходник, но вход достигнут.
  assert got[0xab00:0xab20]==bytes(32)
 if mode==3:
  assert got[0xf800:0xf803]==kernel[0x1300:0x1303]
 if returning:assert got[ds['bios_vars.koi7']]==255,'caller KOI restored'
 if mode<2:
  assert got[0x9000:0xc000]==m[0x9000:0xc000], 'native screen modified'
 results.append(dict(mode=mode,staged=staged,returning=returning,entryReached=True,args=args,screenUnchanged=mode<2))
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print('OK:',len(results),'bridge mode/relocation cases')
