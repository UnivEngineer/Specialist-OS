"""8080 code: wildcard matching, including star retries and DOS *.*."""
from pathlib import Path
from dos_state import initialize_dos_state
from relocated_component import load_at
import fnmatch,re,struct,subprocess,tempfile
R=Path(__file__).resolve().parents[2]
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text('utf-8'),re.M)}
s=sy(R/'bin/banked/KERNEL.sym')
base=bytearray(65536);a=(R/'bin/banked/KERNEL.bin').read_bytes();base[256:256+len(a)]=a
initialize_dos_state(base, s)
origin=s['DOS_ARENA_START']
a,e=load_at(R/'source/enumerate/ENUM.asm','ENUM','OVERLAY_ORIGIN',origin)
base[origin:origin+len(a)]=a
word=lambda n:struct.pack('<H',n)
names=['','A','ABC','A.TXT','ALPHA.TXT','BETA.BAT','NOEXT','AAB','ABAB','FOO.BAR','SHORT.X']
patterns=['*','*.*','*.TXT','*.txt','????.BAT','A*','*A*','*A*B','A?B','?','*?','*.*X','*A?','*B?','A**B','Z*']
with tempfile.TemporaryDirectory(prefix='mxos-enum-') as d:
 p=Path(d)
 for pattern in patterns:
  for name in names:
   m=bytearray(base);m[e['enMask']:e['enMask']+len(pattern)+1]=pattern.encode()+b'\0';m[e['enNameText']:e['enNameText']+len(name)+1]=name.encode()+b'\0'
   struct.pack_into('<H',m,e['enPattern'],e['enMask'])
   stub=b'\x31\x00\x90\xcd'+word(e['enWildcard'])+b'\xf5\xc1\x79\x32\x00\x89\xc3\xff\x88'
   m[0x8800:0x8800+len(stub)]=stub;(p/'in').write_bytes(m)
   subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8800','-end','88ff','-counter','1000000','-output',str(p/'out'),str(p/'in')],capture_output=True,check=True)
   out=(p/'out').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x88ff,(pattern,name,'loop')
   matched=bool(out[0x8900]&64);expected=pattern=='*.*' or fnmatch.fnmatchcase(name.upper(),pattern.upper())
   assert matched==expected,(pattern,name,matched,expected)
print('ENUM wildcard CPU PASS',len(names)*len(patterns),'cases; 8080, *, ?, retry, *.*, case')

