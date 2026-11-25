"""8080: РЎР‚Р ВµР В°Р В»РЎРЉР Р…РЎвЂ№Р в„– Р С—Р В»Р В°Р Р…Р С‘РЎР‚Р С•Р Р†РЎвЂ°Р С‘Р С” FAT Р С‘ Р С‘РЎРѓР С—Р С•Р В»Р Р…Р С‘РЎвЂљР ВµР В»РЎРЉ Р С—Р В°РЎвЂЎР С”Р С‘, Р С•РЎв‚¬Р С‘Р В±Р С”Р С‘ Р С‘ Р С”РЎС“РЎР‚РЎРѓР С•РЎР‚РЎвЂ№."""
from pathlib import Path
from dos_state import initialize_dos_state
import re, struct, subprocess, json, argparse
R=Path(__file__).resolve().parents[2]
O=R/'docs/bench/cpm32-system-2026-11-13/batch-cpu'; O.mkdir(parents=True,exist_ok=True)
W=lambda n:struct.pack('<H',n&65535)
def sy(path):
 return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',path.read_text(),re.M)}
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--bundle',type=Path,default=R/'bin');parser.add_argument('--batch-size',type=int,choices=(16,32,64),default=32);args=parser.parse_args()
O=O/str(args.batch_size);O.mkdir(exist_ok=True)
k=sy(args.bundle/'banked/KERNEL.sym'); t=sy(args.bundle/'banked/TRANSPORT.sym')
base=bytearray(65536)
for name,start in [('KERNEL',256),('LOCALBLOCK',k['MEM_IMAGE_LOCAL_START']),('TRANSPORT',0xfc00),('CLIENT',0xf800)]:
 b=(args.bundle/'banked'/f'{name}.bin').read_bytes();base[start:start+len(b)]=b
if "dosWorkStart" in k:  # Historical benchmark bundles carry initialized data in the image.
 initialize_dos_state(base, k, args.bundle)
# The only capacity-dependent instruction; otherwise the machine code is identical.
start=k['streamBatchStart']; pattern=bytes([0x3e,k['DOS_IO_BATCH_SECTORS'],0x32])
at=base.index(pattern,start,start+32);base[at+1]=args.batch_size
asm=""" ORG 0C000h
clusterSector:
 ex de,hl
 dec hl
 dec hl
 add hl,hl
 add hl,hl
 ld de,100
 add hl,de
 ex de,hl
 ret
clusterCheck:
 ld a,d
 cp 0FFh
 jp z,failed
 or a
 ret
clusterNext:
 ld a,(0C804h)
 cp e
 jp z,failed
 inc de
 xor a
 ret
failed:
 ld a,15
 scf
 ret
cacheMiss:
 scf
 ret
partial:
 jp STREAM_DONE
io:
 push hl
 push de
 push bc
 ld hl,(0C800h)
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 ld (hl),c
 inc hl
 ld (hl),b
 inc hl
 ld (0C800h),hl
 ld hl,0C802h
 inc (hl)
 ld a,(0C803h)
 cp (hl)
 pop bc
 pop de
 pop hl
 jp z,failed
 xor a
 ret
 END
""".replace('STREAM_DONE',str(k['streamCopyDone']))
(O/'hooks.asm').write_text(asm)
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080',f'--raw={O/"hooks.bin"}',f'--sym={O/"hooks.sym"}',str(O/'hooks.asm')],check=True,capture_output=True)
h=sy(O/'hooks.sym');b=(O/'hooks.bin').read_bytes();base[0xc000:0xc000+len(b)]=b
for original,hook in [('fatClusterToSector','clusterSector'),('fatCheckCluster','clusterCheck'),('fatReadCluster','clusterNext'),('streamCacheContains','cacheMiss'),('streamCached','partial')]:
 base[k[original]:k[original]+3]=b'\xc3'+W(h[hook])
