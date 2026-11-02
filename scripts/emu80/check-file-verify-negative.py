"""Полная проверка файла ловит ошибки всех порций и не перезаписывает оригинал."""
import argparse,json,runpy,struct
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--report-directory',type=Path,default=ROOT/'docs/diskbench/readback-negative-002');a=p.parse_args()
ns=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'));Matrix=ns['Matrix'];out=a.report_directory;out.mkdir(parents=True,exist_ok=True)
if (out/'results.json').exists():raise RuntimeError('Report already exists')
Matrix.__init__.__globals__['OUT']=out
x=Matrix()
try:
 x.start('negative');x.install('FlashFst');x.init_app(7,6);s=x.s;m=x.m
 image=ns['RUNTIME']/'specialist-os/flash-disk.bin';before=image.read_bytes()
 m.write(s['v_pass'],b'\1');x.func(s['fill_payload'],af=0)
 original=m.read(0x2000,8192)
 assert not x.func(0xC845,hl=s['tmpDescriptor'])['flags']['C']
 assert not x.func(s['raw_build_chain'])['flags']['C']
 chain=struct.unpack('<32H',m.read(s['rawSectors'],64))
 def verify(label):
  x.func(s['measure_begin']);result=x.func(s[label]);x.func(s['measure_end']);return result['flags']['C']
 assert not verify('verify_file_data') and not verify('verify_raw_data')
 errors=[]
 for i in [0,11,12,31]:
  sector=bytearray(original[i*256:(i+1)*256]);sector[255]^=1
  m.write(0x6500,sector);assert not x.func(0xC863,bc=0x0100,de=chain[i],hl=0x6500)['flags']['C'];x.func(0xC863,bc=0x0400)
  assert verify('verify_file_data') and verify('verify_raw_data'),i
  assert m.read(0x2000,8192)==original
  m.write(0x6500,original[i*256:(i+1)*256]);assert not x.func(0xC863,bc=0x0100,de=chain[i],hl=0x6500)['flags']['C'];x.func(0xC863,bc=0x0400)
  errors.append(i*256+255)
 # Сравнение HIGH не подменяет оригинал прочитанными данными.
 m.write(0x2000,bytes([original[0]^1]));assert x.func(s['read_high'],bc=0)['flags']['C'];assert m.read(0x2000,1)==bytes([original[0]^1]);m.write(0x2000,original[:1])
 assert not verify('verify_file_data') and not verify('verify_raw_data')
 assert not x.func(0xC84B,hl=s['tmpDescriptor'])['flags']['C']
 x.check_integrity(before,image.read_bytes())
 result={'caughtCorruptByteOffsets':errors,'fileAndRawChecks':True,'originalBufferPreserved':True,'temporaryFileRemoved':True,'metadataPreserved':True}
 (out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result)
finally:x.stop()
