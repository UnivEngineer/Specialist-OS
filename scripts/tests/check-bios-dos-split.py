"""8080: рендер BIOS без DOS, независимый физический запрос и таблицы ABI."""
from pathlib import Path
import json, re, subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/bios-dos-split-2026-11-06/cpu';OUT.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/BIOS.sym').read_text(),re.M)}
W=lambda n:n.to_bytes(2,'little')
base=bytearray(65536)
for name,start in [('BIOS',0xC000),('DOS.native',0x4000)]:
 data=(ROOT/f'bin/{name}.sys').read_bytes();base[start:start+len(data)]=data
base[S['bios_vars']:S['bios_vars']+S['BIOS_VARIABLES']]=base[S['initVars']:S['initVarsEnd']]
base[S['FONT_ADDR']:S['FONT_ADDR']+1280]=(ROOT/'assets/system-rom/FONTKOI.FNT').read_bytes()
base[S['v_dosModuleReady']]=1
for line in (ROOT/'source/dos/entries.inc').read_text().splitlines():
 if line.strip().startswith('jp '):
  name=line.split()[1];addr=S['dos_'+name]
  assert base[addr:addr+3]==b'\xc3'+W(S[name]),name

def run(memory,body):
 source=OUT/'call.asm'
 source.write_text(f' INCLUDE "{(ROOT/"source/include/mxos.inc").as_posix()}"\n ORG 02000h\n ld sp,STACK_ADDR\n'+body+'\n jp 02100h\n',encoding='utf-8')
 subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(OUT/'call.bin'),str(source)],check=True,capture_output=True)
 code=(OUT/'call.bin').read_bytes();memory[0x2000:0x2000+len(code)]=code
 (OUT/'input.bin').write_bytes(memory)
 result=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','2000','-end','2100','-counter','20000000','-output',str(OUT/'output.bin'),str(OUT/'input.bin')],capture_output=True,check=True)
 cycles=int(result.stdout.strip());data=(OUT/'output.bin').read_bytes()
 assert int.from_bytes(data[65542:65544],'little')==0x2100
 return data[:65536],cycles

render=''' call bios_clearScreenOld
 ld hl,0
 call bios_setCursorPos
 ld c,'A'
 call bios_printChar
 ld c,'b'
 call bios_printChar
 ld hl,09020h
 ld bc,0208h
 call bios_uiInverseRect
 ld hl,09000h
 ld bc,09200h
 ld de,0108h
 call bios_uiCopyRect
'''
normal,t1=run(bytearray(base),render)
without=bytearray(base);without[0x4000:0x8C00]=b'\x76'*0x4C00;without[S['v_dosModuleReady']]=0
standalone,t2=run(without,render)
assert normal[0x9000:0xC000]==standalone[0x9000:0xC000]
assert t1==t2

rows=[]
for drive,mode,error in [(1,2,False),(7,2,False),(7,1,False),(8,2,True),(7,0,True),(7,4,True)]:
 memory=bytearray(without)
 memory[0x3000:0x3005]=bytes.fromhex('3E 5A 77 AF C9')
 memory[S['v_drive']]=1
 memory[S['v_drives']+2:S['v_drives']+4]=W(0x3000)
 memory[S['v_drives']+14:S['v_drives']+16]=W(0x3000)
 memory[S['j_diskDriver']+1:S['j_diskDriver']+3]=W(0x3000)
 memory[0x1000]=0
 result,t=run(memory,f''' ld a,{drive}
 ld bc,0{mode:02X}31h
 ld de,01234h
 ld hl,01000h
 call bios_blockIO
 push af
 push bc
 push de
 push hl
 pop hl
 ld (01100h),hl
 pop hl
 ld (01102h),hl
 pop hl
 ld (01104h),hl
 pop hl
 ld (01106h),hl
''')
 assert result[0x1100:0x1106]==W(0x1000)+W(0x1234)+W((mode<<8)|0x31)
 assert bool(result[0x1106]&1)==error
 assert result[0x1000]==(0 if error else 0x5A)
 assert result[S['v_drive']]==1
 assert result[S['j_diskDriver']+1:S['j_diskDriver']+3]==W(0x3000)
 rows.append(dict(drive=drive,mode=mode,error=error,cycles=t))
(OUT/'result.json').write_text(json.dumps(dict(renderCycles=t1,withoutDos=True,physicalIO=rows),indent=2)+'\n')
print('BIOS standalone render and physical IO PASS; DOS JMP table PASS')
