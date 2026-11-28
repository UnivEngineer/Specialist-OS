"""Lazy FILEOPS, stepped copying, confirmation/cancel and arena ownership."""
from pathlib import Path
import argparse,re,runpy,struct
R=Path(__file__).resolve().parents[2];O=R/'.local/fileops-prepare-check';O.mkdir(parents=True,exist_ok=True)
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
try:
 fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
 k=sy(R/'bin/banked/KERNEL.sym');b=sy(R/'bin/BIOS.sym');n=sy(fixture['nc_symbols'])
 x.start('PREPARE',**fixture);m.call('debug_break');m.call('bp_clear');font=m.read(b['FONT_ADDR'],b['FONT_BYTES'])
 h['main_bios'](m,k['mx_printChar'],bc=31)
 call(k['MXOS_DOS_CLI'],a=k['CLI_OP_OBSERVER'],hl=0)
 data=bytes(range(256))*35+bytes(range(77))
 cli('MKDIR B:/SRC/SUB/DEEP');save('SRC/A.BIN',data);save('SRC/SUB/X.TXT',b'hello\n')
 begin('SRC');j,phases=finish();assert j[3]==2,j
 assert int.from_bytes(j[k['FILEOPS_JOB.total']:k['FILEOPS_JOB.total']+4],'little')==len(data)+6,j
 assert j[k['FILEOPS_JOB.known']]==1
 assert int.from_bytes(j[k['FILEOPS_JOB.totalFiles']:k['FILEOPS_JOB.totalFiles']+2],'little')==2,j
 assert int.from_bytes(j[k['FILEOPS_JOB.required']:k['FILEOPS_JOB.required']+2],'little')==40,j
 assert phases[0]==1 and 0 in phases,phases
 assert read('DST/A.BIN',len(data))==data
 step(4)
 print('PASS preparation: full byte total, new directories and exact 40-cluster estimate',flush=True)
 begin('SRC');j,phases=finish();assert j[3]==2 and int.from_bytes(j[k['FILEOPS_JOB.required']:k['FILEOPS_JOB.required']+2],'little')==0,j
 step(4)
 print('PASS existing destination directory/file reuse needs zero extra clusters',flush=True)

 begin('SRC','SKIPPED');j=step();assert j[k['FILEOPS_JOB.phase']]==1
 step(5);j,phases=finish();assert j[3]==2 and j[k['FILEOPS_JOB.known']]==0,j
 step(4);assert read('SKIPPED/A.BIN',len(data))==data
 print('PASS skip preparation continues copying with unknown total',flush=True)
 begin('SRC','ABORTED');step();j=step(3);assert j[3]==3 and j[5]==12,j
 step(4);cli('CD B:/ABORTED',error=True)
 page(1);assert m.read(k['enumActive'],1)==b'\0';page(0)
 print('PASS Abort during preparation creates nothing and releases ENUM',flush=True)
 begin('DST',mode=2);j,phases=finish();assert j[3]==2 and j[k['FILEOPS_JOB.phase']]==2,j
 step(4);cli('CD B:/DST',error=True)
 assert read('SRC/A.BIN',len(data))==data
 cli('DEL B:/SKIPPED');cli('CD B:/SKIPPED',error=True)
 cli('CD B:/SRC/SUB');cli('DEL B:/SRC',error=True);cli('B:/');cli('H:')
 print('PASS recursive API/CLI deletion; current directory and ancestors are protected',flush=True)
 # Make a readonly child and verify refusal without corruption.
 cli('ATTRIB +R B:/SRC/A.BIN');cli('DEL B:/SRC',error=True)
 assert read('SRC/A.BIN',len(data))==data
 cli('ATTRIB -R B:/SRC/A.BIN');cli('ERASE B:/SRC');cli('CD B:/SRC',error=True)
 print('PASS readonly failure, ERASE tree and intact source data',flush=True)
 # Fill only this disposable B: and verify preflight refuses before creating target.
 cli('MKDIR B:/BIG');save('BIG/A.BIN',data);cli('MKDIR B:/FILL')
 filled=0
 for i in range(40):
  try:save('FILL/F%02d.BIN'%i,bytes(8192))
  except AssertionError:
   page(1);err=m.read(k['v_dosError'],1)[0];page(0);assert err==k['DOS_ERR_FULL'],err
   break
  filled+=1
 else:raise AssertionError('B: did not fill')
 begin('BIG','NOFIT');j,phases=finish();assert j[3]==3 and j[5]==k['DOS_ERR_FULL'],j
 step(4);cli('CD B:/NOFIT',error=True)
 cli('DEL B:/FILL');cli('DEL B:/BIG')
 print('PASS full-volume preflight refusal, no target creation and recursive cleanup',filled,flush=True)
 # Empty DIR is diagnostic output, not a BAT failure. An invalid parent is
 # a real error and must prevent the following command from running.
 call(k['MXOS_DOS_SET_SHELL'],a=1)
 cli('MKDIR B:/EMPTYDIR')
 save('DIRZERO.BAT',b'@DIR B:/EMPTYDIR\r@MKDIR B:/AFTERDIR\r')
 m.write(0x3400,b'B:/DIRZERO.BAT\0')
 call(k['MXOS_DOS_FILE_EXEC_NATIVE'],hl=0x3400)
 cli('CD B:/AFTERDIR');cli('CD B:/')
 save('DIRBAD.BAT',b'@DIR B:/ABSENT/\r@MKDIR B:/NOTRUN\r')
 m.write(0x3400,b'B:/DIRBAD.BAT\0')
 result=call(k['MXOS_DOS_FILE_EXEC_NATIVE'],hl=0x3400,error=True)
 assert result['af']>>8==k['DOS_ERR_NOT_FOUND'],result
 cli('CD B:/NOTRUN',error=True)
 print('DIR/BAT PASS: empty listing continues; invalid parent aborts',flush=True)
 print('FILEOPS PREPARE/DELETE PASS',flush=True)
except Exception:
 if x.pid:
  m.call('debug_break');m.screenshot(O/'failure.png');page(1)
  for v in ['v_dosError','fpFrame','fpFree','fpWanted','fpClusterBytes','foJob','foPathText']:
   ss=sy(R/'bin/FILEOPS.sym');addr=k.get(v,ss.get(v));print(v,m.read(addr,110 if v=='foJob' else 16).hex(),flush=True)
  page(0)
 raise
finally:x.stop()