base[k['LOCAL_BLOCK_ENTRY']:k['LOCAL_BLOCK_ENTRY']+3]=b'\xc3'+W(h['io'])
base[t['bankPage']]=1;base[t['bankPageCount']]=16
base[k['MEM_NEAR_COPY']:k['MEM_NEAR_COPY']+2]=W(k['memcpy_bc_hl'])
base[k['kernelExternalDrive']]=k['EXTERNAL_DRIVE']
base[k['DOS_DRIVER_REGISTRY']+k['EXTERNAL_DRIVE']*k['DRIVER_RECORD_SIZE']]=1
base[k['klInstall']:k['klInstall']+2]=b'\xaf\xc9'
base[k['v_drive']]=7;base[k['v_diskInfo.blockShift']]=8;base[k['v_diskInfo.sectorShift']]=8
base[k['v_diskInfo.sectorSize']:k['v_diskInfo.sectorSize']+2]=W(256)
base[k['v_diskInfo.clusterBytes']:k['v_diskInfo.clusterBytes']+2]=W(1024)
rows=[]
for n,offset,checked,fail,fatfail in [(n,o,c,0,0) for n in (0,1,255,256,257,1024,8192,16384) for o in (0,256,777) for c in (0,1)]+[(8192,0,0,f,0) for f in (1,2,16,32)]+[(8192,0,0,0,4)]:
 mem=bytearray(base);mem[0xc800:0xc805]=W(0xc900)+bytes([0,fail,fatfail]);mem[k['v_streamChecked']]=checked
 mem[k['v_streamOffset']:k['v_streamOffset']+2]=W(offset)
 # Direct DOS buffer; no callback and no emulator page model required.
 stub=b'\x31'+W(0xef00)+b'\x01'+W(n)+b'\x11'+W(2)+b'\x21'+W(0x8001)+b'\x3e\x01\xcd'+W(k['fatStream'])
 stub+=b'\xf5\xe1\x22'+W(0xc810)+b'\x60\x69\x22'+W(0xc812)+b'\xc3'+W(0xc8ff)
 mem[0xc820:0xc820+len(stub)]=stub; (O/'in.bin').write_bytes(mem)
 result=subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','c820','-end','c8ff','-counter','8000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True,text=True)
 out=(O/'out.bin').read_bytes(); assert out[65542:65544]==W(0xc8ff),(n,offset,'PC')
 assert out[65544:65546]==W(0xef00),(n,offset,'SP')
 actual=[]
 for i in range(out[0xc802]):actual.append(struct.unpack_from('<HBB',out,0xc900+4*i))
 expected=[];left=n;pos=offset;cluster=2;done=0;error=False
 while left:
  if pos==1024:
   if cluster==fatfail:error=True;break
   cluster+=1;pos=0
  chunk=min(256-pos%256,left)
  if chunk==256:
   expected.append((100+(cluster-2)*4+pos//256,7,5 if checked else 1))
   if fail and len(expected)==fail:error=True;break
  pos+=chunk;left-=chunk;done+=chunk
 if pos==1024:
  if cluster==fatfail:error=True
  else:cluster+=1;pos=0
 assert actual==expected,(n,offset,checked,fail,fatfail,actual,expected)
 assert bool(out[0xc810]&1)==error,(n,offset,fail,fatfail,'CF',out[0xc810:0xc812].hex())
 for label,value in [('v_streamRemaining',n-done),('v_streamPtr',0x8001+done),('v_streamOffset',pos),('v_streamCluster',cluster)]:
  assert out[k[label]:k[label]+2]==W(value),(n,offset,fail,fatfail,label,int.from_bytes(out[k[label]:k[label]+2],'little'),value)
 rows.append(dict(bytes=n,offset=offset,checked=checked,ioError=fail,fatError=fatfail,sectors=len(actual),ticks=int((result.stdout+result.stderr).strip())))
(O/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
print('DOS BATCH CPU PASS',len(rows),'cases; real planner/worker, partial sectors, checked op, FAT and I/O errors, SP/cursor')

# Склейка соседних LBA использует один вызов копировщика на весь участок.
asm2=f""" ORG 0C100h
 push hl
 push de
 push bc
 ld hl,(0C800h)
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 push hl
 ld hl,({k['klRamCopy']+6})
 ex de,hl
 pop hl
 ld (hl),e
 inc hl
 ld (hl),d
 inc hl
 ld (0C800h),hl
 ld hl,0C802h
 inc (hl)
 ld a,(0C803h)
 cp (hl)
 pop bc
 pop de
 pop hl
 jp z,{h['failed']}
 xor a
 ret
 END
"""
(O/'runs.asm').write_text(asm2)
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080',f'--raw={O/"runs.bin"}',str(O/'runs.asm')],check=True,capture_output=True)
b=(O/'runs.bin').read_bytes();base[0xc100:0xc100+len(b)]=b
base[k['klBlock']:k['klBlock']+3]=b'\xC3'+W(0xc100)
runrows=[]
for drive,op in ((k['SYSTEM_DRIVE'],2),(k['RAM_DRIVE'],1),(k['RAM_DRIVE'],2)):
 for sectors in (list(range(100,132)),list(range(126,158)),list(range(250,254))+list(range(256,284)),[100+3*i for i in range(32)]):
  for fail in (0,2):
   mem=bytearray(base);mem[k['v_drive']]=drive;mem[k['kdPage']]=3
   mem[k['sbPacket']:k['sbPacket']+6]=bytes([drive,op])+W(0x8001)+W(len(sectors))
   mem[k['sbList']:k['sbList']+2*len(sectors)]=b''.join(W(x) for x in sectors)
   mem[0xc800:0xc804]=W(0xc900)+bytes([0,fail])
   stub=b'\x31'+W(0xef00)+b'\xcd'+W(k['sbExecute'])+b'\xf5\xe1\x22'+W(0xc810)+b'\x60\x69\x22'+W(0xc812)+b'\xc3'+W(0xc8ff)
   mem[0xc820:0xc820+len(stub)]=stub;(O/'in.bin').write_bytes(mem)
   subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','c820','-end','c8ff','-counter','8000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
   out=(O/'out.bin').read_bytes();assert out[65542:65544]==W(0xc8ff)
   expected=[];done=0;error=False
   for sector in sectors:
    if expected and expected[-1][0]+expected[-1][1]//256==sector and sector%(128 if drive==k['SYSTEM_DRIVE'] else 256):expected[-1]=(expected[-1][0],expected[-1][1]+256)
    else:expected.append((sector,256))
   if fail and len(expected)>=fail:expected=expected[:fail];error=True
   done=sum(length//256 for _,length in (expected[:-1] if error else expected))
   actual=[struct.unpack_from('<HH',out,0xc900+4*i) for i in range(out[0xc802])]
   assert actual==expected,(drive,op,fail,actual,expected)
   assert bool(out[0xc810]&1)==error and out[0xc812:0xc814]==W(done)
   assert out[k['kdPage']]==3 and out[65544:65546]==W(0xef00)
   assert out[k['klRamCopy']+6:k['klRamCopy']+8]==W(256)
   runrows.append(dict(drive=drive,operation=op,runs=actual,failed=error))
(O/'runs.json').write_text(json.dumps(runrows,indent=2)+'\n')
print('DOS EXTENTS CPU PASS',len(runrows),'cases; contiguous/scattered LBA, 32/64 KiB boundaries, partial errors, route restored')
