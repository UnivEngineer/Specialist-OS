"""RUN из строки NC: единая загрузка тела и подмена ядра без верхнего RUN."""
from pathlib import Path
import json,re,runpy,struct,sys,argparse
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/images'))
from executable import Image,encode
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--menu-only',action='store_true');parser.add_argument('--high-only',action='store_true');parser.add_argument('--upper-only',action='store_true');parser.add_argument('--runtime',type=Path,default=ROOT/'.local/rom-pio-19782');parser.add_argument('--port',type=int,default=19782);args=parser.parse_args()
OUT=ROOT/'docs/run-body-041';OUT.mkdir(parents=True,exist_ok=True)
g=h['Matrix'].__init__.__globals__;g['RUNTIME']=args.runtime.resolve();g['OUT']=OUT
def symbols(name):
 return {key:int(value,16) for key,value in re.findall(
  r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/f'bin/{name}.sym').read_text(),re.M)}
s,r,n,nc_core,cli=(symbols(name) for name in ('BIOS','RUN','NC','NC_CORE','CLI'))
raw=bytearray((ROOT/'bin/nested-Games2M.bin').read_bytes())
w=lambda at:struct.unpack_from('<H',raw,at)[0]
sector=w(11);fat=w(14)*sector;root=(w(14)+raw[16]*w(22))*sector
start=root+((w(17)*32+sector-1)//sector)*sector
# От исходного образа нужны только загрузочный сектор и геометрия FAT.
# Каталог игр может быть заполнен целиком.
assert raw[16]==1
raw[fat+4:]=bytes(len(raw)-fat-4)
slots=[i for i in range(root,start,32) if raw[i] in (0,0xe5,0xff)]
free=[c for c in range(2,(len(raw)-start)//sector+2) if w(fat+c*2)==0]
fixtures={}
compact_size=(ROOT/'bin/fonts/Cpm4Compact.i80').stat().st_size
high_load=max(r['SYSTEM_PROGRAM_ADDR'],(r['FONT_ENGINE_BASE']+compact_size+127)&~127)

def allocate(count, fragmented=False):
 step=2 if fragmented else 1
 chain=free[:count*step:step]
 assert len(chain)==count
 for cluster in chain:free.remove(cluster)
 return chain

for i,(name,load) in enumerate([('UCOM.COM',0),('UEXE.EXE',0),('URKS.RKS',0),('UHIGH.EXE',high_load),('UBAD.RKS',0),('URFH.EXE',0xd300)]):
 size=min(1025,r['SYSTEM_PROGRAM_END']-load) if name=='UHIGH.EXE' else 1025 if load else 4097
 payload=bytes([0x3e,0x40+i,0x32,0,0x88,0xc9])+bytes((j*19+7)&255 for j in range(size-6))
 packed=bytearray(encode(Image(payload,load,load),name.split('.')[1]))
 if name=='UBAD.RKS':packed[-1]^=1
 fixtures[name]=(payload,load,0x40+i)
 count=(len(packed)+sector-1)//sector;chain=allocate(count,name.endswith('RKS'))
 for k,c in enumerate(chain):
  struct.pack_into('<H',raw,fat+c*2,chain[k+1] if k+1<count else 0xffff)
  at=start+(c-2)*sector;chunk=packed[k*sector:(k+1)*sector];raw[at:at+sector]=chunk+bytes(sector-len(chunk))
 stem,ext=name.split('.');entry=bytearray((stem.ljust(8)+ext.ljust(3)).encode()+bytes(21))
 struct.pack_into('<H',entry,26,chain[0]);struct.pack_into('<I',entry,28,len(packed));raw[slots[i]:slots[i]+32]=entry
# A non-root parent must survive prefix reads and the final body load.
dir_cluster=allocate(1)[0]
struct.pack_into('<H',raw,fat+dir_cluster*2,0xffff)
dir_at=start+(dir_cluster-2)*sector
raw[dir_at:dir_at+sector]=bytes(sector)
def dir_entry(name,cluster,attr=0,size=0):
 e=bytearray(name+bytes(21));e[11]=attr
 struct.pack_into('<H',e,26,cluster);struct.pack_into('<I',e,28,size)
 return e
raw[slots[len(fixtures)]:slots[len(fixtures)]+32]=dir_entry(b'RUNTEST    ',dir_cluster,0x10)
raw[dir_at:dir_at+32]=dir_entry(b'.          ',dir_cluster,0x10)
raw[dir_at+32:dir_at+64]=dir_entry(b'..         ',0,0x10)
fixtures['RUNTEST/DEEP.EXE']=fixtures['UEXE.EXE']
payload,load,_=fixtures['RUNTEST/DEEP.EXE'];packed=encode(Image(payload,load,load),'EXE')
count=(len(packed)+sector-1)//sector;chain=allocate(count,fragmented=True)
for k,c in enumerate(chain):
 struct.pack_into('<H',raw,fat+c*2,chain[k+1] if k+1<count else 0xffff)
 at=start+(c-2)*sector;chunk=packed[k*sector:(k+1)*sector];raw[at:at+sector]=chunk+bytes(sector-len(chunk))
raw[dir_at+64:dir_at+96]=dir_entry(b'DEEP    EXE',chain[0],size=len(packed))
image=OUT/'fixtures.bin';image.write_bytes(raw)
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{args.port}/mcp';rows=[]
def tap(key):m.tap(key)
def keyboard(text,line_address):
 m.call('bp_clear');m.call('emu_run_for',ms=500);tap('esc');m.call('emu_run_for',ms=250)
 keys={' ':'space',':':'colon','.':'period','-':'minus','/':'slash'}
 for c in text:
  if c=='_':
   m.call('kbd_send',keys=['shift'],action='press')
   try:tap('0')
   finally:m.call('kbd_send',keys=['shift'],action='release')
  else:tap(keys.get(c,c.lower()))
 assert m.read(line_address,len(text)).upper()==text.upper().encode(),text
def command(text,choice=None):
 keyboard(text,s['CMD_LINE'])
 m.screenshot(OUT/'command.png')
 m.stop_at(r['start']);tap('enter');m.until(r['start'],120,page=0)
 assert m.read(0,32)==(ROOT/'bin/RUN.i80').read_bytes()[:32]
 if choice is not None:
  m.stop_at(r['modeKey']);m.until(r['modeKey'],120,page=0)
  m.screenshot(OUT/('menu-'+str(choice)+'.png'))
  m.call('bp_clear');m.call('emu_run_for',ms=250)
  m.stop_at(r['RUN_LOAD_STAGE']);tap(str(choice))
 else:m.stop_at(r['RUN_LOAD_STAGE'])
 m.until(r['RUN_LOAD_STAGE'],120,page=0)
def returned():
 m.stop_at(n['mainLoop'])
 poll=nc_core['ncBankInputPoll']
 m.call('bp_set',addr=poll,type='exec')
 body=(ROOT/'bin/NC_CORE.i80').read_bytes()
 for _ in range(120):
  state=m.call('cpu_state');pc=state['registers']['pc']['dec']
  page=m.read(x.pageAddress,1)[0]
  if pc==n['mainLoop'] and page==0 and m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]:
   return 'MAIN'
  if pc==poll and page>1 and m.read(poll,3)==body[poll-0x100:poll-0x100+3]:
   console()
   cli_main()
   return 'EXEC'
  if state['breaked']:m.call('debug_step',mode='into')
  m.call('emu_run_for',ms=1000)
 raise AssertionError(('NC not restored',m.call('cpu_state')))
def cli_main():
 keyboard('C:/DOS/NC_MN.COM',cli['CMD_LINE'])
 m.stop_at(n['mainLoop']);tap('enter');m.until(n['mainLoop'],120,page=0)
 assert m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]
def console():
 m.call('bp_clear');m.call('emu_run_for',ms=500)
 x.console()
try:
 x.start('run-body',disk=image)
 font_before=m.read(s['FONT_ADDR'],s['FONT_BYTES'])
 cases=[('UEXE.EXE','MX',1),('UEXE.EXE','MON',4),('URKS.RKS','MON',None),('RUNTEST/DEEP.EXE','MX',1)] if args.menu_only else [(name,mode,None) for name,mode in [('UCOM.COM','MX'),('UEXE.EXE','MX'),('UEXE.EXE','STD'),('URKS.RKS','MX'),('URKS.RKS','MON'),('UEXE.EXE','RF'),('UHIGH.EXE','MX')]]
 if args.high_only:cases=[('URFH.EXE','RF',None)]
 if args.upper_only:cases=[('UHIGH.EXE','MX',None)]
 for name,mode,choice in cases:
  payload,load,tag=fixtures[name]
  if name=='UHIGH.EXE':
   console()
   keyboard('C:/DOS/FONT.COM 4X8/COMPACT',cli['CMD_LINE'])
   m.stop_at(cli['cliInput']);tap('enter');m.until(cli['cliInput'],120,page=0)
   cache_start=int.from_bytes(m.read(s['GLYPH_CACHE_START'],2),'little')
   assert cache_start<=load and load+len(payload)<=r['SYSTEM_PROGRAM_END']
   cli_main()
   resident_font=m.read(s['FONT_ADDR'],cache_start-s['FONT_ADDR'])
  text=f'C:/DOS/RUN.COM H:/{name}'+('' if args.menu_only else f' -{mode}')
  command(text,choice)
  request=m.read(r['RUN_BODY_REQUEST'],r['FILE_BODY_IO'])
  header=r['FILE_BODY_IO.header'];tail=r['FILE_BODY_IO.tail']
  expected=(4,2) if name.endswith('RKS') else (r['IMAGE_HEADER_SIZE'],0)
  assert (request[header],request[tail])==expected,(name,request.hex())
  if mode=='RF':
   m.stop_at(0xf85a);m.until(0xf85a,120)
   ramfos=(ROOT/'bin/RAMFOS.mon').read_bytes();assert m.read(0xc000,0x1300)==ramfos[:0x1300];assert m.read(0xf800,0x7e0)==ramfos[0x1300:]
  m.stop_at(load);m.until(load,120)
  assert m.read(load,len(payload))==payload,(name,mode,'payload',m.call('cpu_state'))
  if name=='UHIGH.EXE':
   assert int.from_bytes(m.read(s['GLYPH_CACHE_LIMIT'],2),'little')==load,'upper utility did not claim cache'
  if mode=='MX':
   if name=='UHIGH.EXE':
    assert m.read(s['FONT_ADDR'],len(resident_font))==resident_font
   else:assert m.read(s['FONT_ADDR'],len(font_before))==font_before
  if mode=='STD':
   assert m.read(s['bios_upper.koi7'],1)==b'\xff'
  if mode=='MON':assert m.read(0xc000,4096)==(ROOT/'bin/Monitor2.mon').read_bytes()
  m.screenshot(OUT/(name.replace('/','-')+'-'+mode+'.png'))
  profile=returned()
  if mode=='STD':
   assert m.read(s['FONT_ADDR'],len(font_before))==font_before,'font not restored after STD'
  if name=='UHIGH.EXE':
   assert int.from_bytes(m.read(s['GLYPH_CACHE_LIMIT'],2),'little')==s['GLYPH_CACHE_END'],'cache not restored'
  row=dict(file=name,mode=mode,payloadBytes=len(payload),entryReached=True,returnedNC=True,returnProfile=profile)
  rows.append(row);print(row,flush=True)
 if not args.menu_only and not args.high_only and not args.upper_only:
  command('C:/DOS/RUN.COM H:/UBAD.RKS -MX')
  m.write(0x8800,b'\0');profile=returned();assert m.read(0x8800,1)!=bytes([fixtures['UBAD.RKS'][2]])
  rows.append(dict(file='UBAD.RKS',badChecksumRejected=True,returnedNC=True,returnProfile=profile))
 (OUT/('menu-results.json' if args.menu_only else 'high-results.json' if args.high_only else 'upper-results.json' if args.upper_only else 'results.json')).write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
finally:x.stop()
