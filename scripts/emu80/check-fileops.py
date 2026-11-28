"""Lazy FILEOPS, stepped copying, confirmation/cancel and arena ownership."""
from pathlib import Path
import argparse,re,runpy,struct
R=Path(__file__).resolve().parents[2];O=R/'.local/fileops-check';O.mkdir(parents=True,exist_ok=True)
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
try:
 fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
 k=sy(R/'bin/banked/KERNEL.sym');b=sy(R/'bin/BIOS.sym')
 n=sy(fixture['nc_symbols']);x.start('FILEOPS',**fixture);m.call('debug_break');m.call('bp_clear');font=m.read(b['FONT_ADDR'],b['FONT_BYTES'])
 h['main_bios'](m,k['mx_printChar'],bc=31)
 call(k['MXOS_DOS_CLI'],a=k['CLI_OP_OBSERVER'],hl=0)
 h['main_bios'](m,k['mx_printChar'],bc=31)
 m.write(0x2500,b'\x3a\x00\x39\x3c\x32\x00\x39\xc9');m.write(0x3900,b'\0')
 call(k['MXOS_DOS_CLI'],a=k['CLI_OP_OBSERVER'],hl=0x2500)
 data=bytes(range(256))*35+bytes(range(77));save('SOURCE.BIN',data)
 # Different per-drive CWDs exercise both the loader and every STEP return.
 for command_line in ('MD B:/CONTEXT','CD B:/CONTEXT','CD C:/DRIVERS','H:'):
  m.write(0x3400,command_line.encode()+b'\0'+bytes(100));call(k['MXOS_DOS_CLI'],hl=0x3400)
 m.write(0x3900,b'\0')
 job();states=[]
 for i in range(100):
  j=step();states.append((j[3],int.from_bytes(j[6:10],'little')))
  if j[3]>=2:break
 assert j[3]==2,(j,states)
 assert int.from_bytes(j[6:10],'little')==len(data),states
 step(4);assert read('TARGET.BIN',len(data))==data
 assert m.read(0x3900,1)!=b'\0','cross-bank filesystem observer was not called'
 print('PASS stepped copy',states,flush=True)
 job();j=step()
 while j[k['FILEOPS_JOB.phase']]==k['FILEOPS_PHASE_PREPARE'] and j[3]==0:j=step()
 assert j[3]==1,j
 j=step(3);assert j[3]==3 and j[5]==12,j
 step(4)
 print('PASS conflict cancellation',flush=True)
 job('CANCEL');j=step();j=step();j=step(3)
 assert j[3]==3 and j[5]==12,j
 step(4)
 page(1);assert m.read(k['fileopsActive'],1)==b'\0';assert m.read(k['fileopsReady'],1)==b'\1';page(0)
 print('PASS mid-file cancellation and idle cache',flush=True)
 assert read('SOURCE.BIN',len(data))==data
 # Recursive asynchronous operation: DFS state survives unrelated DOS calls
 # and overwrite confirmation resumes in the destination's volume context.
 def cli(value):
  print('PREPARE',value,flush=True)
  m.write(0x3400,value.encode()+b'\0'+bytes(100));call(k['MXOS_DOS_CLI'],hl=0x3400)
 for value in ('MD B:/TROOT','MD B:/TROOT/A','COPY B:/SOURCE.BIN B:/TROOT/A/ONE.BIN','COPY C:/README.TXT B:/TROOT/TWO.TXT','H:'):cli(value)
 for policy in (None,2,3):
  job('TDEST',source=b'TROOT      ',extension='   ');confirmations=0
  for _ in range(250):
   j=step()
   if j[3]==1:
    confirmations+=1;assert policy is not None
    m.write(0x3604,bytes([policy]));j=step(2)
   if j[3]>=2:break
  assert j[3]==2,(policy,j)
  assert confirmations==(0 if policy is None else 1),(policy,confirmations)
  # Processed-file count includes Skip/No to all, as does progress.
  assert int.from_bytes(j[10:12],'little')==2,(policy,j)
  step(4)
  assert read('TDEST/A/ONE.BIN',len(data))==data
 print('PASS stepped recursive copy, Yes/No to all, complete per-drive CWD preservation',flush=True)
 # Exact module ranges: disjoint DLLs coexist, and overlapping loads cannot
 # overwrite a pinned FILEOPS. Idle eviction is selective.
 e=runpy.run_path(str(R/'scripts/images/executable.py'))
 page(1)
 origin=int.from_bytes(m.read(k['fileopsOverlay'],2),'little')
 page(0)
 from dos_arena import free_intervals
 def dos_read(address,size):
  page(1);data=m.read(address,size);page(0);return data
 available=(address for start,limit in free_intervals(k,dos_read) for address in range(start,limit))
 for name,origin in [('UPPER.DLL',next(available)),('OTHER.DLL',next(available)),('OVER.DLL',origin)]:
  save(name,e['encode'](e['Image'](b'\xc9',origin,origin,1),'com'))
 def load(name,error=False):
  m.write(0x3400,('B:\\'+name).encode()+b'\0'+bytes(110))
  return call(k['MXOS_DOS_MODULE_LOAD'],hl=0x3400,a=2,error=error)['hl']
 def unload(handle):call(k['MXOS_DOS_MODULE_UNLOAD'],hl=handle)
 first,second=load('UPPER.DLL'),load('OTHER.DLL')
 load('UPPER.DLL',True)
 unload(first);unload(second)
 job('PINNED');load('OVER.DLL',True);j=step(4)
 load_handle=load('OVER.DLL')
 page(1);assert m.read(k['fileopsReady'],1)==b'\0';page(0)
 unload(load_handle)
 job('RELOAD');step(4)
 page(1);assert m.read(k['fileopsReady'],1)==b'\1';origin=int.from_bytes(m.read(k['fileopsOverlay'],2),'little');limit=int.from_bytes(m.read(k['fileopsLoadedEnd'],2),'little');cached=m.read(origin,limit-origin);page(0)
 print('PASS two disjoint DLLs, pinned overlap refusal, idle eviction, lazy FILEOPS reload',flush=True)
 m.write(0x3400,b'C:/DRIVERS/RAMDISK.COM\0'+bytes(100))
 call(k['MXOS_DOS_FILE_EXEC_NATIVE'],hl=0x3400)
 page(1)
 if m.read(k['fileopsReady'],1)==b'\1':assert m.read(origin,len(cached))==cached
 page(0)
 # Both flash variants and ROM install from different measured origins.
 for installer in ('AT2MXF.COM','ROM2MX.COM','AT2MX.COM'):
  m.write(0x3400,('C:/DRIVERS/'+installer).encode()+b'\0'+bytes(100))
  call(k['MXOS_DOS_FILE_EXEC_NATIVE'],hl=0x3400)
  page(1)
  if m.read(k['fileopsReady'],1)==b'\1':assert m.read(origin,len(cached))==cached
  page(0)
  if installer=='AT2MXF.COM':
   # Корень Games2M заполнен. Подкаталог допускает расширение.
   probe_file='BLDRDASH/FOCHECK.BIN'
   probe=bytes((i*73+19)&255 for i in range(513));save(probe_file,probe,'H');assert read(probe_file,len(probe),'H')==probe
  else:assert read(probe_file,len(probe),'H')==probe
 cli('DEL H:/'+probe_file)
 print('PASS relocated FLASH/FST/ROM installers, flash write/read, selective FILEOPS cache eviction',flush=True)
 # Real NC editor/keyboard exercises the synchronous CLI frontend as well.
 def tap(key):
  z=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
  if not z.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
 def text(value):
  for c in value:
   if c=='_':
    m.call('kbd_send',keys=['shift'],action='press')
    try:tap('0')
    finally:m.call('kbd_send',keys=['shift'],action='release')
   else:tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(c,c.lower()))
 # AUTOEXEC запускает банковый NC. После настоящего Reset выбираем ту же MAIN-оболочку.
 poll=sy(R/'bin/NC_CORE.sym')['ncBankInputPoll'];cli_sym=sy(R/'bin/CLI.sym')
 m.stop_at(poll);m.call('emu_reset')
 for _ in range(160):
  m.until(poll,120)
  if m.read(k['bankPage'],1)[0]>1:break
  m.call('debug_step',mode='into')
 else:raise AssertionError('Resident NC did not reach the keyboard after Reset')
 x.console()
 for value,target in [('FONT 6X8/ROTATE',cli_sym['cliInput']),('C:/DOS/NC_MN.COM',n['mainLoop'])]:
  m.call('bp_clear');m.call('emu_run_for',ms=500);tap('esc');text(value)
  assert m.read(cli_sym['CMD_LINE'],len(value)).upper()==value.encode()
  m.stop_at(target);tap('enter');m.until(target,120,page=0)
 page(1);assert m.read(k['fileopsPublic'],1)==b'\0';page(0)
 def command(value):
  m.call('bp_clear');m.call('emu_run_for',ms=500);tap('esc');text(value)
  assert m.read(n['vars.cmdLine'],len(value)).upper()==value.upper().encode()
  m.screenshot(O/'nc-command.png');m.stop_at(n['mainLoop_cursorReady']);tap('enter');m.until(n['mainLoop_cursorReady'],30,page=0)
  m.call('bp_clear');m.call('emu_run_for',ms=250)
  print('COMMAND',value,flush=True)
 command('MD B:/TREE')
 command('COPY C:/README.TXT B:/TREE/ONE.TXT')
 command('COPY B:/TREE B:/TREE2')
 command('MOVE B:/TREE2 B:/TREE3')
 print('PASS NC command editor, recursive COPY and metadata MOVE',flush=True)
 # Выбрать результат рекурсивного MOVE на панели и скопировать его настоящей F5.
 def selected():
  panel=m.read(n['state.activePanel'],1)[0]
  cur=m.read(n['state.panelA_curFile']+panel,1)[0]
  ix=m.read(n['FILE_INDEX_A']+panel*256+cur,1)[0]
  slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
  base=int.from_bytes(m.read(n['ncCatalogBases']+slot*2,2),'little')
  return m.read(base+16*ix,11)
 if m.read(n['state.activePanel'],1)!=b'\0':tap('tab')
 command('B:');command('CD B:/TREE3');tap('home')
 for _ in range(100):
  if selected().upper()==b'ONE     TXT':break
  tap('down')
 else:raise AssertionError(('cannot select moved file ONE.TXT',selected()))
 m.stop_at(n['input']);tap('f5');m.until(n['input'],90,page=0)
 m.call('bp_clear');m.call('emu_run_for',ms=250)
 original=m.read(n['vars.input'],128).split(b'\0')[0]
 for _ in original:tap('backspace')
 text('B:/F5.TXT')
 assert m.read(n['vars.input'],9).upper()==b'B:/F5.TXT'
 m.screenshot(O/'nc-f5-input.png')
 m.stop_at(n['mainLoop_cursorReady']);tap('enter');m.until(n['mainLoop_cursorReady'],90,page=0)
 m.call('bp_clear');m.call('emu_run_for',ms=250)
 get_file=runpy.run_path(str(R/'scripts/emu80/check-copy.py'))['get_file']
 expected=get_file(fixture['system'].read_bytes(),'README.TXT')[1]
 assert int.from_bytes(m.read(n['vars.fileJob.bytes'],4),'little')==len(expected)
 assert m.read(n['vars.fileJob.error'],1)==b'\0'
 m.screenshot(O/'nc-f5-return.png')
 print('PASS F5 stepped frontend and return to NC',flush=True)
 m.screenshot(O/'nc-after-reset.png')
 print('PASS Reset and NC',flush=True)
 assert read('F5.TXT',len(expected))==expected
except Exception:
 if not x.pid:raise
 m.call('debug_break');m.screenshot(O/'failure.png');page(1)
 for name in ('cliRequest','v_dosError','v_drive','v_directory'):
  print('DIAG',name,m.read(k[name],86 if name=='cliRequest' else 2).hex(),flush=True)
 page(0)
 raise
finally:x.stop()
