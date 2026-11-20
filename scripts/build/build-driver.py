"""Link each one-shot installer against the exclusive upper arena boundary."""
from pathlib import Path
import runpy,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
cpu_helpers = __import__('runpy').run_path(str(ROOT/'scripts/images/cpu.py'))
cpu_format = cpu_helpers['format_cpu']
def cpu_fields(path): return cpu_helpers['fields_from_symbols'](path.read_text('utf-8'))
def build(source):
    source=Path(source).resolve();name=source.stem
    h=runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    k=h['symbols'](ROOT/'bin/banked/KERNEL.sym')
    # Never publish the deliberately wrong measurement origin to image builders.
    (ROOT/'.local').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='driver-build-',dir=ROOT/'.local') as folder:
        target=Path(folder)/f'{name}.i80'
        h['assemble'](source,target,[])
        origin=k['DOS_BANK_DRIVER_ADDR']
        for _ in range(8):
            meta=h['symbols'](target.with_suffix('.sym'))
            bss=meta.get('IMAGE_BSS_SIZE',0)
            if bss==0xffff:raise ValueError('DOS initializer requires a known workspace size')
            end=meta.get('IMAGE_MEM_END',origin+target.stat().st_size+bss)
            new=origin+k['DOS_BANK_DRIVER_END']-end
            if new<k['DOS_BANK_DRIVER_ADDR']: raise ValueError('Installer exceeds the shared DOS arena')
            if new==origin:break
            origin=new
            h['assemble'](source,target,[f'DRIVER_ORIGIN=0x{origin:04X}'])
        else: raise ValueError('Installer alignment did not stabilize')
        size=target.stat().st_size
        meta=h['symbols'](target.with_suffix('.sym'))
        assert meta.get('IMAGE_MEM_BEGIN',origin)>=k['DOS_BANK_DRIVER_ADDR']
        assert meta.get('IMAGE_MEM_END',origin+size+meta.get('IMAGE_BSS_SIZE',0))==k['DOS_BANK_DRIVER_END']
        for ext in ('.i80','.sym','.lst'):
            target.with_suffix(ext).replace(ROOT/f'bin/{name}{ext}')
    (ROOT/f'bin/{name}.cpu').write_text(cpu_format(origin,flags=1,fields=cpu_fields(ROOT/f'bin/{name}.sym')),encoding='ascii')
    print(f'{name}: {origin:04X}..{k["DOS_BANK_DRIVER_END"]:04X} exclusive, {size} bytes')
if __name__=='__main__':build(sys.argv[1])
