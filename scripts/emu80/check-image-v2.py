"""Native EXE v2 converter through the actual NC keyboard and FILEHANDLE."""
from pathlib import Path
import argparse,runpy,re,struct,sys,subprocess
R=Path(__file__).resolve().parents[2];O=R/'.local/image-v2-emu';O.mkdir(exist_ok=True)
p=argparse.ArgumentParser();p.add_argument('--runtime',type=Path,default=R/'.local/rom-pio-19782');p.add_argument('--port',type=int,default=19782);p.add_argument('--font-only',action='store_true');p.add_argument('--modules-only',action='store_true');p.add_argument('--modes-only',action='store_true');a=p.parse_args()
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g.update(OUT=O,RUNTIME=a.runtime.resolve())
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue;if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp'
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text('utf-8'),re.M)}
n=sy(R/'bin/NC.sym');k=sy(R/'bin/banked/KERNEL.sym');b=sy(R/'bin/BIOS.sym');W=lambda n:struct.pack('<H',n)
sys.path.insert(0,str(R/'scripts/images'));from executable import decode
api=runpy.run_path(str(R/'scripts/emu80/file-api.py'))
def command(text, shell='NC'):
 m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
 keys={' ':'space',':':'colon','.':'period','/':'slash','-':'minus','=':'equals'}
 for c in text:m.tap(keys.get(c,c.lower()))
 m.screenshot(O/'command.png')
 actual=m.read(n['CMD_LINE'],len(text))
 assert actual.upper()==text.upper().encode(),(text,actual)
 m.stop_at(b['biosBankShell']);m.tap('enter');m.until(b['biosBankShell'],120,page=0)
 m.stop_at(b['biosTryShell']);m.until(b['biosTryShell'],120,page=0)
 ready=n['mainLoop'] if shell=='NC' else sy(R/'bin/CLI.sym')['cliPrompt']
 m.stop_at(ready);m.until(ready,120,page=0)
 assert m.read(0,32)==(R/'bin'/f'{shell}.i80').read_bytes()[:32]
 m.screenshot(O/'result.png')
def read(path):
 m.call('debug_break')
 m.write(0x3400,path.encode()+bytes(128-len(path)))
 m.write(0x3200,W(0x3400)+W(0x4000)+W(19000)+bytes(34))
 try: api['path_io'](m,lambda op,hl=0,a=0:api['main_call'](m,k,op,hl,a),k,0x3200)
 except Exception:
  print('OPENED',m.read(0x2e00,64).hex(),flush=True)
  raise
 size=int.from_bytes(m.read(0x3206,2),'little');return m.read(0x4000,size)
def restore_nc():
 m.stop_at(b['biosTryShell']);m.call('regs_set',pc=b['bios_reboot'],sp=0x8fdf);m.until(b['biosTryShell'],120,page=0)
 m.stop_at(n['mainLoop']);m.until(n['mainLoop'],120,page=0)
def font_state():
 m.write(0xfffd,b'\0')
 state=(m.read(k['fontConfigValid'],1),m.read(k['fontConfigPending'],1),m.read(k['fontConfigRestoring'],1))
 m.write(0xfffc,b'\0')
 return state
def raw_call(op,hl=0,de=0,bc=0,af=2,error=None):
 m.write(0x2f80,struct.pack('<4H',hl,de,bc,af));m.write(0x2800,b'\xcd\x00\xf8\xc3\x00\x8b');m.stop_at(0x8b00)
 m.call('regs_set',pc=0x2800,sp=0x8fdf,bc=op,de=0x2f80);z=m.until(0x8b00,90,page=0)
 r={key:v['dec'] for key,v in z['registers'].items()};assert r['sp']==0x8fdf
 assert bool(r['af']&1)==(error is not None),(hex(op),r,error)
 if error is not None:assert r['af']>>8==error,(r,error)
 return r
def save(name,data):
 m.write(0x3400,name.encode()+bytes(128-len(name)));m.write(0x4000,data);m.write(0x3200,W(0x3400)+W(0x4000)+W(len(data))+bytes(34))
 api['path_io'](m,lambda op,hl=0,a=0:api['main_call'](m,k,op,hl,a),k,0x3200,writing=True)
def allocation_count():
 m.write(0xfffd,b'\0');table=m.read(k['MEM_ALLOC_TABLE'],32*6);m.write(0xfffc,b'\0');return sum(table[i]!=255 for i in range(0,len(table),6))
def load(name,target,error=None):
 m.write(0x3400,name.encode()+bytes(128-len(name)));return raw_call(k['MXOS_DOS_MODULE_LOAD'],hl=0x3400,af=(target<<8)|2,error=error)['hl']
