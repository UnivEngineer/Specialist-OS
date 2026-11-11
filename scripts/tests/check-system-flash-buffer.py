"""Actual 8080 FlashSys+BIOS memcpy: 128/256-byte pages and fixed buffer boundaries.
Flat RAM model tests loading/poll-success logic; physical mapper/timing needs Emu80.
"""
import re,struct,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/a-buffer-2026-11-10/cpu';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/FlashSys.sym').read_text(),re.M)}
W=lambda x:struct.pack('<H',x)
base=bytearray(65536);raw=(R/'bin/FlashSys.i80').read_bytes();n=s['resident_file_end']-s['resident_file'];base[s['Driver']:s['Driver']+n]=raw[s['resident_file']:s['resident_file']+n]
bios=(R/'bin/BIOS.sys').read_bytes();base[0xc000:0xc000+len(bios)]=bios
base[s['SectorLimit']+1]=8
count=0
for half in (0,128):
 for source in (0x4001,0x8401,s['BIOS_DISK_BUFFER']):
  for sector in (0,127,2047,2048):
   mem=bytearray(base);mem[s['v_halfPage']]=half
   payload=bytes((i*73+sector+half)&255 for i in range(256));mem[source:source+256]=payload
   guards=(s['BIOS_DISK_BUFFER']-1,s['BIOS_DISK_BUFFER_END']);before=[mem[x] for x in guards]
   stub=b'\x31'+W(s['BIOS_BANK_STACK_TOP']-64)+b'\x01'+W(0x0100)+b'\x11'+W(sector)+b'\x21'+W(source)+b'\xcd'+W(s['Driver'])
   stub+=b'\x22'+W(0x8702)+b'\xeb\x22'+W(0x8704)+b'\x60\x69\x22'+W(0x8706)+b'\xf5\xe1\x22'+W(0x8700)+b'\xc3'+W(0x88ff)
   mem[0x8800:0x8800+len(stub)]=stub;(O/'in.bin').write_bytes(mem)
   subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8800','-end','88ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
   out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x88ff
   assert bool(out[0x8700]&1)==(sector==2048),(half,source,sector,'CF')
   assert struct.unpack_from('<HHH',out,0x8702)==(source,sector,0x0100)
   assert [out[x] for x in guards]==before
   if sector<2048:
    target=(sector&127)*256;assert out[target:target+256]==payload,(half,source,sector,'data')
   count+=1
print('SYSTEM FLASH BUFFER PASS:',count,'cases; 128/256 pages, low/high/self source, last/outside sector, preserved registers, buffer guards')
