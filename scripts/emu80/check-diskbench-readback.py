"""Прямое HIGH-чтение и полная/посекторная проверка: тестовый Emu80."""
import argparse,json,runpy,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--passes',type=int,default=3);a=p.parse_args()
ns=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));Matrix=ns['Matrix']
out=ROOT/'docs/diskbench/readback-001';out.mkdir(parents=True,exist_ok=True);Matrix.__init__.__globals__['OUT']=out
x=Matrix();m=x.m;s=x.s
m.call('debug_break');nc=ns['symbols'](ROOT/'bin/NC.sym');m.stop_at(nc['mainLoop']);m.tap('ESC');m.until(nc['mainLoop'])
x.install('FlashSys')
records=[]
scenarios=[(0,'FlashSys',mode) for mode in [1,5,6]]+[(7,driver,mode) for driver in ['Flash','FlashFst'] for mode in [1,5,6]]+[(1,'RAMD',mode) for mode in [1,6]]
for drive,driver,mode in scenarios:
 if driver in ['Flash','FlashFst']:x.install(driver)
 if driver=='RAMD':x.format_ram()
 info=x.init_app(drive,mode);m.write(s['v_budget'],bytes([a.passes]));m.write(s['v_operations'],(32*a.passes).to_bytes(2,'little'))
 # Счётчики 10-байтных операций не изменены, верхняя запись отсутствует.
 disk=None if drive==1 else ns['RUNTIME']/'specialist-os'/('system-flash.bin' if drive==0 else 'flash-disk.bin')
 before=disk.read_bytes() if disk else x.snapshot_metadata()
 x.traps(s['file_cleanup']);m.call('regs_set',pc=s['file_write_case'],sp=0x8FDF)
 for attempt in range(1,a.passes+1):
  x.until(s['file_cleanup']);assert m.read(s['v_pass'],1)==bytes([attempt])
  assert m.read(0x2000,8192)==bytes(attempt^(v>>8)^(v&255) for v in range(0x2000,0x4000))
  if attempt==a.passes:x.traps(s['file_results'])
  else:m.call('debug_step',mode='into')
 x.until(s['file_results']);assert m.read(s['v_fileOwned'],1)==b'\0'
 low=m.read(s['resultsLow'],40);high=m.read(s['resultsHigh'],40)
 metrics={name:struct.unpack_from('<IIH',low,i*10) for i,name in enumerate(['rawWrite','rawRead','fileWrite','fileRead'])}
 if drive==0:
  metrics['rawReadHigh']=struct.unpack_from('<IIH',high,10);metrics['fileReadHigh']=struct.unpack_from('<IIH',high,30)
  metrics['rawWriteHigh']=struct.unpack_from('<IIH',high,0)
  assert metrics['rawWriteHigh'][0]>0 and struct.unpack_from('<IIH',high,20)==(0,0,0)
 verify=[int.from_bytes(m.read(s[name],4),'little') for name in ['statRawVerify','statFileVerify']]
 assert all(v>0 for v in verify)==(mode==6),(driver,mode,verify)
 assert metrics['rawWrite'][2]==(a.passes if driver=='FlashFst' else 0)
 assert metrics['fileWrite'][2]==(2*a.passes if driver=='FlashFst' else 0)
 if drive==0:assert metrics['rawReadHigh'][0]<metrics['rawRead'][0],metrics
 m.stop_at(s['exit_wait']);m.until(s['exit_wait'])
 key=f'{drive}-{driver}-{mode}';m.screenshot(out/(key+'.png'))
 after=disk.read_bytes() if disk else x.snapshot_metadata();x.check_integrity(before,after)
 row=dict(info,mode=mode,passes=a.passes,bytes=8192,metrics=metrics,verifyTicks=verify)
 records.append(row);(out/'results.json').write_text(json.dumps(records,indent=2)+'\n')
 print(key,{name:round(8192*a.passes*7812.5/1000/value[0],2) for name,value in metrics.items()},'verify',verify,flush=True)
