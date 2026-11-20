"""Matched-build system overlay; no application binds private DOS addresses."""
from pathlib import Path
import re, runpy
ROOT=Path(__file__).resolve().parents[2]
cpu_helpers = __import__('runpy').run_path(str(ROOT/'scripts/images/cpu.py'))
cpu_format = cpu_helpers['format_cpu']
def cpu_fields(path): return cpu_helpers['fields_from_symbols'](path.read_text('utf-8'))
def build(sy=None):
    h=runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    if sy is None: sy=h['symbols'](ROOT/'bin/banked/KERNEL.sym')
    for component,name,origin in [('fileops','FILEOPS',sy['DOS_FILEOPS_ADDR']),('enumerate','ENUM',sy['DOS_SERVICE_ADDR'])]:
        names=re.findall(r'^\s*DW (\S+)', (ROOT/f'source/{component}/host.inc').read_text(), re.M)
        (ROOT/f'bin/{component}-imports.inc').write_text(''.join(f'{n} = 0{sy[n]:04X}h\n' for n in names),encoding='utf-8')
        h['assemble'](ROOT/f'source/{component}/{name}.asm',ROOT/f'bin/{name}.i80',[])
        meta=h['symbols'](ROOT/f'bin/{name}.sym')
        limit=sy['DOS_FILEOPS_ADDR'] if name=='ENUM' else sy['DOS_BANK_DRIVER_END']
        assert meta.get('IMAGE_MEM_END',origin+(ROOT/f'bin/{name}.i80').stat().st_size+meta.get('IMAGE_BSS_SIZE',0))<=limit, name+' workspace overlaps the neighboring service'
        (ROOT/f'bin/{name}.cpu').write_text(cpu_format(origin,flags=1,fields=cpu_fields(ROOT/f'bin/{name}.sym')),encoding='ascii')
if __name__=='__main__': build()
