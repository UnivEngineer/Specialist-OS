"""CPU 8080: настоящий секторный загрузчик DOS с моделью физического чтения."""
from pathlib import Path
import json,re,runpy,struct,subprocess,sys
root=Path(__file__).resolve().parents[2];out=root/'docs/compat/sector-load-001/cpu';out.mkdir(exist_ok=True)
sys.path.insert(0,str(root/'scripts/images'))
from executable import Image,encode
sy={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(root/'bin/DOS.sym').read_text(),re.M)}
rs={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(root/'bin/RUN.sym').read_text(),re.M)}
mock=out/'mock.asm';mock.write_text("""    ORG 0F900h
stat:
    or a
    ret
next:
    push hl
    push bc
    ld h,d
    ld l,e
    add hl,hl
    ld bc,07400h
    add hl,bc
    ld e,(hl)
    inc hl
    ld d,(hl)
    pop bc
    pop hl
    ret
sector:
    dec de
    dec de
    ret
cache:
    push bc
    ld hl,0FB00h
    ld b,2
    call read
    pop bc
    ret
read:
    ld (07200h),hl
    push hl
    push de
    push bc
    ld hl,(07202h)
    ld a,(07200h)
    ld (hl),a
    inc hl
    ld a,(07201h)
    ld (hl),a
    inc hl
    ld (07202h),hl
    ld hl,(07204h)
    ld a,h
    cp d
    jp nz,readOK
    ld a,l
    cp e
    jp z,readFail
readOK:
    ld a,e
    add a,030h
    ld h,a
    ld l,0
    push hl
    ld hl,(07200h)
    ex de,hl
    pop hl
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
readFail:
    pop bc
    pop de
    pop hl
    scf
    ret
    END
""",encoding='utf-8')
subprocess.run([str(root/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(mock.with_suffix('.bin')),'--sym='+str(mock.with_suffix('.sym')),str(mock)],capture_output=True,check=True)
ms={k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)',mock.with_suffix('.sym').read_text(),re.M)}
def patch(m,label,target):
 m[sy[label]:sy[label]+3]=b'\xc3'+target.to_bytes(2,'little')
def run(size,load=0x1000,rks=False,fragmented=False,corrupt=False,error=-1,full_run=False,flag_first=False,dos_exec=False,return_run=False):
 payload=bytes((i*37+11)&255 for i in range(size))
 if full_run or dos_exec:payload=bytes.fromhex('C3 00 83')+payload[3:]
 if return_run:payload=b'\xc9'+payload[1:]
 packed=bytearray(encode(Image(payload,load,load),'rks' if rks else 'com'))
 if corrupt:packed[-1]^=1
 m=bytearray(65536);code=(root/'bin/DOS.sys').read_bytes();m[0xc000:0xc000+len(code)]=code
 rcode=(root/'bin/RUN.i80').read_bytes();m[rs['start']:rs['start']+len(rcode)]=rcode
 m[0xf900:0xf900+len(mock.with_suffix('.bin').read_bytes())]=mock.with_suffix('.bin').read_bytes()
 patch(m,'fileSelectDrive',ms['stat']);patch(m,'printCharA',ms['stat']);patch(m,'j_printString',ms['stat']);patch(m,'fileGetSetDrive',ms['stat']);patch(m,'fileLoadInfo',ms['stat']);patch(m,'fatReadCluster',ms['next']);patch(m,'fatClusterToSector',ms['sector']);patch(m,'getSectorAddressInCache',ms['cache']);patch(m,'j_diskDriver',ms['read']);patch(m,'fileTransferClusters',sy['fatFileTransferClusters']);patch(m,'fileTransfer',sy['fatFileTransferClusters']) if 'fileTransfer' in sy else None
 # Публичный вход остаётся тем же; диспетчер томов здесь не моделируется.
 m[sy['bios_fileTransfer']:sy['bios_fileTransfer']+3]=b'\xc3'+sy['fatFileTransferClusters'].to_bytes(2,'little')
 count=(len(packed)+255)//256;chain=[2+i*(3 if fragmented else 1) for i in range(count)]
 for i,c in enumerate(chain):
  chunk=packed[i*256:(i+1)*256];pos=0x3000+(c-2)*256
  assert pos+256<=0x7000,(size,fragmented)
  m[pos:pos+256]=chunk+bytes([0xEE])*(256-len(chunk))
  struct.pack_into('<H',m,0x7400+c*2,chain[min(i+1,count-1)])
 m[sy['bios_vars.koi7']]=255 if return_run else 0
 ctx=sy['v_tmpFileDescr'];m[ctx:ctx+32]=bytes(32);struct.pack_into('<H',m,ctx+26,chain[0]);struct.pack_into('<I',m,ctx+28,len(packed))
 struct.pack_into('<H',m,0x7202,0x7300);struct.pack_into('<H',m,0x7204,error&65535)
 m[load-1 if load else 0xFFFF]=0xA5;m[load+size]=0x5A
 for pos in range(0x9000,0xc000):m[pos]=(pos*13+17)&255
 # Проход через первый сектор и штатные проверки заголовка; RKS
 # нормализует поля в контексте, как это делает RUN.
 code=bytes.fromhex('31')+sy['STACK_ADDR'].to_bytes(2,'little')+bytes((0x3E,4 if rks else 16))+b'\x21'+ctx.to_bytes(2,'little')+b'\xcd'+sy['imageOpen'].to_bytes(2,'little')+bytes.fromhex('DA 00 83')
 if rks:
  for offset,value in ((6,load),(8,load),(10,size)):
   code+=b'\x21'+value.to_bytes(2,'little')+b'\x22'+(ctx+offset).to_bytes(2,'little')
  code+=b'\x11'+load.to_bytes(2,'little')
 # Штатный ввод между imageOpen/imageLoad использует общие v_input_*:
 # подготовленный сектор должен пережить такой ввод.
 code+=bytes.fromhex('21 55 55 22')+sy['v_input_start'].to_bytes(2,'little')
 code+=b'\xcd'+sy['imageLoad'].to_bytes(2,'little')+bytes.fromhex('DA 00 83')
 if rks:code+=b'\xcd'+rs['rksChecksum'].to_bytes(2,'little')
 code+=bytes.fromhex('C3 00 83')
 if full_run:
  name='GAME.RKS' if rks else 'GAME.COM'
  command=('-MX '+name+' /Q') if flag_first else (name+' /Q -MX')
  raw=command.encode()+b'\0';m[0x7100:0x7100+len(raw)]=raw
  code=b'\x31'+sy['STACK_ADDR'].to_bytes(2,'little')+bytes.fromhex('11 00 71 CD')+rs['start'].to_bytes(2,'little')+bytes.fromhex('C3 00 83')
 if dos_exec:
  raw=b'GAME.COM /Q\0';m[0x7100:0x7100+len(raw)]=raw
  code=b'\x31'+sy['STACK_ADDR'].to_bytes(2,'little')+bytes.fromhex('21 00 71 CD')+sy['bios_fileExec'].to_bytes(2,'little')+bytes.fromhex('C3 00 83')
 m[0x8200:0x8200+len(code)]=code
 inp=out/'input.bin';target=out/'output.bin';inp.write_bytes(m)
 p=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','8300','-counter','10000000','-output',str(target),str(inp)],check=True,capture_output=True,text=True,timeout=15)
 got=target.read_bytes();assert int.from_bytes(got[65542:65544],'little')==0x8300,(size,rks,got[65536:].hex())
 failed=bool(got[65536]&1) if not (full_run or dos_exec) else False
 assert failed==(corrupt or error>=0),(size,load,rks,fragmented,corrupt,error,got[65536:].hex())
 assert got[0x9000:0xc000]==m[0x9000:0xc000], 'screen modified'
 assert got[load-1 if load else 0xFFFF]==0xA5 and got[load+size]==0x5A,(size,load,'tail guard')
 if return_run:assert got[sy['bios_vars.koi7']]==255,'caller KOI preserved'
 if not failed:
  if not rks and not (full_run or dos_exec):assert int.from_bytes(got[65540:65542],'little')==load,'entry address'
  if dos_exec:
   args=int.from_bytes(got[65548:65550],'little');assert got[args:args+3]==b'/Q\r','DOS arguments'
  if full_run:
   args=0x8e00 if load>=0xc000 else sy['SYSTEM_PROGRAM_ADDR']
   assert got[args:args+3]==b'/Q\0','program arguments'
  assert got[load:load+size]==payload,(size,load,rks,'payload',got[load:load+16].hex())
  if not (full_run or dos_exec):assert int.from_bytes(got[65544:65546],'little')==sy['STACK_ADDR']
  ptr=int.from_bytes(got[0x7202:0x7204],'little');trace=[int.from_bytes(got[i:i+2],'little') for i in range(0x7300,ptr,2)]
  assert all(x==0xFB00 or (rks and x==rs['sectorBuffer']) or (0 if (full_run and load>=0xc000) or (dos_exec and load>=sy['DISK_DRIVER_ADDR']) else load)<=x<(0 if (full_run and load>=0xc000) or (dos_exec and load>=sy['DISK_DRIVER_ADDR']) else load)+size for x in trace),(size,trace)
  if size>=512:assert any(x!=0xFB00 for x in trace),(size,'no direct sector')
 return dict(size=size,load=load,fullRun=full_run,dosExec=dos_exec,rks=rks,fragmented=fragmented,corrupt=corrupt,errorSector=error)
results=[]
for size in (1,2,15,16,17,239,240,241,250,251,252,253,254,255,256,257,495,496,497,506,507,508,509,511,512,513,1025,4097):
 for rks in (False,True):
  for fragmented in (False,True):results.append(run(size,rks=rks,fragmented=fragmented))
for load in (0,0x10F3,0x8EED):
 for rks in (False,True):results.append(run(239,load=load,rks=rks))
for rks in (False,True):results.append(run(513,rks=rks,corrupt=True))
for error in (0,1,2):
 for rks in (False,True):results.append(run(513,error=error,rks=rks))
results.append(run(251,rks=True,error=1))
results.append(run(252,rks=True,error=1))
for load in (0x1000,sy['SYSTEM_PROGRAM_ADDR']):
 for rks in (False,True):
  for first in (False,True):results.append(run(513,load=load,rks=rks,full_run=True,flag_first=first))
for load in (0,0x1000,sy['SYSTEM_PROGRAM_ADDR'],0xF100,sy['DISK_DRIVER_ADDR']):
 results.append(run(509 if load==sy['DISK_DRIVER_ADDR'] else 513,load=load,dos_exec=True))
for load in (0x1000,sy['SYSTEM_PROGRAM_ADDR']):
 for rks in (False,True):results.append(run(513,load=load,rks=rks,full_run=True,return_run=True))
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n');print('OK:',len(results),'stream cases, body/guards/screen verified',flush=True)
