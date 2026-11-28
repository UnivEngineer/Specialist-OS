"""8080 execution checks for transient import/generation and raw PRE loading."""
from pathlib import Path
import re, subprocess, runpy
R=Path(__file__).resolve().parents[2]; O=R/'.local/prepared-check'; O.mkdir(exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
S=sy(R/'bin/FONT.sym'); W=lambda n:n.to_bytes(2,'little')
H=S['IMAGE_HEADER_SIZE'];HC=S['IMAGE_HEADER_CHECKSUM']
build=runpy.run_path(str(R/'scripts/build/build-font-prerender.py'))['build']
def call(m,entry,a=0,hl=0):
 m=bytearray(m);code=b'\x31'+W(0x8fe0)+b'\x3e'+bytes([a])+b'\x21'+W(hl)+b'\xcd'+W(entry)+b'\xc3'+W(0xeff0)
 m[0xee00:0xee00+len(code)]=code;(O/'in.bin').write_bytes(m)
 subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','ee00','-end','eff0','-counter','20000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=10)
 b=(O/'out.bin').read_bytes();assert b[65542:65544]==W(0xeff0);return bytearray(b[:65536]),b[65536]&1
# Deliberately unrelated high bits verify masking; all codes and rows covered.
font=bytes((i*73+i//8*17)&255 for i in range(2048))
for name,mode in [('Rotate',0),('Packed6K',1),('Fast12K',2)]:
 m=bytearray(65536)
 for addr,file in [(0xc000,'bin/BIOS.sys'),(0,'bin/FONT.i80'),(0x5000,'bin/fonts/Init6.i80')]:
  data=(R/file).read_bytes();m[addr:addr+len(data)]=data
 body=(R/f'bin/fonts/{name}.i80').read_bytes();stage=S['staging']+H;m[stage:stage+len(body)]=body
 m[0x4000:0x4800]=font;m[S['initContext']:S['initContext']+6]=W(stage)+W(0x4000)+W(0x6400)
 m,cf=call(m,0x5000,hl=S['initContext']);assert not cf
 paged=bytes(font[c*8+y]&63 for y in range(8) for c in range(256));expected=bytes(((b<<6)|b)&255 for b in paged) if mode==1 else paged
 assert m[stage+0xc00:stage+0x1400]==expected,(name,'import')
 m,cf=call(m,0x5000,a=1,hl=S['initContext']);assert not cf
 if mode:
  planes=build(font,'packed' if mode==1 else 'full')
  assert m[0x6400:0x6400+len(planes)]==planes,name
 print(name,'import and generation PASS')
# Parser and direct-load descriptor: no PRE scratch buffer, exact size checked.
for args in [b'-FILE',b'-IMPORT A:/FONTS/6x8/KOI8.pf',b'-IMPORT A:/MyFont.pf -OUT B:/Custom.fnt']:
 m=bytearray(65536);data=(R/'bin/FONT.i80').read_bytes();m[:len(data)]=data;m[0x4000:0x4000+len(args)+1]=args+b'\0';m[S['argTail']:S['argTail']+2]=W(0x4000)
 m,cf=call(m,S['parseOption']);assert not cf,args
 if args==b'-FILE':assert m[S['useFile']]==1
 else:assert m[S['importPath']:S['importPath']+2]==b'A:'
print('Parser PASS')

for actual,expected_error in [(10240,0),(10239,1)]:
 m=bytearray(65536)
 for addr,file in [(0xc000,'bin/BIOS.sys'),(0,'bin/FONT.i80')]:
  data=(R/file).read_bytes();m[addr:addr+len(data)]=data
 for addr in [S['fileReadAll'],S['mx_callRegs']]:m[addr:addr+2]=b'\xaf\xc9'
 m[S['mx_callRegs']:S['mx_callRegs']+6]=bytes.fromhex('E3 23 23 E3 AF C9')
 m[S['useFile']]=1;m[S['lowerBase']:S['lowerBase']+2]=W(0x6400)
 m[S['staging']+H+44:S['staging']+H+46]=W(10240)
 m[S['preparedIO']+6:S['preparedIO']+8]=W(actual)
 path=b'A:\\FONTS\\6x8\\Fast.fnt\0';m[S['path']:S['path']+len(path)]=path
 m,cf=call(m,S['loadPrepared']);assert cf==expected_error
 assert m[S['preparedIO']+2:S['preparedIO']+6]==W(0x6400)+W(10240)
print('Direct destination and exact PRE length PASS')

# Real generated initializer containers must use the same native header as
# the engine. A stale 16-byte base otherwise rejects every current DLL.
files=runpy.run_path(str(R/'scripts/tests/check-nested-image.py'))['read_tree']((R/'images/C-Flash-512k-MMX-System-Emu80.bin').read_bytes())[0]
for size,engine,initializer in [('6x8','Rotate','Init6'),('8x8','Rotate','InitA')]:
 for damage in [None,'header','payload']:
  m=bytearray(65536)
  for addr,file in [(0xc000,'bin/BIOS.sys'),(0,'bin/FONT.i80')]:
   data=(R/file).read_bytes();m[addr:addr+len(data)]=data
  m[S['fileReadAll']:S['fileReadAll']+2]=b'\xaf\xc9'
  m[S['mx_callRegs']:S['mx_callRegs']+6]=bytes.fromhex('E3 23 23 E3 AF C9')
  body=files[f'FONTS/{size}/{engine}.fnt'];at=S['staging'];m[at:at+len(body)]=body
  word_at=lambda at,v:m.__setitem__(slice(at,at+2),W(v))
  word_at(S['imageBytes'],len(body)-H)
  path=f'A:\\FONTS\\{size}\\{engine}.fnt\0'.encode();m[S['path']:S['path']+len(path)]=path
  dll=files[f'FONTS/{size}/{initializer}.dll'];base=S['FONT_INIT_ADDR']-H
  m[base:base+len(dll)]=dll;word_at(S['initializerIO']+6,len(dll))
  if damage=='header':m[base+HC]^=1
  if damage=='payload':m[S['FONT_INIT_ADDR']+8]^=1
  m,cf=call(m,S['loadInitializer']);assert cf==(damage is not None),(initializer,damage)
print('Native initializer headers and damaged checksums PASS')

# Execute the export path with mocked file I/O; check exact ASCIZ names and
# refreshed native checksums. This catches copying past an unterminated suffix.
def checksum(data):
 total=sum(data);prefix=total-data[-1]
 return (((prefix+(prefix>>8))&255)<<8)|(total&255)
for output in [b'B:\\Custom',b'B:\\Custom.fnt',b'A:\\FONTS\\6x8\\Custom']:
 m=bytearray(65536)
 for addr,file in [(0xc000,'bin/BIOS.sys'),(0,'bin/FONT.i80')]:
  data=(R/file).read_bytes();m[addr:addr+len(data)]=data
 for addr in [S['fileWriteAll'],S['fileReadAll'],S['mx_callRegs'],S['mon2_printStringOld']]:m[addr:addr+2]=b'\xaf\xc9'
 m[S['mx_callRegs']:S['mx_callRegs']+6]=bytes.fromhex('E3 23 23 E3 AF C9')
 body=(R/'bin/fonts/Fast12K.i80').read_bytes();stage=S['staging']
 header=bytearray(b'\xcd MX\x02\x04'+W(0xd800)+W(0xd800)+W(len(body))+b'\0'*4).ljust(H,b'\0')
 m[stage:stage+H+len(body)]=header+body
 m[S['imageBytes']:S['imageBytes']+2]=W(len(body));m[S['lowerBase']:S['lowerBase']+2]=W(0x6400)
 m[S['outputPath']:S['outputPath']+len(output)+1]=output+b'\0'
 ini=b'A:\\FONTS\\6x8\\Init6.dll\0';m[S['initializerPath']:S['initializerPath']+len(ini)]=ini
 m[S['initializerIO']+6:S['initializerIO']+8]=W(363)
 m,cf=call(m,S['exportChosen']);assert not cf
 expected=output.split(b'.')[0]+b'.fnt\0';assert m[S['outputPath']:S['outputPath']+len(expected)]==expected
 expected=output.rsplit(bytes([92]),1)[0]+bytes([92])+b'Init6.dll\0';assert m[S['exportAuxPath']:S['exportAuxPath']+len(expected)]==expected
 assert m[stage+12:stage+14]==W(checksum(body))
 assert m[stage+HC:stage+HC+2]==W(checksum(m[stage:stage+HC]))
 if expected!=ini:assert m[S['exportIO']+2:S['exportIO']+4]==W(S['FONT_INIT_ADDR']-H)
print('Export filenames and native checksums PASS')

# No mountable drive: import remains installed and export is skipped cleanly.
m=bytearray(65536);data=(R/'bin/FONT.i80').read_bytes();m[:len(data)]=data
m[S['importPath']]=ord('X')
m[S['mx_callRegs']:S['mx_callRegs']+2]=b'\x37\xc9'
m[S['mon2_printStringOld']]=0xc9
m,cf=call(m,S['saveImported'])
assert not cf and m[S['exportDrive']]==8 and m[S['outputPath']]==0
print('Unavailable RAM disk: skip export PASS')
