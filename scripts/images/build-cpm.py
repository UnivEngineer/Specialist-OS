"""Загрузочный ROM CP/M Syabr и исходный SD-диск из внешнего ZIP."""
import argparse
import hashlib
import json
from pathlib import Path
import subprocess
import zipfile

ROOT = Path(__file__).resolve().parents[2]


def checksum(data):
    b = c = 0
    for i, value in enumerate(data):
        total = c + value
        c = total & 255
        if i + 1 < len(data):
            b = (b + value + (total >> 8)) & 255
    return b << 8 | c


def build(archive):
    with zipfile.ZipFile(archive) as source:
        rks = source.read('CPM/CPM.RKS')
        disk = source.read('CPM/CPM.DSK')
        readme = source.read('CPM/README.TXT')
    start, end = int.from_bytes(rks[:2], 'little'), int.from_bytes(rks[2:4], 'little')
    body = rks[4:-2]
    if start != 0x100 or end - start + 1 != len(body) or checksum(body) != int.from_bytes(rks[-2:], 'little'):
        raise ValueError('Invalid CPM.RKS header or checksum')
    if end >= 0x7F00 or len(disk) != 8388608:
        raise ValueError('Unsupported CP/M layout or disk size')
    output = ROOT / 'bin/CPM-romboot'
    output.parent.mkdir(exist_ok=True)
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    f'-DCPM_BODY_SIZE={len(body)}', '--lst=' + str(output.with_suffix('.lst')),
                    '--raw=' + str(output.with_suffix('.bin')), 'romboot.asm'], cwd=ROOT / 'source/cpm', check=True)
    rom = bytearray(65536)
    boot = output.with_suffix('.bin').read_bytes()
    rom[0x8000:0x8000 + len(boot)] = boot
    rom[0x8100:0x8100 + len(body)] = body
    images = ROOT / 'images'
    images.mkdir(exist_ok=True)
    files = {'CPM-system.bin': bytes(rom), 'CPM-seed.dsk': disk, 'CPM-original.rks': rks,
             'CPM-README.TXT': readme}
    for name, data in files.items():
        (images / name).write_bytes(data)
    manifest = dict(format='CP/M Syabr 0.1', archive=str(archive),
                    archive_sha256=hashlib.sha256(archive.read_bytes()).hexdigest(),
                    load_address=start, end_address=end, body_bytes=len(body),
                    files={name: dict(bytes=len(data), sha256=hashlib.sha256(data).hexdigest()) for name, data in files.items()})
    (images / 'CPM-images.json').write_text(json.dumps(manifest, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'CPM-system.bin: 64 KiB; CPM-seed.dsk: 8 MiB; original payload: {len(body)} bytes at {start:04X}h')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, default=Path('D:/Projects/RetroPC/Specialist/OS/CP-M/syabr-cpm-0.1.zip'))
    build(parser.parse_args().archive)
