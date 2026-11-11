"""Собственный видимый Emu80: вложенные игры H: и FileHandle внутри каталога."""
from pathlib import Path
import argparse,hashlib,json,runpy

R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True);a=p.parse_args()
out=R/'docs/fat/directories-001/emulator';out.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=out;g['RUNTIME']=a.runtime.resolve()
m=h['Matrix']();s=h['symbols'](R/'bin/DOS.sym')
files,dirs=runpy.run_path(str(R/'scripts/tests/check-nested-image.py'))['read_tree']((R/'bin/nested-Games2M.bin').read_bytes())
report={}
def ok(address,**regs):
    state=m.func(address,**regs)
    assert not state['flags']['C'],(address,state)
    return state
def cd(path):
    raw=path.encode();m.m.write(0x7400,raw+bytes(128-len(raw)))
    ok(s['bios_directory'],af=2<<8,hl=0x7400)
def current():return ok(s['bios_directory'],af=0)['registers']['de']['dec']
def handle(op):return ok(s['bios_fileHandle'],af=op<<8,hl=0x7000)
def open_file(name,mode):
    base,_,ext=name.partition('.');f=bytearray(64);f[:11]=(base.ljust(8)+ext.ljust(3)).encode();f[40:42]=bytes([7,mode]);m.m.write(0x7000,f);handle(0)
def transfer(op,count,payload=None):
    if payload is not None:m.m.write(0x4000,payload)
    m.m.write(0x7031,(0x4000).to_bytes(2,'little')+count.to_bytes(2,'little'));handle(op)
    assert int.from_bytes(m.m.read(0x7033,2),'little')==count
    return m.m.read(0x4000,count)
try:
    m.start('nested-games',disk=R/'bin/nested-Games2M.bin');m.select(7)
    for path,payload in files.items():
        if not path.startswith('Almaz/'):continue
        parent,name=path.rsplit('/',1);cd('/'+parent)
        assert current()==dirs[parent]
        ok(s['bios_directory'],af=4<<8,hl=0x7500)
        assert m.m.read(0x7500,128).split(b'\0')[0].upper()==('H:'+chr(92)+parent.replace('/',chr(92))).encode().upper()
        open_file(name,1);actual=bytearray()
        for offset in range(0,len(payload),8192):actual+=transfer(1,min(8192,len(payload)-offset))
        handle(5);assert bytes(actual)==payload,path
        report[path]=dict(bytes=len(payload),sha256=hashlib.sha256(payload).hexdigest())
    cd('/');m.m.write(0x7400,b'H:/Almaz/STD/Almaz2.rks /Q\0')
    ok(s['bios_fileNamePrepare'],hl=0x7400,de=0x7200)
    ok(s['bios_fileLoadInfo'],hl=0x7200)
    assert int.from_bytes(m.m.read(0x721C,4),'little')==len(files['Almaz/STD/Almaz2.rks'])
    assert current()==0
    cd('/Almaz/MXOS');saved=current();m.select(0);m.select(7);assert current()==saved
    m.m.write(0x7600,b'RKS')
    state=ok(s['bios_fileAssociation'],hl=0x7600,de=0x4000,bc=8192)
    assert m.m.read(state['registers']['hl']['dec'],11)==b'A:\\RUN.COM\r'
    assert current()==saved
    payload=b'NESTED FILE WRITE\r\n';open_file('DIRTEST.TXT',15);transfer(2,len(payload),payload);cd('/');handle(5)
    cd('/');cd('Almaz/MXOS');open_file('DIRTEST.TXT',1);assert transfer(1,len(payload))==payload;handle(5)
    m.m.write(0x7200,b'DIRTEST TXT');ok(s['bios_fileDelete'],hl=0x7200)
    # Две папки одного диска: контексты в расширенном запросе копирования.
    q=bytearray(84);q[:4]=(0x4000).to_bytes(2,'little')+(8192).to_bytes(2,'little');q[4:6]=bytes([7,7])
    q[6:17]=b'Almaz2  rks';q[38:49]=b'COPY    RKS'
    q[80:82]=dirs['Almaz/STD'].to_bytes(2,'little');q[82:84]=dirs['Almaz/MXOS'].to_bytes(2,'little')
    m.m.write(0x7000,q);ok(s['bios_fileCopy'],hl=0x7000)
    cd('/Almaz/MXOS');open_file('COPY.RKS',1);copied=bytearray();expected=files['Almaz/STD/Almaz2.rks']
    for offset in range(0,len(expected),8192):copied+=transfer(1,min(8192,len(expected)-offset))
    handle(5);assert bytes(copied)==expected
    m.m.write(0x7200,b'COPY    RKS');ok(s['bios_fileDelete'],hl=0x7200)
    report['sameDriveDirectoryCopy']=True
    cd('/Almaz/MXOS');payload=b'CD /Almaz/STD\r\nCHDIR ../MXOS\r\n';open_file('CDTEST.BAT',15);transfer(2,len(payload),payload);handle(5)
    cd('/');m.m.write(0x7400,b'H:/Almaz/MXOS/CDTEST.BAT\0');ok(s['bios_fileExec'],hl=0x7400)
    assert current()==dirs['Almaz/MXOS']
    m.m.write(0x7200,b'CDTEST  BAT');ok(s['bios_fileDelete'],hl=0x7200)
    report['nestedBatCd']=True
    report['writeReadDelete']=True;report['perDriveDirectoryPreserved']=True
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('NESTED EMU80 PASS',len(files),'game files; nested read/write/copy and BAT CD')
finally:m.stop()
