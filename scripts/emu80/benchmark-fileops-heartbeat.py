"""Old commit vs heartbeat: keyboard CLI/F5 timings and matched-core API control."""
from pathlib import Path
import runpy,re,struct,json,hashlib,shutil,statistics,subprocess,argparse
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--port',type=int,default=19782);p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782');p.add_argument('--repeats',type=int,default=3);p.add_argument('--phase',choices=['old','new','all'],default='all');p.add_argument('--frontends',default='API,CLI,NC');p.add_argument('--label');p.add_argument('--no-prepare',action='store_true');p.add_argument('--kinds',default='file,tree');a=p.parse_args();fronts=a.frontends.split(',');kinds=a.kinds.split(',');cases=[(k,'ONE.BIN' if k=='file' else 'TREE') for k in kinds]
O=R/'docs/fileops/heartbeat-2026-11-15';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text('utf-8'),re.M)}
word=lambda n:struct.pack('<H',n)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g.update(OUT=O,RUNTIME=a.runtime.resolve());g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
rows=json.loads((O/'results.json').read_text()) if (O/'results.json').exists() else []
filedata=bytes(range(256))*64
payloads={f'D{d}/F{i}.BIN':bytes([d*4+i])*1024 for d in range(4) for i in range(4)}

def run(version):
 global x,m,k,n,c,root
 root=R/'.local/copy-heartbeat-old-b54fc5b' if version=='old' else R
 out=O/(a.label or version);out.mkdir(exist_ok=True)
 phase_rows=[]
 for filename in ['DOS.sys','DOS.sym','BIOS.sys','MEM.sys','NC.sym','NC.i80','CLI.sym','FILEOPS.i80','FILEOPS.sym']:
  q=root/'bin'/filename
  if q.exists():shutil.copyfile(q,out/filename)
 shutil.copyfile(root/'bin/banked/KERNEL.sym',out/'KERNEL.sym')
 shutil.copyfile(root/'images/C-Flash-512k-MMX-System-Emu80.bin',out/'system.bin')
 k=sy(out/'KERNEL.sym');n=sy(out/'NC.sym');c=sy(out/'CLI.sym')
 x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp'
 def until(pc,seconds=60,page=None):
  for _ in range(seconds*4):
   z=m.call('cpu_state')
   if z['registers']['pc']['dec']==pc:
    if page is None or m.read(k['bankPage'],1)==bytes([page]):return z
    m.call('debug_step',mode='into');continue
   m.call('emu_run_for',ms=250)
  raise RuntimeError(m.call('cpu_state'))
 m.until=until
 def page(v):m.write(0xfffd if v else 0xfffc,bytes([v-1 if v else 0]))
 def cpu():return m.call('cpu_state')
 def call(op,hl=0,de=0,bc=0,aa=0,keep=False):
  page(0);saved=cpu()['registers'] if keep else None
  m.write(0x8d00,struct.pack('<4H',hl,de,bc,aa*256+2));m.write(0x8d20,b'\xcd\x00\xf8\xc3\x30\x8d')
  m.stop_at(0x8d30);m.call('regs_set',pc=0x8d20,sp=0x8df0,bc=op,de=0x8d00)
  z=m.until(0x8d30,180,page=0);rr={kk:v['dec'] for kk,v in z['registers'].items()}
  assert not rr['af']&1,(version,hex(op),rr)
  if keep:
   m.call('bp_clear');m.call('regs_set',**{s:saved[s]['dec'] for s in ('pc','sp','af','bc','de','hl')})
  return rr
 def command(t,keep=False):
  m.write(0x8c00,t.encode()+b'\0'+bytes(127-len(t)));return call(k['MXOS_DOS_CLI'],hl=0x8c00,keep=keep)
 def file_io(address,writing=False):
  if version=='old':
   return call(k['MXOS_DOS_FILE_SAVE_PATH' if writing else 'MXOS_DOS_FILE_LOAD_PATH'],hl=address)
  callback=lambda op,hl=0,a=0:call(op,hl=hl,aa=a)
  return runpy.run_path(str(R/'scripts/emu80/file-api.py'))['path_io'](m,callback,k,address,writing)
 def save(t,data):
  m.write(0x8c00,('B:/'+t).encode()+b'\0'+bytes(120-len(t)));m.write(0x4000,data)
  m.write(0x8c80,word(0x8c00)+word(0x4000)+word(len(data))+bytes(34));file_io(0x8c80,True)
 def verifycopy(dst,tree):
  expected={dst+'/'+name:data for name,data in payloads.items()} if tree else {dst:filedata}
  for name,data in expected.items():
   m.write(0x8c00,('B:/'+name).encode()+b'\0'+bytes(120-len(name)))
   m.write(0x8c80,word(0x8c00)+word(0x4000)+word(len(data))+bytes(34))
   file_io(0x8c80)
   assert int.from_bytes(m.read(0x8c86,2),'little')==len(data),('length',version,name)
   assert m.read(0x4000,len(data))==data,('content',version,name)
  print('CONTENT PASS',version,dst,flush=True)
 def cleanup(dst,tree,keep=False):
  if tree:
   for name in payloads:command('DEL B:/'+dst+'/'+name,keep)
   for d in range(4):command('RD B:/'+dst+'/D'+str(d),keep)
   command('RD B:/'+dst,keep)
  else:command('DEL B:/'+dst,keep)
 def tap(key):
  z=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
  if not z.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
 def text(s):
  for ch in s:tap({' ':'space',':':'colon','/':'slash','.':'period','-':'minus'}.get(ch,ch.lower()))
 def ready():m.call('bp_clear');m.call('emu_run_for',ms=400)
 def nccommand(t):
  ready();tap('esc');text(t);m.stop_at(n['mainLoop_cursorReady']);tap('enter');m.until(n['mainLoop_cursorReady'],60,page=0);ready()
 lastui=0
 def measure(start,end,enter=True):
  nonlocal lastui
  lastui=0;uiStart=None
  m.stop_at(start)
  if enter:tap('enter')
  z=m.until(start,60,page=0);t0=z['cpu_ticks']
  if a.no_prepare and start==n['copyFileInt']:
   m.stop_at(n['ncFileopsCall']);m.until(n['ncFileopsCall'],60,page=0)
   assert cpu()['registers']['af']['dec']>>8==0
   m.write(n['vars.fileJob.flags'],b'\1') # Public BEGIN flag; all input still uses the NC editor.
  m.stop_at(end)
  aux={}
  if version=='new':
   f=sy(out/'FILEOPS.sym');aux={f['foPrepare']:'prepare_start',f['fpReady']:'prepare_end'}
   for addr in aux:m.call('bp_set',addr=addr,type='exec')
  stamps={}
  pictures=set()
  if version=='new':
   for marker in ('ncFileopsLoop','ncProgressPaint'):m.call('bp_set',addr=n[marker],type='exec')
  for _ in range(600):
   z=cpu();pc=z['registers']['pc']['dec'];bank=m.read(k['bankPage'],1)[0]
   if bank==0 and pc==n.get('ncProgressPaint'):uiStart=z['cpu_ticks']
   if bank==0 and pc==n.get('ncFileopsLoop') and uiStart is not None:
    lastui+=z['cpu_ticks']-uiStart;uiStart=None
   if pc==end and bank==0:break
   if bank==1 and pc in aux:stamps.setdefault(aux[pc],z['cpu_ticks'])
   if bank==0 and pc==n.get('ncFileopsLoop') and m.read(n['ncProgressVisible'],1)==b'\1':
    phase=m.read(n['vars.fileJob.phase'],1)[0]
    if phase not in pictures:
     pictures.add(phase);m.screenshot(out/('phase-'+str(phase)+'.png'))
   m.call('emu_run_for',ms=500)
  else:raise AssertionError(('timeout',version,z))
  ticks=z['cpu_ticks']-t0
  m.call('bp_clear')
  return ticks,stamps.get('prepare_end',0)-stamps.get('prepare_start',0) if 'prepare_end' in stamps else 0
 def record(front,kind,iteration,ticks,prep=0):
  row=dict(version=a.label or version,frontend=front,kind=kind,iteration=iteration,bytes=16384,no_prepare=a.no_prepare,ticks=ticks,ms=ticks/2000,prepare_ms=prep/2000,ui_ms=lastui/2000,kib_s=16384*2000000/ticks/1024)
  rows.append(row);phase_rows.append(row);(out/'results.json').write_text(json.dumps(phase_rows,indent=2)+'\n')
  print(json.dumps(row),flush=True)
 try:
  x.start('copy '+version,system=out/'system.bin',nc_binary=out/'NC.i80',nc_symbols=out/'NC.sym',bios=out/'BIOS.sys',dos=out/'DOS.sys')
  m.call('debug_break');m.call('bp_clear');call(k['MXOS_DOS_CLI'],aa=k['CLI_OP_OBSERVER'],hl=0)
  command('MD B:/TREE')
  for d in range(4):command('MD B:/TREE/D'+str(d))
  save('ONE.BIN',filedata)
  for path,data in payloads.items():save('TREE/'+path,data)
  # Matched-core control: same current disk/filesystem with no UI.
  if version=='new' and 'API' in fronts:
   for kind,src in cases:
    for mode in ['sync','heartbeat','prepare']:
     for iteration in range(a.repeats):
      dst='CONTROL' if kind=='tree' else 'CONTROL.BIN'
      req=bytearray(k['FILE_COPY_REQUEST']);req[4:6]=b'\1\1'
      def fat(name):
       b,_,e=name.partition('.');return (b.ljust(8)+e.ljust(3)).encode()
      req[6:17]=fat(src);req[38:49]=fat(dst)
      if mode=='sync':
       m.write(0x8c80,req);before=cpu()['cpu_ticks'];call(k['MXOS_DOS_CLI'],hl=0x8c80,aa=1);ticks=cpu()['cpu_ticks']-before
      else:
       job=bytearray(k['FILEOPS_JOB']);job[k['FILEOPS_JOB.request']:k['FILEOPS_JOB.request']+len(req)]=req
       job[k['FILEOPS_JOB.flags']]=int(mode=='heartbeat');m.write(0x8c80,job)
       before=cpu()['cpu_ticks'];call(k['MXOS_DOS_FILEOPS'],hl=0x8c80)
       for _ in range(300):
        call(k['MXOS_DOS_FILEOPS'],hl=0x8c80,aa=1);state=m.read(0x8c83,1)[0]
        if state>=2:break
       assert state==2,m.read(0x8c80,k['FILEOPS_JOB'])
       call(k['MXOS_DOS_FILEOPS'],hl=0x8c80,aa=4);ticks=cpu()['cpu_ticks']-before
      record('API-'+mode,kind,iteration,ticks);cleanup(dst,kind=='tree')
  m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120,page=0);ready()
  # Real CLI commands; timing starts after the editor accepts the line.
  m.stop_at(c['cliPrompt']);tap('f10');m.until(c['cliPrompt'],60,page=0)
  for kind,src in (cases if 'CLI' in fronts else []):
   for iteration in range(a.repeats):
    dst='CRESULT' if kind=='tree' else 'CRESULT.BIN'
    ready();text('COPY B:/'+src+' B:/'+dst)
    assert m.read(c['CMD_LINE'],5).upper()==b'COPY '
    ticks,prep=measure(c['cliHistoryAdd'],c['cliPrompt'])
    record('CLI',kind,iteration,ticks,prep)
    if iteration<a.repeats-1:cleanup(dst,kind=='tree',keep=True)
  ready();text('EXIT');m.stop_at(n['mainLoop']);tap('enter');m.until(n['mainLoop'],60,page=0)
  nccommand('B:')
  def selected():
   panel=m.read(n['state.activePanel'],1)[0];cur=m.read(n['state.panelA_curFile']+panel,1)[0]
   ix=m.read(n['FILE_INDEX_A']+panel*256+cur,1)[0];slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
   base=int.from_bytes(m.read(n['ncCatalogBases']+slot*2,2),'little');return m.read(base+ix*16,11)
  for kind,src in (cases if 'NC' in fronts else []):
   for iteration in range(a.repeats):
    nccommand('REM') # process cleanup's dirty notifications before choosing a file
    tap('home')
    fatname=(src.partition('.')[0].ljust(8)+src.partition('.')[2].ljust(3)).encode()
    for _ in range(100):
     if selected().upper()==fatname:break
     tap('down')
    else:raise AssertionError(('select',fatname,selected()))
    tap('f5');original=m.read(n['vars.input'],128).split(b'\0')[0]
    for _ in original:tap('backspace')
    dst='NRESULT' if kind=='tree' else 'NRESULT.BIN';text('B:/'+dst)
    assert m.read(n['vars.input'],len(dst)+3).upper()==('B:/'+dst).encode()
    ticks,prep=measure(n['copyFileInt'],n['mainLoop_cursorReady'])
    record('NC-F5',kind,iteration,ticks,prep)
    if iteration<a.repeats-1:cleanup(dst,kind=='tree',keep=True)
  m.screenshot(out/'nc-final.png')
  # Destructive verification scratch is used only after the final UI sample.
  if 'CLI' in fronts:
   for kind in kinds:verifycopy('CRESULT' if kind=='tree' else 'CRESULT.BIN',kind=='tree')
  if 'NC' in fronts:
   for kind in kinds:verifycopy('NRESULT' if kind=='tree' else 'NRESULT.BIN',kind=='tree')
 finally:x.stop()

for v in (['old','new'] if a.phase=='all' else [a.phase]):run(v)
