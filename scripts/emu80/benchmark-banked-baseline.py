"""Ранний дисковый baseline перед банковым DOS; использует процедуры DISKBEN."""
from pathlib import Path
import argparse,runpy,json,hashlib

ROOT=Path(__file__).resolve().parents[2]

def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,default=ROOT/'.local/monitor2-001')
    p.add_argument('--report',type=Path,default=ROOT/'docs/banked/baseline-001/disks')
    a=p.parse_args();out=a.report.resolve();out.mkdir(parents=True,exist_ok=True)
    if (out/'raw.json').exists():p.error('Choose a new report directory; existing results are preserved')
    h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    g=h['Matrix'].__init__.__globals__;g['OUT']=out;g['RUNTIME']=a.runtime.resolve()
    m=h['Matrix']()
    try:
        m.start('pre-banked-DOS');m.inventory()
        for drive,name in [(0,'FlashSys'),(7,'Flash')]:
            m.install(name)
            for mode in (1,6):m.writable(drive,name,mode)
        m.install('RAMD');m.select(1)
        for mode in (1,6):m.writable(1,'RAMD',mode,'R4')
        m.report['binaries']={n:hashlib.sha256((ROOT/'bin'/n).read_bytes()).hexdigest()
                              for n in ['DOS.sys','DOSMOD.i80','DISKBEN.i80','RAMDISK.i80','Flash.i80','FlashSys.i80']}
        m.report['scope']='Current main-memory DOS; not a measurement of banked filesystem integration'
        m.save()
    finally:m.stop()

if __name__=='__main__':main()
