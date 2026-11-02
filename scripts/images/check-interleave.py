"""Проверка чередования данных Games2M и совпадения с входным манифестом."""
import json
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    image = b''.join((ROOT / f'images/FlashDisk_chip{i}_512K.bin').read_bytes() for i in range(1, 5))
    word = lambda n: struct.unpack_from('<H', image, n)[0]
    secsize = word(11)
    fatstart = word(14) * secsize
    rootsector = word(14) + image[16] * word(22)
    datastart = rootsector + (word(17) * 32 + secsize - 1) // secsize
    assert len(image) == 2097152 and datastart < 2048
    entries = {image[i:i+11].decode('ascii').strip(): image[i:i+32]
               for i in range(rootsector * secsize, datastart * secsize, 32) if image[i] != 255}
    label_lines = [line.strip()[7:].strip() for line in
                   (ROOT/'scripts/images/manifests/FlashDisk.txt').read_text('utf-8-sig').splitlines()
                   if line.strip().startswith('@label ')]
    assert len(label_lines) == 1
    assert image[0x2B:0x36] == label_lines[0].ljust(11).encode('ascii')
    allclusters = set()
    chips = [0] * 4
    files = 0
    for line in (ROOT / 'scripts/images/manifests/FlashDisk.txt').read_text(encoding='utf-8-sig').splitlines():
        if not line.strip() or line.lstrip().startswith(('#','@')):
            continue
        fields = line.split()
        name, source = fields[:2]
        stem, _, ext = name.rpartition('.')
        key = stem.ljust(8) + ext.ljust(3)
        entry = entries[key.strip()]
        expected = (ROOT / 'assets' / source).read_bytes()
        if Path(source).suffix.lower() == '.rks':
            start, end = struct.unpack_from('<HH', expected)
            expected = expected[4:4 + max(0, end - start + 1)]
        count = (len(expected) + secsize - 1) // secsize
        c = struct.unpack_from('<H', entry, 26)[0]
        data = bytearray()
        filechips = []
        for n in range(count):
            assert 2 <= c < word(19) - datastart + 2
            assert c not in allclusters, (name, c)
            allclusters.add(c)
            sector = datastart + c - 2
            chip = sector // 2048
            chips[chip] += 1
            filechips.append(chip)
            data += image[sector * secsize:(sector + 1) * secsize]
            following = struct.unpack_from('<H', image, fatstart + 2*c)[0]
            if n == count - 1:
                assert following == c, name
            c = following
        assert bytes(data[:len(expected)]) == expected, name
        assert all(b == (a + 1) % 4 for a, b in zip(filechips, filechips[1:])), name
        files += 1
    assert max(chips) - min(chips) <= 1
    # Все ненулевые записи данных принадлежат ровно одному файлу.
    for c in range(2, word(19) - datastart + 2):
        assert (struct.unpack_from('<H', image, fatstart + 2*c)[0] != 0) == (c in allclusters)
    report = dict(files=files, dataSectors=len(allclusters), sectorsPerChip=chips,
                  metadataChip=0, allPayloadsMatch=True, noSharedClusters=True,
                  numbering='physical, compatible with Flash and FlashFst')
    path = ROOT / 'docs/flash/interleave-001/image.json'
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
