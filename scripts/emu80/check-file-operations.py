"""Общий файловый ABI DOS в собственном видимом Flash512 Emu80.

Вызовы ABI проверяются отдельно от клавиатурных сценариев NC.
Запуск и закрытие собственного экземпляра выполняет вызывающий сценарий.
"""
import argparse,hashlib,json,re,runpy,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
Machine=runpy.run_path(str(ROOT/'scripts/emu80/check-copy.py'))['FileMachine']
get_file=runpy.run_path(str(ROOT/'scripts/emu80/check-copy.py'))['get_file']

def main():
 p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True);p.add_argument('--directions-only',action='store_true');p.add_argument('--report-directory',type=Path,default=ROOT/'docs/dos/file-ops-004');a=p.parse_args();runtime=a.runtime.resolve();out=a.report_directory.resolve();out.mkdir(parents=True,exist_ok=True)
 query='(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
 owner=subprocess.check_output(['powershell.exe','-NoProfile','-Command',query],text=True).strip();assert Path(owner).resolve()==runtime/'Emu80qt-mcp.exe'
 def syms(name):return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin'/name).read_text(),re.M)}
 ds,ns=syms('DOS.sym'),syms('NC.sym');m=Machine('http://127.0.0.1:19781/mcp');m.call('bp_clear');m.call('emu_run_for',ms=5000);m.call('debug_break');assert m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]
 rows=[];image=runtime/'specialist-os/flash-disk.bin'
 loaded=get_file((runtime/'specialist-os/system-flash.bin').read_bytes(),b'DOS     SYS')[1]
 (out/'tested-DOS.sys').write_bytes(loaded)
 def record(**row):rows.append(row);(out/'abi.json').write_text(json.dumps(rows,indent=2)+'\n');print(row,flush=True)
 def sub(label,**regs):
  m.call('bp_clear');m.call('bp_set',addr=0x8100,type='exec');return m.subroutine(ds[label] if isinstance(label,str) else label,**regs)
 def ok(label,**regs):
  s=sub(label,**regs);assert not s['flags']['C'],(label,s);return s
 def drive(n):ok('bios_fileSelectDrive',af=n<<8)
 def current():return sub('bios_fileGetSetDrive',de=0)['registers']['af']['dec']>>8
 def create(name,data):
  drive(1);d=bytearray(32);d[:11]=name;d[28:32]=len(data).to_bytes(4,'little');m.write(0x7500,d)
  ok('bios_fileCreatePrepare',hl=0x7500,de=(len(data)+255)//256)
  m.write(0x7540,m.read(0x751A,2)+bytes(2))
  for at in range(0,len(data),8192):
   chunk=data[at:at+8192];m.write(0x4000,chunk)
   ok('bios_fileStream',hl=0x4000,de=0x7540,bc=len(chunk),af=0x0100)
  ok('bios_flushVolume')
 def request(source,dest,size=2065,buffer=0x4001,source_drive=1,dest_drive=7):
  req=bytearray(ds['FILE_COPY_REQUEST']);req[:4]=buffer.to_bytes(2,'little')+size.to_bytes(2,'little');req[4:6]=bytes((source_drive,dest_drive));req[6:17]=source;req[38:49]=dest;m.write(0x7200,req)
  m.write(buffer-1,bytes([0xA5])*(size+2))
 def copy(expected=0):
  before=current();s=sub('bios_fileCopy',hl=0x7200);assert s['flags']['C']==bool(expected) and s['registers']['af']['dec']>>8==expected,s
  assert current()==before
  req=m.read(0x7200,4);buf=int.from_bytes(req[:2],'little');size=int.from_bytes(req[2:],'little');assert m.read(buf-1,1)==b'\xA5' and m.read(buf+size,1)==b'\xA5'
  if size%256:assert m.read(buf+size//256*256,size%256)==bytes([0xA5])*(size%256)
 def load(name,n):
  m.write(0x7500,name+bytes(21));m.write(0x5000,bytes((n+255)//256*256));ok('bios_fileLoad2',hl=0x7500,de=0x5000);return m.read(0x5000,n)
 def absent(name):
  try:get_file(image.read_bytes(),name)
  except StopIteration:return
  raise AssertionError('Unexpected destination '+repr(name))
 def directions():
  data=bytes((i*19+3)&255 for i in range(513));source=b'DIRSRC  BIN';create(source,data)
  request(source,b'DIRRAM  BIN',size=513,dest_drive=1);copy();assert load(b'DIRRAM  BIN',513)==data
  record(case='same-drive copy B to B',bytes=513,passed=True)
  for i in range(1000):
   name=f'QR{i:06}BIN'.encode()
   try:get_file(image.read_bytes(),name)
   except StopIteration:break
  else:raise AssertionError('No test name')
  request(source,name,size=513);copy();assert get_file(image.read_bytes(),name)[1]==data
  request(name,b'DIRBACK BIN',size=513,source_drive=7,dest_drive=1);copy();assert load(b'DIRBACK BIN',513)==data
  record(case='copy H to B',bytes=513,passed=True)
 try:
  drive(1);m.write(0x7500,bytes([0xA5])*10);s=ok('bios_fileGetDrives',hl=0x7501,bc=8);n=s['registers']['bc']['dec']>>8;drives=list(m.read(0x7501,n));assert drives==[0,1,7] and current()==1 and m.read(0x7500,1)==m.read(0x7509,1)==b'\xA5';record(case='drive list',drives=drives,original_drive_preserved=True)
  if a.directions_only:
   directions();print('FILE OPERATIONS DIRECTIONS PASS',flush=True);return
  for i,n in enumerate([0,1,255,256,257,513,65536,70001]):
   source=f'S{i:07}BIN'.encode();dest=f'D{i:07}BIN'.encode();data=bytes((j*37+i)&255 for j in range(n));create(source,data);request(source,dest,size=513 if n<1024 else 2065);copy();entry,actual=get_file(image.read_bytes(),dest);assert actual==data;record(case='copy',bytes=n,sha256=hashlib.sha256(actual).hexdigest(),guard_preserved=True,disk_preserved=True)
  directions()
  source=b'S0000007BIN';dest=b'D0000007BIN';old=get_file(image.read_bytes(),dest)[1];request(source,dest);copy(2);assert get_file(image.read_bytes(),dest)[1]==old;record(case='existing destination',unchanged=True)
  request(source,source,dest_drive=1);copy(2);record(case='same source and destination',rejected=True)
  request(b'NOFILE  BIN',b'MISSING BIN');copy(1);absent(b'MISSING BIN');record(case='missing source',no_destination=True)
  request(source,b'ABSENT  BIN',dest_drive=2);copy(1);record(case='absent drive',rejected=True)
  # Ошибка записи после создания защищённой копии: очистка должна удалить её.
  m.write(0x7500,source+bytes(21));ok('bios_fileGetSetAttr',hl=0x7500,af=0x0000,bc=0x0501)
  request(source,b'PARTIAL BIN');entry=ds['fatStream'];old_code=m.read(entry,3);assert old_code[0]==0x32
  stub=bytes.fromhex('F5 FE 01 C2 09 88 F1 37 C9 F1')+old_code+b'\xC3'+(entry+3).to_bytes(2,'little');m.write(0x8800,stub)
  try:
   m.write(entry,bytes.fromhex('C3 00 88'));copy(1)
  finally:m.write(entry,old_code)
  absent(b'PARTIAL BIN');record(case='write error after create',protected_partial_removed=True)
  request(source,b'PROTECT BIN');copy();assert get_file(image.read_bytes(),b'PROTECT BIN')[0][11]&5==5
  drive(7);m.write(0x7500,b'PROTECT BIN'+bytes(21));assert sub('bios_fileDeleteOnDrive',hl=0x7500)['flags']['C'];record(case='protected deletion',rejected=True)
  m.write(0x7500,b'D0000000BIN'+bytes(21));m.write(0x7540,b'RENAMED BIN'+bytes(21));ok('bios_fileRenameOnDrive',hl=0x7500,de=0x7540);get_file(image.read_bytes(),b'RENAMED BIN');m.write(0x7500,b'RENAMED BIN'+bytes(21));ok('bios_fileDeleteOnDrive',hl=0x7500);absent(b'RENAMED BIN');record(case='rename and delete',passed=True)
  m.write(0x7500,b'BIGSRC  BIN'+bytes(21));drive(1)
  # Большой исходник для отдельной проверки F5/F6/F8 через NC.
  data=bytes((j*29+7)&255 for j in range(70001));create(b'BIGSRC  BIN',data);(out/'payload.bin').write_bytes(data)
 finally:
  m.call('bp_clear');m.call('emu_reset');m.call('emu_run_for',ms=5000)
 assert m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]
 print('FILE OPERATIONS EMULATOR PASS',len(rows),flush=True)

if __name__=='__main__':main()
