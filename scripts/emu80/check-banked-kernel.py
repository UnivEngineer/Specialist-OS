"""Current banked-DOS regression suite over FILEHANDLE and MODULE APIs.

The old injected-kernel/FILESTREAM harness was removed with that ABI. Tests
start their own visible, muted emulator; applications install no MEM tails.
"""
from pathlib import Path
import argparse,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--runtime',type=Path,default=ROOT/'.local/rom-pio-19782')
 p.add_argument('--port',type=int,choices=range(19781,19785),default=19782)
 choice=p.add_mutually_exclusive_group()
 for name in ('files-only','images-only','copies-only','modules-only'):choice.add_argument('--'+name,action='store_true')
 a=p.parse_args()
 scripts=['check-unified-file-api.py','check-modules.py','check-fileops.py','check-run-body.py']
 if a.files_only:scripts=scripts[:1]
 if a.images_only:scripts=scripts[-1:]
 if a.copies_only:scripts=['check-fileops.py']
 if a.modules_only:scripts=['check-modules.py']
 for name in scripts:
  subprocess.run([sys.executable,str(ROOT/'scripts/emu80'/name),'--runtime',str(a.runtime),'--port',str(a.port)],check=True)
 print('BANK DOS API SUITE PASS')
if __name__=='__main__':main()
