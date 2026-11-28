"""Файлы каталога и точные байты, которые получит RUN в каждой пробе."""
from collections import Counter, defaultdict
import csv
from dataclasses import replace
import hashlib
import io
import json
from pathlib import Path
import struct

from executable import Image, checksum, decode, encode, parse_cpu

ROOT = Path(__file__).resolve().parents[2]
GAMES = ROOT / 'assets/games'
MODES = {'AUTO': 0, 'MX': 1, 'STD': 2, 'MON': 3, 'RF': 4}
LAFAN = GAMES / 'lafan-directory.csv'


def sha(data):
    return hashlib.sha256(data).hexdigest()


def lafan_directory(archive):
    """Повторно извлечь 32-байтные заголовки из ODI, проверив точные байты тела по секторам."""
    with (GAMES / 'catalog.csv').open(encoding='utf-8-sig', newline='') as stream:
        rows = [r for r in csv.DictReader(stream) if r['File'].startswith('unknown/lafan/')]
    disks = defaultdict(list)
    for row in rows:
        disk = Path(row['Origin']).parts[2] + '.odi'
        disks[disk].append(row)
    output = io.StringIO()
    writer = csv.writer(output, lineterminator='\n')
    writer.writerow(['File', 'PayloadSHA256', 'Disk', 'DiskSHA256', 'DirectoryOffset', 'HeaderHex'])
    for disk, games in sorted(disks.items()):
        raw = (archive / disk).read_bytes()
        digest = sha(raw)
        found = defaultdict(list)
        for off in range(0, len(raw) - 31, 32):
            header = raw[off:off+32]
            if header[:3] != b'\xd3'*3 or not 1 <= header[19] <= 10 or header[20] >= 80:
                continue
            # Линейный образ: 10 секторов по 1024 байта на логической дорожке, номера с единицы.
            first = (header[20] * 10 + header[19] - 1) * 1024
            start, end = struct.unpack_from('<HH', header, 24)
            size = end - start + 1
            if size <= 0 or first + size > len(raw):
                continue
            found[sha(raw[first:first+size])].append((off, header))
        for row in sorted(games, key=lambda r: r['File']):
            key = sha((GAMES / row['File']).read_bytes())
            hits = found[key]
            if len(hits) != 1:
                raise ValueError(f'{disk}: {row["File"]}: найдено записей с нужной нагрузкой: {len(hits)}')
            offset, header = hits[0]
            writer.writerow((row['File'], key, disk, digest, f'0x{offset:05x}', header.hex()))
    return output.getvalue()


def apply_lafan_headers(records):
    with LAFAN.open(encoding='utf-8', newline='') as stream:
        entries = {row['File']: row for row in csv.DictReader(stream)}
    for rec in records:
        name = rec['file'].removeprefix('assets/games/')
        if name not in entries or not rec['payload_sha256']:
            continue
        row = entries[name]
        header = bytes.fromhex(row['HeaderHex'])
        if row['PayloadSHA256'] != rec['payload_sha256'] or len(header) != 32 or header[:3] != b'\xd3'*3:
            raise ValueError('Заголовок ODI относится к другой нагрузке: ' + name)
        start, end, cs = struct.unpack_from('<HHH', header, 24)
        if end - start + 1 != rec['payload_bytes']:
            raise ValueError('Диапазон ODI и размер нагрузки разошлись: ' + name)
        if rec['load'] is not None and rec['load'] != start:
            rec['address_discrepancy'] = f'catalog=0x{rec["load"]:04x}, ODI=0x{start:04x}'
        rec.update(load=start, entry=start, address_source='mx_dos_directory',
                   entry_source='load_address_probe' if Path(name).suffix != '.bin' else 'mx_dos_exec_at_load',
                   directory=dict(disk=row['Disk'], disk_sha256=row['DiskSHA256'], offset=int(row['DirectoryOffset'], 0),
                                  load=start, end=end, checksum=cs, checksum_ok=cs == checksum(payload(rec))))
        if rec['entry_source'] == 'load_address_probe':
            rec['source_path_note'] = 'Диапазон взят из ODI. Пробный вход RUN с начала нагрузки; отдельная точка входа и действия оригинального загрузчика неизвестны'


