"""Штатный bootstrap: BIOS в main, DOS в банке 0; холодный и тёплый сброс."""
from pathlib import Path
import runpy, json, argparse


def verify_mem_profiles(m,page,BIN,b,k,ls,s,symbols):
 client=(BIN/'banked/CLIENT.bin').read_bytes()
 local=(BIN/'banked/LOCALBLOCK.bin').read_bytes()
 transport=(BIN/'banked/TRANSPORT.bin').read_bytes()
 for bank,kind in ((0,1),(1,2)):
  page(bank)
  client_size=b['BANK_MAIN_CLIENT_BYTES'] if bank==0 else len(client)
  actual=bytearray(m.read(b['mx_callPacket'],client_size))
  expected=bytearray(client[:client_size])
  if 'memTextRemaining' in ls:  # Сохранённые сборки с рабочими полями TEXT внутри CLIENT.
   at=ls['memTextRemaining']-b['mx_callPacket']
   expected[at:at+9]=actual[at:at+9]
  if bank==0 and 'bankMxRegsRouteCall' in ls:
   at=ls['bankMxRegsRouteCall']+1-b['mx_callPacket']
   expected[at:at+2]=b['sysRegsEntry'].to_bytes(2,'little')
  if 'MEM_FORWARD_RESTORE' in ls:
   at=ls['MEM_FORWARD_RESTORE']+1-b['mx_callPacket']
   expected[at:at+2]=actual[at:at+2]  # SP последнего прямого MOVE.
  assert actual==expected
  assert m.read(b['LOCAL_BLOCK_ENTRY'],ls['localRamCopy']-b['LOCAL_BLOCK_ENTRY'])==local[b['LOCAL_BLOCK_ENTRY']-b['MEM_IMAGE_LOCAL_START']:ls['localRamCopy']-b['MEM_IMAGE_LOCAL_START']]
  assert m.read(b['MEM_BANK_KIND'],1)==bytes([kind])
 data_size=symbols(BIN/'MEM.sym')['MEM_DATA_SIZE']
 data_start=b['MEM_DATA_START']
 assert data_start+data_size==b['BANK_WINDOW']
 page(2)
 actual=m.read(data_start,data_size)
 expected=bytearray(transport[data_start-0xfc00:data_start-0xfc00+data_size])
 ts=symbols(BIN/'banked/TRANSPORT.sym')
 # После первого обращения изменяемы стек MOVE и параметры последнего CPM32.
 variable = [(ts['bankCopyLocalParams'],8)]
 variable += [(ts[name],2) for name in
              ('bankCopySourcePair','bankCopyDestPair','bankCopyReturnPair','bankCopyCount')]
 for address,length in variable:
  start=address-data_start
  expected[start:start+length]=actual[start:start+length]
 if 'mdDataForwardTemplate' in k:
  offset=k['mdDataForwardTemplate']-k['DOS_CODE_START']
  length=k['bankMoveDataBytes']
  start=k['bankMoveDataEntry']-data_start
  expected[start:start+length]=(BIN/'DOS.sys').read_bytes()[offset:offset+length]
  start=k['bankMoveDirectOperand']-data_start
  expected[start:start+2]=k['bankMoveDataEntry'].to_bytes(2,'little')
  for address in (ts['bankMoveHome']+1,s['moveDataOverlayReturn']+1):
   start=address-data_start
   expected[start:start+2]=actual[start:start+2]
 assert actual==expected
 page(0)


ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--normal',action='store_true',help='Сохранён для прежних команд; сборка всегда штатная');p.add_argument('--report',type=Path,default=ROOT/'docs/banked/bootstrap-current')
p.add_argument('--bundle',type=Path,help='Frozen directory with bin/, System.bin and FONT.PF')
p.add_argument('--runtime',type=Path,default=ROOT/'.local/monitor2-001');p.add_argument('--port',type=int,default=19781)
a=p.parse_args();a.report.mkdir(parents=True,exist_ok=True)
BIN=(a.bundle/'bin') if a.bundle else ROOT/'bin'
FONT=(a.bundle/'FONT.PF') if a.bundle else ROOT/'assets/system-rom/fonts/6x8/KOI8.pf'
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['RUNTIME']=a.runtime.resolve();g['OUT']=a.report.resolve()
g['mcp_owner']=lambda:h['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {a.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
x=h['Matrix']();m=x.m;m.url=f'http://127.0.0.1:{a.port}/mcp';s=h['symbols'](BIN/'bootstrap/BOOT.sym');b=h['symbols'](BIN/'BIOS.sym');k=h['symbols'](BIN/'banked/KERNEL.sym');n=h['symbols'](BIN/'NC.sym');ls=h['symbols'](BIN/'banked/LOCALBLOCK.sym');rows=[]
def page(value):m.write(0xFFFD if value else 0xFFFC,bytes([value-1 if value else 0]))
def nc():
 for _ in range(20):
  m.stop_at(n['mainLoop']);state=m.until(n['mainLoop'],120)
  if m.read(0,32)==(BIN/'NC.i80').read_bytes()[:32]:return state
  m.call('debug_step',mode='into')
 raise AssertionError('NC was not restored')
try:
 x.start('bootstrap',system=a.bundle/'System.bin' if a.bundle else None,nc_binary=BIN/'NC.i80',nc_symbols=BIN/'NC.sym',dos=BIN/'DOS.sys')
 m.call('debug_break');page(1);m.write(k['v_dosModuleReady'],b'\0');page(0)
 for mode in ('cold','warm'):
  m.stop_at(s['stage'])
  m.call('emu_reset')
  m.until(s['stage'],40);ticks={}
  for label in ('stage','biosLoaded','dosLoaded','bootDone'):
   m.stop_at(s[label]);state=m.until(s[label],40);ticks[label]=state['cpu_ticks']
   if label=='biosLoaded':
    image=(BIN/'BIOS.sys').read_bytes();assert m.read(0xC000,len(image))==image
   if label=='dosLoaded':
    page(1)
    if mode=='cold':
     image=(BIN/'DOS.sys').read_bytes();assert m.read(k['DOS_BANK_CODE_ADDR'],len(image))==image
    else:
     assert m.read(0xA800,17)==b'DOS survives warm'
     page(2);assert m.read(0xE000,17)==b'RAM disk survives'
    page(0)
  linear=(FONT).read_bytes()
  font=bytes(linear[c*8+y] for y in range(8) for c in range(256))
  assert m.read(b['FONT_ADDR'],len(font))==font
  state=nc();ticks['nc']=state['cpu_ticks'];assert m.read(b['v_memTop'],2)==bytes.fromhex('ff8b')
  # Общий код MEM и разные данные установки main/DOS; минимальный профиль данных.
  verify_mem_profiles(m,page,BIN,b,k,ls,s,h['symbols'])

  page(1);assert m.read(k['memoryRamPages'],1)==b'\x04';assert m.read(k['v_dosModuleReady'],1)==b'\1';m.write(0xA800,b'DOS survives warm')
  page(2)
  if mode!='cold':assert m.read(0xE000,17)==b'RAM disk survives'
  m.write(0xE000,b'RAM disk survives');page(0)
  # Следующий Reset обязан восстановить обе части внешнего шрифта.
  m.write(b['FONT_ADDR'],b'\xA5');m.write(b['FONT_ADDR']+len(font)-1,b'\x5A')
  row=dict(mode=mode,ticks=ticks,biosMs=(ticks['biosLoaded']-ticks['stage'])/2000,dosAndProbeMs=(ticks['dosLoaded']-ticks['biosLoaded'])/2000,bootMs=(ticks['nc']-ticks['stage'])/2000,dosReloaded=mode=='cold');rows.append(row);print(row,flush=True);m.screenshot(a.report/(mode+'.png'))
 # Программные входы сохраняют BIOS; выбор оболочки принадлежит DOS.
 cli=h['symbols'](BIN/'CLI.sym')
 t=h['symbols'](BIN/'banked/TRANSPORT.sym')
 marker=b['biosVersion']+5;original=m.read(marker,1);m.write(marker,b'9')
 driver=m.read(b['SYSTEM_FLASH_ADDR'],16)
 for shell in ('NC','CLI'):
  if shell=='CLI':
   m.call('bp_clear');m.call('emu_run_for',ms=500)
   m.stop_at(cli['cliPrompt']);m.tap('f10');m.until(cli['cliPrompt'],120)
  target=n['mainLoop'] if shell=='NC' else cli['cliPrompt']
  binary=(BIN/f'{shell}.i80').read_bytes()[:32]
  for entry,std in ((0xC000,False),(0xC800,False),(0xC800,True)):
   # Изолированная проверка ABI; возврат реального COM проверяет system-019.
   m.stop_at(target);start=m.call('cpu_state')['cpu_ticks']
   if std:m.write(0xFFFF,b'\0')
   m.call('regs_set',pc=entry,sp=0x8FDF);state=m.until(target,120)
   assert m.read(0,32)==binary
   assert m.read(marker,1)==b'9','Software restart reloaded BIOS'
   assert m.read(b['SYSTEM_FLASH_ADDR'],16)==driver
   page(1);assert m.read(k['v_shell'],1)==bytes([shell=='CLI'])
   assert m.read(0xA800,17)==b'DOS survives warm'
   assert m.read(t['bankDepth'],1)==b'\1'
   assert m.read(b['LOCAL_BLOCK_ENTRY'],ls['localRamCopy']-b['LOCAL_BLOCK_ENTRY'])==local[b['LOCAL_BLOCK_ENTRY']-b['MEM_IMAGE_LOCAL_START']:ls['localRamCopy']-b['MEM_IMAGE_LOCAL_START']]
   assert int.from_bytes(m.read(t['bankServiceSP'],2),'little') <= b['DOS_BANK_STACK_TOP']
   page(2);assert m.read(0xE000,17)==b'RAM disk survives';page(0)
   row=dict(mode=f'{entry:04X}',std=std,shell=shell,biosReloaded=False,dosReloaded=False,returnMs=(state['cpu_ticks']-start)/2000)
   rows.append(row);print(row,flush=True)
  m.screenshot(a.report/('software-'+shell+'.png'))
 # Reset обязан восстановить даже повреждённый драйвер main, сохранив CLI.
 m.write(b['SYSTEM_FLASH_ADDR'],b'\x76');m.stop_at(cli['cliPrompt']);m.call('emu_reset');m.until(cli['cliPrompt'],120)
 assert m.read(0,32)==(BIN/'CLI.i80').read_bytes()[:32]
 assert m.read(marker,1)==original and m.read(b['SYSTEM_FLASH_ADDR'],16)==driver
 page(1);assert m.read(k['v_shell'],1)==b'\1';assert m.read(0xA800,17)==b'DOS survives warm'
 page(2);assert m.read(0xE000,17)==b'RAM disk survives';page(0)
 rows.append(dict(mode='reset-corrupt-main',shell='CLI',biosRestored=True,driverRestored=True,dosReloaded=False))
 print('RESET RESTORED BIOS/DRIVER; CLI/DOS/B: PRESERVED',flush=True)
 # Старое ядро нельзя сохранять при несовместимой версии транспорта.
 page(1);m.write(b['MEM_RUNTIME_REVISION_ADDR'],b'\0\0')
 m.write(k['kernelVersion'],b'\0');page(0)
 m.stop_at(s['dosLoaded']);m.call('emu_reset');m.until(s['dosLoaded'],120)
 page(1);image=(BIN/'DOS.sys').read_bytes()
 assert m.read(k['DOS_BANK_CODE_ADDR'],len(image))==image
 page(2);assert m.read(0xE000,17)==b'RAM disk survives';page(0)
 nc();page(1)
 assert int.from_bytes(m.read(b['MEM_RUNTIME_REVISION_ADDR'],2),'little')==b['MEM_RUNTIME_REVISION']
 page(0);rows.append(dict(mode='reset-incompatible-MEM',dosReloaded=True,ramDataPreserved=True))
 print('INCOMPATIBLE MEM RELOADS DOS; RAM DATA PRESERVED',flush=True)
 (a.report/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
finally:x.stop()
