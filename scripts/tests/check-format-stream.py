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
for sectors in (256,4096,8192):
 for ram in (False,True):
  asm=f""" ORG 09000h
    ld a,b
    cp 3
    jp nz,write
    ld de,{sectors}
    ld bc,0
    or a
    ret
write:
    push hl
    push de
    push bc
    ld c,b
    ld a,e
    add a,20h
    ld d,a
    ld e,0
    ld a,c
    cp 2
    jp nz,copy
    ex de,hl
copy:
    ld b,0
loop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
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
  for at,p in [(0,config.binary_dir/'FORMAT.i80'),(0xc000,config.binary_dir/'DOS.sys'),(0x9000,out/'driver.bin')]:
   b=p.read_bytes();m[at:at+len(b)]=b
  m[d['bios_fileGetSetDrive']:d['bios_fileGetSetDrive']+2]=bytes.fromhex('AF C9')
  m[d['bios_diskDriver']:d['bios_diskDriver']+3]=bytes.fromhex('C3 00 90')
  m[d['j_diskDriver']+1:d['j_diskDriver']+3]=struct.pack('<H',0x9000)
  m[d['bios_printString']]=0xc9
  args=b'B: -Y'+(b' -R' if ram else b'')+b'\0';m[0x7000:0x7000+len(args)]=args
  m[0x8000:0x800c]=bytes.fromhex('31 DF 8F 11 00 70 CD 00 00 C3 00 81')
  m[f['buffer']-1]=0xa5;m[f['buffer']+256]=0x5a
  (out/'input.bin').write_bytes(m)
  run=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8000','-end','8100','-counter','10000000','-output',str(out/'output.bin'),str(out/'input.bin')],check=True,capture_output=True)
  measurements.append(dict(sectors=sectors,ram=ram,cycles=int(run.stdout.strip())))
  b=(out/'output.bin').read_bytes();assert b[65542:65546]==bytes.fromhex('00 81 DF 8F'),(sectors,ram)
  disk=b[0x2000:0x7000];word=lambda at:struct.unpack_from('<H',disk,at)[0]
  assert word(11)==256 and word(19)==sectors
  fatsectors=word(22);rootsectors=(word(17)*32+255)//256;start=1+fatsectors+rootsectors
  assert word(256)==0xfff8 and word(258)==0xffff
  for i in range(2,fatsectors*128):
   expected=0xffff if i>=sectors-start+2 else 0xfff7 if ram and (i-2+start)%256==255 else 0
   assert word(256+i*2)==expected,(sectors,ram,i,word(256+i*2),expected)
  root=disk[(1+fatsectors)*256:start*256]
  assert root[:11]==(b'RAM DISK   ' if ram else b'NO NAME    ') and root[11]==8 and root[12:]==bytes(len(root)-12)
  assert b[f['buffer']-1]==0xa5 and b[f['buffer']+256]==0x5a
print('PASS: 6 streamed formats, 256/4096/8192 sectors, FAT/root/BAD/buffer guards')

if config.timings:print(json.dumps(measurements))
