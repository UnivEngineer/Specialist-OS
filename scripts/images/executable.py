"""Независимый эталон COM/EXE/RKS для Python-проверок. Сборщик использует executable.ps1."""
from dataclasses import dataclass
from pathlib import Path
import struct

MAGIC = b'\xcd\x20MX\x01\x00'
HEADER_SIZE = 16


def checksum(data):
    if not data:
        return 0
    total = sum(data[:-1]) * 0x101
    return (total & 0xFF00) | (((total & 255) + data[-1]) & 255)


@dataclass
class Image:
    data: bytes
    load: int = 0
    entry: int = 0
    flags: int = 0

    def validate(self):
        if not self.data or not 0 <= self.load < 65536 or self.load + len(self.data) > 65536:
            raise ValueError('Invalid image address or size')
        if not self.load <= self.entry < self.load + len(self.data):
            raise ValueError('Entry is outside the image')
        if self.flags & ~1:
            raise ValueError("Unknown COM/EXE flags")
        return self


def decode(data, kind, load=None, entry=None, path=None):
    kind = kind.lower().lstrip('.')
    if kind in ('com', 'exe'):
        if len(data) < HEADER_SIZE or data[:5] != MAGIC[:5]:
            raise ValueError('Expected MXOS2 COM/EXE header')
        start, run, size, cs, hs = struct.unpack_from('<5H', data, 6)
        if len(data) != HEADER_SIZE + size or checksum(data[:14]) != hs or checksum(data[16:]) != cs:
            raise ValueError('Invalid COM/EXE size or checksum')
        return Image(data[16:], start, run, data[5]).validate()
    if kind == 'rks':
        if len(data) < 6:
            raise ValueError('Short RKS')
        start, end = struct.unpack_from('<HH', data)
        if end < start or len(data) != end - start + 7 or checksum(data[4:-2]) != int.from_bytes(data[-2:], 'little'):
            raise ValueError('Invalid RKS size or checksum')
        return Image(data[4:-2], start, start if entry is None else entry).validate()
    if kind == 'cpu' and path is not None and Path(path).with_suffix('.i80').exists():
        fields = data.decode('ascii').split()
        if len(fields) not in (2, 3):
            raise ValueError('CPU requires load and entry addresses')
        return Image(Path(path).with_suffix('.i80').read_bytes(), int(fields[0], 16), int(fields[1], 16), int(fields[2], 16) if len(fields) == 3 else 0).validate()
    if kind == 'cpu':
        return Image(data, 0, 0).validate()
    # Старый EXE/BIN принимается только как явно заданный RAW-вход сборщика.
    start = 0 if load is None else load
    return Image(data, start, start if entry is None else entry).validate()


def encode(image, kind):
    image.validate()
    kind = kind.lower().lstrip('.')
    if kind in ('com', 'exe'):
        if len(image.data) > 65535:
            raise ValueError('COM payload exceeds 65535 bytes')
        header = MAGIC[:5] + bytes([image.flags]) + struct.pack('<4H', image.load, image.entry, len(image.data), checksum(image.data))
        return header + struct.pack('<H', checksum(header)) + image.data
    if kind == 'rks':
        if image.flags:
            raise ValueError('RKS cannot represent executable flags')
        if image.entry != image.load:
            raise ValueError('RKS cannot represent a separate entry address')
        return struct.pack('<HH', image.load, image.load + len(image.data) - 1) + image.data + struct.pack('<H', checksum(image.data))
    return image.data


def convert(path, target, load=None):
    path = Path(path)
    kind = path.suffix.lower().lstrip('.')
    # До миграции старые EXE без сигнатуры — допустимый вход, но не выход.
    data = path.read_bytes()
    if kind in ('exe', 'com') and data[2:4] != b'MX':
        kind = 'raw'
    image = decode(data, kind, load=load, path=path)
    return encode(image, target), image


def convert_batch(requests):
    """Один процесс на весь манифест; порядок результатов совпадает с запросами."""
    import base64
    result = []
    for request in requests:
        source = request['source']
        try:
            target = request['format'].lower()
            if target not in ('com', 'exe', 'rks'):
                raise ValueError('Expected COM, EXE or RKS target')
            packed, _ = convert(source, target, request.get('load'))
        except (OSError, ValueError) as error:
            raise ValueError(f'{source}: {error}') from error
        result.append(base64.b64encode(packed).decode('ascii'))
    return result


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, nargs='?')
    parser.add_argument('output', type=Path, nargs='?')
    parser.add_argument('--format', type=str.lower, choices=('com','exe','rks'))
    parser.add_argument('--batch', type=Path, help='UTF-8 JSON: список source/format/load')
    parser.add_argument('--load', type=lambda s: int(s, 0))
    parser.add_argument('--stdout', action='store_true')
    args = parser.parse_args()
    if args.batch:
        import json
        if args.source or args.output or args.format or args.load is not None or args.stdout:
            parser.error('--batch cannot be combined with single-file options')
        print(json.dumps(convert_batch(json.loads(args.batch.read_text('utf-8-sig')))))
        raise SystemExit(0)
    if args.source is None or args.format is None:
        parser.error('source and --format are required')
    result = convert(args.source, args.format, args.load)[0]
    if args.stdout:
        import base64
        print(base64.b64encode(result).decode('ascii'))
    elif args.output:
        args.output.write_bytes(result)
    else:
        parser.error('output or --stdout is required')