def source_record(row):
    path = GAMES / row['File']
    load = int(row['LoadAddress'], 0) if row['LoadAddress'] else None
    rec = dict(file=path.relative_to(ROOT).as_posix(), sha256=None, bytes=None, payload_sha256=None,
               payload_bytes=None, load=load, entry=load, format=None, routes=[], profiles=[],
               catalog={k: row[k] for k in ('RunMode', 'Platform', 'Family', 'Status', 'Variant',
                                              'OriginalName', 'ArchiveProfile', 'Notes', 'Evidence', 'Ramfos')})
    if not path.is_file():
        rec['blocked'] = 'Файл каталога отсутствует'
        return rec
    raw = path.read_bytes()
    rec.update(sha256=sha(raw), bytes=len(raw), body_file=rec['file'], body_offset=0)
    suffix = path.suffix.lower()
    name = path.name.lower()
    data = raw
    address_source = 'catalog' if load is not None else None
    try:
        if name.endswith(('.rks', '.rks.bin')):
            offset = 0
            if raw.startswith(bytes.fromhex('70 8f 82 8f')) and raw[278:279] == b'\xe6':
                offset = 279
                rec['format'] = 'lik'
            elif raw.startswith(b'\xd9' * 3):
                sync = raw.find(b'\xe6', 768, 800)
                if sync < 0:
                    raise ValueError('Нет синхробайта перед диапазоном ленточного RKS')
                offset = sync + 1
                rec['format'] = 'tape_rks'
            else:
                rec['format'] = 'rks'
            start, end = struct.unpack_from('<HH', raw, offset)
            size = end - start + 1
            begin = offset + 4
            if size <= 0 or begin + size > len(raw):
                raise ValueError('Неполная объявленная нагрузка RKS')
            data = raw[begin:begin + size]
            tail = raw[begin + size:]
            rec.update(load=start, entry=start, body_offset=begin)
            address_source = 'container'
            if rec['format'] == 'lik':
                if tail:
                    raise ValueError('Лишние байты за объявленным диапазоном LIK')
                rec['source_path_note'] = 'Диапазон и нагрузка LIK переупакованы для RUN; загрузчик LIK отдельно не испытан'
            else:
                rec['container_sum_ok'] = tail == checksum(data).to_bytes(2, 'little')
                if not rec['container_sum_ok']:
                    rec['container_defect'] = f'Хвост {tail.hex()}, ожидается {checksum(data).to_bytes(2, "little").hex()}'
                if offset:
                    rec['source_path_note'] = 'Проба RUN с заголовка RKS после ленточного префикса'
        elif suffix == '.cpu' and path.with_suffix('.i80').is_file():
            image = decode(raw, 'cpu', path=path)
            rec.update(format='cpu_i80', load=image.load, entry=image.entry, body_file=path.with_suffix('.i80').relative_to(ROOT).as_posix(),
                       descriptor_monitor=parse_cpu(raw)['monitor'])
            data = image.data
            address_source = 'cpu_descriptor'
        elif suffix in ('.com', '.exe') and raw[2:4] == b'MX':
            image = decode(raw, suffix)
            data = image.data
            rec.update(format='mxos', load=image.load, entry=image.entry, body_offset={1: 16, 2: 32, 3: 40}[raw[4]])
            address_source = 'container'
        else:
            rec['format'] = 'raw'
            if suffix == '.cpu' and not path.with_suffix('.i80').exists():
                try:
                    parse_cpu(raw)
                except ValueError:
                    pass
                else:
                    raise ValueError('У текстового CPU отсутствует соседний I80')
            if suffix == '.cpu' and row['OriginMatch'] == 'payload+checksum':
                if raw[-2:] != checksum(raw[:-2]).to_bytes(2, 'little'):
                    raise ValueError('Не совпадает сумма после объявленной сырой нагрузки')
                data = raw[:-2]
                rec['format'] = 'raw_with_checksum'
            if '/lafan/' in rec['file']:
                rec['source_path_note'] = 'Архивные байты Lafan; маршрут RUN проверяет переупакованную нагрузку'
    except (ValueError, OSError, struct.error) as error:
        rec['blocked'] = str(error)
        return rec
    rec.update(payload_sha256=sha(data), payload_bytes=len(data), address_source=address_source)
    if row['LoadAddress'] and int(row['LoadAddress'], 0) != rec['load']:
        rec['address_discrepancy'] = row['LoadAddress']
    if name == 'zoo-lik-levels.rks.bin':
        rec.update(blocked='Набор уровней ZOO, самостоятельной точки входа нет (README семейства)', entry=None)
    return rec


def payload(rec):
    start = rec['body_offset']
    data = (ROOT / rec['body_file']).read_bytes()[start:start + rec['payload_bytes']]
    if sha(data) != rec['payload_sha256']:
        raise ValueError('Изменилась нагрузка ' + rec['file'])
    return data


