"""8080 register CHAR/RAW calls after COPY: no TEXT profile or byte staging."""
from pathlib import Path
import re,runpy,struct,subprocess,json
R=Path(__file__).resolve().parents[2];O=R/'docs/mem/text-scalar-2026-11-13';O.mkdir(parents=True,exist_ok=True)
sy=lambda p:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',p.read_text(),re.M)}
b=sy(R/'bin/BIOS.sym');s=sy(R/'bin/banked/LOCALBLOCK.sym');t=sy(R/'bin/banked/TRANSPORT.sym');W=lambda n:struct.pack('<H',n)
base=bytearray(65536);runpy.run_path(str(R/'scripts/tests/mem-image.py'))['install'](base,b)
base[t['bankPage']]=1;base[t['bankPageCount']]=16;base[t['bankServiceSP']:t['bankServiceSP']+2]=W(0xe000)
# Capture the native scalar handler; the 8080 model ignores mapper writes.
# Actual transport, register packets and both distinct stack ranges still run.
rows=[]
for mode,op in ((3,b['MXOS_BIOS_PRINT_CHAR']),(4,b['MXOS_BIOS_PRINT_CHAR_RAW'])):
 for value in (0,9,10,13,27,65,128,255):
  m=bytearray(base);m[0xffdf]=s['BANK_PROFILE_COPY'];m[s['MEM_COMMAND_SHADOW']:s['MEM_COMMAND_SHADOW']+32]=(R/'bin/banked/WINDOW.bin').read_bytes()
  at=b['sysBIOSTable']+(op-0x100)*2;m[at:at+2]=W(0x8000)
  stub=b'\xf5'+(b'\x79' if mode==3 else b'')+b'\x32'+W(0x7500)+b'\xf1\xc9';m[0x8000:0x8000+len(stub)]=stub
  bc=0x1200|(value if mode==3 else 0x4d);aval=value if mode==4 else 0x5a
  code=b'\x31'+W(0x7000)+b'\x01'+W(bc)+b'\x11'+W(0x5678)+b'\x21'+W(0x3000)+b'\x3e'+bytes([aval])+b'\xcd'+W(b['mx_callRegs'])+W(op)
  code+=b'\x22'+W(0x7510)+b'\xeb\x22'+W(0x7512)+b'\x60\x69\x22'+W(0x7514)+b'\x21\0\0\x39\x22'+W(0x7516)+b'\xc3'+W(0x88ff)
  m[0x8800:0x8800+len(code)]=code;(O/'in.bin').write_bytes(m)
  subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8800','-end','88ff','-counter','1000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True,timeout=15)
  out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x88ff
  assert out[0x7500]==value,(mode,value,out[0x7500])
  assert struct.unpack_from('<4H',out,0x7510)==(0x3000,0x5678,bc,0x7000),(mode,value,out[0x7510:0x7518].hex())
  rows.append(dict(mode=mode,value=value))
(O/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
print('MEM COMMAND SCALAR PASS',len(rows),'public CHAR/RAW cases; NUL, controls, high codes, BC/DE/HL/SP, no temporary character buffer')
