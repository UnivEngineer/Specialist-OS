"""Фиксированный ABI BIOS, плотный образ DOS и непересекающиеся буферы."""
from pathlib import Path
import argparse,json,runpy
ROOT=Path(__file__).resolve().parents[2]

def main():
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report-directory',type=Path,default=ROOT/'docs/source-layout-039/layout')
    args=parser.parse_args();out=args.report_directory.resolve();out.mkdir(parents=True,exist_ok=True)
    h=runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    bios=h['symbols'](ROOT/'bin/BIOS.sym');dos=h['symbols'](ROOT/'bin/banked/KERNEL.sym')
    layout=json.loads((ROOT/'bin/System.layout.json').read_text())
    image=(ROOT/'bin/DOS.sys').read_bytes();binary=(ROOT/'bin/BIOS.sys').read_bytes()
    assert layout['dos']['start']==dos['DOS_BANK_CODE_ADDR']==0x100
    assert layout['dos']['end']==dos['kernelEnd']
    assert len(image)==layout['dos']['bytes']==dos['kernelEnd']-dos['dosCoreStart']
    assert image==(ROOT/'bin/banked/KERNEL.bin').read_bytes()
    assert dos['dosCoreEnd']==dos['dosExtensionStart']
    assert dos['kernelEnd']<=dos['DOS_BANK_DATA_ADDR']
    assert image[:3]==b'\xC3'+dos['dosBoot'].to_bytes(2,'little')
    assert len(binary)==layout['bios']['bytes']
    io=(ROOT/'bin/IO.sys').read_bytes()
    assert len(io)==layout['io']['bytes']
    assert layout['io']['end']<=bios['LOCAL_BLOCK_ENTRY']
    assert bios['LOCAL_DRIVER_END']-bios['LOCAL_DRIVER_ADDR']>=412
    assert bios['SYSTEM_FLASH_END']<=bios['LOCAL_DRIVER_ADDR']
    assert bios['biosLowEnd']<=bios['FONT_ADDR']
    assert bios['io_vars']+bios['IO_VARIABLES']<=bios['STD_IO_ADDR']
    assert bios['FONT_ADDR']+len((ROOT/'assets/system-rom/FONTKOI.FNT').read_bytes())<bios['FONT_ARENA_END']
    assert bios['bios_upper']+bios['BIOS_UPPER_VARIABLES'] <= bios['io_vars']
    assert bios['BIOS_VARIABLES'] == 33
    for field,address in [('keyboardHook',0x8FDF),('tapeError',0x8FE1),('monitorFont',0x8FE7),('currentChar',0x8FE9),('cursorVisible',0x8FEA),('inputBusy',0x8FEB),('keyLocks',0x8FEC),('oldSP',0x8FF6),('inverse',0x8FFA),('cursorY',0x8FFC),('readDelay',0x8FFF)]:
        assert bios['bios_vars.'+field] == address,field
    intervals=sorted((v['address'],v['address']+v['size']) for v in layout['blocks']+layout['windows'] if v['size'])
    assert all(a[1]<=b[0] for a,b in zip(intervals,intervals[1:]))
    for item in layout['windows']:
        if 'target' in item:
            offset=item['address']-layout['bios']['start']
            assert binary[offset:offset+3]==b'\xC3'+bios[item['target']].to_bytes(2,'little')
    spans=[(0,0x100,'vectors'),(dos['dosCoreStart'],dos['kernelEnd'],'image'),(dos['LOCAL_BLOCK_ENTRY'],dos['mx_call'],'local-block')]
    for address,size,name in [('DOS_BANK_DATA_ADDR',4096,'exchange'),('DOS_BANK_OBJECT_ADDR',86,'object'),
                              ('DOS_BANK_PATH_ADDR',128,'path'),('DOS_DRIVER_REGISTRY',640,'registry'),
                              ('DOS_DRIVER_IMAGE',576,'driver-image'),('DOS_BANK_CACHE_ADDR',8192,'cache'),
                              ('DOS_BANK_CACHE_TABLE',520,'cache-table'),
                              ('DOS_BANK_PARTIAL_ADDR',512,'partial'),
                              ('DOS_CACHE_BOOT_ADDR',512,'BPB'),
                              ('DOS_BANK_CMD_ADDR',130,'command'),('DOS_BANK_COMPAT_ADDR',33,'variables'),
                              ('DOS_BANK_DRIVER_ADDR',dos['DOS_BANK_DRIVER_END']-dos['DOS_BANK_DRIVER_ADDR'],'initializer'),('BAT_BUFFER',dos['BAT_BUFFER_SIZE'],'BAT'),
                              ('DOS_BANK_STACK_BOTTOM',1024,'stack'),('mx_call',0x800,'tail')]:
        spans.append((dos[address],dos[address]+size,name))
    spans.sort();assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:])),spans
    _,groups=runpy.run_path(str(ROOT/'scripts/build/build-bios-dos.py'))['component_groups']()
    for region,units in groups.items():
        for unit in units:
            for filename in unit['files']:
                path=Path(filename).resolve();assert path.is_file(),path
                assert path.is_relative_to(ROOT/f'source/{region}') or path.is_relative_to(ROOT/'source/common') or (region=='dos' and path.is_relative_to(ROOT/'source/cli')) or (region=='bios' and path.is_relative_to(ROOT/'source/io')),path
    result=dict(biosBytes=len(binary),dosBytes=len(image),dosStart=dos['dosCoreStart'],dosEnd=dos['kernelEnd'],
                freeBeforeBuffers=dos['DOS_BANK_DATA_ADDR']-dos['kernelEnd'],regions=spans,fixedSlots=len(layout['windows']))
    (out/'result.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
    print('BIOS/DOS LAYOUT PASS',len(binary),len(image),result['fixedSlots'],'fixed slots')

if __name__=='__main__':main()
