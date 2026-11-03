"""Нативные 128/256/512-КБ образы: ROM-чтение, FlashSys, SDP и сохранность."""
import argparse
import hashlib
import json
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
Machine = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))['FileMachine']
symbols = runpy.run_path(str(Path(__file__).with_name('check-flash-allocation.py')))['symbols']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', required=True, type=Path)
    p.add_argument('--locked', action='store_true')
    p.add_argument('--verify-persisted', action='store_true')
    args = p.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужен отдельный Emu80-flash-test')
    image = args.runtime / 'specialist-os/system-flash.bin'
    capacity = image.stat().st_size
    assert capacity in (131072, 262144, 524288)
    size = capacity // 1024
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    nc = (ROOT / 'bin/NC.i80').read_bytes()[:symbols(ROOT / 'bin/NC.lst')['state']-SYSTEM['NC_LOAD_ADDR']]
    for _ in range(180):
        m.call('emu_run_for', ms=500)
        if m.read(SYSTEM['NC_LOAD_ADDR'],len(nc)) == nc:
            break
    else:
        raise RuntimeError('NC не загружен')
    m.call('bp_set', addr=0x8100, type='exec')
    assert not m.subroutine(0xC842, af=0x0100, de=1)['flags']['C']
    assert not m.subroutine(0xC842, af=0x0000, de=1)['flags']['C']
    native = symbols(ROOT / 'bin/DOS.lst')['diskDriver']
    driver = 0xC863 if args.locked else symbols(ROOT / 'bin/FlashSys.lst')['Driver']
    for sector in [0,127,128,255,256,capacity//256-1]:
        assert not m.subroutine(native, bc=0x0200, de=sector, hl=0x4001)['flags']['C']
        assert m.read(0x4001,256) == image.read_bytes()[sector*256:(sector+1)*256]
    target = capacity//256 - 1
    reportpath = ROOT / f'docs/rom-banks/flash-{size}.json'
    reportpath.parent.mkdir(parents=True,exist_ok=True)
    if args.verify_persisted:
        report = json.loads(reportpath.read_text())
        assert not m.subroutine(driver, bc=0x0200, de=target, hl=0x4000)['flags']['C']
        assert hashlib.sha256(m.read(0x4000,256)).hexdigest() == report['sha256']
        assert image.stat().st_size == capacity and image.with_suffix('.bin.sdp').read_bytes() == b'\1'
        report['persistentAfterRestart'] = True
        reportpath.write_text(json.dumps(report,indent=2)+'\n')
        print(f'{size} KiB persisted data and SDP OK')
        return
    before = image.read_bytes()
    if args.locked:
        m.write(0x4000,bytes(v^255 for v in before[-256:]))
        assert m.subroutine(driver,bc=0x0100,de=target,hl=0x4000)['flags']['C']
        assert image.read_bytes() == before
        print(f'{size} KiB ROM/J4: reads OK, write rejected, image unchanged')
        return
    for salt in (0x39,0xA6):
        payload=bytes((i*31+salt)&255 for i in range(256))
        m.write(0x4000,payload)
        assert not m.subroutine(driver,bc=0x0100,de=target,hl=0x4000)['flags']['C']
        assert not m.subroutine(driver,bc=0x0200,de=target,hl=0x5000)['flags']['C']
        assert m.read(0x5000,256) == payload
        actual = image.read_bytes()
        assert len(actual)==capacity and actual[:-256]==before[:-256] and actual[-256:]==payload
    # Команды идентификации не программируют данные; /WE разрешён.
    m.write(0xFFFE,b'\0')
    for addr,val in ((0x5555,0xAA),(0x2AAA,0x55),(0x5555,0x90)):
        m.write(addr,bytes((val,)))
    expected = {128:0xD5,256:0xDA,512:0xA4}[size]
    assert m.read(0,2) == bytes((0x1F,expected))
    for addr,val in ((0x5555,0xAA),(0x2AAA,0x55),(0x5555,0xF0)):
        m.write(addr,bytes((val,)))
    m.write(0xFFFC,b'\0')
    report = dict(sizeKiB=size,pageSize=128 if size==128 else 256,
                  deviceId=expected,sector=target,sha256=hashlib.sha256(payload).hexdigest(),
                  nativeReads=True,sysWriteReadback=True,fileLengthUnchanged=True,
                  otherBytesUnchanged=True,persistentAfterRestart=False)
    reportpath.write_text(json.dumps(report,indent=2)+'\n')
    print(report)


if __name__=='__main__':
    main()
