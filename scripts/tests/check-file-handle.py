"""Настоящее ядро 8080: файловый дескриптор, seek, рост и частичный обмен."""
from pathlib import Path
import json,re,struct,subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/file-api-001/cpu';OUT.mkdir(parents=True,exist_ok=True)
S={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/DOS.sym').read_text(),re.M)}
W=lambda x:struct.pack('<H',x)
mock=OUT/'driver.asm'
mock.write_text('''    ORG 0A000h
driver:
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
    jp copyStart
read:
    ld d,h
    ld e,l
    ld h,b
    ld l,c
copyStart:
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
''',encoding='utf-8')
subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(OUT/'driver.bin'),str(mock)],check=True,capture_output=True)
memory=bytearray(65536)
kernel=(ROOT/'bin/BIOS.sys').read_bytes();memory[0xC000:0xC000+len(kernel)]=kernel
module=(ROOT/'bin/DOS.native.sys').read_bytes();memory[S['DOS_MODULE_LOAD_ADDR']:S['DOS_MODULE_LOAD_ADDR']+len(module)]=module
memory[S['v_dosModuleReady']]=1
memory[S['mx_call']:S['mx_call']+S['sysTailEnd']-S['sysTail']]=memory[S['sysTail']:S['sysTailEnd']]
memory[:8192]=bytes(8192);memory[0x100:0x104]=bytes.fromhex('f8 ff ff ff')
memory[0xA000:0xA000+(OUT/'driver.bin').stat().st_size]=(OUT/'driver.bin').read_bytes()
memory[S['v_drives']+2:S['v_drives']+4]=W(0xA000);memory[S['j_diskDriver']:S['j_diskDriver']+3]=b'\xC3'+W(0xA000)
memory[S['v_drive']]=1
for name,value in dict(sectorSize=256,sectorsPerCluster=1,resSectors=1,fatSectors=1,dirSectors=1,totalSectors=32,rootDirMaxFiles=8,descrPerSector=8,fatNumCells=128,dirStartSector=2,dataStartSector=3,allocationStep=0,dataClusters=29,blockSize=256,clusterBytes=256,physicalBlocks=32).items():
 struct.pack_into('<H',memory,S['v_diskInfo.'+name],value)
for name,value in dict(isValid=2,sectorShift=8,blockShift=8,fatCount=1).items():memory[S['v_diskInfo.'+name]]=value
struct.pack_into('<H',memory,S['v_logicalCapacity'],32)
rows=[]
test_sp=S['STACK_ADDR']
def run(addr,a=0,hl=0):
 global memory,last_cycles
 code=b'\x31'+W(test_sp)+b'\x21'+W(hl)+bytes([0x3E,a,0xCD])+W(addr)+bytes.fromhex('F5 E1 22 00 3B C3 00 3D')
 memory[0x3C00:0x3C00+len(code)]=code
 (OUT/'in.bin').write_bytes(memory)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','3c00','-end','3d00','-counter','50000000','-output',str(OUT/'out.bin'),str(OUT/'in.bin')],capture_output=True,check=True)
 last_cycles=int(p.stdout.strip())
 snapshot=(OUT/'out.bin').read_bytes();assert int.from_bytes(snapshot[65542:65544],'little')==0x3D00
 memory=bytearray(snapshot[:65536]);return memory[0x3B01],bool(memory[0x3B00]&1)
def call(op,f=0x2000,error=0):
 got=run(S['bios_fileHandle'],op,f);assert got==(error,bool(error)),(op,got,error)
 rows.append(dict(cycles=last_cycles,op=op,error=error,position=int.from_bytes(memory[f+32:f+36],'little'),size=int.from_bytes(memory[f+28:f+32],'little'),count=int.from_bytes(memory[f+51:f+53],'little')))
def open_file(name=b'CHECK   BIN',mode=15,f=0x2000,error=0):
 memory[f:f+64]=bytes(64);memory[f:f+11]=name;memory[f+40:f+42]=bytes([1,mode]);call(0,f,error)
def io(op,n,data=None,f=0x2000,error=0):
 memory[f+49:f+53]=W(0x2400)+W(n)
 if data is not None:memory[0x2400:0x2400+n]=data
 call(op,f,error);return bytes(memory[0x2400:0x2400+int.from_bytes(memory[f+51:f+53],'little')])
def seek(n,origin=0,f=0x2000,error=0):
 memory[f+53:f+57]=struct.pack('<i',n);memory[f+57]=origin;call(3,f,error)

