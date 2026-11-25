"""8080 regressions: disposable MAIN handoff, banked KEY and skipped bytes."""
from pathlib import Path
from relocated_component import load_at
import re, subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'.local/nc-bank-regressions'
OUT.mkdir(parents=True,exist_ok=True)
W=lambda n:int(n).to_bytes(2,'little')
TICKS='D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'
def sy(name):
 return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin'/f'{name}.sym').read_text(encoding='utf-8'),re.M)}
def asm(text):
 (OUT/'stub.asm').write_text(text,encoding='utf-8')
 subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--raw={OUT / "stub.bin"}',str(OUT/'stub.asm')],check=True,capture_output=True)
 return (OUT/'stub.bin').read_bytes()
def run(memory,entry,code=None):
 code=code or b'\x31'+W(0xef70)+b'\xcd'+W(entry)+b'\xc3'+W(0xe400)
 memory[0xe000:0xe000+len(code)]=code
 (OUT/'input.bin').write_bytes(memory)
 proc=subprocess.run([TICKS,'-m8080','-pc','e000','-end','e400','-counter','30000000','-output',str(OUT/'output.bin'),str(OUT/'input.bin')],check=True,capture_output=True,timeout=20)
 data=(OUT/'output.bin').read_bytes()
 assert data[65542:65544]==W(0xe400),('PC',data[65542:65544].hex())
 return data

