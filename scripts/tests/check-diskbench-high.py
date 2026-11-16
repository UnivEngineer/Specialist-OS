"""8080: FILE HIGH screen frame, limits, alignment and preserved failure status."""
from pathlib import Path
import re,struct,subprocess,json
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/main-high-2026-11-13/frame-cpu';O.mkdir(parents=True,exist_ok=True)
W=lambda n:struct.pack('<H',n)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
s=sy(R/'source-apps/diskbench/build/DISKMAIN.sym');base=bytearray(65536);b=(R/'bin/DISKMAIN.i80').read_bytes();base[0x2100:0x2100+len(b)]=b
asm=f''' ORG 0E000h
copy:
 push hl
 push de
 push bc
 inc hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 push de
 inc hl
 inc hl
 ld c,(hl)
 inc hl
 ld b,(hl)
 inc hl
 ld e,(hl)
 inc hl
 ld d,(hl)
 pop hl
 push de
 ld e,c
 ld d,b
 pop bc
 ld a,d
 cp 090h
 jp nz,loop
 ld a,(0{s['IO_COLOR']:04X}h)
 ld (0E305h),a
loop:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec bc
 ld a,b
 or c
 jp nz,loop
 pop bc
 pop de
 pop hl
 xor a
 ret
command:
 pop hl
 inc hl
 inc hl
 push hl
 xor a
 ret
file:
 ld a,(0{s['IO_COLOR']:04X}h)
 ld (0E304h),a
 ld hl,0E300h
 inc (hl)
 ld a,1
 ld (0{s['operation']:04X}h),a
 call 0{s['prepareFileBuffer']:04X}h
 ld a,(0E301h)
 or a
 ret z
 scf
 ret
 END
'''
(O/'hooks.asm').write_text(asm);subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080',f'--raw={O/"hooks.bin"}',f'--sym={O/"hooks.sym"}',str(O/'hooks.asm')],capture_output=True,check=True)
h=sy(O/'hooks.sym');b=(O/'hooks.bin').read_bytes();base[0xe000:0xe000+len(b)]=b
for original,hook in [('mx_memcpyFar','copy'),('mx_callRegs','command'),('filePass','file'),('fileReadPass','file')]:base[s[original]:s[original]+3]=b'\xc3'+W(h[hook])
rows=[]
for kib in (1,8,9,24):
 for offset in (0,1):
  for readonly in (False,True):
   for failure in (0,4):
    m=bytearray(base);m[s['config']+2]=int(not readonly);m[s['config']+3]=kib*4;m[s['config']+10]=offset;m[s['pass']]=2;m[0xe301]=failure;m[s['IO_COLOR']]=s['COLOR_BIOS']
    m[s['dataBuffer']:s['dataBuffer']+2]=W(s['buffer']+offset)
    m[s['bufferAddress']:s['bufferAddress']+2]=W(s['buffer']+offset)
    screen=bytes((i*13+(i>>8)+7)&255 for i in range(0x3000));m[0x9000:0xc000]=screen
    stub=b'\x31'+W(0xef00)+b'\xcd'+W(s['fileHigh'])+b'\xf5\xe1\x22'+W(0xe302)+b'\xc3'+W(0xe3ff);m[0xe320:0xe320+len(stub)]=stub;(O/'in.bin').write_bytes(m)
    subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','e320','-end','e3ff','-counter','12000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True)
    out=(O/'out.bin').read_bytes();assert out[65542:65544]==W(0xe3ff) and out[65544:65546]==W(0xef00)
    assert out[0x9000:0xc000]==screen,(kib,offset,readonly,failure,'screen')
    assert out[s['dataBuffer']:s['dataBuffer']+2]==W(s['buffer']+offset)
    assert out[s['fileStats']:s['fileStats']+2]==W(s['stats']+20)
    assert out[s['copyRequest']+4:s['copyRequest']+6]==W(s['config'])
    assert out[s['copyRequest']+6:s['copyRequest']+8]==W(11)
    assert out[s['IO_COLOR']]==s['COLOR_BIOS']
    if kib<=8:assert out[0xe304]==0 and out[0xe305]==s['COLOR_BIOS']
    assert out[0xe300]==int(kib<=8)
    assert bool(out[0xe302]&1)==bool(failure and kib<=8)
    if failure and kib<=8:assert out[0xe303]==failure
    rows.append(dict(KiB=kib,offset=offset,readonly=readonly,failure=failure,screenRestored=True))
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print('FILE HIGH FRAME CPU PASS',len(rows),'cases')
