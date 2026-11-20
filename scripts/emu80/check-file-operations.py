"""Shared FILEHANDLE/FILEOPS integration in an owned visible emulator."""
from pathlib import Path
import argparse,subprocess,sys
ROOT=Path(__file__).resolve().parents[2]
def main():
 p=argparse.ArgumentParser(description=__doc__)
 p.add_argument('--runtime',type=Path,default=ROOT/'.local/rom-pio-19782')
 p.add_argument('--port',type=int,default=19782)
 a=p.parse_args()
 subprocess.run([sys.executable,str(ROOT/'scripts/emu80/check-fileops.py'),'--runtime',str(a.runtime),'--port',str(a.port)],check=True)
if __name__=='__main__':main()
