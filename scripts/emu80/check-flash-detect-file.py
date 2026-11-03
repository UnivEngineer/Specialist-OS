"""Двухчиповый 1-МиБ том через DOS и физический размер независимо от BPB."""
import json, runpy, struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
n=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
Matrix=n['Matrix'];out=ROOT/'docs/flash/detect-001';Matrix.__init__.__globals__['OUT']=out/'filesystem'
(out/'filesystem').mkdir(exist_ok=True)
x=Matrix();results={}
try:
    x.start('two-chip-filesystem',disk=out/'flash-2.bin',chip_count=2)
    x.install('FlashFst')
    x.m.write(0xF100,(ROOT/'bin/FORMAT.i80').read_bytes());x.m.write(0x8400,b'H: -Y\r\0')
    x.m.write(0x8FF9,b'\0')
    assert not x.func(0xF100,de=0x8400)['flags']['C']
    x.select(7)
    image=n['RUNTIME']/'specialist-os/flash-disk.bin'
    before=image.read_bytes(); assert len(before)==1048576 and struct.unpack_from('<H',before,19)[0]==4096
    payload=bytes((i*17+(i>>8))&255 for i in range(4096))
    descriptor=b'IDTEST  TMP'+bytes(7)+bytes(2)+bytes(8)+struct.pack('<I',len(payload))
    assert len(descriptor)==32
    x.m.write(0x6000,descriptor);x.m.write(0x2000,payload)
    assert not x.func(0xC845,hl=0x6000,de=0x2000)['flags']['C']
    x.m.write(0x3000,bytes(4096))
    assert not x.func(0xC866,hl=0x6000,de=0x3000)['flags']['C']
    assert x.m.read(0x3000,4096)==payload
    # Фактическая FAT-цепочка должна использовать оба чипа.
    actual=image.read_bytes();fat=struct.unpack_from('<H',actual,14)[0]*256
    root=fat+actual[16]*struct.unpack_from('<H',actual,22)[0]*256
    entry=next(actual[i:i+32] for i in range(root,root+2048,32) if actual[i:i+11]==b'IDTEST  TMP')
    cluster=struct.unpack_from('<H',entry,26)[0];data_start=root//256+8;chips=set()
    for _ in range(16):
        chips.add((data_start+cluster-2)//2048)
        cluster=struct.unpack_from('<H',actual,fat+cluster*2)[0]
    assert chips=={0,1}
    retained=out/'two-chip-file.bin';retained.write_bytes(actual)
    x.start('two-chip-restart',disk=retained,chip_count=2)
    assert [int.from_bytes(x.m.read(x.ds['v_drives']+d*2,2),'little') for d in [0,7]]==[SYSTEM['SYSTEM_FLASH_ADDR'],SYSTEM['DISK_DRIVER_ADDR']]
    x.select(7)                         # После AUTOEXEC здесь обычный FLASH.
    x.m.write(0x6000,descriptor);x.m.write(0x3000,bytes(4096))
    assert not x.func(0xC866,hl=0x6000,de=0x3000)['flags']['C']
    assert x.m.read(0x3000,4096)==payload
    x.install('FlashFst');x.select(7)
    assert not x.func(0xC84B,hl=0x6000)['flags']['C']
    assert image.read_bytes()[:root]==before[:root]
    results['twoChipDosFile']=dict(bytes=4096,readback=True,afterProcessRestart=True,deleted=True,fatRestored=True,chips=sorted(chips))
    # Физический чип 512 КиБ, BPB тома 128 КиБ: запись всё равно страницами 256.
    system=bytearray((ROOT/'images/SystemRom_512K.bin').read_bytes())
    struct.pack_into('<H',system,19,512)
    path=out/'physical512-volume128.bin';path.write_bytes(system)
    x.start('physical-vs-bpb',system=path)
    x.select(0);r=x.func(0xC863,bc=0x0300);assert r['registers']['de']['dec']==2048
    image=n['RUNTIME']/'specialist-os/system-flash.bin';before=image.read_bytes()
    payload=bytes((i*71+0x23)&255 for i in range(256));x.m.write(0x4001,payload)
    assert not x.func(0xC863,bc=0x0500,de=2047,hl=0x4001)['flags']['C']
    assert image.read_bytes()[:-256]==before[:-256] and image.read_bytes()[-256:]==payload
    results['bpbIndependent']=dict(physicalKiB=512,volumeKiB=128,physicalPageBytes=256,fullPageWrite=True)
    (out/'filesystem/results.json').write_text(json.dumps(results,indent=2)+'\n')
    print(results)
finally:x.stop()
