"""Воспроизводимый DISKBEN A:, 8 КиБ, три прохода, отдельно WRITE/VERIFY."""
from pathlib import Path
import runpy,argparse
ROOT=Path(__file__).resolve().parents[2];p=argparse.ArgumentParser();p.add_argument('--name',default='before');a=p.parse_args();out=ROOT/'docs/font-arena-044'/a.name;out.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));h['Matrix'].__init__.__globals__.update(OUT=out,RUNTIME=ROOT/'.local/monitor2-001')
x=h['Matrix']()
try:
 x.start('A-regression');x.console()
 for mode in (1,6):x.writable(0,'FlashSys',mode)
finally:x.stop()
