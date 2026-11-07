"""CLI и изменение каталогов на настоящем коде DOS/8080 с блочным RAM-томом."""
from pathlib import Path
import re,struct,subprocess,argparse
R=Path(__file__).resolve().parents[2]
O=R/'docs/cli/cli-001/cpu';O.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOS.sym').read_text(),re.M)}
M={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOSMOD.sym').read_text(),re.M)}
W=lambda x:struct.pack('<H',x)
parser=argparse.ArgumentParser();parser.add_argument('--sector-size',type=int,choices=[128,256,512],default=256);args=parser.parse_args()
sector=args.sector_size;physical=min(sector,256);spc=2 if sector==128 else 1
root=2*sector;data_start=3*sector
# Драйвер B=2 читает, B=1 пишет один 256-байтный аппаратный блок.
asm=O/'driver.asm';asm.write_text(""" ORG 0A000h
 ld a,b
 cp 4
 ret z
 push hl
 push de
 push bc
 ld a,b
 ld b,e
 ld c,0
 cp 2
 jp z,read
 ld d,b
 ld e,c
 jp copy
read:
 ld d,h
 ld e,l
 ld h,b
 ld l,c
copy:
 ld bc,256
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
 or a
 ret
""",encoding='utf-8')
text=asm.read_text(encoding='utf-8').replace('ld bc,256','ld bc,'+str(physical))
if physical==128:text=text.replace('ld b,e\n ld c,0','ld b,e\n ld c,0\n push af\n ld a,b\n or a\n rra\n ld b,a\n ld a,c\n rra\n ld c,a\n pop af')
asm.write_text(text,encoding='utf-8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'driver.bin'),str(asm)],check=True,capture_output=True)
m=bytearray(65536)
for p,at in [(R/'bin/BIOS.sys',0xC000),(R/'bin/IO.sys',0xF000),(R/'bin/DOS.native.sys',S['DOS_CODE_START']),(O/'driver.bin',0xA000)]:
 b=p.read_bytes();m[at:at+len(b)]=b
m[S['v_dosModuleReady']]=1;m[S['v_drive']]=0
m[S['j_diskDriver']:S['j_diskDriver']+3]=b'\xC3'+W(0xA000)
m[S['bios_printChar']]=0xC9
# printString вызывает внутренний printChar напрямую: экран содержит тестовый
# драйвер A000h, поэтому отключаем и этот путь вывода (UI проверяется в Emu80).
m[S['printChar']]=0xC9
m[S['v_memTop']:S['v_memTop']+2]=W(S['APP_MEMORY_END']-1)
m[sector:sector+4]=bytes.fromhex('f8 ff ff ff')
for name,value in dict(sectorSize=sector,sectorsPerCluster=spc,resSectors=1,fatSectors=1,dirSectors=1,totalSectors=32,rootDirMaxFiles=sector//32,descrPerSector=sector//32,fatNumCells=sector//2,dirStartSector=2,dataStartSector=3,allocationStep=0,dataClusters=29//spc,blockSize=physical,clusterBytes=sector*spc,physicalBlocks=32*sector//physical).items():struct.pack_into('<H',m,S['v_diskInfo.'+name],value)
for name,value in dict(isValid=2,sectorShift=sector.bit_length()-1,blockShift=physical.bit_length()-1,fatCount=1).items():m[S['v_diskInfo.'+name]]=value
m[S['v_logicalCapacity']:S['v_logicalCapacity']+2]=W(32)
def run(addr,a=0,hl=0,de=0,bc=0):
 global m
 code=b'\x31'+W(S['STACK_ADDR'])+b'\x01'+W(bc)+b'\x21'+W(hl)+b'\x11'+W(de)+bytes([0x3E,a,0xCD])+W(addr)+bytes.fromhex('F5 E1 22 00 5F C3 00 61')
 m[0x6000:0x6000+len(code)]=code;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','6000','-end','6100','-counter','20000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x6100,hex(int.from_bytes(out[65542:65544],'little'))
 m=bytearray(out[:65536]);return m[0x5F01],bool(m[0x5F00]&1)
def command(text,error=0):
 data=text.encode()+b'\0';m[0x5000:0x5000+len(data)]=data
 got=run(S['bios_cli'],0,0x5000);assert got==(error,bool(error)),(text,got,error,m[M['cliLine']:M['cliLine']+128])
 print(text,got,flush=True)
def word(name):return name.ljust(8).encode()+b'   '
def cwd():return int.from_bytes(m[S['v_directory']:S['v_directory']+2],'little')
assert run(S['configureSectorCache'])==(0,False)
command('MD ONE');assert m[root:root+3]==b'ONE';assert m[root+11]==0x10
first=int.from_bytes(m[root+26:root+28],'little');assert first>=2
assert m[data_start:data_start+11]==b'.          ' and m[data_start+32:data_start+43]==b'..         '
assert int.from_bytes(m[data_start+26:data_start+28],'little')==first and m[data_start+28:data_start+32]==bytes(4)
command('CD ONE');assert cwd()==first
command('MKDIR TWO');command('CD TWO');second=cwd();assert second!=first
command('CD /');assert cwd()==0
command('RD ONE',S['DOS_ERR_ACCESS'])
command('RD ONE/TWO');command('RD ONE')
# Данные обычного файла для COPY/MOVE/REN/DEL и потокового TYPE.
m[0x5400:0x5420]=word('FILE')+bytes(21);m[0x541C:0x5420]=struct.pack('<I',5);m[0x5700:0x5705]=b'HELLO'
assert run(S['bios_fileCreate'],hl=0x5400,de=0x5700)==(0,False)
command('REN FILE file');assert b'file       ' in m[root:root+sector]
command('REN file FILE')
command('MD DST');command('COPY FILE DST');assert cwd()==0
for text,output in [('CD DST',4),('A:',4),('MD NEW',0),('DIR',1),('CD',1),('PATH A:/',0),('ATTRIB +R FILE',0),('ATTRIB FILE',1)]:
 data=text.encode()+b'\0';m[0x5000:0x5000+len(data)]=data
 assert run(S['bios_cli'],S['CLI_OP_QUERY'],0x5000)==(0,False)
 assert m[M['cliOutput']]==output and cwd()==0,(text,m[M['cliOutput']])
command('ATTRIB +RH FILE');command('DEL FILE',S['DOS_ERR_ACCESS']);command('ATTRIB -RH +A FILE')
m[0x5400:0x5420]=word('FILE')+bytes(21)
assert run(S['bios_fileGetSetAttr'],hl=0x5400)==(0x20,False)
command('ATTRIB +H DST')
m[0x5400:0x5420]=word('DST')+bytes(21)
assert run(S['bios_fileGetSetAttr'],hl=0x5400)==(0x12,False)
assert run(S['bios_fileGetSetAttr'],hl=0x5400,bc=1)==(0x10,False)
assert run(S['bios_fileGetSetAttr'],hl=0x5400,bc=0x0801)==(S['DOS_ERR_ARGUMENT'],True)
fat_before=bytes(m[sector:2*sector])
command('MOVE FILE DST/MOVED')
assert bytes(m[sector:2*sector])==fat_before,'MOVE changed FAT'
command('MOVE DST/MOVED FILE')
assert bytes(m[sector:2*sector])==fat_before,'MOVE back changed FAT'
# Программный MOVE не требует буфера данных в пределах одного тома.
req=0x5500;m[req:req+S['FILE_COPY_REQUEST']]=bytes(S['FILE_COPY_REQUEST'])
src=req+S['FILE_COPY_REQUEST.source'];dst=req+S['FILE_COPY_REQUEST.destination']
m[src:src+11]=word('FILE');m[dst:dst+11]=word('VIAABI')
got=run(S['bios_cli'],S['CLI_OP_MOVE'],req);assert got==(0,False),(got,m[S['v_dosError']],m[M['cliRequest']:M['cliRequest']+84].hex())
assert bytes(m[sector:2*sector])==fat_before
command('MOVE VIAABI FILE')
command('MOVE FILE DST/FILE',S['DOS_ERR_EXISTS'])
command('ATTRIB +R -R FILE')
m[0x5400:0x5420]=word('FILE')+bytes(21)
assert run(S['bios_fileGetSetAttr'],hl=0x5400)==(0x20,False)
command('CD DST');saved_cwd=cwd()
command('DIR A:');assert cwd()==saved_cwd
command('CD A:');assert cwd()==saved_cwd
command('TYPE FILE');command('REN FILE NEW');command('ERASE NEW');command('CD /')
command('MOVE FILE MOVED');command('DEL MOVED');command('RD DST')
command('MD ONE');command('CD ONE');command('RD /ONE',S['DOS_ERR_ACCESS']);command('CD /');command('RD ONE')
command('MD ONE');command('MD ONE',S['DOS_ERR_EXISTS']);command('RD ONE')
command('MD A/B',S['DOS_ERR_NOT_FOUND'])
for path in ['.','..','/','TOOLONG99','X.']:command('MD '+path,S['DOS_ERR_ARGUMENT'])
m[0x5000:0x5004]=b'CDX\0';assert run(S['bios_cli'],0,0x5000)==(255,False)
# Переименование регистра и непустых каталогов, MOVE без изменения FAT.
command('MD TREE');command('MD TREE/CHILD');command('MD DEST')
command('CD TREE/CHILD');command('MD DEEP')
for leaf in ['FIRST','SECOND']:
 m[0x5400:0x5420]=word(leaf)+bytes(21);m[0x541C:0x5420]=struct.pack('<I',5);m[0x5700:0x5705]=b'HELLO'
 assert run(S['bios_fileCreate'],hl=0x5400,de=0x5700)==(0,False)
command('CD /')
command('REN TREE Tree');assert b'Tree       ' in m[root:root+sector]
command('COPY Tree Tree/CHILD',S['DOS_ERR_ACCESS'])
req=0x5500;m[req:req+S['FILE_COPY_REQUEST']]=bytes(S['FILE_COPY_REQUEST'])
m[req+S['FILE_COPY_REQUEST.source']:req+S['FILE_COPY_REQUEST.source']+11]=word('Tree')
m[req+S['FILE_COPY_REQUEST.destination']:req+S['FILE_COPY_REQUEST.destination']+11]=word('CLONE')
m[req+S['FILE_COPY_REQUEST.buffer']:req+S['FILE_COPY_REQUEST.buffer']+2]=W(0x5700)
m[req+S['FILE_COPY_REQUEST.bufferSize']:req+S['FILE_COPY_REQUEST.bufferSize']+2]=W(256)
assert run(S['bios_cli'],S['CLI_OP_COPY'],req)==(0,False)
command('CD CLONE/CHILD')
for leaf in ['FIRST','SECOND']:
 m[0x5400:0x540B]=word(leaf)
 assert run(S['fileFindAny'],hl=0x5400)[1]==False
 cl=int.from_bytes(m[S['v_tmpFileDescr']+26:S['v_tmpFileDescr']+28],'little')
 assert m[data_start+(cl-2)*sector*spc:data_start+(cl-2)*sector*spc+5]==b'HELLO'
 command('DEL '+leaf)
command('CD /')
command('RD CLONE/CHILD/DEEP');command('RD CLONE/CHILD');command('RD CLONE')
fat_before=bytes(m[sector:root])
command('MOVE Tree Tree/CHILD',S['DOS_ERR_ACCESS'])
command('MOVE Tree DEST')
assert bytes(m[sector:root])==fat_before
command('CD DEST/Tree/CHILD');command('CD ..');command('PWD')
command('CD CHILD');command('DEL FIRST');command('DEL SECOND');command('RD DEEP')
command('CD /');command('RD DEST/Tree/CHILD');command('RD DEST/Tree');command('RD DEST')
# Общий поиск расширения и ассоциаций, без запуска чужой среды.
def create(short,data):
 m[0x5400:0x5420]=short+bytes(21);m[0x541C:0x5420]=struct.pack('<I',len(data));m[0x5700:0x5700+len(data)]=data
 assert run(S['bios_fileCreate'],hl=0x5400,de=0x5700)==(0,False)
create(b'CLI     EXT',b'RKS A:\\RUN.COM\r\nEXE A:\\RUN.COM\r\n')
create(b'GAME    EXE',b'X')
create(b'GAME    RKS',b'X')
for text,want in [('GAME ARG','A:\\RUN.COM GAME.EXE ARG'),('GAME.RKS -MON','A:\\RUN.COM GAME.RKS -MON')]:
 data=text.encode()+b'\0';m[0x5000:0x5000+len(data)]=data
 got=run(M['cliResolveExternal'],hl=0x5000);assert got==(0,False),(text,got,m[S['v_dosError']])
 got=m[S['CMD_LINE']:S['CMD_LINE']+128].split(b'\0')[0].decode();assert got==want,(got,want)
command('A:')
command('MD BIN');command('MOVE GAME.EXE BIN')
for path_setting in ['BIN','MISSING;BIN','A:/BIN']:
 command('PATH '+path_setting)
 data=b'GAME ARG\0';m[0x5000:0x5000+len(data)]=data
 assert run(M['cliResolveExternal'],hl=0x5000)==(0,False)
 got=m[S['CMD_LINE']:S['CMD_LINE']+128].split(b'\0')[0];assert got==b'A:\\RUN.COM A:\\BIN\\GAME.EXE ARG',got
 assert cwd()==0 and m[S['v_directories']:S['v_directories']+2]==b'\0\0'
command('PATH ;')
data=b'GAME\0';m[0x5000:0x5000+len(data)]=data
assert run(M['cliResolveExternal'],hl=0x5000)==(2,True)
command('PATH A:/BIN')
module=(R/'bin/DOS.native.sys').read_bytes();start=S['DOS_CODE_START'];m[start:start+len(module)]=module
assert m[S['v_cliPath']:S['v_cliPath']+6]==b'A:/BIN'
data=b'GAME.EXE ARG\0';m[0x5000:0x5000+len(data)]=data
assert run(M['cliResolveExternal'],hl=0x5000)==(0,False)
assert m[S['CMD_LINE']:S['CMD_LINE']+128].split(b'\0')[0]==b'A:\\RUN.COM A:\\BIN\\GAME.EXE ARG'
# Ассоциация имеет приоритет, а без неё COM/EXE исполняет общий загрузчик.
import sys
sys.path.insert(0,str(R/'scripts/images'))
from executable import Image,encode
probe=bytes.fromhex('3E 5A 32 00 5E C9')
create(b'NATIVE  EXE',encode(Image(probe,0x4000,0x4000),'exe'))
data=b'NATIVE.EXE\0';m[0x5000:0x5000+len(data)]=data
assert run(M['cliResolveExternal'],hl=0x5000)==(0,False)
assert m[S['CMD_LINE']:S['CMD_LINE']+128].split(b'\0')[0]==b'A:\\RUN.COM NATIVE.EXE'
create(b'CLI     EXT',b'RKS A:\\RUN.COM\r\n')
for extension in ['EXE','COM']:
 if extension=='COM':
  command('DEL GAME.RKS');create(b'NATIVE  COM',encode(Image(probe,0x4000,0x4000),'com'))
 data=('NATIVE.'+extension).encode()+b'\0';m[0x5000:0x5000+len(data)]=data;m[0x5E00]=0
 got=run(S['bios_fileExec'],hl=0x5000);assert not got[1],got
 assert m[0x5E00]==0x5A,extension
print('CLI CPU PASS; PATH reload; COM/EXE native and association priority')
