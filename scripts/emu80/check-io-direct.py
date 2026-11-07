"""Прямой обмен A: <-> банк DOS с сохранением окна и восстановлением носителя."""
from pathlib import Path
import runpy,json,argparse
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser();p.add_argument('--chip-kib',type=int,choices=(128,256,512),default=512);p.add_argument('--locked',action='store_true');args=p.parse_args()
OUT=ROOT/f'docs/io-043/direct-{args.chip_kib}{"-locked" if args.locked else ""}'
OUT.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=OUT,RUNTIME=ROOT/'.local/monitor2-001')
runtime=ROOT/'.local/monitor2-001'
if args.chip_kib!=512 and b'System flash image must be 524288 bytes' in (runtime/'Emu80qt-mcp.exe').read_bytes():
    raise SystemExit('This Emu80 build only supports 512 KiB system flash; 128/256 KiB require an updated model.')
x=h['Matrix']();m=x.m;rows=[]
k=h['symbols'](ROOT/'bin/banked/KERNEL.sym');t=h['symbols'](ROOT/'bin/banked/TRANSPORT.sym')
w=lambda n:n.to_bytes(2,'little')
def page(p):m.write(0xFFFD if p else 0xFFFC,bytes([p-1 if p else 0]))
def transfer(mode,block,payload=None,error=None):
    page(1)
    if payload is not None:m.write(0x6000,payload)
    m.write(0x9000,b'\x01'+w(mode<<8)+b'\x11'+w(block)+b'\x21'+w(0x6000)+b'\xcd'+w(k['dosSystemDriver'])+b'\xc9')
    page(0);window=m.read(0xFFC0,32)
    state=x.func(t['bankCall'],af=0x0100,hl=0x9000,bc=0,de=0)
    assert m.read(0xFFC0,12)==window[:12],'Shared selectors damaged'
    if error is not None:
        assert state['flags']['C'] and state['registers']['af']['dec']>>8==error,(mode,block,state)
        return None
    assert not state['flags']['C'],(mode,block,state)
    page(1);data=m.read(0x6000,256);page(0)
    return data
try:
    seed=bytearray((ROOT/'images/SystemRom_emu80_writable.bin').read_bytes()[:args.chip_kib*1024]);seed[19:21]=w(args.chip_kib*4)
    image=OUT/'system.bin';image.write_bytes(seed)
    x.start('io-direct',system=image,locked=args.locked)
    for block in (127,128,255,256,args.chip_kib*4-1):
        old=transfer(2,block)
        payload=bytes((i*73+block*19)&255 for i in range(256))
        try:
            if args.locked:
                transfer(1,block,payload,error=k['DOS_ERR_IO'])
                assert transfer(2,block)==old
                rows.append(dict(block=block,protected=True));continue
            transfer(1,block,payload)
            got=transfer(2,block)
            if got!=payload:
                (OUT/'got.bin').write_bytes(got);(OUT/'expected.bin').write_bytes(payload)
                print('DIFF',[(i,a,b) for i,(a,b) in enumerate(zip(got,payload)) if a!=b][:20],flush=True)
            assert got==payload,block
            rows.append(dict(block=block,read=True,write=True,selectorsPreserved=True))
            print('DIRECT A DOS READ/WRITE',block,'PASS',flush=True)
        finally:
            if not args.locked:transfer(1,block,old)
            assert transfer(2,block)==old,'Restore failed'
    transfer(2,args.chip_kib*4,error=k['DOS_ERR_ARGUMENT'])
    (OUT/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
finally:x.stop()
