"""Потоковый FORMAT: BPB, FAT, BAD-хвосты RAMD, корень и границы буфера."""
from pathlib import Path
import re,subprocess,struct,argparse,json
ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--binary-dir',type=Path,default=ROOT/'bin')
parser.add_argument('--timings',action='store_true')
config=parser.parse_args()
measurements=[]
out=ROOT/'docs/memory-map-004/format';out.mkdir(parents=True,exist_ok=True)
def sy(n):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(config.binary_dir/f'{n}.sym').read_text(),re.M)}
d,f=sy('DOS'),sy('FORMAT')
for sectors, variants in ((256,(False,True)),(4096,(False,True)),(8192,(False,True)),(1440,(False,))):
 for ram in variants:
  floppy=sectors==1440
  block_size=512 if floppy else 256
  technology=f['DISK_TECH_FLOPPY'] if floppy else f['DISK_TECH_RAM']
  capabilities=f['DISK_CAP_WRITE']
  asm=f""" ORG 09000h
    ld a,b
    cp 0FFh
    jp z,info
    cp 3
    jp nz,write
    ld de,{sectors}
    ld hl,08A00h
    ld bc,0
    or a
    ret
info:
    ld bc,{technology*256+capabilities}
    or a
    ret
write:
    push hl
    push de
    push bc
    ld c,b
    ld a,e
    IF {block_size} == 512
    add a,a
    ENDIF
    add a,20h
    ld d,a
    ld e,0
    ld a,c
    cp 2
    jp nz,copy
    ex de,hl
copy:
    ld c,{block_size//256}
    ld b,0
loop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,loop
    dec c
    jp nz,loop
    pop bc
    pop de
    pop hl
    or a
    ret
 END
"""
  (out/'driver.asm').write_text(asm)
  subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--raw={out/"driver.bin"}',str(out/'driver.asm')],check=True,capture_output=True)
  m=bytearray(65536)
  for at,p in [(0,config.binary_dir/'FORMAT.i80'),(0xc000,config.binary_dir/'BIOS.sys'),(0x9000,out/'driver.bin')]:
   b=p.read_bytes();m[at:at+len(b)]=b
  m[d['sysDiskDriver']:d['sysDiskDriver']+3]=bytes.fromhex('C3 00 90')
  m[f['mon2_printStringOld']]=0xc9
  m[f['FONT_ENGINE_ENCODING']]=0xc9
  # The selected drive is B:. Mount/chdir succeed; consume the inline operation word.
  m[0x8a00+f['DISK_INFO.blockShift']]=block_size.bit_length()-1
  caller=d['mx_callRegs']
  m[caller:caller+8]=bytes.fromhex('E1 23 23 E5 3E 01 B7 C9')
  args=b'B: -Y'+(b' -R' if ram else b'')+b'\0';m[0x7000:0x7000+len(args)]=args
  m[0x8000:0x800c]=bytes.fromhex('31 DF 8F 11 00 70 CD 00 00 C3 00 81')
  m[f['buffer']-1]=0xa5;m[f['buffer']+block_size]=0x5a
  m[0x2000:0x7000]=bytes([0xe5])*0x5000
  (out/'input.bin').write_bytes(m)
  run=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8100','-counter','10000000','-output',str(out/'output.bin'),str(out/'input.bin')],check=True,capture_output=True)
  measurements.append(dict(sectors=sectors,ram=ram,cycles=int(run.stdout.strip())))
  b=(out/'output.bin').read_bytes();assert b[65542:65546]==bytes.fromhex('00 81 DF 8F'),(sectors,ram)
  disk=b[0x2000:0x7000];word=lambda at:struct.unpack_from('<H',disk,at)[0]
  assert b[f['buffer']-1]==0xa5 and b[f['buffer']+block_size]==0x5a
  assert word(11)==block_size and word(19)==sectors
  if floppy:
   bpb=struct.unpack_from('<HBHBHHBHHHII',disk,11)
   assert bpb==(512,2,1,2,112,1440,0xf9,3,9,2,0,0)
   assert disk[54:62]==b'FAT12   '
   assert disk[510:512]==b'\x55\xaa'
   fat=b'\xf9\xff\xff'+bytes(1536-3)
   assert disk[512:2048]==fat and disk[2048:3584]==fat
   assert disk[3584:7168]==b'MXOS FLOPPY\x08'+bytes(3584-12)
   assert disk[7168:]==bytes([0xe5])*(len(disk)-7168)
   continue
  fatsectors=word(22);rootsectors=(word(17)*32+255)//256;start=1+fatsectors+rootsectors
  assert word(256)==0xfff8 and word(258)==0xffff
  for i in range(2,fatsectors*128):
   expected=0xffff if i>=sectors-start+2 else 0xfff7 if ram and (i-2+start)%256>=f['RAMDISK_DATA_BLOCK_LIMIT'] else 0
   assert word(256+i*2)==expected,(sectors,ram,i,word(256+i*2),expected)
  root=disk[(1+fatsectors)*256:start*256]
  assert root[:11]==(b'RAM DISK   ' if ram else b'NO NAME    ') and root[11]==8 and root[12:]==bytes(len(root)-12)
print('PASS: 6 MXOS256 cases and FAT12 720 KiB; BPB, FAT, root, BAD tails, buffer guards')

if config.timings:print(json.dumps(measurements))
