"""Публичный moduleLoad/Call/Unload: реальные файлы и банки, без ручного хвоста."""
from pathlib import Path
import runpy,re,struct,json,subprocess,os
R=Path(__file__).resolve().parents[2];O=R/'.local/modules-dos';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text('utf-8'),re.M)}
k=sy(R/'bin/banked/KERNEL.sym');n=sy(R/'bin/NC.sym')
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001');x=h['Matrix']();m=x.m;rows=[]
e=runpy.run_path(str(R/'scripts/images/executable.py'))
# C-обёртки компилируются отдельно от ядра и исполняются тем же стендом.
z=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk');env=os.environ.copy();env['ZCCCFG']=str(z/'lib/config');env['PATH']=str(z/'bin')+os.pathsep+env['PATH']
subprocess.run([str(z/'bin/zcc.exe'),'+cpm','-clib=8080','-compiler=sccz80','-crt0='+str(R/'sdk/tests/file-crt'),'-zorg=8192','-I'+str(R/'sdk/include'),str(R/'sdk/src/mxos_z88dk.asm'),str(R/'sdk/src/mxos_far.c'),str(R/'sdk/src/mxos_module.c'),str(R/'sdk/tests/modules.c'),'-o',str(O/'c-modules.bin'),'-m'],env=env,check=True)
word=lambda v:struct.pack('<H',v)
regs=lambda hl=0,de=0,bc=0,af=2:struct.pack('<4H',hl,de,bc,af)
def page(v):m.write(0xfffd if v else 0xfffc,bytes([v-1 if v else 0]))
def call(op,hl=0,de=0,bc=0,af=2,error=None):
 page(0);m.write(0x3000,regs(hl,de,bc,af));m.write(0x2000,b'\xcd\x00\xf8\xc3\x00\x70')
 m.stop_at(0x7000);m.call('regs_set',pc=0x2000,sp=0x8fdf,bc=op,de=0x3000)
 z=m.until(0x7000,40);r={key:v['dec'] for key,v in z['registers'].items()}
 assert r['sp']==0x8fdf,r
 assert m.read(k['bankPage'],1)==b'\0'
 if error is None:assert not r['af']&1,(hex(op),r)
 else:assert r['af']&1 and (error=='carry' or r['af']>>8==error),(hex(op),error,r)
 page(1);assert m.read(k['bankDepth'],1)==base_depth[1];page(0);assert m.read(k['bankDepth'],1)==base_depth[0]
 assert m.read(font_symbols['FONT_ADDR'],font_symbols['FONT_BYTES'])==font_state,('API damaged font',hex(op))
 return r
def snapshot():
 page(1);data=m.read(k['MEM_ALLOC_TABLE'],32*6);table=m.read(k['MODULE_TABLE'],k['MODULE_MAX']*k['MODULE_RECORD_SIZE']);page(0)
 return sum(data[i]!=255 for i in range(0,len(data),6)),sum(any(table[i:i+2]) for i in range(0,len(table),10))
def save(name,data):
 page(0);m.write(0x3400,('B:\\'+name).encode()+b'\0'+bytes(128-len(name)-4));m.write(0x4000,data)
 m.write(0x3200,word(0x3400)+word(0x4000)+word(len(data))+bytes(34))
 call(k['MXOS_DOS_FILE_SAVE_PATH'],hl=0x3200)
def load(name,target=1,error=None):
 page(0);m.write(0x3400,('B:\\'+name).encode()+b'\0'+bytes(128-len(name)-4))
 r=call(k['MXOS_DOS_MODULE_LOAD'],hl=0x3400,af=target*256+2,error=error)
 return r['hl']
def invoke(handle,packet=regs(0x1234,0x2345,0x3456),error=None):
 page(0);m.write(0x3300,packet);call(k['MXOS_DOS_MODULE_CALL'],hl=handle,de=0x3300,error=error)
 return struct.unpack('<4H',m.read(0x3300,8))
