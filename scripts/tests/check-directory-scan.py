"""8080: shared directory scan and lookup, including root limits and sector boundaries."""
from pathlib import Path
from dos_state import initialize_dos_state
import re,struct,subprocess,json,argparse
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--bundle',type=Path,default=R/'bin');p.add_argument('--report',type=Path,default=R/'docs/diskbench/dos-fast-2026-11-13/directory-cpu');args=p.parse_args();O=args.report;O.mkdir(parents=True,exist_ok=True)
k={n:int(v,16) for n,v in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)',(args.bundle/'banked/KERNEL.sym').read_text(),re.M)}
w=lambda n:struct.pack('<H',n&65535)
base=bytearray(65536); b=(args.bundle/'banked/KERNEL.bin').read_bytes();base[256:256+len(b)]=b
if "dosWorkStart" in k:  # Pre-BSS benchmark bundle.
 initialize_dos_state(base, k, args.bundle)
# Cache hook returns a stable sector address; all iteration and comparisons are production code.
base[k['fatCheckMounted']:k['fatCheckMounted']+2]=b'\xaf\xc9'
base[k['getSectorAddressInCache']:k['getSectorAddressInCache']+3]=b'\xc3'+w(0xe400)
rows=[];cycles=[]
def run(m,name,hl=0,a=0,de=0x2468,preserve=True):
 code=b'\x31'+w(0xe000)+b'\x01'+w(0x1357)+b'\x11'+w(de)+b'\x21'+w(hl)+bytes([0x3e,a,0xcd])+w(k[name])
 code+=b'\x22'+w(0xe200)+b'\xeb\x22'+w(0xe202)+b'\x60\x69\x22'+w(0xe204)+b'\xf5\xe1\x22'+w(0xe206)+b'\xc3'+w(0xe1ff)
 m[0xe100:0xe100+len(code)]=code;(O/'in.bin').write_bytes(m)
 p=subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','e100','-end','e1ff','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,check=True)
 cycles.append(int(p.stdout.strip()));raw=(O/'out.bin').read_bytes();assert raw[65542:65544]==w(0xe1ff),p.stdout;m[:]=raw[:65536]
 assert m[0xe202:0xe204]==w(de),(name,'DE')
 if preserve:assert m[0xe204:0xe206]==w(0x1357),(name,'BC')
 return int.from_bytes(m[0xe200:0xe202],'little'),bool(m[0xe206]&1),m[0xe207]
def fresh(count,sector=256):
 m=bytearray(base);m[k['v_diskInfo.rootDirMaxFiles']:k['v_diskInfo.rootDirMaxFiles']+2]=w(count);m[k['v_diskInfo.dirStartSector']:k['v_diskInfo.dirStartSector']+2]=w(0);m[k['v_diskInfo.descrPerSector']]=sector//32
 hook=bytes.fromhex('d5 62 6b')+b'\x29'*(sector.bit_length()-1)+bytes.fromhex('11 00 c0 19 d1 b7 c9')
 m[0xe400:0xe400+len(hook)]=hook
 return m
