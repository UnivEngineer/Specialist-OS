"""Сравнение копии Monitor и Opt с настоящим BIOS: все печатные коды и фазы."""
from pathlib import Path
import argparse
import json
import re
import subprocess

R = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--out', type=Path, default=R/'docs/bench/engines/monitor')
O = p.parse_args().out.resolve()
O.mkdir(parents=True, exist_ok=True)
s = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (R/'bin/FONTLAB.sym').read_text(), re.M)}
word = lambda x: x.to_bytes(2, 'little')
base = bytearray(65536)
for file, address in [('FONTCOM.i80', 0), ('BIOS.sys', 0xC000), ('IO.sys', 0xF000)]:
    data = (R/'bin'/file).read_bytes()
    base[address:address+len(data)] = data
base[0x9000:0xC000] = bytes((i*37+(i>>8)*13)&255 for i in range(0x3000))

def run(entry, phase, inverse, opt=False):
    m = bytearray(base)
    for field, data in [('charGen', word(0x2800)), ('koi7', b'\0'), ('escMode', b'\xff'), ('inverse', bytes([inverse]))]:
        a = s[('bios_vars.' if field == 'inverse' else 'bios_upper.')+field]
        m[a:a+len(data)] = data
    if opt:
        a = s['monitor.drawEntry']
        m[a:a+3] = b'\xc3'+word(s['legacyOpt.optimizedDraw'])
    code = b'\x31'+word(0x8FE0)
    for i, c in enumerate(range(32, 256)):
        cursor = ((i%32)*3+phase)*256 + 7+(i//32)*8
        code += b'\x21'+word(cursor)+b'\x22'+word(s['bios_vars.cursorY'])
        code += bytes([0x0E, c, 0xCD])+word(entry)
    code += b'\xc3'+word(0x8E80)
    assert 0x7800+len(code) < 0x8E00
    m[0x7800:0x7800+len(code)] = code
    (O/'input.bin').write_bytes(m)
    subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe', '-m8080', '-pc', '7800', '-end', '8e80', '-counter', '20000000', '-output', str(O/'output.bin'), str(O/'input.bin')], capture_output=True, check=True, timeout=20)
    return (O/'output.bin').read_bytes()

checks = 0
for inverse in (0, 1, 255):
    for phase in range(4):
        reference = run(s['bios_printChar'], phase, inverse)
        for opt in (False, True):
            actual = run(s['monitor.scalar'], phase, inverse, opt)
            assert actual[0x9000:0xC000] == reference[0x9000:0xC000], (phase, inverse, opt)
            a = s['bios_vars.cursorY']
            assert actual[a:a+2] == reference[a:a+2], (phase, inverse, opt, 'cursor')
            checks += 224
(O/'results.json').write_text(json.dumps(dict(checks=checks, monitor='PASS', optimized='PASS', reference='BIOS.sys + IO.sys', printableCodes=224, phases=4, inverseModes=3), indent=2)+'\n')
print('MONITOR versus BIOS CPU PASS', checks)
