"""Исправление четырёх известных хвостов RKS без изменения заголовков и нагрузки.

Оригиналы сохраняются как входные архивные .bin, отчёт — локально в docs/.
Повторный запуск проверяет уже исправленные файлы. Неизвестные хеши запрещены.
"""
import hashlib
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
ORIGINALS = {
    'Jet-set.rks': 'b45d38c759f747360e7152e57a5398643f45e5df2536030462ffba1d5583ba4c',
    'KOM13_1.rks': '07dfeb8eb64bcf17eb81289c5b9ba31d5a44105db0b8c363258b98c2b391207d',
    'StarW2.rks': '999775c6ef2bfc73922822131b1a8e83563bad16af1dbc3b376b6ec655909bce',
    'Lines.rks': '06892a22e898f319cc9dafed1ea7e3f2fe4ec8810955c8c7e2fa1d8114d39bd8',
}
# Имя архивной копии сохраняется; рабочий исторический RKS перенесён в семейство.
CURRENT_PATHS = {'StarW2.rks': 'StarWars/legacy/StarWars2-mx.rks'}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def parse(data):
    start = int.from_bytes(data[:2], 'little')
    end = int.from_bytes(data[2:4], 'little')
    size = end - start + 1
    if len(data) < 4 or size <= 0 or len(data) < 4 + size:
        raise ValueError('Неполная нагрузка RKS; исправлять одной суммой нельзя')
    return start, end, data[4:4 + size], data[4 + size:]


def checksum(data):
    # Независимая формула стандартной суммы DOS MXOS / Monitor-2.
    total = sum(data[:-1]) * 0x101
    return (total & 0xFF00) | (((total & 255) + data[-1]) & 255)


def main():
    backups = ROOT / 'assets/games/originals/rks-checksums'
    report_path = ROOT / 'docs/games/rks-repair-001.json'
    prepared = []
    # Сначала проверить все источники; не начинать частичную правку при ошибке.
    for name, expected in ORIGINALS.items():
        path = ROOT / 'assets/games' / CURRENT_PATHS.get(name, name)
        backup = backups / (name + '.bin')
        current = path.read_bytes()
        original = backup.read_bytes() if backup.exists() else current
        if sha(original) != expected:
            raise ValueError(f'Неизвестный оригинал: {name}')
        start, end, payload, tail = parse(original)
        fixed = original[:4 + len(payload)] + checksum(payload).to_bytes(2, 'little')
        if current not in (original, fixed):
            raise ValueError(f'Файл изменён вне этого исправления: {name}')
        entry = {
            'file': path.relative_to(ROOT).as_posix(),
            'backup': backup.relative_to(ROOT).as_posix(),
            'start': start, 'end': end, 'payload_bytes': len(payload),
            'original_bytes': len(original), 'fixed_bytes': len(fixed),
            'original_sha256': expected, 'fixed_sha256': sha(fixed),
            'old_tail_hex': tail.hex(), 'new_tail_hex': fixed[-2:].hex(),
            'header_unchanged': fixed[:4] == original[:4],
            'payload_unchanged': fixed[4:-2] == payload,
        }
        if name == 'Lines.rks':
            reference = Path('D:/Projects/RetroPC/Specialist/Soft/lines/lines.rks')
            entry['author_reference_path'] = reference.as_posix()
            if reference.exists():
                source = reference.read_bytes()
                if source != original:
                    raise ValueError('Указанный автором оригинал Lines отличается от сохранённого')
                entry['author_reference_sha256'] = sha(source)
                entry['author_reference_equals_backup'] = True
        prepared.append((path, backup, original, fixed, entry))
    backups.mkdir(parents=True, exist_ok=True)
    for path, backup, original, fixed, _ in prepared:
        if not backup.exists():
            backup.write_bytes(original)
        path.write_bytes(fixed)
    # Независимый побайтовый проход с переносом, как в bios_calcCS.
    count = 0
    for path in (ROOT / 'assets/games').rglob('*'):
        if not path.is_file() or path.suffix.lower() != '.rks':
            continue
        _, _, payload, tail = parse(path.read_bytes())
        b = c = 0
        for i, value in enumerate(payload):
            total = c + value
            c = total & 255
            if i != len(payload) - 1:
                b = (b + value + (total >> 8)) & 255
        if len(tail) != 2 or int.from_bytes(tail, 'little') != b * 256 + c:
            raise ValueError(f'Неверная сумма или длина: {path}')
        count += 1
    report = {'experiment': 'rks-repair-001', 'date': '2026-10-31',
              'scope': 'container checksum tails only; no code or load address repair',
              'all_rks_valid': count, 'files': [item[-1] for item in prepared]}
    report_path.parent.mkdir(parents=True, exist_ok=True)
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(f'Исправлены/проверены 4 хвоста; все {count} RKS имеют правильную длину и сумму.')


if __name__ == '__main__':
    main()
