"""Owned visible/muted Emu80 comparison of MAIN and resident EXEC NC."""
from pathlib import Path
import argparse,hashlib,json,re,runpy,shutil,subprocess
ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/nc/main-bank-performance-2026-11-17'
RUNTIME=ROOT/'.local/monitor2-001'
PORT=19781
SNAP=OUT/'build'
W=lambda n:int(n).to_bytes(2,'little')
def symbols(name):
 return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(SNAP/'bin'/f'{name}.sym').read_text(encoding='utf-8'),re.M)}
def graphics_name():
 # Historical snapshots contain the earlier separate NC graphics DLL.
 return 'UI_GFX' if (SNAP/'bin/UI_GFX.sym').is_file() else 'NC_GFX'

def graphics_symbols(read_dos, kernel, handle):
 gfx=symbols(graphics_name())
 if 'IMAGE_RELOC_KIND' not in gfx:return gfx
 stride=kernel['MODULE_RECORD_SIZE']
 records=read_dos(kernel['MODULE_TABLE'],kernel['MODULE_MAX']*stride)
 for i in range(0,len(records),stride):
  if records[i:i+2]==W(handle) and records[i+3]==0:
   base=int.from_bytes(records[i+10:i+12],'little')
   # Доступны символы кода и данных DLL, внешние ABI и поля структур остаются абсолютными.
   for key in gfx:
    if key.startswith(('ng','ui','fl')) or key=='entry':
     gfx[key]+=base
   return gfx
 raise ValueError(f'Нет загруженного UI_GFX handle={handle:04X}')
def standard_names():
 return b'NC_MN   COM' in (SNAP/'A.bin').read_bytes()
