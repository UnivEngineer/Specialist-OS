"""Видимый собственный Emu80: NC/E, защита DOSMOD, поток >64 КБ и копирование."""
from pathlib import Path
import argparse,hashlib,json,runpy

R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True);p.add_argument('--report-directory',type=Path,default=R/'docs/file-api-001/emulator');a=p.parse_args()
o=a.report_directory.resolve();o.mkdir(parents=True,exist_ok=True)
h=runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'));g=h['Matrix'].__init__.__globals__;g['OUT']=o;g['RUNTIME']=a.runtime.resolve()
m=h['Matrix']();s=h['symbols'](R/'bin/DOS.sym');e=h['symbols'](R/'bin/E.sym');report={}
def tap(key):
 m.m.call('kbd_send',keys=[key],action='tap',duration_ms=100);m.m.call('emu_run_for',ms=120)
def ok(address,**regs):
 state=m.func(address,**regs)
 if state['flags']['C']:
  print('volume',m.m.read(s['v_volumeTable'],2).hex(),'expected',hex(s['fatVolumeDriver']+s['VOLUME_DRIVER_HEADER']),flush=True)
 assert not state['flags']['C'],state
 return state
def handle(op):return ok(s['bios_fileHandle'],af=op<<8,hl=0x7000)
def open_file(name,mode):
 f=bytearray(64);f[:11]=name;f[40:42]=bytes([1,mode]);m.m.write(0x7000,f);handle(0)
def io(op,count,data=None):
 if data is not None:m.m.write(0x4000,data)
 m.m.write(0x7000+49,(0x4000).to_bytes(2,'little')+count.to_bytes(2,'little'));handle(op)
 assert int.from_bytes(m.m.read(0x7000+51,2),'little')==count
 return m.m.read(0x4000,count)
try:
 m.start('nc-editor-module')
 before=m.m.read(0x8000,0xC00)
 m.m.call('bp_clear');m.m.call('emu_run_for',ms=500);tap('esc');m.m.call('emu_run_for',ms=250)
 command='A:E.COM A:README.TXT';keys={' ':'space',':':'colon','.':'period'}
 for ch in command:tap(keys.get(ch,ch.lower()))
 assert m.m.read(s['CMD_LINE'],len(command)).upper()==command.encode()
 m.m.screenshot(o/'nc-command.png');tap('enter');m.m.call('emu_run_for',ms=1500);m.m.call('debug_break')
 assert m.m.read(e['v_textLimitLo'],2)==bytes.fromhex('00 80')
 assert m.m.read(0x8000,0xC00)==before
 m.m.screenshot(o/'editor.png');report['editor']=dict(textLimit=0x8000,modulePreserved=True,command=command)
 print('NC keyboard -> E: module preserved',flush=True)
 m.start('file-module-abi')
 protected=m.m.read(0x8000,0xC00);m.install('FlashSys');assert m.m.read(0x8000,0xC00)==protected
 m.format_ram()
 data=bytes((i*29+(i>>8))&255 for i in range(70001));name=b'FHUGE   BIN';copy=b'FCOPY   BIN'
 open_file(name,15)
 for at in range(0,len(data),8192):chunk=data[at:at+8192];io(2,len(chunk),chunk)
 assert int.from_bytes(m.m.read(0x7020,4),'little')==len(data);handle(5)
 open_file(name,1)
 for at in range(0,len(data),8192):assert io(1,min(8192,len(data)-at))==data[at:at+8192]
 handle(5)
 req=bytearray(s['FILE_COPY_REQUEST']);req[:4]=(0x4000).to_bytes(2,'little')+(8192).to_bytes(2,'little');req[4:6]=bytes([1,1]);req[6:17]=name;req[38:49]=copy;m.m.write(0x7200,req)
 ok(s['bios_fileCopy'],hl=0x7200)
 open_file(copy,1)
 for at in range(0,len(data),8192):assert io(1,min(8192,len(data)-at))==data[at:at+8192]
 handle(5)
 report['file']=dict(bytes=len(data),sha256=hashlib.sha256(data).hexdigest(),copyVerified=True)
 print('RAMD FileHandle + copy: 70001 bytes verified',flush=True)
 m.start('diskbench-module-release');m.m.call('bp_clear');m.m.call('emu_run_for',ms=500);tap('esc');m.m.call('emu_run_for',ms=250)
 for ch in 'A:DISKBEN.COM':tap(keys.get(ch,ch.lower()))
 tap('enter');m.m.call('emu_run_for',ms=1000);m.m.call('debug_break')
 assert m.m.read(s['v_dosModuleReady'],1)==bytes([0])
 assert m.m.read(s['v_memTop'],2)==bytes.fromhex('ff 8b')
 m.m.screenshot(o/'diskbench.png');report['diskbench']=dict(moduleReleased=True,memtop=0x8BFF)
 (o/'integration.json').write_text(json.dumps(report,indent=2),encoding='utf-8')
finally:m.stop()