assert run(S['configureSectorCache'])==(0,False)
header=bytes(memory[0x8000:0x8040])
for pointer in [0x7FE0,0x8000,0x8FDF,0xFFFF]:assert run(S['bios_fileHandle'],0,pointer)==(7,True)
assert memory[0x8000:0x8040]==header
open_file()
payload=bytes((i*37+11)&255 for i in range(777))
for start,end in [(0,17),(17,256),(256,513),(513,777)]:io(2,end-start,payload[start:end])
assert int.from_bytes(memory[0x201c:0x2020],'little')==777
seek(0);assert io(1,800)==payload;assert memory[0x2000+44]==1
seek(-300,2);assert memory[0x2000+44]==0;assert io(1,200)==payload[477:677]
seek(-100,1);assert io(1,100)==payload[577:677]
seek(250);io(2,19,b'X'*19);payload=payload[:250]+b'X'*19+payload[269:]
call(4);call(5);call(1,error=7)
open_file(mode=1);assert io(1,800)==payload;call(5)
open_file(mode=19);seek(0);io(2,3,b'END');call(5);payload+=b'END'
open_file(mode=1);assert io(1,1000)==payload;seek(-1000,1,error=7);call(5)
open_file(mode=1,name=b'MISSING BIN',error=2)
open_file(mode=3);seek(100001);assert io(1,10)==b''
io(2,1,b'Q',error=4);assert int.from_bytes(memory[0x201c:0x2020],'little')==len(payload)
seek(0);assert io(1,1000)==payload;call(5)
open_file(mode=3);seek(1500);io(2,1,b'Z');call(5)
open_file(mode=1);seek(1500);assert io(1,1)==b'Z';assert io(1,1)==b'';call(5)
open_file(mode=1);io(2,1,b'R',error=6);assert int.from_bytes(memory[0x2000+51:0x2000+53],'little')==0;call(5)
open_file(mode=15,name=b'EMPTY   BIN');call(5)
open_file(mode=1,name=b'EMPTY   BIN');assert io(1,1)==b'';assert memory[0x2000+44]==1;call(5)
# Локальные объекты C находятся выше SP вызывающего, а не в рабочих кадрах DOS.
test_sp=0x8F50
open_file(mode=15,name=b'STACK   BIN',f=0x8F60)
memory[0x8FC0:0x8FC3]=b'ABC';memory[0x8F60+49:0x8F60+53]=W(0x8FC0)+W(3);call(2,0x8F60)
seek(0,f=0x8F60);memory[0x8FC0:0x8FC3]=bytes(3);memory[0x8F60+51:0x8F60+53]=W(3);call(1,0x8F60);assert memory[0x8FC0:0x8FC3]==b'ABC';call(5,0x8F60)
test_sp=S['STACK_ADDR']
# Повтор close после аппаратной ошибки сохраняет открытый объект и dirty-кэш.
open_file(mode=15,name=b'RETRY   BIN');io(2,7,b'RETRIES')
driver=bytes(memory[0xA000:0xA003]);memory[0xA000:0xA003]=b'\x37\xC9\x00'
call(5,error=1);assert memory[0x2000+42]==0x46
memory[0xA000:0xA003]=driver;call(5)
open_file(mode=1,name=b'RETRY   BIN');assert io(1,20)==b'RETRIES';call(5)
open_file(mode=1);call(0,error=7)
seek(65530);seek(20,1);assert int.from_bytes(memory[0x2020:0x2024],'little')==65550
seek(-40,1);seek(-65511,1,error=7);assert int.from_bytes(memory[0x2020:0x2024],'little')==65510
seek(0x7FFFFFFF);seek(0x7FFFFFFF,1);seek(4,1,error=7);assert int.from_bytes(memory[0x2020:0x2024],'little')==0xFFFFFFFE
call(5)
# Цикл и BAD в цепочке завершаются ошибкой, а не выдаются за обычный EOF.
first=int.from_bytes(memory[0x0200+26:0x0200+28],'little')
last=first
while int.from_bytes(memory[0x0100+last*2:0x0102+last*2],'little')<0xFFF8:last=int.from_bytes(memory[0x0100+last*2:0x0102+last*2],'little')
saved=bytes(memory[0x0100+last*2:0x0102+last*2]);memory[0x0100+last*2:0x0102+last*2]=W(first)
run(S['configureSectorCache']);open_file(mode=1);io(1,1600,error=8);assert memory[0x2000+44]==0;call(5)
memory[0x0100+last*2:0x0102+last*2]=saved;run(S['configureSectorCache'])
saved=bytes(memory[0x0100+first*2:0x0102+first*2]);memory[0x0100+first*2:0x0102+first*2]=W(0xFFF7)
run(S['configureSectorCache']);open_file(mode=1);io(1,1600,error=8);call(5)
memory[0x0100+first*2:0x0102+first*2]=saved;run(S['configureSectorCache'])
# Все выделенные кластеры достижимы из корня, включая отказ роста на полном диске.
used={c for c in range(2,31) if int.from_bytes(memory[0x100+c*2:0x102+c*2],'little')}
reachable=set()
for at in range(0x200,0x300,32):
 if memory[at] in (0,0xE5):continue
 c=int.from_bytes(memory[at+26:at+28],'little')
 while c and c<0xFFF8:
  assert c not in reachable and 2<=c<=30,c
  reachable.add(c);c=int.from_bytes(memory[0x100+c*2:0x102+c*2],'little')
assert reachable==used,(reachable,used)
# Освобождение модуля не снимает дополнительное ограничение владельца памяти.
for top,expected in [(0x2FFF,0x2FFF),(0x3FFF,0x3FFF)]:
 memory[S['v_memTop']:S['v_memTop']+2]=W(top);memory[S['v_dosModuleReady']]=1
 assert run(S['bios_releaseDosModule'])==(0,False)
 assert int.from_bytes(memory[S['v_memTop']:S['v_memTop']+2],'little')==expected
 assert memory[S['v_dosModuleReady']]==1