def route_bytes(rec, kind, recipe):
    raw = (ROOT / rec['file']).read_bytes()
    if sha(raw) != rec['sha256']:
        raise ValueError('Изменился архивный файл ' + rec['file'])
    if recipe in ('original', 'archive_as_named'):
        return raw
    if recipe == 'after_tape_header':
        return raw[rec['body_offset'] - 4:]
    if rec['format'] == 'mxos':
        image = decode(raw, Path(rec['file']).suffix)
    elif rec['format'] == 'cpu_i80':
        image = decode(raw, 'cpu', path=ROOT / rec['file'])
    else:
        image = Image(payload(rec), rec['load'], rec['entry'])
    if sha(image.data) != rec['payload_sha256']:
        raise ValueError('Изменилась пара CPU/I80 ' + rec['file'])
    if recipe in ('manifest', 'manifest_auto') and kind == 'exe':
        mode = 'AUTO' if recipe == 'manifest_auto' else rec['catalog']['RunMode']
        if mode == 'NATIVE':
            if image.flags != 4:
                raise ValueError('NATIVE требует флаг MAIN')
        elif mode in MODES and not image.flags:
            image = replace(image, mode=MODES[mode] if not image.monitor else MODES["MON"])
        else:
            raise ValueError('Конфликт режима реестра с заголовком')
    return encode(image, kind)


def build_route(record, route):
    data = route_bytes(record, route['kind'], route['recipe'])
    if sha(data) != route['sha256']:
        raise ValueError('Байты маршрута RUN отличаются от инвентаризации')
    return data


def manifests():
    result = defaultdict(list)
    for manifest in sorted((ROOT / 'assets').glob('H-*.txt')):
        for line in manifest.read_text(encoding='utf-8-sig').splitlines():
            words = line.split()
            if len(words) < 2 or not words[1].lower().startswith('games/'):
                continue
            if Path(words[0]).suffix.lower() not in ('.rks', '.exe', '.com'):
                continue
            result[('assets/' + words[1]).lower()].append((manifest.name, words[0]))
    return result


def routes(rec, disk_entries):
    requests = []
    fmt = rec['format']
    if fmt == 'rks':
        requests.append(('rks', 'original', None, None))
    elif fmt == 'mxos':
        requests.append((Path(rec['file']).suffix[1:].lower(), 'original', None, None))
    elif fmt == 'tape_rks':
        requests.append(('rks', 'after_tape_header', None, None))
    elif not disk_entries:
        kind = 'rks' if rec['entry'] == rec['load'] else 'exe'
        requests.append((kind, 'repacked', None, None))
    mode = rec['catalog']['RunMode']
    confirmed_mx = mode == 'MX' and rec['catalog']['Status'] == 'user-tested'
    for manifest_name, disk_name in disk_entries:
        kind = Path(disk_name).suffix[1:].lower()
        recipe = 'manifest'
        if (manifest_name in ('H-Flash-2M-Games.txt', 'H-ROM-64k-Games.txt')
                and kind == 'exe' and mode not in ('AUTO', 'RF', 'NATIVE') and not confirmed_mx):
            recipe = 'manifest_auto'
        requests.append((kind, recipe, manifest_name, disk_name))
    # У исходного ленточного/LIK-заголовка и файла из MX-DOS свой путь
    # распознавания в RUN. Сохраняем отказ загрузчика отдельно от игры,
    # запущенной с тем же телом в проверочном RKS/EXE.
    archive_kind = None
    if fmt in ('tape_rks', 'lik'):
        archive_kind = 'rks'
    elif rec.get('directory'):
        archive = Path(rec['file'])
        archive_kind = (archive.with_suffix('') if archive.suffix == '.bin' else archive).suffix[1:].lower()
    if archive_kind:
        requests.append((archive_kind, 'archive_as_named', None, None))
    found = {}
    for kind, recipe, manifest_name, disk_name in requests:
        digest = sha(route_bytes(rec, kind, recipe))
        key = f'{kind}-{digest}'
        if key not in found:
            found[key] = dict(id=key, kind=kind, sha256=digest, recipe=recipe, disk_name=disk_name, disk_names=[])
        if disk_name:
            found[key]['disk_names'].append(dict(manifest=manifest_name, name=disk_name))
            if found[key]['disk_name'] is None:
                found[key]['disk_name'] = disk_name
    return list(found.values())


