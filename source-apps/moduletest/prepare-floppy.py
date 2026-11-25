"""Добавить однобанковый тест в отдельную копию штатной FAT12-дискеты MX."""
import argparse
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]


def prepare(source, destination):
    if source.resolve() == destination.resolve():
        raise ValueError('Choose a separate test image')
    data = bytearray(source.read_bytes())
    sector, per_cluster, reserved, copies, entries = struct.unpack_from('<HBHBH', data, 11)
    if data[54:62] != b'FAT12   ' or len(data) != 720 * 1024 or copies != 2:
        raise ValueError('Expected a 720 KiB MX FAT12 image')
    fat = reserved * sector
    size = sector * struct.unpack_from('<H', data, 22)[0]
    root = fat + copies * size
    start = root + ((entries * 32 + sector - 1) // sector) * sector
    capacity = per_cluster * sector
    def next_cluster(number):
        pair, = struct.unpack_from('<H', data, fat + number + number // 2)
        return pair >> (4 * (number % 2)) & 0xfff
    free = iter(n for n in range(2, 2 + (len(data)-start)//capacity) if next_cluster(n) == 0)
    slots = iter(i for i in range(root, root+entries*32, 32) if data[i] in (0, 229))
    for name in ('RELTEST.COM', 'RELOCA.DLL', 'RELOCB.DLL'):
        payload = (ROOT/'bin/moduletest'/name).read_bytes()
        stem, ext = name.split('.')
        short = (stem.ljust(8) + ext.ljust(3)).encode()
        if any(data[i:i+11] == short for i in range(root, root+entries*32, 32)):
            raise ValueError(f'{name} already exists')
        clusters = [next(free) for _ in range((len(payload)+capacity-1)//capacity)]
        for index, number in enumerate(clusters):
            after = clusters[index+1] if index+1 < len(clusters) else 0xfff
            for copy in range(copies):
                at = fat + size * copy + number + number // 2
                pair, = struct.unpack_from('<H', data, at)
                shift = 4 * (number % 2)
                struct.pack_into('<H', data, at, (pair & ~(0xfff << shift)) | after << shift)
            at = start + (number-2) * capacity
            chunk = payload[index*capacity:(index+1)*capacity]
            data[at:at+len(chunk)] = chunk
        at = next(slots)
        entry = bytearray(32)
        entry[:11] = short
        entry[11] = 0x20
        struct.pack_into('<HI', entry, 26, clusters[0], len(payload))
        data[at:at+32] = entry
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(data)


if __name__ == '__main__':
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('destination', type=Path)
    p.add_argument('--source', type=Path, default=ROOT/'images/A-Floppy-720k-MX-System.img')
    args = p.parse_args()
    prepare(args.source, args.destination)