def main():
 c=sy('CLI'); n=sy('NC_CORE'); f=sy('FILEOPS')
 # Actual CLI transfer: erase all old MAIN code before returning. It must RET
 # directly to BIOS, never fetch the instruction after an overwritten CALL.
 mem=bytearray(65536); binary=(ROOT/'bin/CLI.i80').read_bytes();mem[:len(binary)]=binary
 mem[c['cliHistorySave']]=0xc9
 stub=asm(f""" ORG 0E500h
inlineCall:
 ex (sp),hl
 inc hl
 inc hl
 ex (sp),hl
 ret
 DS 0E540h-$,0
execCall:
 push bc
 pop hl
 ld (0E700h),hl
 ex de,hl
 ld (0E702h),hl
 ld hl,0
 ld bc,08C00h
erase:
 ld (hl),0FFh
 inc hl
 dec bc
 ld a,b
 or c
 jp nz,erase
 ret
 END
""")
 mem[0xe500:0xe500+len(stub)]=stub
 mem[c['mx_callRegs']:c['mx_callRegs']+3]=b'\xc3'+W(0xe500)
 mem[c['mx_callPacket']:c['mx_callPacket']+3]=b'\xc3'+W(0xe540)
 mem[c['bios_reboot']:c['bios_reboot']+3]=b'\xc3'+W(0xe400)
 result=run(mem,c['cliExternal'])
 assert result[0xe700:0xe702]==W(c['MXOS_DOS_FILE_EXEC'])
 packet=int.from_bytes(result[0xe702:0xe704],'little')
 assert result[packet:packet+2]==W(c['CMD_LINE'])
 assert result[:0x8c00]==bytes([255])*0x8c00
 print('Disposable MAIN handoff PASS: overwritten CLI returns straight to BIOS')
 # One empty poll, then F1 or Ctrl+Down. For quick input GFX stays idle.
 for key,flags in [(0,0x02),(0x19,0x03)]:
  mem=bytearray(65536)
  binary=(ROOT/'bin/NC_CORE.i80').read_bytes()
  mem[0x100:0x100+len(binary)]=binary
  mem[n['io_bankKeyTicks']:n['io_bankKeyTicks']+2]=b'\xaf\xc9'
  mem[n['io_bankKeyShift']:n['io_bankKeyShift']+2]=b'\xaf\xc9'
  mem[n['mx_queueFlush']]=0xc9
  for entry in ['mx_routeCall','mx_memcpyFar','mx_callRegs']:
   mem[n[entry]:n[entry]+3]=b'\xc3'+W(0xe410)
  stub=asm(f""" ORG 0E500h
 ld a,l
 cp {n['ncInputState'] & 255}
 jp nz,0E410h
 ld a,h
 cp {n['ncInputState'] >> 8}
 jp nz,0E410h
 ld a,(0E700h)
 inc a
 ld (0E700h),a
 dec a
 ret z
 push hl
 ld hl,{key*256+flags}
 push hl
 pop af
 pop hl
 ret
 END
""")
  mem[0xe500:0xe500+len(stub)]=stub
  mem[n['io_bankKeyPoll']:n['io_bankKeyPoll']+3]=b'\xc3'+W(0xe500)
  result=run(mem,n['ncBankGetch'])
  assert result[0xe700]==2
  assert result[65536:65538]==bytes([flags,key])
  assert result[65544:65546]==W(0xef70)
 print('Local NC input PASS: empty poll, F1/NZ, Ctrl/CF, stack, no MAIN call')
 # A new raw arrow key and a held key have different delays.
 for key,ctrl,modal,previous,wanted in [
     (0x1a,0,0,0xff,8),(0x19,0,0,0x19,1),
     (0x08,0,0,0x18,8),(0x08,0,0,0x08,1),
     (0x18,0,0,0xff,8),(0x18,0,0,0x18,1),
     (0x1a,1,0,0x1a,32),(0x19,0,1,0x19,32),(ord('A'),0,0,0xff,32)]:
  mem=bytearray(65536)
  binary=(ROOT/'bin/NC_CORE.i80').read_bytes()
  mem[0x100:0x100+len(binary)]=binary
  mem[n['ncDialogActive']]=modal
  mem[n['ncInputState.lastKey']]=key
  mem[n['ncInputState.repeatCount']]=32
  code=b'\x31'+W(0xef70)+bytes([0x16,previous,0x3e,key,0x37 if ctrl else 0xb7])
  code+=b'\xcd'+W(n['ncBankPanelRepeat'])+b'\xc3'+W(0xe400)
  result=run(mem,0,code)
  assert result[n['ncInputState.repeatCount']]==wanted,(key,ctrl,modal,previous)
 print('NC panel repeat PASS: first/repeat, ordinary keys, Ctrl, modal guard')
 for previous,raw,key,flags,operation in [
     (0xff,0xff,0,0x42,'FILEOPS_STEP'),
     (13,13,13,0x02,'FILEOPS_STEP'),
     (13,0x1b,0x1b,0x02,'FILEOPS_CANCEL'),
     (0xff,ord('A'),ord('a'),0x02,'FILEOPS_CANCEL'),
 ]:
  mem=bytearray(65536)
  binary=(ROOT/'bin/NC_CORE.i80').read_bytes()
  mem[0x100:0x100+len(binary)]=binary
  mem[n['ncInputState.lastKey']]=previous
  stub=asm(f""" ORG 0E500h
    ld (hl),{raw}
    push hl
    ld hl,{key*256+flags}
    push hl
    pop af
    pop hl
    ret
    END
""")
  mem[0xe500:0xe500+len(stub)]=stub
  mem[n['io_bankKeyPoll']:n['io_bankKeyPoll']+3]=b'\xc3'+W(0xe500)
  result=run(mem,n['ncProgressInput'])
  assert result[65537]==n[operation],(previous,key,operation)
 print('NC file progress input PASS: timer API, empty poll, ignored repeat and new key')
 for shift,help_open,info in [(0,1,0),(2,0,1)]:
  mem=bytearray(65536)
  binary=(ROOT/'bin/NC_CORE.i80').read_bytes()
  mem[0x100:0x100+len(binary)]=binary
  mem[n['ncShiftState']]=shift
  mem[n['ncHelp']:n['ncHelp']+6]=b'\x3e\x01\x32'+W(0xe700)+b'\xc9'
  mem[n['loadAndPrintPanel']]=0xc9
  mem[n['mainLoop_cursorReady']]=0xc9
  result=run(mem,n['but_F3'])
  assert result[0xe700]==help_open and result[n['state.panelB_info']]==info
 print('NC F3 routing PASS: Shift+F3 help, F3 info')
 # Stale DOS context after a copy must not redirect the next panel Enter.
 for image,origin in [('NC',0),('NC_CORE',0x100)]:
  panel=sy(image)
  for active in [0,1]:
   mem=bytearray(65536);binary=(ROOT/'bin'/f'{image}.i80').read_bytes();mem[origin:origin+len(binary)]=binary
   mem[panel['state.activePanel']]=active
   mem[panel['state.panelA_drive']+active]=0
   mem[panel['state.panelA_dirLow']+active]=0x34
   mem[panel['state.panelA_dirHigh']+active]=0x12
   mem[panel['vars.pathArg']:panel['vars.pathArg']+8]=b'DRIVERS\0'
   mem[panel['compactName']]=0xc9
   mem[panel['directoryChanged']:panel['directoryChanged']+3]=b'\xc3'+W(0xe400)
   mem[panel['directoryFailure']:panel['directoryFailure']+3]=b'\xc3'+W(0xe410)
   mem[0xe700:0xe705]=bytes([7,0x99,0x99,0,0])
   stub=asm(f""" ORG 0E500h
contextCall:
 ld (0E70Fh),a
 ex (sp),hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 inc hl
 ex (sp),hl
 ld a,c
 cp {panel['MXOS_DOS_FILE_SELECT_DRIVE']&255}
 jp z,drive
 cp {panel['MXOS_DOS_DIRECTORY']&255}
 jp z,directory
 cp {panel['MXOS_DOS_CLI']&255}
 jp nz,failed
 ld a,(0E700h)
 or a
 jp nz,failed
 ld a,(0E701h)
 cp 034h
 jp nz,failed
 ld a,(0E702h)
 cp 012h
 jp nz,failed
 ld a,1
 ld (0E704h),a
 xor a
 ret
drive:
 ld a,(0E70Fh)
 ld (0E700h),a
 xor a
 ret
directory:
 ld a,(0E70Fh)
 cp {panel['DIR_OP_SET']}
 jp nz,failed
 ex de,hl
 ld (0E701h),hl
 ld a,1
 ld (0E703h),a
 xor a
 ret
failed:
 scf
 ret
 END
""")
   mem[0xe500:0xe500+len(stub)]=stub
   mem[panel['mx_callRegs']:panel['mx_callRegs']+3]=b'\xc3'+W(0xe500)
   result=run(mem,panel['enterDirectory'])
   assert result[0xe700:0xe705]==bytes([0,0x34,0x12,1,1]),(image,active,'context')
 print('Panel directory Enter PASS: MAIN/EXEC, both panels, stale other-drive CWD')
 # Skipped 32-bit payloads add with carry and count exactly once; sync calls
 # leave progress unchanged. Exercise actual FILEOPS object code.
 origin=f['DOS_ARENA_START']
 binary,f=load_at(ROOT/'source/fileops/FILEOPS.asm','FILEOPS','OVERLAY_ORIGIN',origin)
 for async_mode in [0,1]:
  mem=bytearray(65536);mem[origin:origin+len(binary)]=binary
  req=0x3000;mem[f['v_copyRequest']:f['v_copyRequest']+2]=W(req)
  mem[req+f['FILE_COPY_REQUEST.source']+f['FILE_DESCRIPTOR.size']:req+f['FILE_COPY_REQUEST.source']+f['FILE_DESCRIPTOR.size']+4]=(0x20123).to_bytes(4,'little')
  mem[f['foAsync']]=async_mode
  mem[f['foJob.bytes']:f['foJob.bytes']+4]=(0xfff0).to_bytes(4,'little')
  mem[f['foJob.files']:f['foJob.files']+2]=W(7)
  result=run(mem,f['foFileSkipped'])
  assert int.from_bytes(result[f['foJob.bytes']:f['foJob.bytes']+4],'little')==0xfff0+(0x20123 if async_mode else 0)
  assert result[f['foJob.files']:f['foJob.files']+2]==W(7+async_mode)
 print('Skipped copy progress PASS: 32-bit carry, count, synchronous path')
if __name__=='__main__':main()
