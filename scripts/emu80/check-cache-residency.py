"""Все 88 секторов метаданных H: одновременно в кэше; принудительный flush."""
import argparse, json, runpy, struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
n=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
parser=argparse.ArgumentParser(description=__doc__);parser.add_argument('--directory',type=Path,default=ROOT/'docs/diskbench/cache-002')
Matrix=n['Matrix']; out=parser.parse_args().directory
Matrix.__init__.__globals__['OUT']=out/'residency'
(out/'residency').mkdir(exist_ok=True)
x=Matrix()
try:
    x.start('residency',system=out/'large-image/SystemRom_emu80_writable.bin',dos=out/'large/DOS.sys')
    x.install('FlashFst'); x.init_app(7,1)
    disk=n['RUNTIME']/'specialist-os/flash-disk.bin'; before=disk.read_bytes()
    for sector in range(1,89):
        x.func(x.ds['getSectorAddressInCache'],de=sector,af=0)
    cache=x.m.read(x.ds['cacheData'],704)
    numbers=[struct.unpack_from('<H',cache,i)[0] for i in range(0,704,8)]
    assert sorted(numbers)==list(range(1,89))
    for i,sector in enumerate(numbers):
        address=struct.unpack_from('<H',cache,i*8+2)[0]
        assert x.m.read(address,256)==before[sector*256:(sector+1)*256]
    # Помечаем последний сектор: содержимое остаётся прежним.
    x.func(x.ds['getSectorAddressInCache'],de=88,af=0x0100)
    assert x.m.read(x.ds['cacheData']+numbers.index(88)*8+6,1)==b'\1'
    assert not x.func(0xC899)['flags']['C']
    cache=x.m.read(x.ds['cacheData'],704)
    assert all(cache[i+6]==0 for i in range(0,704,8))
    assert disk.read_bytes()==before
    result=dict(allFatAndRootSectorsResident=True,allCachedPayloadsMatch=True,sectors=88,explicitVolumeFlushClearsDirty=True,imageUnchanged=True)
    (out/'residency/results.json').write_text(json.dumps(result,indent=2)+'\n')
    print(result)
finally:x.stop()