class Campaign:
 def __init__(self,m):
  self.m=m;self.b=symbols('BIOS');self.k=symbols('banked/KERNEL');self.t=symbols('banked/TRANSPORT');self.page_addr=self.t['bankPage'];self.n=symbols('NC');self.page=0
 def state(self):return self.m.call('cpu_state')
 def page_now(self):return self.m.read(self.page_addr,1)[0]
 def select(self,page):self.m.write(0xfffd if page else 0xfffc,bytes([page-1 if page else 0]))
 def read(self,page,at,size):
  old=self.page_now()
  try:
   if old!=page:self.select(page)
   return self.m.read(at,size)
  finally:
   if old!=page:self.select(old)
 def wait(self,at,page,limit=40,advance=False):
  self.m.call('bp_clear');self.m.call('bp_set',addr=at,type='exec')
  if advance and self.state()['registers']['pc']['dec']==at:self.m.call('debug_step',mode='into')
  for _ in range(limit):
   state=self.state()
   if state['registers']['pc']['dec']==at:
    if self.page_now()==page:return state
    self.m.call('debug_step',mode='into')
   else:self.m.call('emu_run_for',ms=1000)
  self.m.screenshot(OUT/'timeout.png')
  raise RuntimeError(('wait timeout',hex(at),page,self.state()))
 def free(self):self.m.call('bp_clear');self.m.call('emu_run_for',ms=500)
 def tap(self,key):self.m.tap(key)
 def type(self,text):
  self.free();self.tap('esc');self.m.call('emu_run_for',ms=250)
  keys={' ':'space',':':'colon','.':'period','-':'minus','/':'slash',chr(92):'backslash'}
  for char in text:
   if char=='_':
    self.m.call('kbd_send',keys=['shift'],action='press')
    try:self.m.call('kbd_send',keys=['0'],action='tap',duration_ms=100)
    finally:self.m.call('kbd_send',keys=['shift'],action='release')
   else:self.m.call('kbd_send',keys=[keys.get(char,char.lower())],action='tap',duration_ms=100)
   self.m.call('emu_run_for',ms=120)
  actual=self.read(self.page,self.n['vars.cmdLine'],len(text))
  self.m.screenshot(OUT/'typed.png')
  assert actual.upper()==text.encode().upper(),(text,actual)
 def command(self,text):
  self.type(text);self.tap('enter');self.m.call('emu_run_for',ms=5000)
 def launch(self,bank):
  modern=standard_names()
  name=('NC.COM' if bank else 'NC_MN.COM') if modern else ('NC_BANK.COM' if bank else 'NC.COM')
  self.type('A:'+name)
  self.m.screenshot(OUT/('launch-bank.png' if bank else 'launch-main.png'))
  self.m.call('bp_clear');self.m.call('bp_set',addr=self.k['keExternal'],type='exec')
  self.tap('enter')
  state=self.state()
  if state['registers']['pc']['dec']!=self.k['keExternal'] or self.page_now()!=1:state=self.wait(self.k['keExternal'],1)
  start=state['cpu_ticks']
  if not (bank and modern):stop=self.wait(self.b['getch2'],0)
  if bank:
   keys=self.read(1,self.k['residentKeys'],16);slot=next(i for i in range(8) if keys[i*2:i*2+2]==W(0x434e))
   record=self.read(1,self.k['MODULE_TABLE']+slot*self.k['MODULE_RECORD_SIZE'],self.k['MODULE_RECORD_SIZE'])
   self.page=record[3];self.n=symbols('NC_CORE')
   if modern:stop=self.wait(self.n['ncBankInputPoll'],self.page)
   assert self.read(self.page,0x100,12)==(SNAP/'bin/NC_CORE.i80').read_bytes()[:12]
  else:self.page=0;self.n=symbols('NC')
  elapsed=stop['cpu_ticks']-start
  self.m.screenshot(OUT/('ready-bank.png' if bank else 'ready-main.png'))
  print('STARTUP',bank,elapsed/2000,flush=True)
  return elapsed
 def panel(self):return self.read(self.page,self.n['state.activePanel'],1)[0]
 def cursor(self):return self.read(self.page,self.n['state.panelA_curFile']+self.panel(),1)[0]
 def keyboard(self):
  self.free()
  if self.panel()==0:self.tap('tab')
  result={}
  visible=self.read(self.page,self.n['ncVisibleCount'],1)[0]
  for direction,anchor,step in [('down','home',1),('up','end',-1)]:
   self.free();self.tap(anchor);self.m.call('emu_run_for',ms=250)
   start=self.wait(self.b['loc_C23C'],0)['cpu_ticks']
   before=self.cursor();prior_first=self.read(self.page,self.n['state.panelA_firstFile']+self.panel(),1)[0]
   events=[];previous=start
   self.m.call('kbd_send',keys=[direction],action='press')
   try:
    for index in range(visible+8):
     decoded=self.wait(self.b['loc_C311'],0)
     assert decoded['registers']['af']['dec']>>8 == (0x1a if step==1 else 0x19),decoded
     state=self.wait(self.b['getch2'],0)
     cursor=self.cursor();first=self.read(self.page,self.n['state.panelA_firstFile']+self.panel(),1)[0]
     assert cursor==before+(index+1)*step,(direction,index,before,cursor)
     events.append(dict(interval_ticks=state['cpu_ticks']-previous,
                        render_ticks=state['cpu_ticks']-decoded['cpu_ticks'],
                        scroll=first!=prior_first,cursor=cursor,first=first))
     previous=state['cpu_ticks'];prior_first=first
   finally:self.m.call('kbd_send',keys=[direction],action='release')
   result[direction]=events
  return result
 def pipeline(self):
  if not self.page:return None
  self.free()
  if self.panel()==0:self.tap('tab')
  self.tap('home');self.m.call('emu_run_for',ms=250)
  self.wait(self.b['loc_C23C'],0)
  self.m.call('kbd_send',keys=['down'],action='press')
  try:
   stamps=[self.wait(self.b['loc_C311'],0)['cpu_ticks']]
   stamps.append(self.wait(self.n['ncBankGetch'],self.page)['cpu_ticks'])
   handle=int.from_bytes(self.read(self.page,self.n['ncGfxHandle'],2),'little')
   gfx=graphics_symbols(lambda a,n:self.read(1,a,n),self.k,handle)
   stamps.append(self.wait(gfx['ngBatch'],0)['cpu_ticks'])
   stamps.append(self.wait(gfx['ngSaveCursor'],0)['cpu_ticks'])
   stamps.append(self.wait(self.b['getch2'],0)['cpu_ticks'])
  finally:self.m.call('kbd_send',keys=['down'],action='release')
  return dict(labels=['key return + CORE logic/queue','FAR transfer + GFX entry','MAIN batch render','return + KEY entry'],ticks=[b-a for a,b in zip(stamps,stamps[1:])])
 def cli_return(self):
  self.free()
  if self.panel()==0:self.tap('tab')
  self.tap('home');self.m.call('emu_run_for',ms=250)
  cli=symbols('CLI')
  self.m.call('bp_clear');self.m.call('bp_set',addr=cli['cliPrompt'],type='exec');self.tap('f10')
  self.wait(cli['cliPrompt'],0)
  assert self.read(0,0,12)==(SNAP/'bin/CLI.i80').read_bytes()[:12]
  self.free();self.tap('esc')
  for char in 'exit':self.tap(char)
  assert self.read(0,cli['CMD_LINE'],4).upper()==b'EXIT'
  self.m.call('bp_clear');self.m.call('bp_set',addr=self.b['loc_C311'],type='exec');self.tap('enter')
  start=self.wait(self.b['loc_C311'],0)
  assert start['registers']['af']['dec']>>8==13,start
  stop=self.wait(self.b['getch2'],0)
  return stop['cpu_ticks']-start['cpu_ticks']
 def kernel_benches(self):
  self.free()
  if self.panel()==0:self.tap('tab')
  self.tap('home');self.m.call('emu_run_for',ms=250)
  # Enter the real NC key path with a regular Escape before isolated ABI tests.
  self.m.call('bp_clear');self.m.call('bp_set',addr=self.n['ncGetch'],type='exec');self.tap('esc')
  self.wait(self.n['ncGetch'],self.page)
  origin=self.n['NC_SORT_KEYS'];done=origin+0xc0;buffer=origin+0x100
  sp=self.state()['registers']['sp']['dec']
  def timed(code):
   self.m.write(origin,code+b'\xc3'+W(done));self.m.call('bp_clear');self.m.call('bp_set',addr=done,type='exec')
   self.m.call('regs_set',pc=origin,sp=sp)
   start=self.state()['cpu_ticks'];stop=self.wait(done,self.page)
   assert stop['registers']['sp']['dec']==sp,('bench stack',stop)
   return stop['cpu_ticks']-start,stop
  panel=self.panel()
  assert self.read(self.page,self.n['state.panelA_drive']+panel,1)==b'\x07'
  assert self.read(self.page,self.n['state.panelA_filesCnt']+panel,1)==b'\x80'
  slot=self.read(self.page,self.n['vars.cacheSlotA']+panel,1)[0]
  base=int.from_bytes(self.read(self.page,self.n['ncCatalogBases']+slot*2,2),'little')
  records=self.read(self.page,base,128*16)
  index=self.read(self.page,self.n['FILE_INDEX_A']+panel*256,128)
  rows={'record_sha256':hashlib.sha256(records).hexdigest(),'index_sha256':hashlib.sha256(index).hexdigest(),'abi_revision':2}
  if self.page:timed(b'\xcd'+W(self.n['ncBankFlush'])) # Finish earlier UI work outside the benchmark.
  text=b'\xcd'+W(self.n['printWholeFileList'])
  if self.page:text+=b'\xcd'+W(self.n['ncBankFlush'])
  rows['text_ticks']=[timed(text)[0] for _ in range(3)]
  rows['visible_names']=self.read(self.page,self.n['ncVisibleCount'],1)[0]
  # Context setup is deliberately outside the timed DIR_OP_LIST request.
  context=b'\x3e\x07\x11\x01\x00\xcd'+W(self.n['mx_callRegs'])+W(self.n['MXOS_DOS_FILE_SELECT_DRIVE'])
  context+=b'\x11\x00\x00\x3e'+bytes([self.n['DIR_OP_SET']])+b'\xcd'+W(self.n['mx_callRegs'])+W(self.n['MXOS_DOS_DIRECTORY'])
  timed(context)
  query=b'\x21'+W(buffer)+b'\x01\x00\x00\x11'+W(128)+b'\x3e'+bytes([self.n['DIR_OP_LIST']])+b'\xcd'+W(self.n['mx_callRegs'])+W(self.n['MXOS_DOS_DIRECTORY'])
  rows['list_ticks']=[]
  for _ in range(3):
   ticks,state=timed(query);assert not state['flags']['C'],state
   assert state['registers']['de']['dec']==128,state
   rows['list_ticks'].append(ticks)
  # Nonblocking raw key scans, without a pressed key; no cursor/repeat delay.
  scanner=self.n['ncBankKeyScan'] if self.page else self.n['bios_keyScan']
  code=b'\x06\x64\xc5\xcd'+W(scanner)+b'\xc1\x05\xc2'+W(origin+2)
  rows['scan100_ticks']=[timed(code)[0] for _ in range(3)]
  # Prototype only: same BIOS scan source and key table in local EXEC RAM.
  # No banked application or public ABI is patched by this comparison.
  local=origin+0xc00
  table=self.read(0,self.b['v_keybTbl'],96)
  asm=f'''    INCLUDE "{(ROOT/'sdk/asm/mxos.inc').as_posix()}"
    INCLUDE "{(ROOT/'sdk/asm/hardware/spec_mx.inc').as_posix()}"
    ORG {local}
    jp keyScan2
ioKeyMode82:
    ld a,082h
    ld (IO_KEYB_MODE),a
    ret
    INCLUDE "{(ROOT/'source/io/key-scan.inc').as_posix()}"
    INCLUDE "{(ROOT/'source/bios/keyScan.inc').as_posix()}"
keyColumn: DW 0
keyRow: DB 0
    ALIGN 256
v_keybTbl:
    DB {','.join(str(v) for v in table)}
    END
'''
  folder=OUT/'local-scan';folder.mkdir(exist_ok=True)
  (folder/'scan.asm').write_text(asm,encoding='utf-8')
  subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',f'--raw={folder / "scan.bin"}',str(folder/'scan.asm')],check=True,capture_output=True)
  self.m.write(local,(folder/'scan.bin').read_bytes())
  local_code=b'\x06\x64\xc5\xcd'+W(local)+b'\xc1\x05\xc2'+W(origin+2)
  rows['scan100_local_ticks']=[timed(local_code)[0] for _ in range(3)]
  self.m.call('kbd_send',keys=['down'],action='press')
  try:
   rows['scan100_pressed_ticks']=[timed(code)[0] for _ in range(3)]
   rows['scan100_local_pressed_ticks']=[timed(local_code)[0] for _ in range(3)]
  finally:self.m.call('kbd_send',keys=['down'],action='release')
  return rows

