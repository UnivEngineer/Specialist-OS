"""Generate raw ready-to-load planes from an editable linear 256x8 font."""
from pathlib import Path
import argparse


def build(font, mode):
    if len(font) != 2048:
        raise ValueError('Expected 256 glyphs of 8 bytes each')
    if mode in ('graph-packed', 'graph-full'):
        payload = bytearray()
        for shift in (2, 4, 6):
            for y in range(8):
                row = font[y::8]
                head = bytes(b >> shift for b in row)
                tail = bytes((b << (8 - shift)) & 255 for b in row)
                if mode == 'graph-full':
                    payload.extend(head)
                    payload.extend(tail)
                else:
                    payload.extend(a | b for a, b in zip(head, tail))
        return bytes(payload)
    paged = bytes(font[c * 8 + y] & 63 for y in range(8) for c in range(256))
    if mode == 'packed':
        transforms = (lambda b: (b << 2) | (b >> 4),
                      lambda b: (b << 4) | (b >> 2))
    elif mode == 'full':
        transforms = (lambda b: b << 2, lambda b: b >> 2,
                      lambda b: b << 6, lambda b: b >> 4, lambda b: b << 4)
    else:
        raise ValueError(mode)
    payload = b''.join(bytes(f(b) & 255 for b in paged) for f in transforms)
    return payload



if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--font', type=Path, required=True)
    parser.add_argument('--mode', choices=['packed', 'full', 'graph-packed', 'graph-full'], required=True)
    parser.add_argument('--out', type=Path, required=True)
    args = parser.parse_args()
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_bytes(build(args.font.read_bytes(), args.mode))
    print(f'{args.out}: {args.out.stat().st_size} bytes')
