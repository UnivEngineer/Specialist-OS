"""DiskBen: arbitrary buffers, complete-sector fill/compare, and -B parsing on 8080."""
from pathlib import Path
import re,struct,subprocess,json
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/h-write-unroll-2026-11-13/alignment-cpu';O.mkdir(parents=True,exist_ok=True)
W=lambda n:struct.pack('<H',n)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
cpu='D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'
def run(mem,code,stub=0x1000):
 code=b'\x31'+W(0x1e00)+code+b'\xf5\xe1\x22'+W(0x1f00)+b'\xc3'+W(0x1fff)
 mem[stub:stub+len(code)]=code;(O/'in.bin').write_bytes(mem)
 subprocess.run([cpu,'-m8080','-pc',f'{stub:x}','-end','1fff','-counter','10000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
 out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x1fff
 return out
rows=[]
for name,origin in [('DISKMAIN',0x2100),('DISKDOS',0xc000),('DISKBANK',0xc000)]:
 s=sy(R/f'source-apps/diskbench/build/{name}.sym');base=bytearray(65536);raw=(R/f'bin/{name}.i80').read_bytes();base[origin:origin+len(raw)]=raw
 for offset in (0,1,15,255):
  for kib in ((1,8) if name=='DISKDOS' else (1,8,24)):
   address=s['buffer']+offset;count=kib*1024
   if address+count> s['LEGACY_APP_MEMORY_END'] and name=='DISKMAIN':continue
   m=bytearray(base);m[s['dataBuffer']:s['dataBuffer']+2]=W(address);m[s['bufferAddress']:s['bufferAddress']+2]=W(address)
   m[s['config']+3]=kib*4;m[s['pass']]=3;m[s['operation']]=1;m[address-1]=0xa5;m[address+count]=0x5a
   out=run(m,b'\xcd'+W(s['prepareFileBuffer'])+b'\xcd'+W(s['compareFileBuffer']))
   assert not out[0x1f00]&1
   expected=bytes(3^(i>>8)^((address+i)&255) for i in range(count))
   assert out[address:address+count]==expected,(name,offset,kib)
   assert out[address-1]==0xa5 and out[address+count]==0x5a
   bad=bytearray(out[:65536]);bad[address+count-1]^=1
   outbad=run(bad,b'\xcd'+W(s['compareFileBuffer']));assert outbad[0x1f00]&1
   rows.append(dict(worker=name,offset=offset,KiB=kib,lastByteChecked=True))
  m=bytearray(base);m[s['bufferAddress']:s['bufferAddress']+2]=W(address);m[s['pass']]=3;m[s['operation']]=1;m[address-1]=0xa5;m[address+256]=0x5a
  out=run(m,b'\xcd'+W(s['prepareRawBuffer']))
  assert out[address:address+256]==bytes(3^((address+i)&255) for i in range(256))
  assert out[address-1]==0xa5 and out[address+256]==0x5a
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
print('DISKBEN BUFFER CPU PASS',len(rows),'whole-file fill/compare cases; 12 RAW sectors; aligned/unaligned, guards, last-byte mismatch')

front=sy(R/'source-apps/diskbench/build/DISKBEN.sym');base=bytearray(65536)
raw=(R/'bin/DISKBEN.i80').read_bytes();base[:len(raw)]=raw
bios=(R/'bin/BIOS.sys').read_bytes();base[0xc000:0xc000+len(bios)]=bios
arguments=[('-B:ALIGNED',0),('-b:unaligned',1),('-B:UnAlIgNeD',1),('-B:ALIGN',None),('-B:',None),('-B:ALL',None)]
for word,expected in arguments:
 m=bytearray(base);m[0x8d00:0x8d00+len(word)]=word.encode();m[front['argAlign']]=255
 out=run(m,b'\x21'+W(0x8d00)+b'\x06'+bytes([len(word)])+b'\xcd'+W(front['argument_word']),stub=0x8c00)
 assert bool(out[0x1f00]&1)==(expected is None),(word,out[0x1f00:0x1f02].hex())
 if expected is not None:assert out[front['argAlign']]==expected
m=bytearray(base);word='-B:ALIGNED';m[0x8d00:0x8d00+len(word)]=word.encode();m[front['argAlign']]=1
out=run(m,b'\x21'+W(0x8d00)+b'\x06'+bytes([len(word)])+b'\xcd'+W(front['argument_word']),stub=0x8c00)
assert out[0x1f00]&1,'Duplicate alignment must be rejected'
print('DISKBEN ALIGNMENT ARGUMENTS PASS',len(arguments)+1,'cases')

for drive in (0,1):
 m=bytearray(base);m[front['argAlign']]=255;m[front['v_drive']]=drive
 out=run(m,b'\xcd'+W(front['choose_alignment']),stub=0x8c00)
 assert out[front['argAlign']]==0
print('DISKBEN A:/B: alignment prompt skipped PASS')
