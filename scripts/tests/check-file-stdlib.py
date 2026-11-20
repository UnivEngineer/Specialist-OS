"""C/sccz80 -> мост ABI -> настоящее ядро/модуль 8080; без вызовов CP/M."""
from pathlib import Path
import argparse,json,os,runpy,subprocess,re

ROOT=Path(__file__).resolve().parents[2]
parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--z88dk',type=Path,default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk'))
args=parser.parse_args();out=ROOT/'docs/file-api-001/stdlib';out.mkdir(parents=True,exist_ok=True)
env=os.environ.copy();env['ZCCCFG']=str(args.z88dk/'lib/config');env['PATH']=str(args.z88dk/'bin')+os.pathsep+env['PATH']
top=int(re.search(r'^APP_MEMORY_END: EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/DOS.sym').read_text(encoding='utf-8'),re.M).group(1),16)-1
command=[str(args.z88dk/'bin/zcc.exe'),'+cpm',f'-DTEST_MEMTOP={top}','-clib=8080','-compiler=sccz80','-crt0='+str(ROOT/'sdk/tests/file-crt'),'-zorg=8192','-I'+str(ROOT/'sdk/include'),str(ROOT/'sdk/src/mxos_file.c'),str(ROOT/'sdk/src/mxos_file_z88dk.asm'),str(ROOT/'sdk/src/mxos.c'),str(ROOT/'sdk/src/mxos_z88dk.asm'),str(ROOT/'sdk/tests/file-stdio.c'),'-o',str(out/'stdio.bin'),'-m']
subprocess.run(command,cwd=ROOT,env=env,check=True)
# The banked kernel and its buffers cannot be overlaid on MAIN in this test.
# Execute the compiled C consumer in its own visible, muted Emu80 instance.
subprocess.run([__import__('sys').executable,str(ROOT/'scripts/emu80/check-unified-sdk.py')],check=True)
