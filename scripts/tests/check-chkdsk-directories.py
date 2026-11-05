"""CHKDSK/8080: вложения, цепочки каталогов, dot/dotdot и KOI при выходе."""
from pathlib import Path
import re,struct,subprocess
R=Path(__file__).resolve().parents[2];O=R/'docs/cli/cli-001/chkdsk';O.mkdir(parents=True,exist_ok=True)
def symbols(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin'/name).read_text(),re.M)}
s=symbols('DOS.sym');c=symbols('CHKDSK.sym');W=lambda x:struct.pack('<H',x)
asm=O/'driver.asm';asm.write_text(""" ORG 0B000h
 ld a,b
 cp 3
 jp z,size
 cp 2
 jp nz,bad
 push hl
 push de
 push bc
 ld h,d
 ld l,e
 ld h,l
 ld l,0
 ld bc,09000h
 add hl,bc
 pop bc
 pop de
 ex (sp),hl
 ex de,hl
 pop hl
 push hl
 push de
 push bc
 ld bc,256
copy:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec bc
 ld a,b
 or c
 jp nz,copy
 pop bc
 pop de
 pop hl
 or a
 ret
size:
 ld de,32
 ld hl,0
 ld bc,0
 or a
 ret
bad:
 scf
 ret
""",encoding='utf-8')
# Более простой интерфейс: сохранить регистры, HL источника и DE назначения.
text=asm.read_text(encoding='utf-8');start=text.index(' push hl');end=text.index(' ld bc,256',start)
text=text[:start]+""" push hl
 push de
 push bc
 ld b,e
 ld c,0
 ld d,h
 ld e,l
 ld h,b
 ld l,c
 ld bc,09000h
 add hl,bc
"""+text[end:];asm.write_text(text,encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'driver.bin'),str(asm)],capture_output=True,check=True)
def entry(name,cluster=0,size=0,attr=0):
 b=bytearray(32);b[:11]=name; b[11]=attr;struct.pack_into('<HI',b,26,cluster,size);return b
def fixture():
 disk=bytearray(8192);disk[3:11]=b'MXOS256 ';disk[11:13]=W(256);disk[13]=1;disk[14:16]=W(1);disk[16]=1;disk[17:19]=W(8);disk[19:21]=W(32);disk[21]=0xF8;disk[22:24]=W(1)
 for cluster,next_cluster in [(0,0xFFF8),(1,0xFFFF),(2,5),(3,0xFFFF),(4,0xFFFF),(5,0xFFFF),(6,0xFFFF)]:disk[256+2*cluster:258+2*cluster]=W(next_cluster)
 def at(cluster):return (3+cluster-2)*256
 disk[512:544]=entry(b'ONE        ',2,attr=16);disk[544:576]=entry(b'FILE    BIN',6,3)
 for child,parent in [(2,0),(3,2)]:
  begin=at(child);disk[begin:begin+32]=entry(b'.          ',child,attr=16);disk[begin+32:begin+64]=entry(b'..         ',parent,attr=16)
 for i in range(2,8):disk[at(2)+i*32:at(2)+(i+1)*32]=entry(b'LABEL      ',attr=8)
 disk[at(5):at(5)+32]=entry(b'TWO        ',3,attr=16)
 disk[at(3)+64:at(3)+96]=entry(b'FILE    BIN',4,5)
 disk[at(4):at(4)+5]=b'HELLO';disk[at(6):at(6)+3]=b'ABC';return disk
for variant in ['valid','wrong-parent','cycle','duplicate','missing-dot']:
 disk=fixture()
 if variant=='wrong-parent':disk[(3+3-2)*256+58:(3+3-2)*256+60]=W(0)
 if variant=='cycle':disk[256+3*2:258+3*2]=W(3)
 if variant=='missing-dot':disk[(3+3-2)*256]=0xE5
 if variant=='duplicate':disk[576:608]=entry(b'FILE    BIN',0,0)
 m=bytearray(65536)
 for p,at in [(R/'bin/DOS.sys',0xC000),(R/'bin/CHKDSK.i80',0),(O/'driver.bin',0xB000)]:
  raw=p.read_bytes();m[at:at+len(raw)]=raw
 m[0x9000:0xB000]=disk;m[s['bios_diskDriver']:s['bios_diskDriver']+3]=b'\xC3'+W(0xB000)
 m[s['printChar']]=0xC9;m[s['bios_printChar']]=0xC9;m[s['bios_getch']:s['bios_getch']+3]=bytes.fromhex('3E 0D C9')
 m[s['bios_vars.koi7']]=1;m[s['v_drive']]=1
 m[0x5800:0x5806]=b' B:\r\0\0'
 stub=b'\x31'+W(s['STACK_ADDR'])+b'\x11'+W(0x5800)+bytes.fromhex('CD 00 00 C3 00 61');m[0x6000:0x6000+len(stub)]=stub
 (O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6000','-end','6100','-counter','20000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True)
 out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x6100,variant
 errors=int.from_bytes(out[c['errors']:c['errors']+2],'little');assert (errors==0)==(variant=='valid'),(variant,errors)
 assert out[s['bios_vars.koi7']]==1
 print('CHKDSK directories',variant,'errors',errors,'PASS')
