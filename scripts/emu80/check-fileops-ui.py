"""Lazy FILEOPS, stepped copying, confirmation/cancel and arena ownership."""
from pathlib import Path
import argparse,re,runpy,struct
R=Path(__file__).resolve().parents[2];O=R/'.local/fileops-ui-check';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text('utf-8'),re.M)}
k=sy(R/'bin/banked/KERNEL.sym');b=sy(R/'bin/BIOS.sym')
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782')
p.add_argument('--port',type=int,choices=range(19781,19797),default=19782)
args=p.parse_args()
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
g=h['Matrix'].__init__.__globals__;g.update(OUT=O,RUNTIME=args.runtime.resolve())
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{args.port}/mcp'
word=lambda n:struct.pack('<H',n)
def page(v):m.write(0xfffd if v else 0xfffc,bytes([v-1 if v else 0]))
def call(op,hl=0,de=0,bc=0,a=0,error=False):
 page(0);m.write(0x3000,struct.pack('<4H',hl,de,bc,a*256+2));m.write(0x2000,b'\xcd\x60\xf8\xc3\x00\x8b')
 m.stop_at(0x8b00);m.call('regs_set',pc=0x2000,sp=0x8fdf,bc=op,de=0x3000)
 z=m.until(0x8b00,90);r={key:v['dec'] for key,v in z['registers'].items()}
 assert r['sp']==0x8fdf,r
 assert bool(r['af']&1)==error,(hex(op),r)
 assert m.read(b['FONT_ADDR'],b['FONT_BYTES'])==font,'font damaged'
 return r
def file_io(address,writing=False):
 return runpy.run_path(str(R/'scripts/emu80/file-api.py'))['path_io'](m,call,k,address,writing)
def save(name,data,drive='B'):
 m.write(0x3400,(drive+':\\'+name).encode()+b'\0'+bytes(110));m.write(0x4000,data)
 m.write(0x3200,word(0x3400)+word(0x4000)+word(len(data))+bytes(34));file_io(0x3200,True)
def job(name='TARGET',drive=1,source=b'SOURCE  BIN',extension='BIN',mode=0):
 page(1);before=m.read(k['v_directories'],16)+m.read(k['v_directory'],2)+m.read(k['v_drive'],1);page(0)
 j=bytearray(k['FILEOPS_JOB']);j[2]=mode;q=k['FILEOPS_JOB.request'];j[q+4]=1;j[q+5]=drive
 j[q+6:q+17]=source;j[q+38:q+49]=(name.ljust(8)+extension).encode()
 m.write(0x3600,j);call(k['MXOS_DOS_FILEOPS'],hl=0x3600)
 page(1);after=m.read(k['v_directories'],16)+m.read(k['v_directory'],2)+m.read(k['v_drive'],1);page(0)
 assert before==after,('loader changed cwd',before.hex(),after.hex())
 return bytes(j)
def step(op=1):
 page(1);context=m.read(k['v_directories'],16)+m.read(k['v_directory'],2)+m.read(k['v_drive'],1);page(0)
 call(k['MXOS_DOS_FILEOPS'],hl=0x3600,a=op)
 result=m.read(0x3600,k['FILEOPS_JOB'])
 page(1);after=m.read(k['v_directories'],16)+m.read(k['v_directory'],2)+m.read(k['v_drive'],1);page(0)
 assert after==context,('STEP changed cwd',op,context.hex(),after.hex())
 return result
def read(name,length,drive='B'):
 m.write(0x3400,(drive+':\\'+name).encode()+b'\0'+bytes(110));m.write(0x3200,word(0x3400)+word(0x4000)+word(length)+bytes(34));file_io(0x3200);return m.read(0x4000,length)

def cli(t,error=False):
 m.write(0x3400,t.encode()+b'\0'+bytes(127-len(t)));return call(k['MXOS_DOS_CLI'],hl=0x3400,error=error)
