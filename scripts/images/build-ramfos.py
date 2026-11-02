"""Системный ROM RAMFOS и готовые RAM-диски с играми из реестра кандидатов."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import struct
import subprocess
import sys

ROOT = Path(__file__).resolve().parents[2]
START, END = 0x2000, 0xFFFC


def checksum(data):
    b = c = 0
    for i, value in enumerate(data):
        total = c + value
        c = total & 255
        if i + 1 < len(data):
            b = (b + value + (total >> 8)) & 255
    return b * 256 + c


def payload(entry, library):
    source = Path(entry['verified_source'])
    # Пути реестра переносим относительно двух известных корней.
    normalized = str(source).replace('\\', '/')
    for prefix, base in [('D:/Projects/Specialist-OS/', ROOT),
                         ('D:/Projects/RetroPC/Specialist/', library)]:
        if normalized.lower().startswith(prefix.lower()):
            source = base / normalized[len(prefix):]
            break
    raw = source.read_bytes()
    size = entry['payload_bytes']
    matches = [raw[offset:offset + size] for offset in range(len(raw) - size + 1)
               if len(raw) >= offset + size
               and hashlib.sha256(raw[offset:offset + size]).hexdigest() == entry['payload_sha256']]
    if not matches:
        raise ValueError(f"{entry['id']}: неизвестная нагрузка {source}")
    data = matches[0]
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


def assemble(name, defines=()):
    output = ROOT / 'bin' / ('RAMFOS-ROM-' + name)
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    *defines, '--lst=' + str(output.with_suffix('.lst')),
                    '--raw=' + str(output.with_suffix('.bin')), name + '.asm'],
                   cwd=ROOT / 'source/ramfos', check=True)
    return output.with_suffix('.bin').read_bytes()


def build(library):
    (ROOT / 'bin').mkdir(exist_ok=True)
    kernel = assemble('ramfos', ('-DMXOS_KERNEL=0', '-DROM_PAGE_START=02000h', '-DDLG_START_PAGE=0'))
    boot = assemble('romboot')
    assert len(kernel) == 0x3FE0
    packed = kernel[:0x1300] + kernel[0x3800:]
    assert 0x100 + len(packed) <= START
    entries = json.loads((ROOT / 'assets/games/ramfos-candidates.json').read_text(encoding='utf-8'))['entries']
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
    logical[:len(boot)] = boot
    logical[0x100:0x100 + len(packed)] = packed
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
    args = parser.parse_args()
    try:
        build(args.library)
    except (OSError, ValueError, subprocess.CalledProcessError) as error:
        sys.exit(str(error))
