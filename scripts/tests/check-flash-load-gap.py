"""Actual BIOS memcpy load gaps for the MAIN AT29 path (8080, 2 MHz)."""
import re,json
from pathlib import Path
R=Path(__file__).resolve().parents[2]
s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)',(R/'bin/System.linked.sym').read_text(),re.M)}
a=(R/'bin/BIOS.sys').read_bytes()
def code(start,end):return a[s[start]-0xc000:s[end]-0xc000]
# Each pair: POP DE, MOV M,E, INX H, MOV M,D, INX H.
pair=bytes.fromhex('d1 73 23 72 23')
assert code('moveForwardBlock','movePairsBack')[:40]==pair*8
assert code('movePairForwardLoop','moveForwardPageCheck')==pair+bytes.fromhex('0d 0d c2')+s['movePairForwardLoop'].to_bytes(2,'little')
# Between end of a second store and next first store: INX H + loop + POP + MOV.
full_gap=5+5+10+10+7
half_gap=5+5+5+10+10+7
assert max(full_gap,half_gap)<300
print('FLASH LOAD GAP:',json.dumps(dict(cpuHz=2000000,tBLCus=150,fullPageGapUs=full_gap/2,halfPageGapUs=half_gap/2)))
