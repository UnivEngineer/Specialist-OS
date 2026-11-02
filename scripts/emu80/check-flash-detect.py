"""Software ID: число AT29C040A, ёмкость A:, границы, SDP и отказ без ID."""
import argparse, hashlib, json, runpy, struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
n=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
Matrix=n['Matrix']; out=ROOT/'docs/flash/detect-001';out.mkdir(parents=True,exist_ok=True)
Matrix.__init__.__globals__['OUT']=out
syms=runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))['symbols']
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--no-id-only',action='store_true');args=p.parse_args()
x=Matrix(); rows=[]
if args.no_id_only:rows=[r for r in json.loads((out/'results.json').read_text()) if 'noIdRejected' not in r]
def save(): (out/'results.json').write_text(json.dumps(rows,indent=2)+'\n')
def size(drive,expected,step):
    x.select(drive); r=x.func(0xC863,bc=0x0300)
    assert r['registers']['de']['dec']==expected//256
    assert r['registers']['bc']['dec']==step
def check_sectors(drive,image,sectors,flush=False):
    x.select(drive); before=image.read_bytes();capacity=len(before)//256
    for sector in sectors:
        payload=bytes((i*37+sector*13+0x39)&255 for i in range(256))
        x.m.write(0x4001,payload)
        assert not x.func(0xC863,bc=0x0500,de=sector,hl=0x4001)['flags']['C']
        if flush: assert not x.func(0xC863,bc=0x0400)['flags']['C']
        assert not x.func(0xC863,bc=0x0200,de=sector,hl=0x5001)['flags']['C']
        assert x.m.read(0x5001,256)==payload
        actual=image.read_bytes();a=sector*256
        assert actual[:a]==before[:a] and actual[a:a+256]==payload and actual[a+256:]==before[a+256:]
        x.m.write(0x4001,before[a:a+256])
        assert not x.func(0xC863,bc=0x0100,de=sector,hl=0x4001)['flags']['C']
        if flush: assert not x.func(0xC863,bc=0x0400)['flags']['C']
    for op in [1,2,5]:
        assert x.func(0xC863,bc=op<<8,de=capacity,hl=0x4001)['flags']['C']
        assert x.func(0xC863,bc=op<<8,de=0xFFFF,hl=0x4001)['flags']['C']
    assert image.read_bytes()==before

try:
    seed=(ROOT/'images/FlashDisk_emu80_seed.bin').read_bytes()
    for count in ([] if args.no_id_only else [2,1,3,4]):
        disk=out/f'flash-{count}.bin';disk.write_bytes(seed[:count*524288])
        x.start(f'chips-{count}',disk=disk,chip_count=count)
        image=n['RUNTIME']/'specialist-os/flash-disk.bin'
        for name in ['Flash','FlashFst']:
            before=image.read_bytes();sdp=image.with_suffix('.bin.sdp').read_bytes()
            x.install(name);assert image.read_bytes()==before and image.with_suffix('.bin.sdp').read_bytes()==sdp
            size(7,count*524288,2048 if name=='FlashFst' else 0)
            info=x.func(0xC863,bc=0xFF00)['registers']['hl']['dec']
            strings=x.m.read(info,48).split(b'\0');assert strings[1].startswith(str(count).encode()+b'x512K')
            driver_symbols=syms(ROOT/f'bin/{name}.sym')
            assert x.m.read(driver_symbols['v_chipMap'],count)==bytes(i*8 for i in range(count))
            sectors=sorted(set([127,128,255,256,count*2048-1]+([2047,2048] if count>1 else [])))
            check_sectors(7,image,sectors,name=='FlashFst')
            rows.append(dict(driver=name,chips=count,bytes=count*524288,idDoesNotWrite=True,readWriteVerified=True,boundsRejected=True))
            save();print(rows[-1],flush=True)
        if count==2:
            # FORMAT получает физический размер от B=3, не от старого BPB 2 МиБ.
            x.m.write(0xF100,(ROOT/'bin/FORMAT.com').read_bytes());x.m.write(0x8400,b'H: -Y\r\0')
            assert not x.func(0xF100,de=0x8400)['flags']['C']
            assert struct.unpack_from('<H',image.read_bytes(),19)[0]==4096
            size(7,1048576,2048)
            rows.append(dict(formatPhysical1MiB=True));save()
        if count==4:
            # Только преобразование: моделируем пустое гнездо между чипами.
            old=x.m.read(driver_symbols['v_chipMap'],4)
            x.m.write(driver_symbols['v_chipMap'],bytes([0,16,0,0]))
            for logical,physical in [(0,0),(2047,2047),(2048,4096),(4095,6143)]:
                state=x.func(driver_symbols['MapSector'],de=logical,hl=0xBEEF,bc=0xA500)
                assert state['registers']['de']['dec']==physical
                assert state['registers']['hl']['dec']==0xBEEF
                assert state['registers']['bc']['dec']>>8==0xA5
            x.m.write(driver_symbols['v_chipMap'],old)
            rows.append(dict(sparseMappingSynthetic=True,hardwareEmptyMiddleNotTested=True));save()
    for kib in ([] if args.no_id_only else [128,256,512]):
        source=ROOT/f'images/SystemRom_{kib}K.bin'
        x.start(f'system-{kib}',system=source)
        image=n['RUNTIME']/'specialist-os/system-flash.bin';before=image.read_bytes()
        x.install('FlashSys');assert image.read_bytes()==before
        size(0,kib*1024,0)
        s=syms(ROOT/'bin/FlashSys.sym')
        assert x.m.read(s['v_halfPage'],1)==bytes([128 if kib==128 else 0])
        info=x.func(0xC863,bc=0xFF00)['registers']['hl']['dec']
        assert x.m.read(info,48).split(b'\0')[1]==f'{kib}K mapped'.encode()
        check_sectors(0,image,[127,128,255,256,kib*4-1])
        rows.append(dict(driver='FlashSys',physicalKiB=kib,pageBytes=128 if kib==128 else 256,idDoesNotWrite=True,readWriteVerified=True,boundsRejected=True));save();print(rows[-1],flush=True)
    # /WE запрещён: software ID недоступен, старые драйверы/данные сохраняются.
    x.start('no-software-id',locked=True,disk_locked=True)
    for name,drive,address in [('FlashSys',0,0xDC00),('Flash',7,0xDE00),('FlashFst',7,0xDE00)]:
        # H: без установленного драйвера не обязан монтироваться.
        x.select(0);old=x.m.read(x.ds['v_drives']+drive*2,2);resident=x.m.read(address,512)
        x.m.write(0,(ROOT/f'bin/{name}.com').read_bytes());state=x.func(0)
        assert state['flags']['C'];assert x.m.read(address,512)==resident
        assert x.m.read(x.ds['v_drives']+drive*2,2)==old
        rows.append(dict(driver=name,noIdRejected=True,oldDriverPreserved=True));save();print(rows[-1],flush=True)
finally:x.stop()
