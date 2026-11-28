"""Owned batch enumeration, masks, recursive traversal, shared COPY and BAT."""
from pathlib import Path
import argparse,re,runpy,struct
R=Path(__file__).resolve().parents[2];O=R/'.local/enumerate-check';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text('utf-8'),re.M)}
k=sy(R/'bin/banked/KERNEL.sym');b=sy(R/'bin/BIOS.sym')
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782')
p.add_argument('--port',type=int,choices=range(19781,19797),default=19782)
p.add_argument('--ui-only',action='store_true')
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
 return runpy.run_path(str(R/'scripts/emu80/file-api.py'))['path_io'](m,call,k,address,writing,handle=0x2e00)
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

def cli(value,error=False):
 m.write(0x3400,value.encode()+b'\0'+bytes(127-len(value)));return call(k['MXOS_DOS_CLI'],hl=0x3400,error=error)
def request(op,handle=0,pattern='B:/*',flags=0,capacity=3,error=False):
 if op==0:
  m.write(0x3400,pattern.encode()+b'\0'+bytes(127-len(pattern)))
  m.write(0x3600,struct.pack('<5H2B',0,0x3400,0x5000,capacity,0,flags,0))
 elif handle:
  m.write(0x3600,word(handle))
 page(1);before=m.read(k['v_directories'],16)+m.read(k['v_directory'],2)+m.read(k['v_drive'],1);page(0)
 call(k['MXOS_DOS_ENUMERATE'],hl=0x3600,a=op,error=error)
 page(1);after=m.read(k['v_directories'],16)+m.read(k['v_directory'],2)+m.read(k['v_drive'],1);page(0)
 assert before==after,('CWD changed',op,before,after)
 return struct.unpack('<5H2B',m.read(0x3600,12))
def listing(pattern,flags=0,interleave=False):
 q=request(0,pattern=pattern,flags=flags);handle=q[0];out=[]
 for _ in range(400):
  q=request(1,handle=handle)
  for i in range(q[4]):
   record=m.read(0x5000+160*i,160)
   out.append(record[32:].split(b'\0')[0].decode('ascii'))
  if q[-1]:break
  if interleave:
   cli('DIR C:/DRIVERS/*.COM')
 else:raise AssertionError('cursor loop')
 request(2,handle=handle)
 return out
