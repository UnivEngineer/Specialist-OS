"""Actual 8080 CPM32: exact tails, alignment, bounds, registers and window.
Flat model; physical DATA/ROM switching is covered by the Emu80 campaign.
"""
from pathlib import Path
import argparse, json, re, struct, subprocess

ROOT = Path(__file__).resolve().parents[2]
W = lambda n: struct.pack('<H', n)


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9a-fA-F]+)', path.read_text(), re.M)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, default=ROOT/'docs/bench/cpm32-system-2026-11-13/cpu')
    args = parser.parse_args(); args.out.mkdir(parents=True, exist_ok=True)
    sy = symbols(ROOT/'bin/banked/TRANSPORT.sym')
    base = bytearray(65536)
    for name, origin in [('LOCALBLOCK', sy['MEM_IMAGE_LOCAL_START']),
                         ('CLIENT', 0xf800), ('TRANSPORT', 0xfc00)]:
        code = (ROOT/f'bin/banked/{name}.bin').read_bytes()
        base[origin:origin+len(code)] = code
    base[sy['bankPage']] = 0; base[sy['bankPageCount']] = 16
    rows = []
    cases = [(n, so, do, 2, 0) for n in
             [0, 1, 2, 3, 4, 15, 16, 17, 31, 32, 33, 63, 64, 127, 128, 129,
              255, 256, 257, 511, 512, 513, 8191, 8192]
             for so, do in [(0, 0), (1, 1), (0xf0, 0xfc), (0xff, 1)]]
    cases += [(256, 1, 1, 17, 0xe0), (2, 0, 0, 2, 0xe2)]
    for n, so, do, page, error in cases:
        memory = bytearray(base); src, dst = 0x2000+so, 0x5000+do
        if error == 0xe2: src = 0xfeff
        payload = bytes((j*73+(j>>8)+n)&255 for j in range(n))
        if not error: memory[src:src+n] = payload
        memory[dst-1:dst+n+1] = b'\xa5'+bytes(n)+b'\x5a'
        window = bytes(range(31))+b'\0'; memory[0xffc0:0xffe0] = window
        memory[0x8600:0x8608] = bytes([page])+W(src)+bytes([3])+W(dst)+W(n)
        stub = b'\x31'+W(0x8f00)+b'\x01'+W(0x1234)+b'\x11'+W(0x5678)
        stub += b'\x21'+W(0x8600)+b'\xcd'+W(sy['bankCopyFar'])
        stub += b'\x22'+W(0x8702)+b'\xeb\x22'+W(0x8704)+b'\x60\x69\x22'+W(0x8706)
        stub += b'\xf5\xe1\x22'+W(0x8700)+b'\xcd'+W(sy['bankCommandWindow'])+b'\xc3'+W(0x88ff)
        memory[0x8800:0x8800+len(stub)] = stub
        (args.out/'in.bin').write_bytes(memory)
        result = subprocess.run([
            r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
            '-m8080', '-pc', '8800', '-end', '88ff', '-counter', '4000000',
            '-output', str(args.out/'out.bin'), str(args.out/'in.bin')],
            capture_output=True, check=True, text=True)
        actual = (args.out/'out.bin').read_bytes()
        assert actual[65542:65544] == W(0x88ff), (n, so, do, 'PC')
        assert actual[65544:65546] == W(0x8f00)
        assert bool(actual[0x8700]&1) == bool(error), (n, so, do, 'CF')
        if error:
            assert actual[0x8701] == error
            assert actual[dst:dst+n] == bytes(n)
        else:
            assert actual[dst:dst+n] == payload, (n, so, do, 'data')
            assert actual[src:src+n] == payload
        assert actual[dst-1] == 0xa5 and actual[dst+n] == 0x5a
        assert struct.unpack_from('<HHH', actual, 0x8702) == (0x8600, 0x5678, 0x1234)
        assert actual[0xffc0:0xffe0] == window
        rows.append(dict(bytes=n, source=src, destination=dst, error=error,
                         ticks=int((result.stdout+result.stderr).strip())))
    (args.out/'result.json').write_text(json.dumps(rows, indent=2)+'\n')
    print('CPM32 CPU PASS', len(rows), 'cases; 0..8192, tails, guards, SP, registers, COMMAND, errors')


if __name__ == '__main__': main()