def unload(handle,error=None):raw_call(k['MXOS_DOS_MODULE_UNLOAD'],hl=handle,error=error)
try:
 x.start('EXE-v2')
 if a.modes_only:
  from executable import Image,encode
  restore_nc()
  for mode,name in [(1,'MX'),(2,'STD'),(3,'MON'),(4,'RF')]:
   save('B:/'+name+'.EXE',encode(Image(b'\xc9',0,0,0,mode=mode),'exe'))
  restore_nc()
  for mode,name in [(1,'mx'),(2,'std'),(3,'mon'),(4,'rf')]:
   command('run b:/'+name+'.exe')
   print('PASS RUN uses header MODE without menu:',name,flush=True)
 elif a.modules_only:
  from executable import Image,encode
  restore_nc();baseline=allocation_count()
  bank=encode(Image(b'\xc9',0x100,0x100,2,bss=64,memory_begin=0x80,stack=0x80),'dll')
  save('B:/BANK.DLL',bank)
  m.write(0xfffd,b'\x05');m.write(0x101,b'\xa5'*64);m.write(0xfffc,b'\0')
  handle=load('B:/BANK.DLL',1)
  m.write(0xfffd,b'\x05');assert m.read(0x101,64)==b'\xa5'*64;m.write(0xfffc,b'\0')
  for _ in range(3):
   values=struct.pack('<4H',0x1234,0x2345,0x3456,2);m.write(0x3600,values)
   raw_call(k['MXOS_DOS_MODULE_CALL'],hl=handle,de=0x3600);assert m.read(0x3600,8)==values
  unload(handle);assert allocation_count()==baseline
  raw_call(k['MXOS_DOS_MODULE_CALL'],hl=handle,de=0x3600,error=k['MEM_ERROR_HANDLE'])
  bad=bytearray(bank);bad[-1]^=1;save('B:/BAD.DLL',bad)
  load('B:/BAD.DLL',1,error=k['DOS_ERR_BAD_IMAGE']);assert allocation_count()==baseline
  print('PASS BANK: BSS is not cleared, repeated calls/RET/SP, stale handle, failed-load cleanup',flush=True)
  main=encode(Image(b'\xc9',0x6000,0x6000,4,bss=0x400,memory_begin=0x5800),'dll');save('B:/MAIN.DLL',main)
  m.write(0x5800,b'\x5a'*0xC01);handle=load('B:/MAIN.DLL',0)
  assert m.read(0x5800,0x800)==b'\x5a'*0x800 and m.read(0x6001,0x400)==b'\x5a'*0x400
  load('B:/MAIN.DLL',0,error=k['MEM_ERROR_ACTIVE']);unload(handle);assert allocation_count()==baseline
  cache=encode(Image(b'\xc9',0x6000,0x6000,4,bss=0x400,memory_begin=0x5800,options=1),'dll');save('B:/CACHE.DLL',cache)
  preserve=encode(Image(b'\xc9',0x6000,0x6000,4,bss=0,policy=1),'dll');save('B:/KEEP.DLL',preserve)
  first=load('B:/CACHE.DLL',0);load('B:/KEEP.DLL',0,error=k['MEM_ERROR_ACTIVE']);second=load('B:/CACHE.DLL',0)
  raw_call(k['MXOS_DOS_MODULE_CALL'],hl=first,de=0x3600,error=k['MEM_ERROR_HANDLE']);unload(second);assert allocation_count()==baseline
  print('PASS MAIN: below-load workspace/BSS, pinned collision, PRESERVE and cache eviction',flush=True)
  dos=encode(Image(b'\xc9',k['DOS_SERVICE_ADDR'],k['DOS_SERVICE_ADDR'],1,bss=0x80),'dll');save('B:/DOS.DLL',dos)
  m.write(0xfffd,b'\0');m.write(k['DOS_SERVICE_ADDR']+1,b'\x69'*128);m.write(0xfffc,b'\0')
  handle=load('B:/DOS.DLL',2)
  m.write(0xfffd,b'\0');assert m.read(k['DOS_SERVICE_ADDR']+1,128)==b'\x69'*128;m.write(0xfffc,b'\0')
  load('B:/DOS.DLL',2,error=k['MEM_ERROR_ACTIVE']);unload(handle);assert allocation_count()==baseline
  # The real test module makes nested BIOS/DOS calls, owns a FAR allocation
  # and loads/calls/unloads a child. None of this installs a private gateway.
  subprocess.run([sys.executable,str(R/'source-apps/moduletest/build.py')],check=True,capture_output=True)
  for name in ('BANKMOD','MRET'):
   save('B:/'+name+'.DLL',(R/f'bin/moduletest/{name}.DLL').read_bytes())
  handle=load('B:/BANKMOD.DLL',1)
  for action in (1,5,2,8):
   m.write(0x3600,struct.pack('<4H',0,0,0,(action<<8)|2))
   raw_call(k['MXOS_DOS_MODULE_CALL'],hl=handle,de=0x3600)
   packet=struct.unpack('<4H',m.read(0x3600,8));assert not packet[3]&1,(action,packet)
   if action==5:assert packet[0]==0x1234,packet
  unload(handle);assert allocation_count()==baseline
  print('PASS BANK: nested BIOS/DOS, child RET, owned FAR blocks and recursive unload',flush=True)
  restore_nc();print('PASS DOS: BSS, collision, cleanup, return to NC',flush=True)
 elif a.font_only:
  from executable import Image,encode
  def h_driver_record():
   m.write(0xfffd,b'\0')
   try:return m.read(k['DOS_DRIVER_REGISTRY']+7*k['DRIVER_RECORD_SIZE'],k['DRIVER_RECORD_SIZE'])
   finally:m.write(0xfffc,b'\0')
  original_h=h_driver_record()
  assert any(original_h)
  restore_nc()
  fixture=encode(Image(b'\xc9',0,0),'rks');m.write(0x3400,b'B:/RET.RKS'+bytes(118));m.write(0x4000,fixture)
  m.write(0x3200,W(0x3400)+W(0x4000)+W(len(fixture))+bytes(34))
  api['path_io'](m,lambda op,hl=0,a=0:api['main_call'](m,k,op,hl,a),k,0x3200,writing=True)
  restore_nc()
  command('font 6x8/packed')
  assert m.read(b['FONT_ENGINE_HEADER']+7,1)==b'\1', 'Packed engine not installed'
  assert font_state()==(b'\1',b'\0',b'\0')
  initial_font=m.read(b['FONT_ADDR'],b['FONT_BYTES'])
  initial_planes=m.read(0x7c00,4096)
  assert initial_planes==(R/'bin/fonts/Packed6K.pre').read_bytes()
  assert h_driver_record()==original_h
  m.screenshot(O/'font-before.png')
  print('PASS FONT profile registered',flush=True)
  command('run -mx b:/ret.rks')
  assert m.read(b['FONT_ENGINE_HEADER']+7,1)==b'\1', 'Previous font not restored on exit'
  assert font_state()==(b'\1',b'\0',b'\0')
  assert m.read(b['FONT_ADDR'],b['FONT_BYTES'])==initial_font
  assert m.read(0x7c00,4096)==initial_planes
  m.screenshot(O/'font-after-exit.png')
  print('PASS whole-MAIN RUN switches to Rotate and restores Packed on exit',flush=True)
  m.stop_at(b['biosTryShell']);m.call('emu_reset');m.until(b['biosTryShell'],120,page=0)
  m.stop_at(n['mainLoop']);m.until(n['mainLoop'],120,page=0)
  assert m.read(b['FONT_ENGINE_HEADER']+7,1)==b'\1', 'Previous font not restored after Reset'
  assert font_state()==(b'\1',b'\0',b'\0')
  assert m.read(b['FONT_ADDR'],b['FONT_BYTES'])==initial_font
  assert m.read(0x7c00,4096)==initial_planes
  m.screenshot(O/'font-after-reset.png')
  print('PASS warm Reset restores selected FONT profile',flush=True)
  # NC's minimum workspace exceeds Fast12K's ceiling. A rejected FILE_EXEC
  # must unwind back to BIOS and its CLI fallback with the DOS bank intact.
  command('font 6x8/fast', shell='CLI')
  assert m.read(b['v_memTop'],2)==W(0x63ff)
  assert m.read(0x6400,10240)==(R/'bin/fonts/Fast12K.pre').read_bytes()
  assert h_driver_record()==original_h
  print('PASS NC low-memory refusal preserves H:, Fast12K planes and CLI fallback',flush=True)
 else:
  command('exe h:/rain.rks b:/rain.exe -f:main -b:0 -mode:mx')
  data=read('B:/RAIN.EXE');image=decode(data,'exe');assert image.flags==4 and image.bss==0 and image.mode==1
  get=runpy.run_path(str(R/'scripts/emu80/check-copy.py'))['get_file']
  original=decode(get((a.runtime/'specialist-os/flash-disk.bin').read_bytes(),'RAIN.RKS')[1],'rks')
  assert image.data==original.data and image.load==original.load
  print('PASS NC keyboard -> EXE utility -> v2 body, flags, mode, checksum',flush=True)
  # The isolated read fixture replaced MAIN test code; return through BIOS.
  m.stop_at(n['mainLoop']);m.call('regs_set',pc=b['bios_reboot'],sp=0x8fdf);m.until(n['mainLoop'],120,page=0)
  command('exe b:/rain.exe b:/rain1.exe -b:80 -begin:0 -end:6000 -p:preserve -mode:mx')
  data=read('B:/RAIN1.EXE');changed=decode(data,'exe');assert changed.bss==0x80 and changed.memory_end==0x6000 and changed.policy==1 and changed.data==image.data
  print('PASS native header editing preserves body and writes memory metadata',flush=True)
finally:x.stop()