try:
 fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
 k=sy(R/'bin/banked/KERNEL.sym');b=sy(R/'bin/BIOS.sym');n=sy(fixture['nc_symbols'])
 x.start('ENUMERATE',**fixture);m.call('debug_break');m.call('bp_clear');font=m.read(b['FONT_ADDR'],b['FONT_BYTES'])
 h['main_bios'](m,k['mx_printChar'],bc=31)
 call(k['MXOS_DOS_CLI'],a=k['CLI_OP_OBSERVER'],hl=0)
 h['main_bios'](m,k['mx_printChar'],bc=31)
 if not args.ui_only:
  for value in ['MD B:/ROOT','MD B:/ROOT/SUB','MD B:/ROOT/OTHER','MD B:/ROOT/SUB/DEEP']:cli(value)
  for name in ['ROOT/ALPHA.TXT','ROOT/BETA.BAT','ROOT/NOEXT','ROOT/SUB/TWO.TXT','ROOT/SUB/DEEP/THREE.TXT','ROOT/OTHER/X.BAT']:save(name,b'hello\n')
  cli('CD B:/ROOT');cli('H:')
  print('LIST',listing('B:/ROOT/*.TXT'),flush=True)
  assert listing('B:/ROOT/*.TXT')==['B:\\ROOT\\ALPHA.TXT']
  assert sorted(listing('B:/ROOT/*.txt',1))==sorted(['B:\\ROOT\\ALPHA.TXT','B:\\ROOT\\SUB\\TWO.TXT','B:\\ROOT\\SUB\\DEEP\\THREE.TXT'])
  assert listing('B:/ROOT/????.BAT')==['B:\\ROOT\\BETA.BAT']
  all_names=listing('B:/ROOT/*.*');assert len(all_names)==5 and any(x.endswith('NOEXT') for x in all_names),all_names
  assert len(listing('B:/ROOT'))==5
  assert len(listing('B:'))==5
  assert listing('B:/ROOT/Z*.TXT')==[]
  print('PASS batches, wildcards, case, *.*, empty result, saved drive CWD and recursive DFS',flush=True)
  # Pin protection, ownership/handle checks and filesystem calls between batches.
  q=request(0,pattern='B:/ROOT/*',flags=1);handle=q[0]
  request(1,handle=handle+1,error=True)
  cli('DIR C:/DRIVERS/*.COM',error=True) # one active cursor, no state corruption
  request(1,handle=handle)
  request(2,handle=handle)
  request(1,handle=handle,error=True)
  request(0,pattern='B:/MISSING/*.TXT',error=True)
  assert listing('B:/ROOT/*.TXT')==['B:\\ROOT\\ALPHA.TXT']
  q=request(0,pattern='B:/ROOT/*.TXT');handle=q[0]
  m.write(0x3606,word(0));request(1,handle=handle,error=True)
  m.write(0x3606,word(3));m.write(0x3604,word(0x3600));request(1,handle=handle,error=True)
  m.write(0x3604,word(0xFFF0));request(1,handle=handle,error=True)
  m.write(0x3604,word(0x5000));q=request(1,handle=handle);assert q[4]==1
  request(2,handle=handle)
  # A cached enumerator may be replaced only after the active cursor closes.
  e=runpy.run_path(str(R/'scripts/images/executable.py'))
  page(1);origin=int.from_bytes(m.read(k['enumOverlay'],2),'little');page(0)
  save('ENUMOVER.DLL',e['encode'](e['Image'](b'\xc9',origin,origin,1),'com'))
  q=request(0,pattern='B:/ROOT/*');handle=q[0]
  m.write(0x3400,b'B:/ENUMOVER.DLL\0'+bytes(110))
  call(k['MXOS_DOS_MODULE_LOAD'],hl=0x3400,a=2,error=True)
  request(2,handle=handle)
  m.write(0x3400,b'B:/ENUMOVER.DLL\0'+bytes(110))
  loaded=call(k['MXOS_DOS_MODULE_LOAD'],hl=0x3400,a=2)['hl']
  page(1);assert m.read(k['enumReady'],1)==b'\0';page(0)
  call(k['MXOS_DOS_MODULE_UNLOAD'],hl=loaded)
  assert listing('B:/ROOT/*.TXT')==['B:\\ROOT\\ALPHA.TXT']
  print('PASS invalid batch buffers, pinned interval, idle eviction and lazy reload',flush=True)
  print('PASS active cursor, stale handle, failed OPEN cleanup',flush=True)
  cli('DIR B:/ROOT/*.TXT');cli('DIR B:/ROOT/*.TXT -S');cli('DIR');cli('DIR B:')
  cli('COPY B:/ROOT B:/COPIED')
  assert sorted(x.replace('COPIED','ROOT') for x in listing('B:/COPIED/*',1))==sorted(listing('B:/ROOT/*',1))
  page(1);assert m.read(k['fileopsReady'],1)==b'\1' and m.read(k['enumReady'],1)==b'\1';page(0)
  # A BAT crosses the 512-byte refill boundary and invokes a nested BAT.
  save('CHILD.BAT',b'MD B:/FROMBAT\rCOPY B:/ROOT/ALPHA.TXT B:/FROMBAT/ONE.TXT\r')
  save('PARENT.BAT',b'ECHO OFF\r'+b'REM buffer refill exercise\r'*35+b'B:/CHILD.BAT\rCOPY B:/ROOT/BETA.BAT B:/FROMBAT/TWO.BAT\rDIR B:/FROMBAT/*.TXT\r')
  m.write(0x3400,b'B:/PARENT.BAT\0'+bytes(110));call(k['MXOS_DOS_FILE_EXEC'],hl=0x3400)
  assert listing('B:/FROMBAT/*')==['B:\\FROMBAT\\ONE.TXT','B:\\FROMBAT\\TWO.BAT']
  cli('MD B:/MASKCOPY');cli('COPY B:/ROOT/*.TXT B:/MASKCOPY')
  assert listing('B:/MASKCOPY/*')==['B:\\MASKCOPY\\ALPHA.TXT']
  cli('MOVE B:/MASKCOPY/ALPH?.TXT B:/FROMBAT')
  assert listing('B:/MASKCOPY/*')==[]
  assert any(p.endswith('ALPHA.TXT') for p in listing('B:/FROMBAT/*'))
  cli('MD B:/EMPTY');cli('DEL B:/EMPTY');request(0,pattern='B:/EMPTY/*',error=True)
  cli('ERASE B:/FROMBAT/ONE.TXT')
  assert not any(p.endswith('ONE.TXT') for p in listing('B:/FROMBAT/*'))
  cli('MKDIR B:/Games/Almaz-2/MXOS');cli('MKDIR B:/Games/Almaz-2/MXOS')
  assert listing('B:/Games/Almaz-2/*')==['B:\\Games\\Almaz-2\\MXOS']
  cli('MKDIR B:/FROMBAT/TWO.BAT/CHILD',error=True)
  assert read('FROMBAT/TWO.BAT',6)==b'hello\n'
  cli('DEL B:/FROMBAT');request(0,pattern='B:/FROMBAT/*',error=True)
  print('PASS recursive MKDIR, existing parents/leaf and file-in-path refusal',flush=True)
  print('PASS DEL/ERASE files and complete directory trees',flush=True)
  print('PASS shared wildcard COPY/MOVE and simultaneous ENUM/FILEOPS cache',flush=True)
  print('PASS FILEOPS/ENUM coexistence, DIR masks and nested streaming BAT',flush=True)
 # Real keyboard tests: NC F7 and standalone CLI share the resident parser.
 def tap(key):
  z=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
  if not z.get('stoppedByBreakpoint'):m.call('emu_run_for',ms=120)
 def text(value):
  for c in value:
   if c=='_':
    m.call('kbd_send',keys=['shift'],action='press')
    try:tap('0')
    finally:m.call('kbd_send',keys=['shift'],action='release')
   else:tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus','*':'asterisk'}.get(c,c.lower()))
 # AUTOEXEC загружает банковый NC. Панели ниже проверяются по символам NC_MN.
 core=sy(R/'bin/NC_CORE.sym');poll=core['ncBankInputPoll']
 m.stop_at(poll);m.call('emu_reset')
 for _ in range(160):
  m.until(poll,120)
  if m.read(k['bankPage'],1)[0]>1:break
  m.call('debug_step',mode='into')
 else:raise RuntimeError('После Reset банковый NC не достиг опроса клавиатуры')
 m.call('bp_clear');m.call('emu_run_for',ms=500);tap('esc');text('C:/DOS/NC_MN.COM')
 assert m.read(core['vars.cmdLine'],16).upper()==b'C:/DOS/NC_MN.COM'
 m.stop_at(n['mainLoop']);tap('enter');m.until(n['mainLoop'],120,page=0)
 m.screenshot(O/'nc-after-reset.png')
 m.call('bp_clear');m.call('emu_run_for',ms=500)
 # A quiet write to B: must neither reread nor redraw the A:/H: panels.
 for quiet in ['MKDIR B:/QUIET/DEEP','MKDIR B:/QUIET/DEEP']:
  tap('esc');text(quiet)
  before=b''.join(m.read(0x9000+col*256,230) for col in range(48))
  m.stop_at(n['mainLoop_cursorReady'])
  m.call('bp_set',addr=n['loadAndPrintPanel'],type='exec')
  m.call('bp_set',addr=n['lf_ReadCatalog'],type='exec')
  tap('enter');draws=[]
  for _ in range(100):
   st=m.call('cpu_state');pc=st['registers']['pc']['dec']
   bank=m.read(k['bankPage'],1)[0]
   if pc==n['mainLoop_cursorReady'] and bank==0:break
   if bank==0 and pc in (n['loadAndPrintPanel'],n['lf_ReadCatalog']):draws.append((pc,st['registers']['bc']['dec']&255))
   m.call('emu_run_for',ms=500)
  else:raise AssertionError(('quiet timeout',st))
  assert not draws,('quiet command redraws unrelated panels',quiet,draws)
  assert before==b''.join(m.read(0x9000+col*256,230) for col in range(48)),('quiet bitmap changed',quiet)
  m.call('bp_clear');m.call('emu_run_for',ms=250)
 print('PASS quiet MKDIR does not redraw A:/H: or reread their catalogs',flush=True)
 m.stop_at(n['lineEditLoop']);tap('f7');m.until(n['lineEditLoop'],30,page=0)
 assert m.read(n['lineEditBuffer'],2)==word(n['vars.input'])
 m.call('bp_clear');m.call('emu_run_for',ms=200);text('B:/UITREE/Almaz-2/MXOS')
 assert m.read(n['vars.input'],22).upper()==b'B:/UITREE/ALMAZ-2/MXOS'
 m.screenshot(O/'nc-mkdir-tree-input.png')
 m.stop_at(n['mainLoop_cursorReady']);tap('enter');m.until(n['mainLoop_cursorReady'],60,page=0)
 m.call('bp_clear');m.call('emu_run_for',ms=250)
 cs=sy(R/'bin/CLI.sym');m.stop_at(cs['cliPrompt']);tap('f10');m.until(cs['cliPrompt'],60,page=0)
 m.call('bp_clear');m.call('emu_run_for',ms=500)
 text('CD B:/UITREE/Almaz-2/MXOS')
 m.stop_at(cs['cliPrompt']);tap('enter');m.until(cs['cliPrompt'],30,page=0)
 m.call('bp_clear');m.call('emu_run_for',ms=500)
 assert m.read(cs['cliPromptPath'],128).split(b'\0')[0].upper()==b'B:\\UITREE\\ALMAZ-2\\MXOS'
 text('DIR -S')
 assert m.read(cs['CMD_LINE'],6).upper()==b'DIR -S'
 m.stop_at(cs['cliPrompt']);tap('enter');m.until(cs['cliPrompt'],30,page=0)
 m.call('bp_clear');m.call('emu_run_for',ms=500)
 text('EXIT');m.stop_at(n['mainLoop']);tap('enter');m.until(n['mainLoop'],60,page=0)
 m.screenshot(O/'nc-after-cli.png')
 print('PASS real F7 recursive MKDIR, F10 CLI, DIR -S and EXIT return',flush=True)
 print('PASS Reset and NC',flush=True)
except Exception:
 if x.pid:
  m.call('debug_break');m.screenshot(O/'failure.png')
  page(1);es=sy(R/'bin/ENUM.sym')
  if m.read(k['enumReady'],1)==b'\1':
   base=int.from_bytes(m.read(k['enumOverlay'],2),'little')
   for v in ['enMask','enPath','enFrames','enResult']:
    print(v,m.read(base+es[v],48).hex(),flush=True)
  page(0)
 raise
finally:x.stop()
