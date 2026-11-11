"""C SDK: main-арена, FAR-куча, scalar, realloc, перекрытие через реальный DOS ABI."""
from pathlib import Path
import runpy,subprocess,os
R=Path(__file__).resolve().parents[2];O=R/'docs/banked/memory-054/c-sdk';O.mkdir(parents=True,exist_ok=True)
z=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk');env=os.environ.copy();env['ZCCCFG']=str(z/'lib/config');env['PATH']=str(z/'bin')+os.pathsep+env['PATH']
subprocess.run([str(z/'bin/zcc.exe'),'+cpm','-clib=8080','-compiler=sccz80','-crt0='+str(R/'sdk/tests/file-crt'),'-zorg=8192','-I'+str(R/'sdk/include'),str(R/'sdk/src/mxos_z88dk.asm'),str(R/'sdk/src/mxos_far.c'),str(R/'sdk/tests/far-memory.c'),'-o',str(O/'memory.bin'),'-m'],env=env,check=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=O,RUNTIME=R/'.local/monitor2-001');x=h['Matrix']();m=x.m
try:
 x.start('C memory SDK');m.call('debug_break');m.call('bp_clear');code=(O/'memory_code_crt_init.bin').read_bytes();assert 0x2000+len(code)<0x4000
 m.write(0x2000,code);m.stop_at(0x6100);m.call('regs_set',pc=0x2000,sp=0x8fdf);m.until(0x6100,40)
 result=int.from_bytes(m.read(0x5e80,2),'little');assert result==0,result
 print('C SDK FAR MEMORY PASS',len(code),'bytes',flush=True)
finally:x.stop()
