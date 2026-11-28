"""CPU 8080: установка/релокация переходника и завершение смены ядра."""
from pathlib import Path
import re,subprocess,json
root=Path(__file__).resolve().parents[2];out=root/'docs/compat/sector-load-001/bridge-cpu';out.mkdir(parents=True,exist_ok=True)
def symbols(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(root/'bin'/name).with_suffix('.sym').read_text(),re.M)}
ds=symbols('BIOS');rs=symbols('RUN');results=[]
for mode,staged,returning in ((0,False,False),(0,True,False),(1,False,False),(2,False,False),(3,False,False),(3,True,False),(0,False,True),(0,True,True),(1,False,True),(2,False,True),(3,False,True)):
 m=bytearray(65536);d=(root/'bin/BIOS.sys').read_bytes();m[0xc000:0xc000+len(d)]=d;io=(root/'bin/IO.main.bin').read_bytes();m[0xf000:0xf000+len(io)]=io;r=(root/'bin/RUN.i80').read_bytes();m[:len(r)]=r
 # Text ABI belongs to the installed font DLL after its extraction from BIOS.
 font=(root/'bin/fonts/Rotate.i80').read_bytes();m[0xd800:0xd800+len(font)]=font
 # The flat bridge fixture has no DOS page. Only GET_SET_DRIVE is needed
 # before the handoff. Consume its inline DW and retain the caller's HL.
 call=rs['install']
 abi=rs['mx_callRegs']
 drive=rs['MXOS_DOS_FILE_GET_SET_DRIVE']
 assert bytes([0xcd])+abi.to_bytes(2,'little')+drive.to_bytes(2,'little') in r[call:call+16]
 m[abi:abi+6]=bytes.fromhex('E3 23 23 E3 B7 C9')
 load=0xd300 if staged and mode==3 else 0xe600 if staged else 0x3000
 size=513
 # RET in MXOS2 enters C000h. After replacement RET enters the foreign shell.
 payload=bytearray((i*19+7)&255 for i in range(size));payload[:3]=bytes.fromhex('C3 00 83')
 if returning:payload[0]=0xC9
 caller_sp=0x8800  # The application stack must lie below the resident font DLL.
 m[caller_sp:caller_sp+2]=bytes.fromhex('00 83')
 m[0 if staged else load:(0 if staged else load)+size]=payload
 for field,val in [('callerSP',caller_sp),('appEntry',load),('appLoad',load),('appSize',size),('monSP',0x8FDF),('monInit',0xC444)]:m[rs[field]:rs[field]+2]=val.to_bytes(2,'little')
 m[rs['mode']]=mode;m[rs['staged']]=staged;m[rs['callerDrive']]=1;m[rs['callerKoi']]=255 if returning else 0;m[rs['appArgs']:rs['appArgs']+6]=b'HELLO\0'
 if mode>=2:
  kernel=(root/('bin/RAMFOS.mon' if mode==3 else 'bin/Monitor2.mon')).read_bytes();m[0x9000:0x9000+len(kernel)]=kernel
 inp=out/'input.bin';target=out/'output.bin';inp.write_bytes(m)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc',f'{rs["install"]:x}','-end',f'{rs["runBodyReady"]:x}','-counter','10000000','-output',str(target),str(inp)],capture_output=True,text=True,check=True,timeout=15)
 prepared=target.read_bytes();assert int.from_bytes(prepared[65542:65544],'little')==rs['runBodyReady']
 base=int.from_bytes(prepared[rs['bridgeBase']:rs['bridgeBase']+2],'little');inp.write_bytes(prepared[:65536])
 end=0x8300
 if returning:end=rs['RAMFOS_MONITOR'] if mode==3 else 0xc800 if mode==2 else ds['mon2_init']
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc',f'{base:x}','-end',f'{end:x}' ,'-counter','10000000','-output',str(target),str(inp)],capture_output=True,text=True,check=True,timeout=15)
 got=target.read_bytes();pc=int.from_bytes(got[65542:65544],'little');assert pc==end,(mode,staged,returning,hex(pc),p.stdout[-200:])
 # Upper legacy payload can occupy glyph pages. KOI restoration after RET
 # rearranges those pages; only an executing payload must remain byte-exact.
 if not returning or load<0xd800:assert got[load:load+size]==payload,(mode,staged,'payload')
 args=rs['MAIN_COMMAND_ADDR'] if mode<2 else rs['RAMFOS_ARGS'];assert got[args:args+6]==b'HELLO\0',(mode,staged,got[args:args+6])
 if not returning:assert int.from_bytes(got[65548:65550],'little')==args,(mode,staged,'DE arguments')
 if mode==2:
  assert got[0xc000:0xd000]==kernel
  # Очистка экрана Monitor-2 уже уничтожила переходник, но вход достигнут.
  assert got[0xab00:0xab20]==bytes(32)
 if mode==3:
  assert got[0xf800:0xf803]==kernel[0x1300:0x1303]
 if mode<2:
  assert got[0x9000:0xc000]==m[0x9000:0xc000], 'native screen modified'
 results.append(dict(mode=mode,staged=staged,returning=returning,entryReached=True,args=args,screenUnchanged=mode<2))
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print('OK:',len(results),'bridge mode/relocation cases')
