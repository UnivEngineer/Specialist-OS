"""Аргументы DISKBEN на CPU 8080 со штатными argNext/upperCase BIOS."""
from pathlib import Path
import argparse
import json
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--ticks', type=Path, default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    parser.add_argument('--report', type=Path, default=ROOT/'docs/diskbench/args-001/model')
    args = parser.parse_args()
    out = args.report.resolve()
    out.mkdir(parents=True, exist_ok=True)
    sym = {k: int(v, 16) for k, v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)', (ROOT/'source-apps/diskbench/build/DISKBEN.sym').read_text(), re.M)}
    base = bytearray(65536)
    code = (ROOT/'bin/DISKBEN.i80').read_bytes()
    base[:len(code)] = code
    bios = (ROOT/'bin/BIOS.sys').read_bytes()
    base[0xC000:0xC000+len(bios)] = bios
    io = (ROOT/'bin/IO.sys').read_bytes();base[0xF000:0xF000+len(io)] = io
    word = lambda v: struct.pack('<H', v)

    def run(label, text='', mode=0, caps=0):
        mem = bytearray(base)
        encoded = text.encode('ascii')+b'\0'
        mem[0x6000:0x6000+len(encoded)] = encoded
        mem[sym['v_caps']] = caps
        # Результат AF сохраняется до любых изменяющих флаги команд.
        wrapper = b'\x31\x00\xBF\x21\x00\x60\x3E'+bytes([mode])+b'\xCD'+word(sym[label])+b'\xF5\xE1\x22\x00\x80\xC3\xF0\xB8'
        mem[0xB800:0xB800+len(wrapper)] = wrapper
        (out/'input.bin').write_bytes(mem)
        subprocess.run([str(args.ticks), '-m8080', '-pc', 'b800', '-end', 'b8f0', '-counter', '1000000', '-output', str(out/'output.bin'), str(out/'input.bin')], check=True, capture_output=True, timeout=15)
        data = (out/'output.bin').read_bytes()
        assert int.from_bytes(data[65542:65544], 'little') == 0xB8F0
        assert data[0x6000:0x6000+len(encoded)] == encoded
        return data

    valid = [('', (255,255,255,0,0)), ('H: -C=0 -W=1 -S=8', (7,0,1,8,0)),
             ('  -s=24  h: -w=9 -c=2 ', (7,2,9,24,0)), ('-W=0', (255,255,0,0,0)),
             ('-?', (255,255,255,0,1)), ('B: -C=01 -S=0008', (1,1,255,8,0))]
    valid += [(f'{chr(65+d)}: -C={c} -W={w} -S={size}', (d,c,w,size,0))
              for d in (0,7) for c in (0,1,2) for w in (0,9) for size in (1,24)]
    invalid = ['A', 'I:', 'BB:', 'A: B:', 'B: B:', '-C=3', '-C=-1', '-W=10', '-S=0', '-S=25',
               '-S=8192', '-S=256', '-S=65536', '-S=99999999999999999999', '-S=8K', '-S=', '-S',
               '-W=', '-C=', '-C=0 -C=0', '-W=0 -W=1', '-S=8 -S=8', '-X=0', '-C0', '-W 1',
               '-S==8', 'B:/', 'B: junk', '-P=1', '-V=1', '-W=1.0', '-S=+8']
    fields = ('argDrive','argCheck','argWrites','argSize','argHelp')
    for text, expected in valid:
        data = run('parse_arguments', text)
        assert data[0x8000]&1 == 0, text
        assert tuple(data[sym[key]] for key in fields) == expected, text
    for text in invalid:
        assert run('parse_arguments', text)[0x8000]&1, text
    modes = [(0,0,1),(1,0,6),(2,4,5),(2,0,None),(3,7,None),(255,7,None)]
    for mode,caps,expected in modes:
        data = run('select_check', mode=mode, caps=caps)
        assert bool(data[0x8000]&1) == (expected is None), (mode,caps)
        if expected is not None:
            assert data[sym['v_writeMode']] == expected
    report = dict(valid=len(valid), invalid=len(invalid), modes=len(modes), codeBytes=len(code), codeEnd=sym['code_end'], inputUnchanged=True)
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n', encoding='utf-8')
    print('DISKBEN ARGS 8080 PASS', report)


if __name__ == '__main__':
    main()
