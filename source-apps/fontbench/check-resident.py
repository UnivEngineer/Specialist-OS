"""Paired resident/standalone FontBen matrix, real local PIT in visible Emu80.
Install with FONT via NC, then execute the existing 18-stage caller grid.
Never patch the native BIOS case. Standalone hooks belong to FontBen itself.
"""
from pathlib import Path
import runpy,re,time,json,hashlib,sys,os
R=Path(__file__).resolve().parents[2];O=R/os.environ.get('FONTBEN_REPORT_DIR','docs/bench/resident-vs-standalone');O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',p.read_text(),re.M)}
s=sy(R/'bin/FONTCOM.sym');n=sy(R/'bin/NC.sym');f=sy(R/'bin/FONT.sym');t=sy(R/'bin/banked/TRANSPORT.sym')
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/font-import-runtime')
x=h['Matrix']();m=x.m
cases=[('Rotate','6x8/Rotate','BIOS','OFF',6),('PagedOff','6x8/Rotate','ROW6','OFF',6),('Packed6K','6x8/Packed','BIOS','ON',6),('Tripack','6x8/Rotate','TRIPACK','ON',6),('Fast12K','6x8/Fast','BIOS','ON',6),('PagedOn','6x8/Rotate','ROW6','ON',6),('Fast8','8x8/Rotate','BIOS','OFF',8),('Standalone8','8x8/Rotate','ROW8','ON',8),('Cpm4','4x8/Fast','BIOS','OFF',4),('Dup4','4x8/Fast','4DUP','ON',4)]
cases += [('SharedRotate','6x8/Rotate','6RotateRows','OFF',6),('SharedPacked','6x8/Rotate','PACK6K','ON',6),('SharedFull','6x8/Rotate','FAST12K','ON',6),('SharedCompact','6x8/Rotate','COMPACT6','OFF',6),('Shared8','8x8/Rotate','FAST8','OFF',8),('Shared4','4x8/Fast','CPM4','OFF',4),('Colpre','6x8/Rotate','COL6PRE','ON',6)]
cases += [('Window8','8x8/Rotate','WIN8','OFF',8)]
cases.sort(key=lambda c: {'Window8':-1,'Packed6K':0,'Fast12K':1}.get(c[0],2))
inverse='--inverse' in sys.argv
selected=[arg for arg in sys.argv[1:] if arg!='--inverse']
if selected:cases=[c for c in cases if c[0] in selected]
def wait(pc,nc=False,seconds=180):
 end=time.monotonic()+seconds
 while time.monotonic()<end:
  st=m.call('cpu_state');at=st['registers']['pc']['dec']
  if at==pc and m.read(t['bankPage'],1)==b'\0':
   # MAIN programs reuse addresses: NC may hit FONT's breakpoint while
   # dispatching Enter, before FONT is loaded. Check the image, not PC alone.
   expected = 'NC.i80' if nc else 'FONT.i80' if pc==f['committed'] else None
   if expected is None or m.read(0,16)==(R/'bin'/expected).read_bytes()[:16]:return
   m.call('debug_step',mode='into')
  if at==s['gridError'] and pc!=f['committed'] and m.read(t['bankPage'],1)==b'\0':raise AssertionError(('gridError',st,m.read(s['gridControl'],14).hex()))
  m.call('emu_run_for',ms=1000)
 m.screenshot(O/'timeout.png')
 raise AssertionError(('timeout',hex(pc),st))
def command(cmd,pc):
 m.call('bp_clear');m.call('emu_run_for',ms=3000);m.tap('esc');m.call('emu_run_for',ms=250)
 keys={':':'colon','/':'slash','.':'period',' ':'space','-':'minus','=':'equals'}
 for c in cmd:m.tap(keys.get(c,c.lower()))
 assert m.read(n['vars.cmdLine'],len(cmd)).upper()==cmd.upper().encode(),cmd
 m.screenshot(O/'command.png');m.stop_at(pc);m.tap('enter');wait(pc)
def resume(pc,nc=False):
 m.stop_at(pc);m.call('debug_step',mode='into');wait(pc,nc)