def begin(src,dst='DST',mode=0,flags=0):
 j=bytearray(k['FILEOPS_JOB']);j[2]=mode;q=k['FILEOPS_JOB.request'];j[q+4:q+6]=b'\1\1'
 def fat(n):
  a,_,e=n.partition('.');return (a.ljust(8)+e.ljust(3)).encode()
 j[q+6:q+17]=fat(src);j[q+38:q+49]=fat(dst)
 j[k['FILEOPS_JOB.flags']]=flags
 j[k['FILEOPS_JOB.path']:k['FILEOPS_JOB.path']+2]=word(0x5800)
 m.write(0x3600,j);call(k['MXOS_DOS_FILEOPS'],hl=0x3600)
 return m.read(0x3600,k['FILEOPS_JOB'])
def finish():
 phases=[]
 for _ in range(300):
  j=step();phases.append(j[k['FILEOPS_JOB.phase']])
  if j[3]==1:m.write(0x3604,b'\2');j=step(2)
  if j[3]>=2:return j,phases
 raise AssertionError('no completion')

def tap(key):
 z=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
 if not z.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
def text(t):
 for ch in t:tap({' ':'space',':':'colon','/':'slash','.':'period','-':'minus'}.get(ch,ch.lower()))
def ready():m.call('bp_clear');m.call('emu_run_for',ms=350)
def command(t):
 ready();tap('esc');text(t);m.stop_at(n['mainLoop_cursorReady']);tap('enter');m.until(n['mainLoop_cursorReady'],90,page=0);ready()
def select(name):
 ready();tap('home');wanted=(name.partition('.')[0].ljust(8)+name.partition('.')[2].ljust(3)).upper().encode()
 for _ in range(100):
  panel=m.read(n['state.activePanel'],1)[0];cur=m.read(n['state.panelA_curFile']+panel,1)[0]
  ix=m.read(n['FILE_INDEX_A']+panel*256+cur,1)[0];slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
  b=int.from_bytes(m.read(n['ncCatalogBases']+slot*2,2),'little')
  if m.read(b+ix*16,11).upper()==wanted:return
  tap('down')
 raise AssertionError(('select',name))
def startcopy(dst):
 select('TREE');m.stop_at(n['input']);tap('f5');m.until(n['input'],90,page=0);ready();original=m.read(n['vars.input'],128).split(b'\0')[0]
 for _ in original:tap('backspace')
 text('B:/'+dst);m.stop_at(n['ncFileopsLoop']);m.call('bp_set',addr=n['copyFileErrorCode'],type='exec');tap('enter')
 for _ in range(180):
  z=m.call('cpu_state');pc=z['registers']['pc']['dec']
  if m.read(k['bankPage'],1)==b'\0':
   if pc==n['copyFileErrorCode']:
    m.screenshot(O/'input-error.png');raise AssertionError(('input rejected',z,m.read(n['vars.input'],128)))
   if pc==n['ncFileopsLoop']:return
  m.call('emu_run_for',ms=500)
 raise AssertionError(('copy did not start',z))
def progress(phase):
 m.stop_at(n['ncFileopsLoop'])
 for _ in range(300):
  m.call('emu_run_for',ms=500);m.until(n['ncFileopsLoop'],60,page=0)
  if m.read(n['vars.fileJob.phase'],1)==bytes([phase]) and m.read(n['ncProgressVisible'],1)==b'\1':
   assert m.read(n['ncDialogActive'],1)==b'\1','progress fell back to full redraw'
   return
 raise AssertionError(('phase',phase))
def pressop(key,op):
 m.call('kbd_send',keys=[key],action='press')
 try:
  m.stop_at(n['ncFileopsCall'])
  for _ in range(30):
   m.call('emu_run_for',ms=250);z=m.until(n['ncFileopsCall'],30,page=0)
   if z['registers']['af']['dec']>>8==op:return
  raise AssertionError(('key operation',key,op))
 finally:m.call('kbd_send',keys=[key],action='release')
