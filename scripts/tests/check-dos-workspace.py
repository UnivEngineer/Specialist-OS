"""8080: exact DOS module intervals, cached eviction and pinned code."""
from pathlib import Path
import re,struct,subprocess,tempfile
R=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/banked/KERNEL.sym').read_text('utf-8'),re.M)}
word=lambda n:struct.pack('<H',n)
base=(R/'bin/DOS.sys').read_bytes()
with tempfile.TemporaryDirectory(prefix='mxos-intervals-') as d:
 out=Path(d)
 def test(start,end,ranges=(),cached=None,pinned=False,driver=None,conflict=False,evicted=False):
  m=bytearray(65536);m[0x100:0x100+len(base)]=base
  for i,(lo,hi) in enumerate(ranges):
   p=s['MODULE_TABLE']+i*s['MODULE_RECORD_SIZE'];m[p:p+14]=struct.pack('<HBBHHBBHH',i+1,0,s['DOS_IO_PAGE'],lo,0,0,0x80+i,lo,hi)
  if cached:
   m[s['fileopsReady']]=1;m[s['fileopsActive']]=int(pinned);struct.pack_into('<H',m,s['fileopsLoadedEnd'],cached)
  if driver:
   m[s['diActive']]=1;struct.pack_into('<H',m,s['diStart'],driver[0]);struct.pack_into('<H',m,s['diEnd'],driver[1])
  stub=bytes.fromhex('31 00 EF 01 57 24 21')+word(start)+b'\x11'+word(end)+b'\xcd'+word(s['dosArenaCheck'])+bytes.fromhex('F5 C1 60 69 22 00 EE C3 00 E0')
  m[0xDF00:0xDF00+len(stub)]=stub;(out/'in').write_bytes(m)
  subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','DF00','-end','E000','-counter','50000','-output',str(out/'result'),str(out/'in')],check=True,capture_output=True)
  b=(out/'result').read_bytes();af=int.from_bytes(b[0xEE00:0xEE02],'little')
  assert bool(af&1)==conflict,(hex(start),hex(end),hex(af),ranges,cached,driver)
  assert bool(b[s['fileopsReady']])==bool(cached and not evicted),(cached,pinned,evicted)
  assert int.from_bytes(b[65542:65544],'little')==0xE000
  # Return registers remain the exact caller's start/end (BC captured above).
  return b
 test(0xC000,0xC100)
 test(0xC000,0xC100,[(0xC100,0xC101)])
 test(0xC100,0xC101,[(0xC000,0xC100)])
 for lo,hi in [(0xC000,0xC001),(0xC0FF,0xC101),(0xBFFF,0xC001),(0xB000,0xD000),(0xC010,0xC020)]:
  test(lo,hi,[(0xC000,0xC100)],conflict=True)
 test(0xD000,0xD780,[(0xC000,0xC100),(0xC100,0xC200)])
 test(0xD77F,0xD780,[(0xC000,0xC100),(0xD700,0xD780)],conflict=True)
 test(0xD400,0xD780,cached=0xCF00,pinned=True)
 test(0xCEFF,0xCF01,cached=0xCF00,pinned=True,conflict=True)
 test(0xCEFF,0xCF01,cached=0xCF00,evicted=True)
 test(0xC000,0xC010,driver=(0xD400,0xD780))
 test(0xD77F,0xD780,driver=(0xD400,0xD780),conflict=True)
 test(0xD77F,0xD780,[(0xD400,0xD780)],cached=0xCF00,conflict=True)
print('DOS exact intervals PASS: adjacency, containment, 2 DLLs, pinned installer/cache, selective eviction')