def oracle(corpus,font,width,raw):
 out=bytearray(12288);xp=0;y=0
 for c in corpus:
  if not raw and c==13:xp=0;continue
  if not raw and c==10:y+=10;continue
  for row in range(8):
   bits=font[c*8+row]^(((1<<width)-1) if inverse else 0)
   for bit in range(width):
    if bits&(1<<(width-1-bit)):
     px=xp+bit;out[(px//8)*256+y+row]|=128>>(px%8)
  xp+=width
  if xp>=384:xp=0;y+=10
 return out,bytes([y+7,xp//2])
try:
 # Refuse a stale image after a failed build, before opening an emulator.
 disk=O/'bench-h.bin'
 disk.write_bytes(b''.join((R/f'images/FlashDisk_chip{i}_512K.bin').read_bytes() for i in range(1,5)))
 tree=runpy.run_path(str(R/'scripts/tests/check-nested-image.py'))['read_tree'](disk.read_bytes())[0]
 assert s['CORPUS_IO_BUFFER']==0x3500,'corpus scratch overlaps resident table'
 assert tree['BENCH/FontBen/FontBen.com'][32:]==(R/'bin/FONTCOM.i80').read_bytes(),'stale FontBen image'
 x.start('paired-font-grid',system=R/'images/SystemRom_emu80_writable.bin',disk=disk)
 for tag,module,engine,pre,width in cases:
  folder=O/(tag+('-inverse' if inverse else ''));folder.mkdir(exist_ok=True)
  (folder/'verified.json').unlink(missing_ok=True)
  print('INSTALL',tag,flush=True)
  command('FONT '+module,f['committed']);resume(n['mainLoop'],True)
  baseline=m.read(0xc800,0xf6)
  lower=int.from_bytes(m.read(0xd82c,2),'little')
  prebase=0x8c00-lower
  predata=m.read(prebase,lower) if lower else b''
  if lower:assert predata==(R/('bin/fonts/'+tag+'.pre')).read_bytes(),(tag,'generated planes')
  if engine=='WIN8':font=(R/'source/monitor_win/originals/monitor_win.rom').read_bytes()[0x1800:0x2000]
  elif width==8:font=(R/'assets/system-rom/fonts/8x8/Win8.pf').read_bytes()
  elif width==4:
   font=(R/'assets/system-rom/fonts/4x8/Fast.pf').read_bytes()
  elif tag in ('Compact','SharedCompact'):
   src=(R/'bin/fonts/compact-font.bin').read_bytes();lookup=(R/'bin/fonts/compact-map.bin').read_bytes();font=bytes(src[lookup[c]*8+y] for c in range(256) for y in range(8))
  else:font=bytes(b&63 for b in (R/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes())
  command('H:/Bench/FontBen/FontBen.com -E '+engine+' -P '+pre+' -Q'+(' -I' if inverse else ''),s['gridMeasured'])
  if lower:assert m.read(prebase,lower)==predata,(tag,'corpus preparation damaged resident table')
  corpus=m.read(s['gridText'],s['GRID_TEXT_BYTES']);refs=[oracle(corpus,font,width,r) for r in (False,True)]
  (folder/'corpus.bin').write_bytes(corpus);results=[]
  for stage in range(18):
   kind=m.read(s['gridKind'],1)[0];route=m.read(s['gridRoute'],1)[0]
   expect,cursor=refs[kind in (1,3,4)];actual=m.read(0x9000,12288)
   diffs=[i for i in range(12288) if i%256<240 and actual[i]!=expect[i]]
   assert not diffs,(tag,stage,kind,route,'pixels',diffs[:12])
   assert m.read(s['bios_vars.cursorY'],2)==cursor,(tag,stage,'cursor')
   ticks=int.from_bytes(m.read(s['v_elapsed'],4),'little');assert 0<ticks<65536,(tag,stage,ticks)
   results.append(dict(kind=kind,route=route,ticks=ticks,ms=round(ticks*.128,3)))
   (folder/'results.json').write_text(json.dumps(dict(module=module,engine=engine,results=results),indent=2))
   print(tag,stage+1,kind,route,results[-1]['ms'],flush=True)
   if stage==0:m.screenshot(folder/'first.png')
   if stage<17:resume(s['gridMeasured'])
  resume(s['gridAdvanceEngine'])
  assert m.read(0xc800,0xf6)==baseline,(tag,'ABI restoration')
  if lower:assert m.read(prebase,lower)==predata,(tag,'resident table damaged')
  start=int.from_bytes(m.read(s['gridControl']+8,2),'little');end=int.from_bytes(m.read(s['gridWrite'],2),'little')
  assert start<end<start+2304,(tag,'report size')
  (folder/'FONTBEN.TXT').write_bytes(m.read(start,end-start))
  resume(n['mainLoop'],True)
  (folder/'verified.json').write_text(json.dumps(dict(pixels=True,abiRestored=True,residentPreserved=True,returnedToNC=True,cpuHz=2000000,pitHz=2000000,imageSha256=hashlib.sha256((R/'images/SystemRom_emu80_writable.bin').read_bytes()).hexdigest()),indent=2))
  print(tag,'PASS',flush=True)
 command('FONT 6x8/Rotate',f['committed']);resume(n['mainLoop'],True)
finally:x.stop()