def entry(name,attr=0):return name+bytes([attr])+bytes(20)
for count in (1,7,8,9,13,128):
 for target in (0,count//2,count-1):
  m=fresh(count)
  for i in range(count+8):m[0xc000+i*32:0xc020+i*32]=entry(f'F{i:07}TXT'.encode())
  wanted=f'f{target:07}txt'.encode();m[0xe300:0xe30b]=wanted
  ptr,cf,a=run(m,'fileFind',0xe300);assert not cf,(count,target,a)
  lookup_cycles=cycles[-1]
  assert m[k['v_tmpFileDescr']:k['v_tmpFileDescr']+11].upper()==wanted.upper()
  m[0xe300:0xe30b]=f'F{count:07}TXT'.encode();_,cf,a=run(m,'fileFind',0xe300);assert cf
  run(m,'fatDirStart');got=[]
  for i in range(count+1):
   ptr,cf,a=run(m,'fatDirFile')
   if cf:break
   got.append(bytes(m[ptr:ptr+11]))
  assert len(got)==count,(count,len(got));rows.append(dict(root=count,target=target,lookupCycles=lookup_cycles))
# The same scanner respects logical-sector boundaries at 128 and 512 bytes.
for sector,count in ((128,13),(512,33)):
 m=fresh(count,sector)
 for i in range(count+16):m[0xc000+i*32:0xc020+i*32]=entry(f'F{i:07}TXT'.encode())
 run(m,'fatDirStart');got=[]
 for _ in range(count+1):
  ptr,cf,a=run(m,'fatDirFile')
  if cf:break
  got.append(ptr)
 assert len(got)==count,(sector,len(got))
 m[0xe300:0xe30b]=f'F{count-1:07}TXT'.encode();_,cf,a=run(m,'fileFind',0xe300);assert not cf,(sector,count,a,m[k['v_cachedDescrPtr']:k['v_cachedDescrPtr']+2].hex(),m[0xe300:0xe30c].hex())
 rows.append(dict(sector=sector,root=count))
# Mixed deleted/LFN/label/directory/hidden/system entries and escaped E5h.
m=fresh(13)
names=[(b'DELETED TXT',0),(b'LONG    TXT',15),(b'LABEL      ',8),(b'SUBDIR     ',16),(b'KEEP    TXT',6),(b'\x05SCAPED TXT',0)]
for i,(name,attr) in enumerate(names):m[0xc000+i*32:0xc020+i*32]=entry(name,attr)
m[0xc000]=0xe5
run(m,'fatDirStart');got=[]
while True:
 ptr,cf,a=run(m,'fatDirFile')
 if cf:break
 got.append(bytes(m[ptr:ptr+11]))
assert got==[b'KEEP    TXT',b'\x05SCAPED TXT'],got
for name in (b'keep    txt',b'\xe5SCAPED TXT'):
 m[0xe300:0xe30b]=name;_,cf,a=run(m,'fileFind',0xe300);assert not cf,(name,a)
rows.append(dict(case='deleted LFN label directory hidden system escaped E5 and zero terminator'))
# Subdirectory chain crosses a cluster, rejects cycles and preserves I/O errors.
for next_cluster in (3,2,0xfff8):
 m=fresh(8);m[k['v_directory']:k['v_directory']+2]=w(2)
 m[k['v_diskInfo.dataClusters']:k['v_diskInfo.dataClusters']+2]=w(4)
 m[k['v_diskInfo.dataStartSector']:k['v_diskInfo.dataStartSector']+2]=w(0)
 m[k['v_diskInfo.sectorsPerCluster']:k['v_diskInfo.sectorsPerCluster']+2]=w(1)
 # fatReadCluster hook: 2 -> requested link, other -> EOC; production chain walker stays intact.
 hook=bytes([0x7b,0xfe,2,0x11])+w(0xfff8)+bytes([0xc0,0x11])+w(next_cluster)+bytes([0xb7,0xc9])
 m[k['fatReadCluster']:k['fatReadCluster']+len(hook)]=hook
 for i in range(16):m[0xc000+i*32:0xc020+i*32]=entry(f'F{i:07}TXT'.encode())
 run(m,'fatDirStart');got=[]
 for _ in range(40):
  ptr,cf,a=run(m,'fatDirFile')
  if cf:break
  got.append(ptr)
 assert len(got)==(16 if next_cluster==3 else 8 if next_cluster==0xfff8 else 32),(next_cluster,len(got))
 assert cf and (a!=0)==(next_cluster==2),(next_cluster,a)
 rows.append(dict(subdirectoryLink=next_cluster,records=len(got)))
m=fresh(8);m[k['getSectorAddressInCache']:k['getSectorAddressInCache']+4]=bytes([0x3e,4,0x37,0xc9]);run(m,'fatDirStart');_,cf,a=run(m,'fatDirFile');assert cf and a==4
rows.append(dict(case='cache load error preserved'))
# Real lookup: stale hot pointers must never survive LBA/tag changes or invalidation.
if 'v_cacheHotSlots' not in k:
 (O/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print('DIRECTORY SCAN CPU PASS',len(rows));raise SystemExit
m=bytearray(base);head=k['cacheData']+31*k['FAT_CACHE_DATA'];tag=8;lba=321
m[k['cacheData']:k['cacheData']+512]=bytes([255])*512
m[k['partialCacheData']:k['partialCacheData']+8]=bytes([255])*8
m[k['v_cacheProbeCount']]=64;m[k['v_cacheMaxSector']:k['v_cacheMaxSector']+2]=w(1000)
m[k['v_cacheTag']]=tag;m[k['v_cacheLastTag']]=255
m[head:head+8]=w(lba)+w(0x9000)+bytes([0,0,1,tag])
for i in range(4):m[k['v_cacheHotSlots']+i*2:k['v_cacheHotSlots']+i*2+2]=w(head)
for label,number,slot_tag,expected in [('hit dirty',lba,tag,False),('wrong LBA',322,tag,True),('other drive',lba,tag+1,True),('other geometry',lba,tag+8,True),('invalidated',65535,255,True),('reused slot',444,tag,True),('reused hit',444,tag,False)]:
 m[head:head+2]=w(number);m[head+7]=slot_tag
 requested=444 if label=='reused hit' else lba
 ptr,cf,a=run(m,'streamCacheContains',de=requested,preserve=False)
 assert cf==expected,(label,cf)
 if not cf:assert ptr==head
 rows.append(dict(cache=label,miss=cf))
(O/'results.json').write_text(json.dumps(rows,indent=2)+'\n');print('DIRECTORY SCAN CPU PASS',len(rows))
