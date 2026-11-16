"""Link each one-shot installer against the exclusive upper arena boundary."""
from pathlib import Path
import runpy,sys,tempfile
ROOT=Path(__file__).resolve().parents[2]
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
            new=k['DOS_BANK_DRIVER_END']-target.stat().st_size
            if new<k['DOS_BANK_DRIVER_ADDR']: raise ValueError('Installer exceeds the shared DOS arena')
            if new==origin:break
            origin=new
            h['assemble'](source,target,[f'DRIVER_ORIGIN=0x{origin:04X}'])
        else: raise ValueError('Installer alignment did not stabilize')
        size=target.stat().st_size
        assert origin+size==k['DOS_BANK_DRIVER_END']
        for ext in ('.i80','.sym','.lst'):
            target.with_suffix(ext).replace(ROOT/f'bin/{name}{ext}')
    (ROOT/f'bin/{name}.cpu').write_text(f'{origin:04X} {origin:04X} 01\n',encoding='ascii')
    print(f'{name}: {origin:04X}..{k["DOS_BANK_DRIVER_END"]:04X} exclusive, {size} bytes')
if __name__=='__main__':build(sys.argv[1])
