"""Выход STD-игр в MX: сравнение с DOS, предупреждение и восстановление MEM."""
from pathlib import Path
import runpy,re,json
R=Path(__file__).resolve().parents[2];O=R/'docs/compat/almaz-std-exit-050';O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001')
x=h['Matrix']();m=x.m
sy=lambda f:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/f).read_text(),re.M)}
n=sy('bin/NC.sym');b=sy('bin/BIOS.sym');r=sy('bin/RUN.sym');t=sy('bin/banked/TRANSPORT.sym');rows=[]
def loaded_nc():
 for _ in range(120):
  state=m.call('cpu_state');pc=state['registers']['pc']['dec']
  assert pc!=b['biosMemWarning'],'Unexpected MEM warning'
  if pc==n['mainLoop'] and m.read(0,32)==(R/'bin/NC.i80').read_bytes()[:32]:return
  if state['breaked']:m.call('debug_step',mode='into')
  m.call('emu_run_for',ms=1000)
 raise AssertionError(('NC return timeout',m.call('cpu_state')))
try:
 x.start('std-mx-return',disk=R/'bin/nested-Games2M.bin')
 for name,asset,warn in [('Almaz2-8.rks','Almaz-2/Almaz2-8color.rks',True),('Dash2.rks','DASH2.RKS',False)]:
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc')
  text='RUN -MX H:/Almaz-2/STD/'+name
  for ch in text:m.tap({' ':'space',':':'colon','.':'period','/':'slash','-':'minus'}.get(ch,ch.lower()))
  m.screenshot(O/(name+'-command.png'));m.stop_at(r['RUN_LOAD_STAGE']);m.tap('enter');m.until(r['RUN_LOAD_STAGE'],120)
  m.stop_at(0x38);m.until(0x38,120,page=0)
  payload=(R/'assets/games'/asset).read_bytes()[4:-2];assert m.read(0,32)==payload[:32]
  m.call('bp_clear');m.call('emu_run_for',ms=500);m.stop_at(0xc800);m.call('kbd_send',keys=['clear'],action='press');state=m.until(0xc800,120,page=0);m.call('kbd_send',keys=['clear'],action='release')
  first=m.read(0xff00,4);m.write(0xfffd,b'\0');reference=m.read(0xff00,4);code=m.read(0xfc00,t['bankPage']-0xfc00);m.write(0xfffc,b'\0')
  assert (first!=reference)==warn,(name,first.hex(),reference.hex())
  if warn:
   # Дополнительная проверка восстановления остальных кодовых сегментов MEM.
   m.write(b['LOCAL_BLOCK_ENTRY'],b'\xa5'*12);m.write(b['mx_call'],b'\xa5'*12);m.write(0xfc00,b'\xa5'*12)
   m.stop_at(b['biosMemWarning']);m.until(b['biosMemWarning'],120,page=0)
   assert m.read(0xfc00,len(code))==code
   assert m.read(b['mx_call'],b['BANK_CLIENT_BYTES'])==(R/'bin/banked/CLIENT.bin').read_bytes()
   assert m.read(b['LOCAL_BLOCK_ENTRY'],b['MEM_NATIVE_CALL']-b['LOCAL_BLOCK_ENTRY'])==(R/'bin/banked/LOCALBLOCK.bin').read_bytes()[:b['MEM_NATIVE_CALL']-b['LOCAL_BLOCK_ENTRY']]
   m.stop_at(b['getch']);m.until(b['getch'],120,page=0);m.call('bp_clear');m.call('emu_run_for',ms=500)
   assert m.read(0,32)!= (R/'bin/NC.i80').read_bytes()[:32],'Warning did not wait'
   m.screenshot(O/(name+'-warning.png'));m.stop_at(n['mainLoop']);m.tap('space')
  else:
   m.stop_at(n['mainLoop']);m.call('bp_set',addr=b['biosMemWarning'],type='exec')
  loaded_nc();tail=(R/'bin/banked/TRANSPORT.bin').read_bytes()[0x300:];assert m.read(0xff00,len(tail))==tail
  m.screenshot(O/(name+'-returned.png'));row=dict(file=name,exitAddress='C800h',firstBytes=first.hex(),dosBytes=reference.hex(),warningAndKeyWait=warn,memRestored=True,nc=True);rows.append(row);print(row,flush=True)
 (O/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
finally:x.stop()
