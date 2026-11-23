"""Системный ROM RAMFOS и готовые RAM-диски с играми из реестра кандидатов."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import runpy
import struct
import subprocess
import sys

ROOT = Path(__file__).absolute().parents[2]
START, END = 0x2000, 0xFFFC
REGISTRY = ROOT / 'assets/games/ramfos-candidates.json'
MX_MANIFEST = ROOT / 'assets/C-ROM-32k-MX-RAMFOS.txt'


def checksum(data):
    b = c = 0
    for i, value in enumerate(data):
        total = c + value
        c = total & 255
        if i + 1 < len(data):
            b = (b + value + (total >> 8)) & 255
    return b * 256 + c


def source_path(entry, library):
    # Пути реестра переносим относительно двух известных корней.
    normalized = entry['verified_source'].replace('\\', '/')
    for prefix, base in [('D:/Projects/Specialist-OS/', ROOT),
                         ('D:/Projects/RetroPC/Specialist/', library)]:
        if normalized.lower().startswith(prefix.lower()):
            return base / normalized[len(prefix):]
    return Path(normalized)


def payload(entry, library):
    source = source_path(entry, library)
    raw = source.read_bytes()
    size = entry['payload_bytes']
    view = memoryview(raw)
    expected = entry['payload_sha256']
    match = next((offset for offset in range(len(raw) - size + 1)
                  if hashlib.sha256(view[offset:offset + size]).hexdigest() == expected), None)
    if match is None:
        raise ValueError(f"{entry['id']}: неизвестная нагрузка {source}")
    data = raw[match:match + size]
    address = int(entry['load_address'], 16)
    if not data or address + len(data) > 0x9000:
        raise ValueError(f"{entry['id']}: нагрузка вне пользовательского ОЗУ")
    return data, address, str(source)


def record(name, extension, address, data, offset):
    if len(name) > 8 or len(extension) > 3 or not data or address + len(data) > 65536:
        raise ValueError(f'Недопустимый файл {name}.{extension}')
    header = (b'\xd3' * 3 + name.ljust(8).encode('ascii') + b' '
              + extension.ljust(3).encode('ascii') + bytes(9)
              + struct.pack('<HHH', address, address + len(data) - 1, checksum(data)))
    assert len(header) == 30
    return header + data + struct.pack('<H', offset)


def assemble(name, defines=(), prefix='RAMFOS-ROM-'):
    output = ROOT / 'bin' / (prefix + name)
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    *defines, '--lst=' + str(output.with_suffix('.lst')),
                    '--raw=' + str(output.with_suffix('.bin')), name + '.asm'],
                   cwd=ROOT / 'source/ramfos', check=True)
    return output.with_suffix('.bin').read_bytes()


def system_prefix(mx_rom32=False, prebuilt=False):
    (ROOT / 'bin').mkdir(exist_ok=True)
    prefix = 'RAMFOS-MX32-' if mx_rom32 else 'RAMFOS-ROM-'
    platform = ['-DMX_LINEAR_ROM32=1'] if mx_rom32 else []
    start_page = '080h' if mx_rom32 else '0'
    if prebuilt:
        kernel = (ROOT / f'bin/{prefix}ramfos.bin').read_bytes()
        boot = (ROOT / f'bin/{prefix}romboot.bin').read_bytes()
    else:
        kernel = assemble('ramfos', ['-DMXOS_KERNEL=0', '-DROM_PAGE_START=02000h',
                                     '-DDLG_START_PAGE=' + start_page, *platform], prefix)
        boot = assemble('romboot', platform, prefix)
    assert len(kernel) == 0x3FE0
    packed = kernel[:0x1300] + kernel[0x3800:]
    assert 0x100 + len(packed) <= START
    return boot.ljust(0x100, b'\0') + packed


def mx_files():
    entries = []
    for number, line in enumerate(MX_MANIFEST.read_text(encoding='utf-8-sig').splitlines(), 1):
        fields = line.partition('#')[0].split()
        if not fields:
            continue
        if len(fields) != 3 or not re.fullmatch(r'[^.\s/\\]{1,8}\.[^.\s/\\]{1,3}', fields[0]):
            raise ValueError(f'{MX_MANIFEST}:{number}: expected name.ext source load_address')
        name, source, load = fields
        entries.append((name, MX_MANIFEST.parent / source, int(load, 0)))
    return entries


def build_mx_rom(entries, prebuilt=False):
    rom = bytearray(system_prefix(mx_rom32=True, prebuilt=prebuilt).ljust(START, b'\0'))
    for filename, path, address in entries:
        name, extension = filename.split('.')
        rom.extend(record(name, extension, address, path.read_bytes(), len(rom)))
    if len(rom) + 30 > 0x8000:
        raise ValueError(f'{MX_MANIFEST}: ROM directory exceeds 32 KiB')
    target = ROOT / 'images' / MX_MANIFEST.with_suffix('.bin').name
    target.parent.mkdir(exist_ok=True)
    target.write_bytes(rom.ljust(0x8000, b'\0'))
    print(f'{target.name}: 32 KiB, {len(entries)} file(s)')


def build(library):
    prefix = system_prefix()
    entries = json.loads(REGISTRY.read_text(encoding='utf-8'))['entries']
    monitor = (library / 'OS/RAMFOS-master/RomDisk/Monitor2.49152.sys').read_bytes()
    # Системный монитор доступен в каждом томе для штатного RKS-загрузчика.
    if len(monitor) != 0x1000:
        raise ValueError('Ожидается Monitor2.49152.sys для C000h..CFFFh')
    if hashlib.sha256(monitor).hexdigest() != 'e8eabdee87cbbb679f0d6e7c30bcdb2e48a793fc0a7938d8d9411f8bcaa9555a':
        raise ValueError('Неизвестный вариант Monitor2.49152.sys')
    base = record('Monitor2', 'SYS', 0xC000, monitor, START)
    # Весь набор живёт в ДОЗУ; ROM содержит только ядро и системный монитор.
    pages = [bytearray(65536) for _ in range(16)]
    offsets = [0] * 16
    listing = []
    for entry in entries:
        data, address, source = payload(entry, library)
        extension = 'RKS' if entry['load_evidence'] == 'rks' else 'EXE'
        size = 32 + len(data)
        # Первый подходящий диск сохраняет порядок реестра при плотной упаковке.
        page = next((p for p in range(16) if offsets[p] + size + 30 <= 0xFFBB), None)
        if page is None:
            raise ValueError(f"{entry['id']}: набор не помещается в 16 RAM-страниц")
        offset = offsets[page]
        stem = re.sub('[^A-Za-z0-9_]', '', Path(entry['name']).stem)
        name = (stem[:5] or 'GAME') + entry['id'][-3:]
        encoded = record(name, extension, address, data, offset)
        pages[page][offset:offset + len(encoded)] = encoded
        listing.append(dict(id=entry['id'], name=name + '.' + extension,
                            original_name=entry['name'], status=entry['status'],
                            load_address=address, load_evidence=entry['load_evidence'],
                            payload_sha256=entry['payload_sha256'], source=source,
                            bytes=len(data), page=page, offset=offset))
        offsets[page] += len(encoded)
    logical = bytearray(65536)
    logical[:len(prefix)] = prefix
    logical[START:START + len(base)] = base
    # BIG_ROM: адреса >=8000h читаются из STD ROM со сдвигом +4.
    physical = bytes.fromhex('31fff7c7') + logical[0x8000:0xFFFC] + logical[:0x8000]
    assert len(physical) == 65536
    seed = b''.join(pages)
    report = {'format': 'RAMFOS ROM + Emu80 RAM disk', 'rom_start': START,
              'system_image': 'RAMFOS-system.bin', 'system_sha256': hashlib.sha256(physical).hexdigest(),
              'ram_image': 'RAMFOS-games.ramdisk', 'ram_sha256': hashlib.sha256(seed).hexdigest(),
              'page_count': 16, 'page_size': 65536, 'page_end': 0xFFBB,
              'page_used': offsets, 'files': listing}
    images = ROOT / 'images'
    images.mkdir(exist_ok=True)
    (images / report['system_image']).write_bytes(physical)
    (images / report['ram_image']).write_bytes(seed)
    (images / 'RAMFOS-games.json').write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    lines = ['# Состав RAM-дисков RAMFOS', '',
             'Игры — кандидаты, совместимость не подтверждена. Адреса assumed_zero требуют проверки.', '',
             '| Страница | Имя RAMFOS | ID | Историческое имя | Статус |',
             '|---|---|---|---|---|']
    for item in listing:
        lines.append(f"| {item['page']} | `{item['name']}` | {item['id']} | `{item['original_name']}` | {item['status']} |")
    (images / 'RAMFOS-games.md').write_text('\n'.join(lines) + '\n', encoding='utf-8')
    print(f'RAMFOS-system.bin: 64 КБ; RAMFOS-games.ramdisk: 16 x 64 КБ')
    print(f'Всего: {len(entries)} кандидата, занято {sum(bool(n) for n in offsets)} RAM-страниц. Совместимость игр не проверена.')


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--library', type=Path, default=Path('D:/Projects/RetroPC/Specialist'))
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--prebuilt', action='store_true')
    parser.add_argument('--mx-rom32', action='store_true', help='Build the standalone linear MX ROM from its file manifest')
    parser.add_argument('--run-build', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    try:
        if args.mx_rom32:
            entries = mx_files()
            if args.run_build:
                build_mx_rom(entries, args.prebuilt)
            else:
                options = ['image: MxRamfos32', '--input-content', '--input', str(MX_MANIFEST),
                           '--output', str(ROOT / 'images' / MX_MANIFEST.with_suffix('.bin').name)]
                for _, path, _ in entries:
                    options.extend(('--input', str(path)))
                for name in ('ramfos', 'romboot'):
                    if args.prebuilt:
                        options.extend(('--input', str(ROOT / f'bin/RAMFOS-MX32-{name}.bin')))
                        continue
                    options.extend(('--asm', str(ROOT / f'source/ramfos/{name}.asm')))
                    for ext in ('bin', 'lst'):
                        options.extend(('--output', str(ROOT / f'bin/RAMFOS-MX32-{name}.{ext}')))
                if args.force:
                    options.append('--force')
                cached_build = runpy.run_path(str(ROOT / 'scripts/build/build_cache.py'))['main']
                if not args.prebuilt:
                    options.extend(('--input', str(ROOT / 'tasm/sjasmplus.exe')))
                sys.exit(cached_build([*options, '--', __file__, '--mx-rom32', '--run-build',
                                       *(['--prebuilt'] if args.prebuilt else [])]))
        elif args.run_build:
            build(args.library)
        else:
            entries = json.loads(REGISTRY.read_text(encoding='utf-8'))['entries']
            monitor = args.library / 'OS/RAMFOS-master/RomDisk/Monitor2.49152.sys'
            options = ['image: RAMFOS', '--input', str(REGISTRY), '--input', str(monitor), '--hash', str(monitor)]
            for entry in entries:
                options.extend(('--input', str(source_path(entry, args.library))))
            for name in ('ramfos', 'romboot'):
                options.extend(('--asm', str(ROOT / f'source/ramfos/{name}.asm')))
                for extension in ('bin', 'lst'):
                    options.extend(('--output', str(ROOT / f'bin/RAMFOS-ROM-{name}.{extension}')))
            for name in ('RAMFOS-system.bin', 'RAMFOS-games.ramdisk', 'RAMFOS-games.json', 'RAMFOS-games.md'):
                options.extend(('--output', str(ROOT / 'images' / name)))
            if args.force:
                options.append('--force')
            cached_build = runpy.run_path(str(ROOT / 'scripts/build/build_cache.py'))['main']
            sys.exit(cached_build([*options, '--', __file__, '--run-build', '--library', str(args.library)]))
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
