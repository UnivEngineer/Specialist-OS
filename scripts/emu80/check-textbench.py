"""TEXTBEN: обычный запуск из NC, четыре времени и возврат в оболочку."""
from pathlib import Path
import runpy,re,json,time,argparse
R=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report',type=Path,default=R/'docs/bench/text-current');O=p.parse_args().report.resolve();O.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001');x=h['Matrix']();m=x.m
sy=lambda f:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/f).read_text(),re.M)}
s=sy('bin/TEXTCOM.sym');n=sy('bin/NC.sym')
try:
 x.start('textbench');m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('esc')
 command='A:/BENCH/FONTBEN/TEXTBEN.COM'
 for ch in command:m.tap({'.':'period',':':'colon','/':'slash'}.get(ch,ch.lower()))
 assert m.read(n['vars.cmdLine'],len(command)).upper()==command.encode();m.screenshot(O/'command.png');m.stop_at(0);m.tap('enter');m.until(0,120,page=0)
 assert m.read(0,32)==(R/'bin/TEXTCOM.i80').read_bytes()[:32]
 m.stop_at(s['waitKey']);m.call('emu_run');deadline=time.monotonic()+180
 while time.monotonic()<deadline:
  if m.call('cpu_state')['registers']['pc']['dec']==s['waitKey']:break
  time.sleep(.25)
 else:raise RuntimeError('TEXTBEN timeout')
 times={k:round(int.from_bytes(m.read(s[v],4),'little')*.128,3) for k,v in [('mainChar','charTicks'),('mainString','stringTicks'),('bankChar','bankCharTicks'),('bankString','v_elapsed')]};m.screenshot(O/'results.png')
 assert m.read(s['bankHandle'],2)==b'\0\0','module not unloaded'
 assert all(v>0 for v in times.values()),times
 k=sy('bin/banked/KERNEL.sym')
 try:
  m.write(0xfffd,b'\0');alloc=m.read(k['MEM_ALLOC_TABLE'],32*6);mods=m.read(k['MODULE_TABLE'],k['MODULE_MAX']*k['MODULE_RECORD_SIZE'])
 finally:m.write(0xfffc,b'\0')
 assert all(alloc[i]==255 for i in range(0,len(alloc),6)), 'allocation leak'
 assert all(mods[i:i+2]==b'\0\0' for i in range(0,len(mods),k['MODULE_RECORD_SIZE'])), 'module leak'
 m.call('bp_clear');m.call('emu_run_for',ms=500)
 m.stop_at(n['mainLoop']);m.tap('enter');m.until(n['mainLoop'],120,page=0);m.screenshot(O/'returned.png')
 (O/'results.json').write_text(json.dumps(dict(ms=times,returnedToNC=True,publicModuleABI=True,modulesUnloaded=True,allocationLeaks=0),indent=2)+'\n');print(times,flush=True)
finally:x.stop()
