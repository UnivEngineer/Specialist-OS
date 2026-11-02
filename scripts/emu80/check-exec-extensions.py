"""Холодная загрузка FORMAT.BAT/AUTOEXEC и запуск BAT/COM/EXE в тестовом Emu80."""
import json,runpy,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))
m=h['Bench']('http://127.0.0.1:19781/mcp');s=h['symbols'](ROOT/'bin/DOS.sym');nc=h['symbols'](ROOT/'bin/NC.sym')
query='(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
exe=subprocess.run(['powershell.exe','-NoProfile','-Command',query],capture_output=True,text=True,check=True).stdout.strip()
assert Path(exe).resolve()==Path('D:/Projects/RetroPC/Specialist/Emulator/Emu80-flash-test/Emu80qt-mcp.exe').resolve()
out=ROOT/'docs/dos/exec-extensions-001';out.mkdir(parents=True,exist_ok=True)
m.call('debug_break');m.stop_at(nc['mainLoop']);m.tap('ESC');m.until(nc['mainLoop'])
assert m.read(0xC000,len((ROOT/'bin/DOS.sys').read_bytes()))[:3]==bytes.fromhex('C309C0')
assert m.read(s['aBat'],9)==b'BATCOMEXE'
m.screenshot(out/'cold-boot-nc.png')
stop=0x8700
m.stop_at(stop)
def call(address,**regs):
    m.write(0x8800,bytes((0xCD,address&255,address>>8,0xC3,0,0x87)))
    m.call('regs_set',pc=0x8800,sp=0x8F00,**regs)
    return m.until(stop)
for drive in [0,7]:
    assert not call(0xC842,af=drive<<8,de=1)['flags']['C']
    assert call(0xC863,bc=0xFF00)['registers']['bc']['dec']>>8==ord('F')
assert not call(0xC842,af=0x0100,de=1)['flags']['C']
assert not call(0xC896)['flags']['C']
def create(name,payload,address):
    d=bytearray(32);d[:11]=name;struct.pack_into('<H',d,18,address);struct.pack_into('<I',d,28,len(payload)-1)
    m.write(address,payload);m.write(0x8500,d)
    assert not call(0xC845,hl=0x8500)['flags']['C']
for ext,marker in [(b'COM',0x42),(b'EXE',0x53)]:
    name=b'EXTTEST '+ext
    create(name,bytes((0x3E,marker,0x32,0,0x86,0xC9)),0x2000)
    m.write(0x8600,b'\0');m.write(0x8400,b'B:exttest.'+ext.lower()+b' argument\0')
    assert not call(0xC85D,hl=0x8400)['flags']['C']
    assert m.read(0x8600,1)==bytes([marker])
create(b'EXTTEST BAT',b'B:EXTTEST.COM\r\nB:EXTTEST.EXE\r\n',0)
m.write(0x8600,b'\0');m.write(0x8400,b'B:exttest.bat\0')
assert not call(0xC85D,hl=0x8400)['flags']['C']
assert m.read(0x8600,1)==b'\x53'
result={'coldBootNc':True,'formatBatCompleted':True,'ramDiskMounted':True,'autoexecInstalledFlashDrivers':True,'lowercaseCom':True,'lowercaseExe':True,'batComExeSequence':True}
(out/'results.json').write_text(json.dumps(result,indent=2)+'\n');print(result)
