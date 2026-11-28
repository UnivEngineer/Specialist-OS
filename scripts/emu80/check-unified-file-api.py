"""Public FILEHANDLE and local whole-file wrappers on the real banked DOS."""
from pathlib import Path
import argparse,re,runpy,struct
R=Path(__file__).resolve().parents[2];O=R/'.local/unified-file-api';O.mkdir(parents=True,exist_ok=True)
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
def call(op,hl=0,de=0,bc=0,a=0,error=False,caller=0):
 page(caller);m.write(0x3000,struct.pack('<4H',hl,de,bc,a*256+2));m.write(0x2000,b'\xcd\x60\xf8\xc3\x00\x8b')
 m.stop_at(0x8b00);m.call('regs_set',pc=0x2000,sp=0x8fdf,bc=op,de=0x3000)
 z=m.until(0x8b00,90);r={key:v['dec'] for key,v in z['registers'].items()}
 assert r['sp']==0x8fdf,r
 assert bool(r['af']&1)==error,(hex(op),r)
 assert m.read(k['bankPage'],1)==bytes([caller]),r
 page(0)
 assert m.read(b['FONT_ADDR'],b['FONT_BYTES'])==font,'font damaged'
 return r
def io(path, data=None, capacity=8192):
 name=path.encode()+b'\0';m.write(0x3400,name+bytes(128-len(name)))
 f=bytearray(64);f[41]=1 if data is None else 14;f[49:51]=word(0x3400)
 m.write(0x3200,f);call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=7)
 if data is not None:m.write(0x4000,data)
 n=capacity if data is None else len(data)
 m.write(0x3200+49,word(0x4000)+word(n));call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=1 if data is None else 2)
 count=int.from_bytes(m.read(0x3200+51,2),'little');got=m.read(0x4000,count)
 call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=5)
 return got
