"""Matched-build system overlay; no application binds private DOS addresses."""
from pathlib import Path
import re, runpy
ROOT=Path(__file__).resolve().parents[2]
def build(sy=None):
    h=runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    if sy is None: sy=h['symbols'](ROOT/'bin/banked/KERNEL.sym')
    for component,name,origin in [('fileops','FILEOPS',sy['DOS_BANK_DRIVER_ADDR']),('enumerate','ENUM',sy['DOS_SERVICE_ADDR'])]:
        names=re.findall(r'^\s*DW (\S+)', (ROOT/f'source/{component}/host.inc').read_text(), re.M)
        (ROOT/f'bin/{component}-imports.inc').write_text(''.join(f'{n} = 0{sy[n]:04X}h\n' for n in names),encoding='utf-8')
        h['assemble'](ROOT/f'source/{component}/{name}.asm',ROOT/f'bin/{name}.i80',[])
        (ROOT/f'bin/{name}.cpu').write_text(f'{origin:04X} {origin:04X} 01\n',encoding='ascii')
if __name__=='__main__': build()
