"""GET32/PUT32: real 8080 MEM code, register/range/profile contracts.
Flat RAM validates CPU logic; physical pages are covered by Emu80 separately.
"""
import re,struct,runpy,subprocess
from pathlib import Path
R=Path(__file__).resolve().parents[2];O=R/'docs/diskbench/dos-overhead-2026-11-10/dword-cpu';O.mkdir(parents=True,exist_ok=True)
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/BIOS.sym').read_text(),re.M)}
base=bytearray(65536);runpy.run_path(str(R/'scripts/tests/mem-image.py'))['install'](base,s)
W=lambda v:struct.pack('<H',v)
count=0
for caller in (0,1,4,16):
 for target in (0,1,3,16,17,255):
  for address in (0x4001,0xfefc,0xfefd,0xfffc,0xffff):
   for put in (False,True):
    m=bytearray(base);m[s['bankPage']]=caller;m[s['bankPageCount']]=16
    if caller==0:
     # MAIN does not install the BANK local-I/O tail: a disk driver lives there.
     client={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/banked/CLIENT.sym').read_text(),re.M)}
     at=client['memPutDwordTemplate'];m[at:at+27]=bytes([0xFF])*27
    valid=target<=16 and address<=0xfefc
    if address==0x4001:m[address-1:address+5]=bytes.fromhex('aa 12 34 56 78 bb')
    elif address==0xfefc:m[address:address+4]=bytes.fromhex('12345678')
    before=bytes(m[address:min(address+4,65536)])
    entry=s['mx_put32' if put else 'mx_get32']
    stub=b'\x31'+W(0x8f00)+b'\x3e'+bytes([target])+b'\x01'+W(0xabcd)+b'\x11'+W(0xef01)+b'\x21'+W(address)+b'\xcd'+W(entry)
    stub+=b'\x22'+W(0x8302)+b'\xeb\x22'+W(0x8304)+b'\x60\x69\x22'+W(0x8306)+b'\xf5\xe1\x22'+W(0x8300)+b'\xc3'+W(0x82ff)
    m[0x8200:0x8200+len(stub)]=stub;(O/'in.bin').write_bytes(m)
    subprocess.run([r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','82ff','-counter','1000000','-output',str(O/'out.bin'),str(O/'in.bin')],check=True,capture_output=True)
    out=(O/'out.bin').read_bytes();assert int.from_bytes(out[65542:65544],'little')==0x82ff,(caller,target,address,put,'return')
    assert bool(out[0x8300]&1)==(not valid),(caller,target,address,put,'CF')
    assert int.from_bytes(out[0x8302:0x8304],'little')==address
    assert int.from_bytes(out[65544:65546],'little')==0x8f00,'SP'
    if valid:
     assert out[0x8301]==0
     if put:assert out[address:address+4]==bytes.fromhex('01efcdab')
     else:assert out[0x8304:0x8308]==bytes.fromhex('12345678')
     assert out[s['BANK_PROFILE']]==s['BANK_PROFILE_COPY']
     assert out[s['bankCopySavedWindow']:s['bankCopySavedWindow']+32]==base[s['BANK_WINDOW']:s['BANK_WINDOW']+32]
    elif address<0xffc0:assert out[address:address+4]==before
    if put or not valid:assert out[0x8304:0x8308]==bytes.fromhex('01efcdab')
    if address==0x4001:assert out[address-1]==0xaa and out[address+4]==0xbb
    count+=1
print('DWORD PASS:',count,'cases; registers, little endian, unaligned/end/range/bank errors, SP, COPY profile, COMMAND snapshot')