def verify_snapshot():
 image=(SNAP/'A.bin').read_bytes()
 word=lambda at:int.from_bytes(image[at:at+2],'little')
 sector=word(11);root=(word(14)+image[16]*word(22))*sector
 data=root+((word(17)*32+sector-1)//sector)*sector
 directory=image[root:root+word(17)*32]
 def payload(name):
  base,_,ext=name.partition('.')
  key=(base.ljust(8)+ext.ljust(3)).upper().encode('ascii')
  entry=next(directory[i:i+32] for i in range(0,len(directory),32) if directory[i:i+11].upper()==key)
  cluster=int.from_bytes(entry[26:28],'little');out=b''
  while 2<=cluster<0xfff8:
   at=data+(cluster-2)*image[13]*sector;out+=image[at:at+image[13]*sector]
   cluster=word(word(14)*sector+cluster*2)
  return out[:int.from_bytes(entry[28:32],'little')]
 for name in ['DOS.sys','BIOS.sys','IO.sys','MEM.sys']:
  assert payload(name)==(SNAP/'bin'/name).read_bytes(),('stale system image',name)
 executables=[('NC_MN.COM','NC'),('NC.COM','NC_DOS')] if standard_names() else [('NC.COM','NC'),('NC_BANK.COM','NC_BANK')]
 executables += [('NC_CORE.DLL','NC_CORE'),(graphics_name()+'.DLL',graphics_name()),('CLI.COM','CLI')]
 for name,stem in executables:
  data=payload(name)
  header_bytes=data[symbols('NC')['IMAGE_HEADER_BYTES']] if data[4] in (2,3) else 16
  assert data[header_bytes:]==(SNAP/'bin'/(stem+'.i80')).read_bytes(),('stale application',name)

def main():
 global RUNTIME,PORT
 parser=argparse.ArgumentParser(description=__doc__)
 parser.add_argument('--runtime',type=Path,default=RUNTIME)
 parser.add_argument('--port',type=int,choices=range(19781,19785),default=PORT)
 parser.add_argument('--resume',action='store_true',help='Continue incomplete rows from this frozen build')
 parser.add_argument('--refresh',action='store_true',help='Build OS/image and freeze current outputs before measuring')
 parser.add_argument('--verify-only',action='store_true',help='Check frozen image against binaries; no emulator')
 args=parser.parse_args();RUNTIME=args.runtime.resolve();PORT=args.port
 if args.resume and args.refresh:parser.error('--resume and --refresh are mutually exclusive')
 OUT.mkdir(parents=True,exist_ok=True)
 helpers=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
 if args.refresh or not (SNAP/'A.bin').exists():
  if args.verify_only:parser.error('Snapshot missing; run with --refresh first')
  helpers['command'](['powershell.exe','-NoProfile','-File','scripts/build/build-project.ps1','OS'])
  helpers['command'](['powershell.exe','-NoProfile','-File','scripts/images/build-image.ps1','SystemEmuWritable'])
  SNAP.mkdir(parents=True,exist_ok=True)
  shutil.copytree(ROOT/'bin',SNAP/'bin',dirs_exist_ok=True,ignore=shutil.ignore_patterns('*.lst','*.log','__pycache__'))
  shutil.copyfile(ROOT/'images/C-Flash-512k-MMX-System-Emu80.bin',SNAP/'A.bin')
  shutil.copyfile(ROOT/'images/H-Flash-2M-Games.bin',SNAP/'H128.bin')
 verify_snapshot()
 if args.verify_only:
  print('Frozen system/application payloads match the measured binaries');return
 def owner():
  return helpers['shell'](f'$listener=Get-NetTCPConnection -State Listen -LocalPort {PORT} -ErrorAction SilentlyContinue; if ($listener) {{$listener[0].OwningProcess}};exit 0')
 helpers['Matrix'].__init__.__globals__.update(RUNTIME=RUNTIME,OUT=OUT,mcp_owner=owner)
 x=helpers['Matrix']();x.m.url=f'http://127.0.0.1:{PORT}/mcp'
 report={'cpu_hz':2000000,'hardware_measured':False,'rows':[]}
 previous=OUT/'results.json'
 if args.resume and previous.exists():report=json.loads(previous.read_text(encoding='utf-8'))
 launcher='NC_DOS' if standard_names() else 'NC_BANK'
 report['artifacts']={name:hashlib.sha256((SNAP/name).read_bytes()).hexdigest() for name in ['A.bin','H128.bin','bin/NC.i80','bin/NC_CORE.i80',f'bin/{graphics_name()}.i80',f'bin/{launcher}.i80','bin/DOS.sys','bin/BIOS.sys','bin/MEM.sys']}
 report['emulator_sha256']=hashlib.sha256((RUNTIME/'Emu80qt-mcp.exe').read_bytes()).hexdigest()
 try:
  for font in ['Rotate','Compact','Fast8','Cpm4']:
   for bank in [False,True]:
    old=next((r for r in report['rows'] if r['font']==font and r['profile']==('BANK' if bank else 'MAIN')),None)
    if old and 'cli_return_ticks' in old and old.get('abi',{}).get('abi_revision')==2:continue
    x.start(('BANK-' if bank else 'MAIN-')+font,disk=SNAP/'H128.bin',system=SNAP/'A.bin',nc_binary=SNAP/'bin/NC.i80',nc_symbols=SNAP/'bin/NC.sym',bios=SNAP/'bin/BIOS.sys',dos=SNAP/'bin/DOS.sys')
    c=Campaign(x.m)
    if font!='Rotate':
     folder='4x8' if font=='Cpm4' else '8x8' if font=='Fast8' else '6x8'
     c.command('FONT '+folder+'/'+font)
     assert c.read(0,c.b['FONT_ENGINE_HEADER']+7,1)==bytes([dict(Compact=3,Fast8=4,Cpm4=5)[font]]),font
    startup=c.launch(bank)
    ram_pages=c.read(0,c.b['bios_upper.ramPageCount'],1)[0]
    row=old or {'profile':'BANK' if bank else 'MAIN','font':font,'startup_ticks':startup}
    row['ram_pages']=ram_pages
    row.setdefault('startup_trials_ticks',[row['startup_ticks']])
    if old:row['startup_trials_ticks'].append(startup)
    if not old:
     row['keys']=c.keyboard();print('KEYS DONE',row['profile'],font,flush=True)
    row['pipeline']=c.pipeline()
    row['cli_return_ticks']=c.cli_return();print('RETURN',row['profile'],font,row['cli_return_ticks']/2000,flush=True)
    row['abi']=c.kernel_benches()
    if not old:report['rows'].append(row)
    (OUT/'results.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
 finally:x.stop()
if __name__=='__main__':main()
