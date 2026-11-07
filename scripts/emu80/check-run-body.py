"""RUN из строки NC: единая загрузка тела и подмена ядра без верхнего RUN."""
from pathlib import Path
import json,runpy,struct,sys,argparse
ROOT=Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/images'))
from executable import Image,encode
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--menu-only',action='store_true');parser.add_argument('--high-only',action='store_true');parser.add_argument('--upper-only',action='store_true');args=parser.parse_args()
OUT=ROOT/'docs/run-body-041';OUT.mkdir(parents=True,exist_ok=True)
g=h['Matrix'].__init__.__globals__;g['RUNTIME']=ROOT/'.local/monitor2-001';g['OUT']=OUT
s=h['symbols'](ROOT/'bin/BIOS.sym');r=h['symbols'](ROOT/'bin/RUN.sym');n=h['symbols'](ROOT/'bin/NC.sym')
raw=bytearray((ROOT/'bin/nested-Games2M.bin').read_bytes())
w=lambda at:struct.unpack_from('<H',raw,at)[0]
sector=w(11);fat=w(14)*sector;root=(w(14)+raw[16]*w(22))*sector
start=root+((w(17)*32+sector-1)//sector)*sector
slots=[i for i in range(root,start,32) if raw[i] in (0,0xe5,0xff)]
free=[c for c in range(2,(len(raw)-start)//sector+2) if w(fat+c*2)==0]
fixtures={}
for i,(name,load) in enumerate([('UCOM.COM',0),('UEXE.EXE',0),('URKS.RKS',0),('UHIGH.EXE',0xe600),('UBAD.RKS',0),('URFH.EXE',0xd300)]):
 payload=bytes([0x3e,0x40+i,0x32,0,0x88,0xc9])+bytes((j*19+7)&255 for j in range((1025 if load else 4097)-6))
 packed=bytearray(encode(Image(payload,load,load),name.split('.')[1]))
 if name=='UBAD.RKS':packed[-1]^=1
 fixtures[name]=(payload,load,0x40+i)
 count=(len(packed)+sector-1)//sector;chain,free=free[:count],free[count:]
 for k,c in enumerate(chain):
  struct.pack_into('<H',raw,fat+c*2,chain[k+1] if k+1<count else 0xffff)
  at=start+(c-2)*sector;chunk=packed[k*sector:(k+1)*sector];raw[at:at+sector]=chunk+bytes(sector-len(chunk))
 stem,ext=name.split('.');entry=bytearray((stem.ljust(8)+ext.ljust(3)).encode()+bytes(21))
 struct.pack_into('<H',entry,26,chain[0]);struct.pack_into('<I',entry,28,len(packed));raw[slots[i]:slots[i]+32]=entry
image=OUT/'fixtures.bin';image.write_bytes(raw)
x=h['Matrix']();m=x.m;rows=[]
def tap(key):m.tap(key)
def command(text,choice=None):
 m.call('bp_clear');m.call('emu_run_for',ms=500);tap('esc');m.call('emu_run_for',ms=250)
 keys={' ':'space',':':'colon','.':'period','-':'minus','/':'slash'}
 for c in text:tap(keys.get(c,c.lower()))
 assert m.read(s['CMD_LINE'],len(text)).upper()==text.upper().encode(),text
 m.screenshot(OUT/'command.png')
 if choice is not None:
  m.stop_at(r['modeKey']);tap('enter')
  for _ in range(100):
   m.until(r['modeKey'],120)
   if m.read(0,32)==(ROOT/'bin/RUN.i80').read_bytes()[:32]:break
   m.call('debug_step',mode='into')
  else:raise AssertionError('RUN menu not reached')
  m.screenshot(OUT/('menu-'+str(choice)+'.png'));m.stop_at(r['RUN_LOAD_STAGE'])
  m.call('emu_run_for',ms=500);tap(str(choice))
 else:
  m.stop_at(r['RUN_LOAD_STAGE']);tap('enter')
 m.until(r['RUN_LOAD_STAGE'],120)
def returned():
 m.stop_at(n['mainLoop'])
 for _ in range(100):
  m.until(n['mainLoop'],120)
  if m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]:return
  m.call('debug_step',mode='into')
 raise AssertionError('NC not restored')
try:
 x.start('run-body',disk=image)
 cases=[('UEXE.EXE','MX',1),('UEXE.EXE','MON',4),('URKS.RKS','MON',None)] if args.menu_only else [(name,mode,None) for name,mode in [('UCOM.COM','MX'),('UEXE.EXE','MX'),('UEXE.EXE','STD'),('URKS.RKS','MX'),('URKS.RKS','MON'),('UEXE.EXE','RF'),('UHIGH.EXE','MX')]]
 if args.high_only:cases=[('URFH.EXE','RF',None)]
 if args.upper_only:cases=[('UHIGH.EXE','MX',None)]
 for name,mode,choice in cases:
  payload,load,tag=fixtures[name]
  text=f'A:/RUN.COM H:/{name}'+('' if args.menu_only else f' -{mode}')
  command(text,choice)
  request=m.read(r['RUN_BODY_REQUEST'],43)
  assert request[34:36]==(b'\x04\x02' if name.endswith('RKS') else b'\x10\x00')
  if mode=='RF':
   m.stop_at(0xf85a);m.until(0xf85a,120)
   core=(ROOT/'bin/RAMFOS.mon').read_bytes();assert m.read(0xc000,0x1300)==core[:0x1300];assert m.read(0xf800,0x7e0)==core[0x1300:]
  m.stop_at(load);m.until(load,120)
  assert m.read(load,len(payload))==payload,(name,mode,'payload',m.call('cpu_state'))
  if name=='UHIGH.EXE':
   assert int.from_bytes(m.read(s['GLYPH_CACHE_LIMIT'],2),'little')==load,'upper utility did not claim cache'
  if mode in ('MX','STD'):
   font=(ROOT/'assets/system-rom/FONTKOI.FNT').read_bytes();assert m.read(s['FONT_ADDR'],len(font))==font
  if mode=='MON':assert m.read(0xc000,4096)==(ROOT/'bin/Monitor2.mon').read_bytes()
  m.screenshot(OUT/(name+'-'+mode+'.png'))
  returned()
  if name=='UHIGH.EXE':
   assert int.from_bytes(m.read(s['GLYPH_CACHE_LIMIT'],2),'little')==s['GLYPH_CACHE_END'],'cache not restored'
  row=dict(file=name,mode=mode,payloadBytes=len(payload),entryReached=True,returnedNC=True)
  rows.append(row);print(row,flush=True)
 if not args.menu_only and not args.high_only and not args.upper_only:
  command('A:/RUN.COM H:/UBAD.RKS -MX')
  m.write(0x8800,b'\0');returned();assert m.read(0x8800,1)!=bytes([fixtures['UBAD.RKS'][2]])
  rows.append(dict(file='UBAD.RKS',badChecksumRejected=True,returnedNC=True))
 (OUT/('menu-results.json' if args.menu_only else 'high-results.json' if args.high_only else 'upper-results.json' if args.upper_only else 'results.json')).write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
finally:x.stop()