def select_profiles(rec, registry):
    mode = rec['catalog']['RunMode']
    hardware = {e['kind'] for e in rec['evidence']['hardware']}
    abi = rec['evidence']['abi']
    targets = {e['target'] for e in abi if e['kind'] == 'ramfos_shared_vector'}
    families = {e['kind'] for e in abi}
    strong_rf = ('ramfos_variable' in families or 'ramfos_f800_vector' in families or len(targets) >= 2
                 or (registry or {}).get('status') == 'strong-candidate')
    legacy = 'std_ppi' in hardware or 'legacy_monitor_entry' in families
    archive = rec['catalog']['ArchiveProfile'].lower()
    trials = []
    def offer(profile, reason):
        if profile not in [t['profile'] for t in trials]:
            trials.append(dict(profile=profile, reason=reason))
    if mode in (*MODES, 'NATIVE') and mode != 'AUTO':
        offer(mode, 'Режим указан в catalog.csv')
    if mode != 'NATIVE':
        if archive in ('r6_0000.mon', 'spmx.rom'):
            offer('RF', 'Архивный образ ' + rec['catalog']['ArchiveProfile'])
        if strong_rf:
            reason = next((e['reason'] for e in abi if e['kind'] in ('ramfos_variable', 'ramfos_f800_vector')), None)
            if not reason:
                reason = ('Сочетание входов ' + ', '.join(f'{addr:04X}h' for addr in sorted(targets))
                          if len(targets) >= 2 else 'Совпадение SHA-256 с сильным кандидатом в реестре RAMFOS')
            offer('RF', reason)
        if mode == 'AUTO' and rec['catalog']['Platform'] == 'MX':
            offer('MX', 'Аппаратная платформа MX в реестре, отдельная проверка с BIOS MXOS2')
        if mode == 'STD':
            offer('MON', 'Сравнение поведения игры для STD с историческими входами Monitor-2 и шрифтом KOI-7')
        elif legacy:
            offer('MON', 'Входы Monitor-2 или адреса ВВ55 в STD')
        if mode == 'AUTO' and hardware & {'mx_ppi', 'mx_pit', 'mx_color', 'mx_page'}:
            offer('MX', 'Доступ к аппаратным регистрам MX')
    if not trials:
        offer('MON', 'Разведочная проба Monitor-2; родная среда пока неизвестна')
    return trials


def campaign_inventory(previous, analyse, public):
    catalog = GAMES / 'catalog.csv'
    with catalog.open(encoding='utf-8-sig', newline='') as stream:
        records = [source_record(row) for row in csv.DictReader(stream)]
    apply_lafan_headers(records)
    prior = {r['file'].lower(): r for r in previous}
    disk = manifests()
    registry = {r['payload_sha256']: r for r in json.loads((GAMES / 'ramfos-candidates.json').read_text('utf-8'))['entries']}
    addresses = defaultdict(set)
    for r in records:
        if r['payload_sha256'] and r['load'] is not None and r['entry'] is not None:
            addresses[r['payload_sha256']].add((r['load'], r['entry']))
    missing = []
    cache = {}
    for rec in records:
        old = prior.get(rec['file'].lower())
        covered = bool(old and old.get('sha256') == rec['sha256'] and 'error' not in old)
        rec['previous_audit'] = 'covered' if covered else ('not_selected' if not old else 'changed_or_unreadable')
        if not covered:
            missing.append(rec['file'])
        if rec['load'] is None and len(addresses[rec['payload_sha256']]) == 1:
            rec['load'], rec['entry'] = next(iter(addresses[rec['payload_sha256']]))
            rec['address_source'] = 'exact_catalog_payload_match'
        if rec['load'] is None:
            rec.setdefault('blocked', 'Не установлены адрес загрузки и точка входа; нет однозначного совпадения SHA-256 с адресованной нагрузкой')
        hit = registry.get(rec['payload_sha256'])
        if hit:
            rec['ramfos_registry'] = {'id': hit['id'], 'status': hit['status'], 'match': 'payload_sha256'}
        rec['evidence'] = {'abi': [], 'hardware': [], 'pointer': []}
        if 'blocked' not in rec:
            key = rec['payload_sha256'], rec['load'], rec['entry']
            if key not in cache:
                scan = analyse({'load': rec['load'], 'entry': rec['entry']}, payload(rec), public, environment=True)
                cache[key] = scan['decoded_instructions'], scan['evidence']
            rec['decoded_instructions'], rec['evidence'] = cache[key]
            rec['routes'] = routes(rec, disk.get(rec['file'].lower(), []))
        rec['profile_reasons'] = select_profiles(rec, hit)
        rec['profiles'] = [t['profile'] for t in rec['profile_reasons']]
    summary = dict(catalog_rows=len(records), source_files=sum(r['sha256'] is not None for r in records),
                   distinct_payloads=len({r['payload_sha256'] for r in records if r['payload_sha256']}),
                   previous_audit_records=len(previous), previous_catalog_rows=len(records)-len(missing),
                   previously_uncovered=len(missing), runnable_rows=sum(bool(r['routes']) for r in records),
                   blocked_rows=sum(not r['routes'] for r in records),
                   launch_containers=len({t['id'] for r in records for t in r['routes']}),
                   container_profile_pairs=len({(t['id'], p) for r in records for p in r['profiles'] for t in r['routes']}),
                   address_sources=dict(Counter(r['address_source'] for r in records if r.get('address_source'))))
    return dict(schema=1, catalog_sha256=sha(catalog.read_bytes()), summary=summary,
                previously_uncovered=missing, files=records)