try:
 fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
 x.start('UNIFIED',disk=R/'bin/nested-Games2M.bin',**fixture)
 m.call('debug_break');m.call('bp_clear');font=m.read(b['FONT_ADDR'],b['FONT_BYTES'])
 h['main_bios'](m,k['mx_printChar'],bc=31)
 call(k['MXOS_DOS_CLI'],hl=0,a=k['CLI_OP_OBSERVER'])
 for op in (0x20d,0x20e,0x20f):
  result=call(op,error=True);assert result['af']>>8==k['DOS_ERR_UNSUPPORTED']
 print('PASS retired preallocation/volume-registration IDs unsupported',flush=True)
 for n in (0,1,255,256,257,4097,8193):
  data=bytes((v*31+7)&255 for v in range(n));io('B:/DATA.BIN',data);assert io('B:/DATA.BIN',capacity=n+1)==data,n
  print('PASS open-path/write/read/EOF/close',n,flush=True)
 # The last readable byte may be the terminator. Bytes after it stay out
 # of KD_PATH, even for a terminator at the start of a new window.
 for size in (1, 31, 32, 33, 64, 65, 127, 128):
  line = b' ' * (size - 1) + b'\0'
  page(1);m.write(k['KD_PATH'], b'\xA5' * 128)
  page(0);m.write(0x3400, line + b'\x5A' * (128 - size))
  call(k['MXOS_DOS_CLI'], hl=0x3400, a=k['CLI_OP_QUERY'])
  page(1);assert m.read(k['KD_PATH'], 128) == line + b'\xA5' * (128 - size), size
  page(0)
 m.write(0x3400, b' ' * 128 + b'\0')
 result = call(k['MXOS_DOS_CLI'], hl=0x3400, a=k['CLI_OP_QUERY'], error=True)
 assert result['af'] >> 8 == k['DOS_ERR_ARGUMENT']
 high = 0xFC00 - 128
 saved = m.read(high, 128)
 try:
  for size in (1, 17, 33, 128):
   address = 0xFC00 - size
   m.write(address, b' ' * (size - 1) + b'\0')
   call(k['MXOS_DOS_CLI'], hl=address, a=k['CLI_OP_QUERY'])
  m.write(0xFBFF, b' ')
  for address in (0xFBFF, 0xFC00, 0xFFFF):
   result = call(k['MXOS_DOS_CLI'], hl=address, a=k['CLI_OP_QUERY'], error=True)
   assert result['af'] >> 8 == k['DOS_ERR_ARGUMENT'], hex(address)
  name = b'B:/DATA.BIN\0'
  address = 0xFC00 - len(name)
  m.write(address, name)
  f = bytearray(64);f[41] = 1;f[49:51] = word(address)
  m.write(0x3200, f);call(k['MXOS_DOS_FILEHANDLE'], hl=0x3200, a=k['FILE_OP_OPEN_PATH'])
  assert m.read(0x3231, 2) == word(address)
  call(k['MXOS_DOS_FILEHANDLE'], hl=0x3200, a=k['FILE_OP_CLOSE'])
  for size in (8, 32, 33, 128):
   command = b'CD B:/' + b' ' * (size - 7) + b'\r'
   page(1);m.write(k['KD_PATH'], b'\xA5' * 128);page(0)
   m.write(0xFC00 - size, command)
   call(k['MXOS_DOS_FILE_EXEC'], hl=0xFC00 - size)
   page(1);assert m.read(k['KD_PATH'], 128) == command + b'\xA5' * (128 - size), size
   page(0)
 finally:
  page(0);m.write(high, saved)
 m.write(0x3400, b'B:/\0')
 call(k['MXOS_DOS_DIRECTORY'], hl=0x3400, a=k['DIR_OP_CHDIR'])
 m.write(0x3400, b'\xA5' * 128)
 call(k['MXOS_DOS_DIRECTORY'], hl=0x3400, a=k['DIR_OP_GETCWD'])
 assert m.read(0x3400, 128) == b'B:\\\x00' + b'\xA5' * 124
 print('PASS exact DOS strings, window and MAIN range boundaries, and GETCWD tail', flush=True)
 executable = runpy.run_path(str(R/'scripts/images/executable.py'))
 fixture = executable['encode'](executable['Image'](b'\xc9', 0x100, 0x100), 'dll')
 io('B:/CHUNK.DLL', fixture)
 m.write(0x3400, b'B:/CHUNK.DLL\0')
 handle = call(k['MXOS_DOS_MODULE_LOAD'], hl=0x3400, a=k['MODULE_TARGET_BANK'])['hl']
 page(1);exec_page = m.read(k['mdBankId'], 1)[0];page(0)
 assert exec_page > 1, exec_page
 try:
  # The resident execution path ends at F7FCh. Use only the four free
  # bytes there, and exercise longer strings lower in the same bank.
  for size, address in ((1, 0xF7FF), (4, 0xF7FC), (32, 0x3800), (33, 0x3800), (128, 0x3800)):
   page(exec_page);m.write(address, b' ' * (size - 1) + b'\0')
   call(k['MXOS_DOS_CLI'], hl=address, a=k['CLI_OP_QUERY'], caller=exec_page)
  page(exec_page);m.write(0xF7FF, b' ')
  for address in (0xF7FF, 0xF800, 0xFFFF):
   result = call(k['MXOS_DOS_CLI'], hl=address, a=k['CLI_OP_QUERY'], error=True, caller=exec_page)
   assert result['af'] >> 8 == k['DOS_ERR_ARGUMENT'], hex(address)
  path = b'A.B\0'
  address = 0xF800 - len(path)
  page(exec_page);m.write(address, path);m.write(0x3200, b'\xA5' * 12)
  result = call(k['MXOS_DOS_FILE_NAME_PREPARE'], hl=address, de=0x3200, bc=0x5AC3, caller=exec_page)
  assert result['hl'] == 0xF7FF and result['de'] == 0x3200 and result['bc'] == 0x5AC3, result
  page(exec_page);assert m.read(0x3200, 12) == b'A       B  \xA5';page(0)
 finally:
  call(k['MXOS_DOS_MODULE_UNLOAD'], hl=handle)
 print('PASS EXEC range, file name cursor, and exact fixed descriptor output', flush=True)
 # One live object cannot be reopened or used after CLOSE.
 m.write(0x3400,b'B:/DATA.BIN'+bytes(117));f=bytearray(64);f[41]=1;f[49:51]=word(0x3400)
 m.write(0x3200,f);call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=7)
 before=m.read(0x3200,32);m.write(0x3400,b'B:/NOFILE.BIN'+bytes(115))
 r=call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=7,error=True);assert r['af']>>8==7
 assert m.read(0x3200,32)==before,'reopen damaged the live descriptor'
 m.write(0x3231,word(0xf1bf)+word(2));r=call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=1,error=True);assert r['af']>>8==7
 m.write(0x3231,word(0xffff)+word(0));call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=1)
 r=call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=2,error=True);assert r['af']>>8==6
 call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=5)
 r=call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=1,error=True);assert r['af']>>8==7
 f=bytearray(64);f[:11]=b'DATA    BIN';f[40]=1;f[41]=1;m.write(0x3200,f)
 call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=8);call(k['MXOS_DOS_FILEHANDLE'],hl=0x3200,a=5)
 print('PASS OPEN_AT, reopen/closed object, read-only and protected/empty ranges',flush=True)
 m.write(0x3400,b'MKDIR B:/TREE/SUB'+bytes(110));call(k['MXOS_DOS_CLI'],hl=0x3400)
 io('B:/TREE/SUB/DEEP.BIN',b'full path');assert io('B:/TREE/SUB/DEEP.BIN')==b'full path'
 print('PASS nested full path',flush=True)
 m.write(0x3400,b'COPY B:/TREE B:/CLONE'+bytes(100));call(k['MXOS_DOS_CLI'],hl=0x3400)
 assert io('B:/CLONE/SUB/DEEP.BIN')==b'full path'
 page(1);assert m.read(k['fileopsReady'],1)==b'\1';page(0)
 print('PASS cached FILEOPS and recursive COPY',flush=True)
 m.write(0x3400,b'DEL B:/CLONE'+bytes(115));call(k['MXOS_DOS_CLI'],hl=0x3400)
 import subprocess
 fixture_source=O/'library.asm'
 fixture_source.write_text('    INCLUDE "'+(R/'sdk/asm/mxos.inc').as_posix()+'"\n    ORG 06000h\n    INCLUDE "'+(R/'sdk/asm/lib/file-io.inc').as_posix()+'"\n    INCLUDE "'+(R/'sdk/asm/lib/file-image.inc').as_posix()+'"\n',encoding='utf-8')
 subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow','--raw='+str(O/'library.bin'),'--sym='+str(O/'library.sym'),str(fixture_source)],check=True,capture_output=True)
 lib=sy(O/'library.sym');m.write(0x6000,(O/'library.bin').read_bytes())
 def local(entry,expected=0):
  page(0);m.write(0x2000,b'\xcd'+word(lib[entry])+b'\xc3\x00\x8b')
  m.stop_at(0x8b00);m.call('regs_set',pc=0x2000,sp=0x8fdf,hl=0x3200)
  z=m.until(0x8b00,90,page=0);af=z['registers']['af']['dec']
  assert (af>>8 if af&1 else 0)==expected,(entry,af,expected)
 m.write(0x3400,b'B:/LIB.BIN'+bytes(118));m.write(0x4000,b'hello wrapper')
 m.write(0x3200,word(0x3400)+word(0x4000)+word(13)+bytes(34));local('fileWriteAll')
 m.write(0x4000,bytes(13));local('fileReadAll');assert m.read(0x4000,13)==b'hello wrapper'
 m.write(0x3204,word(1));local('fileReadAll',7);assert m.read(0x3206,2)==b'\0\0'
 m.write(0x3202,word(0x3200)+word(8));local('fileWriteAll',7)
 assert io('B:/LIB.BIN')==b'hello wrapper','invalid buffer truncated file'
 print('PASS local ASM read/write wrappers, capacity and overlap rejection',flush=True)
 print('UNIFIED FILE API PASS',flush=True)
except BaseException:
 if x.pid:
  m.call('debug_break');m.screenshot(O/'failure.png')
 raise
finally:x.stop()
