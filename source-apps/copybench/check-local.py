"""8080 correctness of private local-parameter CPM16/32 candidates (flat RAM)."""
from pathlib import Path
import argparse, json, runpy, struct, subprocess

ROOT = Path(__file__).resolve().parents[2]
H = runpy.run_path(str(Path(__file__).with_name('compare.py')))
W = lambda n: struct.pack('<H', n)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args(); out = args.out.resolve(); out.mkdir(parents=True, exist_ok=True)
    rows = []
    for chunk, aligned in [(16, 0), (32, 0), (32, 1), (32, 2)]:
        raw, sy = H['assemble'](Path(__file__).with_name('cpm-local.asm'),
                               out/f'local-{chunk}-{aligned}',
                               [f'CHUNK={chunk}', f'ALIGNED={aligned}'])
        for size in ([256] if aligned == 1 else ([256, 8192] if aligned else [32, 256, 8192])):
            phases = [(0, 0), (0, 1), (0, 0xfc)] if aligned else [(0, 0), (1, 1), (0xf0, 0xfc)]
            for so, do in phases:
                m = bytearray(65536)
                split = sy['tailImage']-0x7000
                m[0x7000:sy['tailImage']] = raw[:split]
                m[0xff00:0xff00+len(raw)-split] = raw[split:]
                src, dst = 0x2000+so, 0x4800+do
                payload = bytes((j*37+19)&255 for j in range(size))
                m[src:src+size] = payload
                m[dst-1:dst+size+1] = b'\xa5'+bytes(size)+b'\x5a'
                initial = bytes(range(31))+b'\0'; m[0xffc0:0xffe0] = initial
                m[0x1900:0x1908] = bytes([2])+W(src)+bytes([3])+W(dst)+W(size)
                code = b'\x31'+W(0x8f00)+b'\x01'+W(0x1234)+b'\x11'+W(0x5678)
                code += b'\x21'+W(0x1900)+b'\xcd'+W(sy['copy'])
                code += b'\x22'+W(0x1a02)+b'\xeb\x22'+W(0x1a04)
                code += b'\x60\x69\x22'+W(0x1a06)
                code += b'\xcd'+W(sy['restoreCommand'])+b'\xc3'+W(0x1800)
                m[0x1000:0x1000+len(code)] = code
                (out/'in.bin').write_bytes(m)
                subprocess.run([
                    r'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                    '-m8080', '-pc', '1000', '-end', '1800', '-counter', '4000000',
                    '-output', str(out/'out.bin'), str(out/'in.bin')],
                    capture_output=True, check=True)
                actual = (out/'out.bin').read_bytes()
                assert actual[65542:65544] == W(0x1800)
                assert actual[65544:65546] == W(0x8f00)
                assert actual[dst:dst+size] == payload
                assert actual[src:src+size] == payload
                assert actual[dst-1] == 0xa5 and actual[dst+size] == 0x5a
                assert struct.unpack_from('<HHH', actual, 0x1a02) == (0x1900, 0x5678, 0x1234)
                assert actual[0xffc0:0xffe0] == initial
                rows.append(dict(chunk=chunk, aligned=aligned, bytes=size,
                                 source=src, destination=dst, tailBytes=sy['tailEnd']-0xff00))
    (out/'result.json').write_text(json.dumps(rows, indent=2)+'\n')
    print('CPM LOCAL CPU PASS', len(rows), 'cases; source/destination, guards, SP, registers, COMMAND')


if __name__ == '__main__': main()
