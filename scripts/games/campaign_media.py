"""Сменные носители для проверки RUN. Пользовательские образы открываются только на чтение."""
import hashlib
from pathlib import Path
import runpy
import struct

ROOT = Path(__file__).resolve().parents[2]
read_tree = runpy.run_path(str(ROOT / "scripts/tests/check-nested-image.py"))["read_tree"]
SYSTEM = ROOT / "images/C-Flash-512k-MMX-System-Emu80.bin"
DISK = ROOT / "images/H-Flash-2M-Games.bin"


def sha(data):
    return hashlib.sha256(data).hexdigest()


def trial_disk(packed, kind, base=DISK):
    """Расширить корень копии H: на сектор, сохранить исходные каталоги и файлы."""
    original = base.read_bytes()
    image = bytearray(original)
    word = lambda at: struct.unpack_from("<H", image, at)[0]
    sector, copies = word(11), image[16]
    fat, fat_size = word(14) * sector, word(22) * sector
    root = fat + copies * fat_size
    entries = word(17)
    first = root + ((entries * 32 + sector - 1) // sector) * sector
    size = image[13] * sector
    total = word(19) * sector
    after = first + sector
    clusters = (total - after) // size
    if total != len(image) or sector < 64 or (entries * 32) % sector:
        raise ValueError("Неизвестная геометрия H:")
    if any(word(fat + 2 * c) for c in range(2 + clusters, 2 + (total-first)//size)):
        raise ValueError("Последний кластер занят, расширение корня невозможно")
    image[after:] = image[first:total-sector]
    image[first:after] = bytes(sector)
    struct.pack_into("<H", image, 17, entries + sector // 32)
    free = iter(c for c in range(2, clusters+2) if not word(fat + c*2))
    marker = ("READ-" + sha(packed)[:16].upper()).encode("ascii")
    files = {"PROBE." + kind.upper(): packed, "CHECK.TXT": marker}
    existing, _ = read_tree(original)
    if set(files).intersection(name.upper() for name in existing):
        raise ValueError("Служебное имя уже занято на H:")
    for i, (name, content) in enumerate(files.items()):
        chain = []
        for start in range(0, len(content), size):
            cluster = next(free, None)
            if cluster is None:
                raise ValueError("Недостаточно свободных кластеров H:")
            chain.append(cluster)
            pos = after + (cluster-2)*size
            image[pos:pos+size] = content[start:start+size].ljust(size, b"\0")
        for cluster, link in zip(chain, chain[1:]+[0xffff]):
            for copy in range(copies):
                struct.pack_into("<H", image, fat + copy*fat_size + cluster*2, link)
        stem, ext = name.split(".")
        entry = first + 32*i
        image[entry:entry+11] = (stem.ljust(8) + ext.ljust(3)).encode("ascii")
        struct.pack_into("<HI", image, entry+26, chain[0], len(content))
    restored, _ = read_tree(image)
    if restored != existing | files:
        raise ValueError("Содержимое подготовленного H: не прошло обратное чтение FAT")
    return image, marker