def returned():
 m.stop_at(n['mainLoop_cursorReady']);m.until(n['mainLoop_cursorReady'],90,page=0)
 assert m.read(n['ncDialogActive'],1)==b'\0'
 assert m.read(n['vars.fileJob.handle'],2)==b'\0\0'
 ready()
try:
 fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
 k=sy(R/'bin/banked/KERNEL.sym');b=sy(R/'bin/BIOS.sym');n=sy(fixture['nc_symbols'])
 x.start('NC-PREPARE',**fixture);m.call('debug_break');m.call('bp_clear');font=m.read(b['FONT_ADDR'],b['FONT_BYTES'])
 h['main_bios'](m,k['mx_printChar'],bc=31)
 call(k['MXOS_DOS_CLI'],a=k['CLI_OP_OBSERVER'],hl=0)
 cli('MKDIR B:/TREE/SUB')
 for i in range(20):save('TREE/SUB/F'+str(i)+'.BIN',bytes([i])*1024)
 m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120,page=0);ready();command('B:')
 startcopy('SKIPNC');progress(1);m.screenshot(O/'prepare.png')
 pressop('esc',5);progress(0)
 assert m.read(n['vars.fileJob.known'],1)==b'\0'
 m.screenshot(O/'copy-unknown.png');returned()
 assert m.read(n['vars.fileJob.error'],1)==b'\0'
 print('PASS real NC F5, preparation Cancel/Escape, unknown total, fixed bitmap modal',flush=True)
 startcopy('ABORTNC');progress(1);m.screenshot(O/'prepare-abort.png')
 pressop('tab',1) # tab changes the selection and then performs an ordinary STEP
 assert m.read(n['ncProgressChoice'],1)==b'\1'
 pressop('enter',3);returned()
 assert m.read(n['vars.fileJob.error'],1)==b'\x0c'
 print('PASS preparation Abort selected with Tab/Enter',flush=True)
 startcopy('FULLNC');progress(0)
 assert m.read(n['vars.fileJob.known'],1)==b'\1'
 assert int.from_bytes(m.read(n['vars.fileJob.total'],4),'little')==20*1024
 m.screenshot(O/'copy-known.png')
 previous=bytes(25*9)
 for sample in range(3):
  # Stop only after a completed paint; bar pixels must never be erased.
  before=m.read(n['ncProgressPercent'],1)
  for _ in range(100):
   m.call('emu_run_for',ms=500);m.until(n['ncFileopsLoop'],90,page=0)
   if m.read(n['ncProgressPercent'],1)!=before:break
  modal=int.from_bytes(m.read(n['uiModalCurrent'],2),'little')
  bar_y=m.read(modal+1,1)[0]+11
  # The final columns contain the current modal's percentage field.
  bar=b''.join(m.read(0x9000+(10+col)*256+bar_y,9) for col in range(25))
  width=m.read(n['ncProgressBarWidth'],1)[0]
  assert width%8==0 and width<=224
  assert bar==bytes([255])*(min(width,200)//8*9)+bytes(25*9-min(width,200)//8*9),(width,bar.hex())
  assert all((old&new)==old for old,new in zip(previous,bar)),'bar pixels were cleared'
  previous=bar
  m.screenshot(O/('copy-step-'+str(sample)+'.png'))
 returned()
 print('PASS known total and progress percentage',flush=True)
 select('SKIPNC');m.stop_at(n['ncDeleteKey']);tap('f8');m.until(n['ncDeleteKey'],90,page=0);ready();m.screenshot(O/'delete-confirm.png');m.stop_at(n['ncFileopsLoop']);tap('enter');m.until(n['ncFileopsLoop'],90,page=0)
 progress(2);m.screenshot(O/'delete-progress.png');returned()
 assert m.read(n['vars.fileJob.error'],1)==b'\0'
 print('PASS real NC F8 recursive directory deletion and bitmap restore',flush=True)
 m.screenshot(O/'return.png')
 print('NC FILEOPS UI PASS',flush=True)
finally:x.stop()
