"""Существующий TMP и отказ записи: отдельный тестовый Emu80."""
import json, runpy
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))
m=h['Bench']('http://127.0.0.1:19781/mcp');s=h['symbols'](ROOT/'source-apps/diskbench/build/DISKBEN.sym')
# Адрес и профиль те же, что проверены check-diskbench-file.py.
query=__import__('subprocess').run(['powershell.exe','-NoProfile','-Command','(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'],capture_output=True,text=True,check=True).stdout.strip()
assert Path(query).resolve()==Path('D:/Projects/RetroPC/Specialist/Emulator/Emu80-flash-test/Emu80qt-mcp.exe').resolve()
def subroutine(address,**registers):
    m.write(0x8000,bytes((0xCD,address&255,address>>8,0xC3,0,0x81)))
    m.call('regs_set',pc=0x8000,sp=0x8F00,**registers)
    return m.until(0x8100)
m.subroutine=subroutine
m.call('debug_break');m.stop_at(0x8100)
m.subroutine(0xC842,af=0x0700,de=1)
image=Path(query).parent/'specialist-os/flash-disk.bin'
def setup():
    m.write(0,(ROOT/'bin/DISKBEN.i80').read_bytes())
    m.stop_at(0x8100)
    m.subroutine(s['detect_driver'])
    m.write(s['v_drive'],b'\7');m.write(s['v_budget'],b'\1');m.write(s['v_case'],b'\2')
    m.write(s['v_capacity'],(8192).to_bytes(2,'little'))
    m.subroutine(0xC863,bc=0x0200,de=0,hl=0x2000)
    assert not m.subroutine(s['geometry'])['flags']['C']
    m.subroutine(0xC8A2)
setup()
m.write(0x2000,b'\x3C'*8192)
assert not m.subroutine(0xC845,hl=s['tmpDescriptor'],de=0x2000)['flags']['C']
before=image.read_bytes();original=m.read(0xC864,2)
m.stop_at(s['exit_wait']);m.call('regs_set',pc=s['file_write_case'],sp=0x8FDF);m.until(s['exit_wait'])
assert m.read(s['v_fileOwned'],1)==b'\0'
assert image.read_bytes()==before and m.read(0xC864,2)==original
m.stop_at(0x8100)
assert not m.subroutine(0xC866,hl=s['tmpDescriptor'],de=0x2000)['flags']['C']
assert m.read(0x2000,8192)==b'\x3C'*8192
assert not m.subroutine(0xC84B,hl=s['tmpDescriptor'])['flags']['C']
setup();before=image.read_bytes();original=m.read(0xC864,2)
# B=1 отклоняется; все прочие функции передаются настоящему драйверу.
# MOV A,B / CPI 1 / JNZ 6008 / STC / RET / JMP original.
m.write(0x6000,bytes.fromhex('78FE01C2086037C9C3')+original)
m.write(0xC864,(0x6000).to_bytes(2,'little'))
m.stop_at(s['exit_wait']);m.call('regs_set',pc=s['file_write_case'],sp=0x8FDF);m.until(s['exit_wait'])
assert m.read(0xC864,2)==(0x6000).to_bytes(2,'little')
assert image.read_bytes()==before
m.write(0xC864,original)
result={'existingTmpPreserved':True,'writeFailureReported':True,'driverPointerRestored':True,'failedWritesChangedNoSectors':True}
(ROOT/'docs/diskbench/file-003/safety.json').write_text(json.dumps(result,indent=2)+'\n')
print(result)
