"""8080: read-only guard accepts new FAT and rejects old/live FE sectors."""
from pathlib import Path
import re,struct,subprocess,tempfile
R=Path(__file__).resolve().parents[2];sy={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/bootstrap/BOOT.sym').read_text(),re.M)};W=lambda n:struct.pack('<H',n)
boot=(R/'bin/bootstrap/BOOT.bin').read_bytes();count=0
with tempfile.TemporaryDirectory(prefix='mxos-boot-guard-') as d:
 out=Path(d)
 for pages in (1,4,15):
  for change in ('none','old-fe','live-last-fe','missing-ff','bad-bpb'):
   m=bytearray(65536);m[sy['stage']:sy['stage']+len(boot)]=boot
   m[sy['pageCount']]=16;m[0xffca:0xffce]=bytes.fromhex('3e004ec9')
   m[3:11]=b'MXOS256 ';m[11:24]=W(256)+bytes([1])+W(1)+bytes([1])+W(64)+W(pages*256)+bytes([0xf8])+W(pages*2)
   start=9+pages*2
   for bank in range(pages):
    at=256+2*(bank*256+254-start+2);m[at:at+4]=bytes.fromhex('f7fff7ff')
   at=256+2*(254-start+2)
   if change=='old-fe':m[at:at+2]=W(0)
   if change=='live-last-fe':
    at=256+2*((pages-1)*256+254-start+2);m[at:at+2]=W(0xffff)
   if change=='missing-ff':m[at+2:at+4]=W(0)
   if change=='bad-bpb':m[13]=2
   original=bytes(m[:0x2100])
   m[0x7000:0x700d]=b'\x31'+W(0x8f00)+b'\xcd'+W(sy['guardRamDisk'])+b'\xaf\x32'+W(0x700f)+b'\xc3'+W(0x7010)
   m[sy['guardBlocked']:sy['guardBlocked']+8]=b'\x3e\x01\x32'+W(0x700f)+b'\xc3'+W(0x7010)
   (out/'in').write_bytes(m)
   subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','7000','-end','7010','-counter','1000000','-output',str(out/'out'),str(out/'in')],capture_output=True,check=True)
   b=(out/'out').read_bytes();assert b[0x700f]==(change!='none'),(pages,change,b[0x700f]);assert b[:0x2100]==original,(pages,change,'disk altered');count+=1
print('BOOT GUARD CPU PASS',count,'cases; 1/4/15 banks, both BAD entries, old/live tails, BPB and read-only rejection')
