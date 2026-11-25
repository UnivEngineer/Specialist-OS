"""8080: exact DOS module intervals, cached eviction and pinned code."""
from pathlib import Path
from dos_state import initialize_dos_state, install_dos_mem
import re,struct,subprocess,tempfile
R=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/banked/KERNEL.sym').read_text('utf-8'),re.M)}
word=lambda n:struct.pack('<H',n)
base=(R/'bin/DOS.sys').read_bytes()
with tempfile.TemporaryDirectory(prefix='mxos-intervals-') as d:
 out=Path(d)
 # Bootstrap installs the image over arbitrary RAM. Its RAM-disk flag and
 # both guards must survive the same cold initializer the bank executes.
 memory=bytearray([0xA5])*65536
 install_dos_mem(memory, R/'bin')
 memory[0x100:0x100+len(base)]=base
 start=s['dosWorkStart'];defaults=s['dosDefaultsEnd'];end=s['dosWorkEnd']
 initial=bytes(memory[s['dosColdDefaults']:s['dosColdDefaults']+defaults-start])
 stub=b'\x31'+word(0xEF00)+b'\xCD'+word(s['dosInitVariables'])+b'\xC3'+word(0xE000)
 memory[0xDF00:0xDF00+len(stub)]=stub
 (out/'in').write_bytes(memory)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','DF00','-end','E000','-counter','200000','-output',str(out/'result'),str(out/'in')],check=True,capture_output=True)
 result=(out/'result').read_bytes()
 assert int.from_bytes(result[65542:65544],'little')==0xE000
 assert result[start:defaults]==initial and result[defaults:end-1]==bytes(end-1-defaults)
 assert result[start-1]==result[s['memoryRamFresh']]==result[end]==0xA5
 path = bytes([s['BOOT_VOLUME_LETTER']]) + b':\\DOS'
 assert result[s['v_cliPath']:s['v_cliPath']+128] == path.ljust(128, b'\0')
 assert result[s['v_drives']:s['v_drives']+4]==word(s['diskDriverDummy'])+word(s['diskDriver'])
 def test(start,end,ranges=(),cached=None,pinned=False,driver=None,floppy=None,conflict=False,evicted=False):
  m=bytearray(65536);m[0x100:0x100+len(base)]=base
  initialize_dos_state(m, s)
  for i,(lo,hi) in enumerate(ranges):
   p=s['MODULE_TABLE']+i*s['MODULE_RECORD_SIZE'];m[p:p+14]=struct.pack('<HBBHHBBHH',i+1,0,s['DOS_IO_PAGE'],lo,0,0,0x80+i,lo,hi)
  if cached:
   struct.pack_into('<H',m,s['fileopsOverlay'],base_addr);m[s['fileopsReady']]=1;m[s['fileopsActive']]=int(pinned);struct.pack_into('<H',m,s['fileopsLoadedEnd'],cached)
  if floppy:
   struct.pack_into('<H',m,s['diFloppyStart'],s['DOS_FLOPPY_ADDR']);struct.pack_into('<H',m,s['diFloppyEnd'],floppy)
  if driver:
   m[s['diActive']]=1;struct.pack_into('<H',m,s['diStart'],driver[0]);struct.pack_into('<H',m,s['diEnd'],driver[1])
  m[s['v_imageHeader']+s['IMAGE_POLICY']]=s['IMAGE_POLICY_MANAGED']
  stub=bytes.fromhex('31 00 EF 01 57 24 21')+word(start)+b'\x11'+word(end)+b'\xcd'+word(s['dosArenaImage'])+bytes.fromhex('F5 C1 60 69 22 00 EE C3 00 E0')
  m[0xDF00:0xDF00+len(stub)]=stub;(out/'in').write_bytes(m)
  subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','DF00','-end','E000','-counter','50000','-output',str(out/'result'),str(out/'in')],check=True,capture_output=True)
  b=(out/'result').read_bytes();af=int.from_bytes(b[0xEE00:0xEE02],'little')
  assert bool(af&1)==conflict,(hex(start),hex(end),hex(af),ranges,cached,driver)
  assert bool(b[s['fileopsReady']])==bool(cached and not evicted),(cached,pinned,evicted)
  assert int.from_bytes(b[65542:65544],'little')==0xE000
  # Return registers remain the exact caller's start/end (BC captured above).
  return b
 test(0x9000,0x9100)
 test(0x9000,0x9100,[(0x9100,0x9101)])
 test(0x9100,0x9101,[(0x9000,0x9100)])
 for lo,hi in [(0x9000,0x9001),(0x90FF,0x9101),(0x8FFF,0x9001),(0x8000,0xA000),(0x9010,0x9020)]:
  test(lo,hi,[(0x9000,0x9100)],conflict=True)
 test(0xA000,0xB500,[(0x9000,0x9100),(0x9100,0x9200)])
 test(0xB4FF,0xB500,[(0x9000,0x9100),(0xB400,0xB500)],conflict=True)
 base_addr=s['DOS_ARENA_START']+0x459; cached_end=base_addr+0x80
 test(cached_end,cached_end+2,cached=cached_end,pinned=True)
 test(cached_end-1,cached_end+1,cached=cached_end,pinned=True,conflict=True)
 test(cached_end-1,cached_end+1,cached=cached_end,evicted=True)
 floppy=s['DOS_FLOPPY_ADDR']
 test(floppy-1,floppy,floppy=floppy+300)
 test(floppy+299,floppy+301,floppy=floppy+300,conflict=True)
 test(floppy+300,floppy+301,floppy=floppy+300)
 top=s['DOS_BANK_DRIVER_END']
 test(top-0x200,top-0x100,driver=(top-0x100,top))
 test(top-0x101,top-0xFF,driver=(top-0x100,top),conflict=True)
 test(0x9000,0x9010,driver=(0xAF00,0xB500))
 test(0xB4FF,0xB500,driver=(0xAF00,0xB500),conflict=True)
 test(0xB4FF,0xB500,[(0xAF00,0xB500)],cached=cached_end,conflict=True)
print('DOS exact intervals PASS: adjacency, containment, 2 DLLs, pinned installer/cache, selective eviction')