memory[S['v_memTop']:S['v_memTop']+2]=W(S['APP_MEMORY_END']-1);memory[S['v_dosModuleReady']]=1
default_memory=bytearray(memory)
geometries=[]
for sector_size,cluster_sectors in [(128,1),(256,4),(512,2)]:
 block_size=min(sector_size,256)
 text=mock.read_text(encoding='utf-8').replace('ld bc,256','ld bc,'+str(block_size))
 if block_size==128:
  text=text.replace('    ld b,e\n    ld c,0','    ld b,e\n    ld c,0\n    push af\n    ld a,b\n    or a\n    rra\n    ld b,a\n    ld a,c\n    rra\n    ld c,a\n    pop af')
 (OUT/'geometry.asm').write_text(text,encoding='utf-8')
 subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(OUT/'geometry.bin'),str(OUT/'geometry.asm')],check=True,capture_output=True)
 memory=bytearray(65536);kernel=(ROOT/'bin/BIOS.sys').read_bytes();memory[0xC000:0xC000+len(kernel)]=kernel
 memory[S['DOS_MODULE_LOAD_ADDR']:S['DOS_MODULE_LOAD_ADDR']+len(module)]=module;memory[S['v_dosModuleReady']]=1
 memory[S['mx_call']:S['mx_call']+S['sysTailEnd']-S['sysTail']]=memory[S['sysTail']:S['sysTailEnd']]
 driver=(OUT/'geometry.bin').read_bytes();memory[0xA000:0xA000+len(driver)]=driver
 memory[S['v_drives']+2:S['v_drives']+4]=W(0xA000);memory[S['j_diskDriver']:S['j_diskDriver']+3]=b'\xC3'+W(0xA000);memory[S['v_drive']]=1
 for name,value in dict(sectorSize=sector_size,sectorsPerCluster=cluster_sectors,resSectors=1,fatSectors=1,dirSectors=1,totalSectors=32,rootDirMaxFiles=sector_size//32,descrPerSector=sector_size//32,fatNumCells=sector_size//2,dirStartSector=2,dataStartSector=3,allocationStep=0,dataClusters=29//cluster_sectors,blockSize=block_size,clusterBytes=sector_size*cluster_sectors,physicalBlocks=32*sector_size//block_size).items():struct.pack_into('<H',memory,S['v_diskInfo.'+name],value)
 for name,value in dict(isValid=2,sectorShift=sector_size.bit_length()-1,blockShift=block_size.bit_length()-1,fatCount=1).items():memory[S['v_diskInfo.'+name]]=value
 memory[S['v_logicalShift']]=sector_size.bit_length()-block_size.bit_length()
 struct.pack_into('<H',memory,S['v_logicalCapacity'],32);memory[sector_size:sector_size+4]=bytes.fromhex('f8 ff ff ff')
 run(S['configureSectorCache']);open_file()
 sample=bytes((i*41+3)&255 for i in range(1703));io(2,1000,sample[:1000]);io(2,703,sample[1000:])
 seek(33);io(2,37,b'Q'*37);sample=sample[:33]+b'Q'*37+sample[70:];call(5)
 open_file(mode=1);assert io(1,2000)==sample;seek(-128,2);assert io(1,128)==sample[-128:];call(5)
 geometries.append(dict(logicalSector=sector_size,physicalBlock=block_size,clusterSectors=cluster_sectors,bytes=len(sample)))
memory=default_memory
# Публичный секторный ABI сохраняет проверку режима и границ LBA.
for mode,lba in [(0,0),(3,0),(255,0),(1,32),(2,32),(2,65535)]:
 memory[0x3E00:0x3E09]=b'\x01'+W(mode<<8)+b'\x11'+W(lba)+b'\xC3'+W(S['bios_sectorIO'])
 assert run(0x3E00,hl=0x2400)==(7,True),(mode,lba)
# Чистый writable-handle закрывается без записи каталога: запрещаем драйверу WRITE.
memory[0x2000:0x2040]=bytes(64);memory[0x2000:0x200B]=b'CHECK   BIN';memory[0x2028:0x202A]=bytes([1,3])
assert run(S['bios_fileHandle'],0,0x2000)==(0,False)
assert run(S['bios_fileHandle'],4,0x2000)==(0,False)
original=bytes(memory[0xA000:0xA003])
stub=bytes.fromhex('78 FE 01 37 C8 78 FE 04 C3 03 A0')
memory[0xA100:0xA100+len(stub)]=stub;memory[0xA000:0xA003]=bytes.fromhex('C3 00 A1')
assert run(S['bios_fileHandle'],5,0x2000)==(0,False)
memory[0xA000:0xA003]=original
(OUT/'results.json').write_text(json.dumps(rows,indent=2),encoding='utf-8')
(OUT/'geometries.json').write_text(json.dumps(geometries,indent=2),encoding='utf-8')
print('FILE HANDLE PASS',len(rows))
