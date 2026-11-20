"""Загрузочный ROM CP/M Syabr и исходный SD-диск из внешнего ZIP."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
import sys
import zipfile

ROOT = Path(__file__).absolute().parents[2]
ARCHIVE_RELATIVE_PATH = 'Projects/RetroPC/Specialist/OS/CP-M/syabr-cpm-0.1.zip'


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
    local_archive = Path('D:/') / ARCHIVE_RELATIVE_PATH
    shared_archive = Path('G:/') / ARCHIVE_RELATIVE_PATH
    default_archive = shared_archive if not local_archive.is_file() and shared_archive.is_file() else local_archive
    parser.add_argument('--archive', type=Path, default=default_archive)
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--run-build', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.run_build:
        build(args.archive)
    else:
        options = ['image: CP/M', '--input', str(args.archive), '--asm', str(ROOT / 'source/cpm/romboot.asm')]
        for name in ('images/CPM-system.bin', 'images/CPM-seed.dsk', 'images/CPM-original.rks',
                     'images/CPM-README.TXT', 'images/CPM-images.json', 'bin/CPM-romboot.bin', 'bin/CPM-romboot.lst'):
            options.extend(('--output', str(ROOT / name)))
        if args.force:
            options.append('--force')
        cached_build = runpy.run_path(str(ROOT / 'scripts/build/build_cache.py'))['main']
        sys.exit(cached_build([*options, '--', __file__, '--run-build', '--archive', str(args.archive)]))
