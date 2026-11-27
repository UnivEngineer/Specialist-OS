"""Проверка чередования данных Games2M и совпадения с входным манифестом."""
import csv
import json
import runpy
from dataclasses import replace
import struct
from pathlib import Path
from executable import convert, encode

ROOT = Path(__file__).resolve().parents[2]


def main():
    image = b''.join((ROOT / f'images/H-Flash-2M-Games-chip{i}.bin').read_bytes() for i in range(1, 5))
    word = lambda n: struct.unpack_from('<H', image, n)[0]
    secsize = word(11)
    fatstart = word(14) * secsize
    rootsector = word(14) + image[16] * word(22)
    datastart = rootsector + (word(17) * 32 + secsize - 1) // secsize
    assert len(image) == 2097152 and datastart < 2048
    assert image == (ROOT / 'images/H-Flash-2M-Games.bin').read_bytes()
    entries = {}
    files_by_path, _ = runpy.run_path(str(ROOT / 'scripts/tests/check-nested-image.py'))['read_tree'](image, entries)
    with (ROOT / 'assets/games/catalog.csv').open(encoding='utf-8-sig', newline='') as catalog:
        games = {'games/' + row['File'].replace('\\', '/').lower(): row for row in csv.DictReader(catalog)}
    label_lines = [line.strip()[7:].strip() for line in
                   (ROOT/'assets/H-Flash-2M-Games.txt').read_text('utf-8-sig').splitlines()
                   if line.strip().startswith('@label ')]
    assert len(label_lines) == 1
    assert image[0x2B:0x36] == label_lines[0].ljust(11).encode('ascii')
    allclusters = set()
    chips = [0] * 4
    capacity = [2048 - datastart, 2048, 2048, 2048]
    next_chip = 0
    files = 0
    for line in (ROOT / 'assets/H-Flash-2M-Games.txt').read_text(encoding='utf-8-sig').splitlines():
        if not line.strip() or line.lstrip().startswith(('#','@')):
            continue
        fields = line.split()
        name, source = fields[:2]
        ext = Path(name).suffix[1:].lower()
        entry = entries[name]
        path = ROOT / 'assets' / source
        address = int(fields[2], 0) if len(fields) >= 3 else 0
        if ext in ('com', 'exe', 'dll', 'fnt', 'rks'):
            expected, program = convert(path, ext, address)
            if source.lower().startswith('games/'):
                game = games[source.lower()]
                assert ext == ('rks' if game['Platform'] in ('STD', 'STD+MON') else 'exe'), name
                if ext == 'exe' and game['RunMode'] != 'NATIVE':
                    mode = game['RunMode']
                    if mode == 'MX' and game['Status'] != 'user-tested':
                        mode = 'AUTO'
                    expected = encode(replace(program, mode={'RF': 4, 'MX': 1}.get(mode, 0)), ext)
        else:
            expected = path.read_bytes()
        stored_size = struct.unpack_from('<I', entry, 28)[0]
        assert entry[18:20] == bytes(2), name
        assert stored_size == len(expected), (name, stored_size, len(expected))
        count = (len(expected) + secsize - 1) // secsize
        c = struct.unpack_from('<H', entry, 26)[0]
        data = bytearray()
        for n in range(count):
            assert 2 <= c < word(19) - datastart + 2
            assert c not in allclusters, (name, c)
            allclusters.add(c)
            sector = datastart + c - 2
            chip = sector // 2048
            while chips[next_chip] == capacity[next_chip]:
                next_chip = (next_chip + 1) % 4
            assert chip == next_chip, (name, n, chip, next_chip)
            chips[chip] += 1
            next_chip = (chip + 1) % 4
            data += image[sector * secsize:(sector + 1) * secsize]
            following = struct.unpack_from('<H', image, fatstart + 2*c)[0]
            if n == count - 1:
                assert 0xFFF8 <= following <= 0xFFFF, name
            c = following
        assert bytes(data[:len(expected)]) == expected, name
        files += 1
    # read_tree также проверил владение кластерами каталогов, обе FAT и свободные записи.
    assert files == len(files_by_path)
    report = dict(files=files, dataSectors=len(allclusters), sectorsPerChip=chips,
                  metadataChip=0, allPayloadsMatch=True, noSharedClusters=True,
                  numbering='physical, compatible with Flash and FlashFst')
    path = ROOT / 'docs/flash/interleave-001/image.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
