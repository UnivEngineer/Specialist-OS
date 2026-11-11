"""Проверка сохранённого формата MXFN; штатный BIOS использует полный FONT.FNT."""
from pathlib import Path
import runpy
R=Path(__file__).resolve().parents[2]
api=runpy.run_path(str(R/'scripts/build/font-format.py'))
compact=(R/'assets/system-rom/FONTKOI.FNT').read_bytes()
info=api['parse'](compact)
font=(R/'assets/system-rom/FONT.FNT').read_bytes()
checks=0
for c in range(256):
    at=info['ranges'][c//32]
    if at:
        at+=(c%32)*8
    elif c in (0xA3,0xB3):
        at=info['yo'][c==0xB3]
    if at:
        assert compact[at:at+8] == font[c*8:c*8+8],c
        checks+=1
for payload in (font,font[:1024]):
    assert api['parse'](api['wrap'](payload))['size']==len(payload)+32
print('MXFN asset format PASS',checks)