def unload(handle,error=None):return call(k['MXOS_DOS_MODULE_UNLOAD'],hl=handle,error=error)
def check(name):rows.append(dict(case=name,allocations=snapshot()));print('PASS',name,flush=True)
try:
 fixture=runpy.run_path(str(R/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,O)
 n=sy(fixture['nc_symbols'])
 x.start('module API',**fixture);m.call('debug_break');m.call('bp_clear')
 font_symbols=sy(R/'bin/BIOS.sym')
 font_state=m.read(font_symbols['FONT_ADDR'],font_symbols['FONT_BYTES'])
 system_driver_entry=m.read(k['SYSTEM_FLASH_ADDR'],3)
 base_depth={}
 for v in (0,1):page(v);base_depth[v]=m.read(k['bankDepth'],1)
 page(0)
 # AUTOEXEC ограничивает B: четырьмя банками; остальные не размечены под диск.
 page(1);assert m.read(k['memoryRamPages'],1)==b'\x04';page(0)
 baseline=snapshot()
 # Изолированные ABI-вызовы печатают служебные сообщения; панели NC здесь
 # не участвуют. Начинаем отдельный консольный экран, а не пишем поверх NC.
 call(k['MXOS_BIOS_PRINT_CHAR'],bc=0x1f)
 call(k['MXOS_BIOS_SET_CURSOR_POS'],hl=0)
 if baseline!=(0,0):
  m.screenshot(O/'startup-failure.png')
  m.write(0x3400,b'A:/DRIVERS/RAMDISK.COM -P 4\0'+bytes(80))
  print('RAMDISK startup failure',baseline,call(k['MXOS_DOS_FILE_EXEC'],hl=0x3400,error='carry'),flush=True)
  raise AssertionError(baseline)
 page(1)
 assert m.read(k['memoryRamOwner'],1)==bytes((k['DOS_IO_PAGE'],))
 assert m.read(k['moduleDosServiceOwner'],1)==b'\0';page(0)
 state=call(k['MXOS_DOS_MEMORY'],bc=0,af=k['MEM_OP_RAMDISK']*256+2)
 assert state['bc']==0x401,state
 call(k['MXOS_DOS_MEMORY'],bc=1,af=k['MEM_OP_RAMDISK']*256+2,error=k['MEM_ERROR_OWNER'])
 check('DOS-owns-RAM-disk-without-resident-or-EXEC-bank')
 save('SERVICE.DLL',e['encode'](e['Image'](b'\xc9',k['DOS_SERVICE_ADDR'],k['DOS_SERVICE_ADDR'],1),'com'))
 service_handle=load('SERVICE.DLL',target=2)
 load('SERVICE.DLL',target=2,error=k['MEM_ERROR_ACTIVE'])
 unload(service_handle);assert snapshot()==baseline
 check('DOS-service-slot-remains-free-for-other-DLLs')
 for name in ('BANKMOD','MAINMOD','DOSMOD','MRET','MODTEST'):
  ext='COM' if name=='MODTEST' else 'DLL'
  save(name+'.'+ext,(R/f'bin/moduletest/{name}.{ext}').read_bytes())
 bad=bytearray((R/'bin/moduletest/BANKMOD.DLL').read_bytes());bad[-1]^=1;save('BAD.COM',bytes(bad))
 bad=bytearray((R/'bin/moduletest/MAINMOD.DLL').read_bytes());bad[-1]^=1;save('BADMAIN.COM',bytes(bad))
 save('RANGE.COM',e['encode'](e['Image'](b'\xc9'*512,0xf000,0xf000),'com'))
 save('DRIVER.COM',e['encode'](e['Image'](b'\xc9',0x100,0x100,1),'com'))
 # DOS-инициализатор повторно принимает готовый B:, не трогает main/данные.
 page(2);ram_guard=m.read(0,4096);page(0);main_guard=m.read(0,256)
 for args,error in (('-P 4',None),('',None),('-P 8','carry')):
  m.write(0x3400,('A:/DRIVERS/RAMDISK.COM '+args+'\0').encode()+bytes(80))
  call(k['MXOS_DOS_FILE_EXEC'],hl=0x3400,error=error)
  assert m.read(0,256)==main_guard
  page(2);assert m.read(0,4096)==ram_guard;page(1)
  assert m.read(k['memoryRamPages'],1)==b'\x04'
  assert m.read(k['diActive'],1)==b'\0';page(0)
  assert snapshot()==baseline
 state=call(k['MXOS_DOS_MEMORY'],bc=0,af=k['MEM_OP_RAMDISK']*256+2);assert state['bc']==0x401,state
 check('DOS-RAMDISK-reinstall-preserves-main-ready-volume-and-bank-pool')
 # RAW B: не должен попадать в освобождённые банки даже первым запросом.
 call(k['MXOS_DOS_FILE_SELECT_DRIVE'],af=0x102)
 page(6);guard=m.read(0,256);page(0)
 for block,mode in ((1024,2),(1024,1),(1023,2)):
  call(k['MXOS_DOS_RAW_IO'],hl=0x5200,de=block,bc=mode*256,error=k['DOS_ERR_ARGUMENT'])
 call(k['MXOS_DOS_RAW_IO'],hl=0x5200,de=1022,bc=0x200)
 call(k['MXOS_DOS_RAW_IO'],hl=0x5200,de=1024,bc=0x100,error=k['DOS_ERR_ARGUMENT'])
 page(6);assert m.read(0,256)==guard;page(0);check('RAM-disk-RAW-boundary-protects-free-banks')
 call(k['MXOS_DOS_MEMORY'],hl=0x6000,de=0x2000,af=k['MEM_OP_ARENA']*256+2)
 load('MISSING.COM',error=k['DOS_ERR_NOT_FOUND']);assert snapshot()==baseline
 for name,err in [('BAD.COM',k['DOS_ERR_BAD_IMAGE']),('RANGE.COM',k['MEM_ERROR_RANGE']),('DRIVER.COM',k['DOS_ERR_BAD_IMAGE'])]:
  load(name,error=err);assert snapshot()==baseline,(name,snapshot())
 load('BADMAIN.COM',target=0,error=k['DOS_ERR_BAD_IMAGE']);assert snapshot()==baseline
 load('MAINMOD.DLL',target=1,error=k['DOS_ERR_BAD_IMAGE']);assert snapshot()==baseline
 load('MRET.DLL',target=3,error=k['MEM_ERROR_RANGE']);assert snapshot()==baseline
 check('load-errors-no-leaks')
 save('BANKAPP.COM',(R/'bin/moduletest/BANKAPP.COM').read_bytes())
 save('BANKAPP.EXE',(R/'bin/moduletest/BANKAPP.COM').read_bytes())
 load('BANKAPP.COM',target=0,error=k['DOS_ERR_BAD_IMAGE']);assert snapshot()==baseline
 # Нативный FILE_EXEC возвращается в main, сохраняя код вызывающего приложения.
 for ext in ('COM','EXE','COM'):
  page(0);m.write(0x5200,word(0));before=m.read(0,256)
  m.write(0x3400,('B:\\BANKAPP.'+ext+' Test arguments\0').encode()+bytes(100))
  call(k['MXOS_DOS_FILE_EXEC'],hl=0x3400)
  assert m.read(0x5200,2)==word(0x55aa),m.read(0x5200,2).hex()
  assert m.read(0,256)==before
  assert snapshot()==baseline,snapshot()
 check('native-BANK-COM-arguments-RET-cleanup')
 for name in ('MAINAPP','DOSAPP'):
  save(name+'.EXE',(R/f'bin/moduletest/{name}.COM').read_bytes())
  page(0);m.write(0x5200,word(0))
  m.write(0x3400,('B:\\'+name+'.EXE Test\0').encode()+bytes(100))
  call(k['MXOS_DOS_FILE_EXEC'],hl=0x3400)
  assert m.read(0x5200,2)==word(0x55aa),(name,m.read(0x5200,2).hex())
  assert snapshot()==baseline,snapshot()
 check('MAIN-and-DRIVER-before-EXE-association')
 # DOS-DLL: арена общая с инициализаторами, но не с FAR-кучей/стеком.
 for target in (0,1):load('DOSMOD.DLL',target=target,error=k['DOS_ERR_BAD_IMAGE'])
 for name in ('BANKMOD.DLL','MAINMOD.DLL','MRET.DLL'):load(name,target=2,error=k['DOS_ERR_BAD_IMAGE'])
 for name,address,body in (('DOSLOW.DLL',k['DOS_BANK_DRIVER_ADDR']-1,b'\xC9'),('DOSEND.DLL',k['DOS_BANK_DRIVER_END']-1,b'\xC9\xC9')):
  save(name,e['encode'](e['Image'](body,address,address,1),'com'))
  load(name,target=2,error=k['MEM_ERROR_RANGE']);assert snapshot()==baseline
 dos=load('DOSMOD.DLL',target=2)
 assert snapshot()==(baseline[0],baseline[1]+1)
 page(1);assert m.read(k['moduleDosOwner'],1)!=b'\0';assert m.read(k['MEM_BANK_KIND'],1)==bytes((k['MEM_BANK_DOS'],));page(0)
 for _ in range(3):assert invoke(dos)[:3]==(0x1235,0x2346,0x3457)
 result=invoke(dos,regs(af=0x702));assert result[3]>>8==1 and result[2]&255==k['MEM_BANK_DOS'],result
 invoke(dos,regs(af=0x102))
 for drive in (0,1,7):
  call(k['MXOS_DOS_FILE_SELECT_DRIVE'],af=drive*256+2)
  result=invoke(dos,regs(af=0xB02));assert not result[3]&1 and result[0]==256,result
 result=invoke(dos,regs(af=0xC02));assert not result[3]&1 and result[0]==37,result
 page(1);guard=m.read(k['DOS_BANK_DRIVER_ADDR'],k['DOS_BANK_DRIVER_END']-k['DOS_BANK_DRIVER_ADDR']);page(0)
 load('DOSMOD.DLL',target=2,error=k['MEM_ERROR_ACTIVE'])
 page(1);assert m.read(k['DOS_BANK_DRIVER_ADDR'],len(guard))==guard;page(0)
 result=invoke(dos,regs(hl=dos,af=0x302));assert result[3]&1 and result[3]>>8==k['MEM_ERROR_ACTIVE'],result
 result=invoke(dos,regs(af=0x902));assert result[3]&1 and result[3]>>8==k['MEM_ERROR_OWNER'],result
 result=invoke(dos,regs(af=0xA02));assert result[3]&1 and result[3]>>8==k['MEM_ERROR_ACTIVE'],result
 invoke(dos,regs(af=0x202));invoke(dos,regs(af=0x802))
 assert snapshot()==(baseline[0]+2,baseline[1]+2),snapshot()
 unload(dos);assert snapshot()==baseline
 invoke(dos,error=k['MEM_ERROR_HANDLE'])
 page(1);assert m.read(k['moduleDosOwner'],1)==b'\0';assert m.read(k['MEM_BANK_KIND'],1)==bytes((k['MEM_BANK_DOS'],));page(0)
 bad=bytearray((R/'bin/moduletest/DOSMOD.DLL').read_bytes());bad[-1]^=1;save('DOSBAD.DLL',bytes(bad))
 load('DOSBAD.DLL',target=2,error=k['DOS_ERR_BAD_IMAGE']);assert snapshot()==baseline
 dos=load('DOSMOD.DLL',target=2);unload(dos);assert snapshot()==baseline
 m.write(0x3400,b'B:/DOSAPP.EXE Test\0'+bytes(100));call(k['MXOS_DOS_FILE_EXEC'],hl=0x3400)
 assert snapshot()==baseline
 check('DOS-DLL-registers-local-RAW-BIOS-DOS-arena-busy-cleanup-rollback')
 save('DRVPROBE.COM',(R/'bin/moduletest/DRVPROBE.COM').read_bytes())
 m.write(0x5202,word(0));m.write(0x3400,b'B:/DRVPROBE.COM\0'+bytes(100))
 call(k['MXOS_DOS_FILE_EXEC'],hl=0x3400)
 probe=int.from_bytes(m.read(0x5202,2),'little');assert probe&1 and probe>>8==k['MEM_ERROR_ACTIVE'],hex(probe)
 assert snapshot()==baseline;check('DOS-DLL-refuses-to-overwrite-running-driver')
 for name,offset in [('BADFLAG.EXE',5),('BADHEAD.EXE',2),('BADCHECK.EXE',14),('BADBODY.EXE',-1),('BADFLAG.COM',5)]:
  data=bytearray((R/'bin/moduletest/MAINAPP.COM').read_bytes())
  data[offset]=3 if offset==5 else data[offset]^1
  if offset in (2,5):data[14:16]=word(e['checksum'](data[:14]))
  save(name,bytes(data));page(0);m.write(0x5200,word(0))
  m.write(0x3400,('B:\\'+name+'\0').encode()+bytes(100))
  call(k['MXOS_DOS_FILE_EXEC'],hl=0x3400,error=k['DOS_ERR_BAD_IMAGE'])
  assert m.read(0x5200,2)==word(0)
  assert snapshot()==baseline
 check('invalid-COM-EXE-no-association-no-key-wait')
 for target,name in [(1,'BANKMOD.DLL'),(0,'MAINMOD.DLL')]:
  handle=load(name,target)
  before=snapshot();assert before==(baseline[0]+1,baseline[1]+1),before
  # Повторная загрузка main по занятому адресу не меняет живой образ.
  if not target:load(name,target,error=k['MEM_ERROR_RANGE']);assert snapshot()==before
  for _ in range(3):
   result=invoke(handle);assert result[:3]==(0x1235,0x2346,0x3457) and result[3]&1,result
  if target:
   # Ошибка входа в банк не оставляет active или изменённый MX_REGS.
   info=invoke(handle,regs(af=0x702));bank=info[3]>>8
   page(bank);old_sp=m.read(k['bankServiceSP'],2);m.write(k['bankServiceSP'],word(k['DOS_BANK_STACK_BOTTOM']))
   invoke(handle,error=k['MEM_ERROR_STACK'])
   page(bank);m.write(k['bankServiceSP'],old_sp);old_depth=m.read(k['bankDepth'],1);m.write(k['bankDepth'],bytes([k['BANK_MAX_DEPTH']]))
   invoke(handle,error=k['MEM_ERROR_STACK'])
   page(bank);m.write(k['bankDepth'],old_depth);page(0)
   invoke(handle)
   call(k['MXOS_DOS_MODULE_CALL'],hl=handle,de=0xf7fd,error=k['DOS_ERR_ARGUMENT'])
   assert snapshot()==before
  check('repeat-registers-'+name)
  result=invoke(handle,regs(af=0x102));assert not result[3]&1,result
  result=invoke(handle,regs(hl=handle,af=0x302));assert result[3]>>8==k['MEM_ERROR_ACTIVE'] and result[3]&1,result
  result=invoke(handle,regs(af=0x402));assert result[3]>>8==k['DOS_ERR_UNSUPPORTED'] and result[3]&1,result
  page(0);m.write(0x5100,bytes.fromhex('1234'))
  invoke(handle,regs(hl=0x5100,de=0xabcd,bc=0,af=0x602));assert m.read(0x5100,2)==bytes.fromhex('cdab')
  result=invoke(handle,regs(af=0x502));assert result[0]==0x1234,result
  assert snapshot()==before,snapshot()
  result=invoke(handle,regs(af=0x202));assert not result[3]&1,result
  assert snapshot()==(baseline[0]+2,baseline[1]+1),snapshot()
  call(k['MXOS_DOS_MEMORY'],hl=result[0],bc=result[2],af=k['MEM_OP_FREE']*256+2,error=k['MEM_ERROR_OWNER'])
  child=invoke(handle,regs(af=0x802))[0]
  unload(child,error=k['MEM_ERROR_OWNER'])
  assert snapshot()==(baseline[0]+3,baseline[1]+2),snapshot()
  check('nested-BIOS-DOS-owner-and-child-'+name)
  unload(handle);assert snapshot()==baseline,snapshot()
  invoke(handle,error=k['MEM_ERROR_HANDLE']);unload(handle,error=k['MEM_ERROR_HANDLE'])
  newer=load(name,target);assert newer!=handle
  invoke(handle,error=k['MEM_ERROR_HANDLE']);unload(newer);assert snapshot()==baseline
  check('unload-cleans-owned-blocks-stale-ID-'+name)
 # Чужой маленький блок исключает весь его банк из кандидатов EXEC.
 foreign=call(k['MXOS_DOS_MEMORY'],hl=1,bc=0xff00,af=k['MEM_OP_ALLOC']*256+2)
 handle=load('BANKMOD.DLL');result=invoke(handle,regs(af=0x702));assert result[3]>>8!=foreign['bc']>>8
 unload(handle);call(k['MXOS_DOS_MEMORY'],hl=foreign['hl'],bc=foreign['bc'],af=k['MEM_OP_FREE']*256+2);assert snapshot()==baseline
 check('foreign-allocation-not-reused-as-EXEC')
 handles=[load('MRET.DLL') for _ in range(k['MODULE_MAX']-baseline[1])]
 load('MRET.DLL',error=k['MEM_ERROR_NOMEM']);assert snapshot()==(baseline[0]+len(handles),k['MODULE_MAX'])
 for handle in handles:unload(handle)
 assert snapshot()==baseline;check('descriptor-table-full-no-leak')
 # Даже при свободной таблице дескрипторов занятые DATA-банки не отбираются.
 blocks=[]
 for _ in range(11-baseline[0]):
  r=call(k['MXOS_DOS_MEMORY'],hl=0xff00,bc=0xff00,af=k['MEM_OP_ALLOC']*256+2)
  blocks.append((r['hl'],r['bc']))
 load('MRET.DLL',error=k['MEM_ERROR_NOMEM']);assert snapshot()==(11,baseline[1])
 for address,bank in blocks:call(k['MXOS_DOS_MEMORY'],hl=address,bc=bank,af=k['MEM_OP_FREE']*256+2)
 assert snapshot()==baseline;check('no-free-bank-no-leak')
 invoke(0,error=k['MEM_ERROR_HANDLE']);unload(0xffff,error=k['MEM_ERROR_HANDLE'])
 # Reset отменяет незавершённую сессию модулей и их дочерние ресурсы.
 abandoned=load('BANKMOD.DLL');invoke(abandoned,regs(af=0x202));invoke(abandoned,regs(af=0x802))
 dos_abandoned=load('DOSMOD.DLL',target=2)
 assert snapshot()==(baseline[0]+3,baseline[1]+3)
 # Штатный запуск main-контроллера через редактор NC, а не прямой FILE_EXEC.
 m.call('bp_clear');m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120)
 assert snapshot()==baseline,snapshot()
 page(1);assert m.read(k['memoryRamOwner'],1)==bytes((k['DOS_IO_PAGE'],));assert m.read(k['moduleDosServiceOwner'],1)==b'\0';page(0)
 invoke(dos_abandoned,error=k['MEM_ERROR_HANDLE'])
 page(1);assert m.read(k['moduleDosOwner'],1)==b'\0';page(0)
 invoke(abandoned,error=k['MEM_ERROR_HANDLE']);check('Reset-invalidates-modules-cleans-resources')
 page(1);m.write(k['v_dosModuleReady'],b'\0');page(0)
 m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120)
 m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
 for ext in ('COM','EXE'):
  page(0);m.write(0x5200,word(0))
  for c in 'B:/BANKAPP.'+ext+' Test':m.tap({' ':'space',':':'colon','/':'slash','.':'period'}.get(c,c.lower()))
  m.screenshot(O/('native-bank-'+ext+'-command.png'))
  m.stop_at(k['keBankDone']);m.tap('enter');m.until(k['keBankDone'],120,page=1)
  page(0);assert m.read(0x5200,2)==word(0x55aa),m.read(0x5200,2).hex()
  m.screenshot(O/('native-bank-'+ext+'-result.png'))
  page(1)                 # Продолжение нативного FILE_EXEC в DOS.
  # NC может ждать подтверждения после консольного вывода BANK-программы.
  # Не полагаемся на то, что Enter запуска заодно снимет это ожидание.
  getch_points={font_symbols['bios_getch'],font_symbols['getch']}
  m.stop_at(n['mainLoop'])
  for getch in getch_points:m.call('bp_set',addr=getch,type='exec')
  for _ in range(120):
   z=m.call('cpu_state');pc=z['registers']['pc']['dec']
   if pc==n['mainLoop'] and m.read(k['bankPage'],1)==b'\0':break
   if pc in getch_points and m.read(k['bankPage'],1)==b'\0':
    m.stop_at(n['mainLoop']);m.call('emu_run_for',ms=500);m.tap('enter');break
   if pc in getch_points or pc==n['mainLoop']:m.call('debug_step',mode='into')
   else:m.call('emu_run_for',ms=1000)
  m.until(n['mainLoop'],120,page=0)
  assert m.read(font_symbols['FONT_ADDR'],font_symbols['FONT_BYTES'])==font_state,'BANK application damaged font'
  assert m.read(k['SYSTEM_FLASH_ADDR'],3)==system_driver_entry,'MEM recovery overwrote MAIN driver A:'
  assert snapshot()==baseline
  # Аппаратный Reset после программного возврата сохраняет DOS/B: и
  # повторно устанавливает MAIN-драйверы из AUTOEXEC перед входом в NC.
  m.stop_at(n['mainLoop']);m.call('emu_reset');m.until(n['mainLoop'],120,page=0)
  assert m.read(k['SYSTEM_FLASH_ADDR'],3)==system_driver_entry
  assert snapshot()==baseline
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
 check('NC-native-BANK-COM-and-EXE-launch')
 for c in 'B:/MODTEST.COM':m.tap({' ':'space',':':'colon','/':'slash','.':'period'}.get(c,c.lower()))
 m.screenshot(O/'command.png')
 m.tap('enter');m.call('emu_run_for',ms=2000);m.call('debug_break')
 controller=sy(R/'bin/moduletest/MODTEST.sym')
 assert m.read(controller['testResult'],2)==word(0x55aa),m.read(controller['testResult'],2).hex()
 m.screenshot(O/'controller-result.png')
 m.call('bp_clear');m.call('emu_run_for',ms=250);m.tap('enter')
 m.stop_at(n['mainLoop']);m.until(n['mainLoop'],120)
 assert snapshot()==baseline,snapshot()
 check('normal-NC-controller-launch-with-B-files-after-DOS-reload')
 # Существующая DLL с корректным контейнером не исполняется из редактора NC.
 for command in ('B:/BANKMOD.DLL','A:/BENCH/FONTBEN/FBMAIN.DLL'):
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc');m.call('emu_run_for',ms=250)
  before=m.read(0,32)
  for c in command:m.tap({':':'colon','/':'slash','.':'period'}.get(c,c.lower()))
  assert m.read(n['vars.cmdLine'],len(command)).upper()==command.encode()
  m.stop_at(k['keExtensionDenied']);m.tap('enter');m.until(k['keExtensionDenied'],120,page=1)
  page(0);assert m.read(0,32)==before
  page(1);m.stop_at(n['mainLoop']);m.until(n['mainLoop'],120,page=0)
  assert snapshot()==baseline
  m.screenshot(O/('dll-rejected-'+command[0]+'.png'))
 check('NC-rejects-DLL-before-loading-main')
 code=(O/'c-modules_code_crt_init.bin').read_bytes();assert 0x2000+len(code)<0x4000
 page(0);m.write(0x2000,code);m.stop_at(0x6100);m.call('regs_set',pc=0x2000,sp=0x8fdf);m.until(0x6100,40)
 result=int.from_bytes(m.read(0x5e80,2),'little');assert result==0,result
 assert snapshot()==baseline;check('C-SDK-module-lifecycle')
 (O/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
 print('MODULE API PASS',len(rows),flush=True)
finally:x.stop()
