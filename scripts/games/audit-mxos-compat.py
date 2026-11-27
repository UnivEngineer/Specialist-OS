"""Статические кандидаты несовместимости библиотеки с ядром MXOS.

Обход прямого потока 8080 от точки входа; данные не сканируются как код подряд.
Косвенные переходы, распаковка и самомодификация делают результат неполным.
Отсутствие замечаний не является доказательством совместимости.
"""
import argparse
from collections import Counter
import csv
from datetime import datetime, timezone
import hashlib
import json
from pathlib import Path
import re
import sys

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0,str(ROOT/'scripts/images'))
from executable import convert
sys.path.insert(0, str(Path(__file__).resolve().parent))
from game_inventory import LAFAN, build_route, campaign_inventory, lafan_directory
UNDEFINED = {0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
             0xCB, 0xD9, 0xDD, 0xED, 0xFD}
JUMPS = {0xC3} | set(range(0xC2, 0x100, 8))
CALLS = {0xCD} | set(range(0xC4, 0x100, 8))
THREE = JUMPS | CALLS | {0x01, 0x11, 0x21, 0x31, 0x22, 0x2A, 0x32, 0x3A}
TWO = {0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x36, 0x3E,
       0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE, 0xD3, 0xDB}

# Переменные и таблицы исторического RAMFOS из source/ramfos/{ramfos.asm,c800.inc,f800.inc}.
RF_VARIABLES = {0xC62C:'v_koi8', 0xC62D:'v_rusLat', 0xC62E:'v_capsLock', 0xC62F:'v_printerEcho',
                0xC63C:'v_selColor', 0xC63D:'v_aramMaxPage', 0xC65C:'v_inverse',
                0xC65E:'v_aramMaxPageI', 0xC65F:'v_tapeMode'}
RF_WORDS = {0xC60C:'v_charGenPtr', 0xC60E:'v_keybMap', 0xC61C:'v_aramPageEnd', 0xC61E:'v_aramPageStart',
            0xC63E:'v_cursorPos', 0xC64C:'v_beepConfig', 0xC64E:'v_oldBeep', 0xC660:'v_tapePulseB',
            0xC662:'v_tapePulseA', 0xC664:'v_tapePulseB2', 0xC666:'v_tapePulseA2',
            0xC668:'v_tapePulseB3', 0xC66A:'v_tapePulseA3'}
for address, symbol in RF_WORDS.items():
    RF_VARIABLES.update({address:symbol,address+1:symbol+'+1'})
RF_VECTORS = {0xC80F, 0xC821, 0xC82D, *range(0xC836, 0xC85B, 3)}
RF_HIGH_VECTORS = set(range(0xF800, 0xF858, 3))


def access_points(target, width, operation):
    """Вход в таблицу и доступ к байту данных различаются даже при одинаковом адресе."""
    for address in range(target, target + width):
        address &= 0xffff
        if address in RF_VARIABLES:
            yield 'abi', 'ramfos_variable', RF_VARIABLES[address], address
        if 0xF800 <= address <= 0xF803 or 0xFF00 <= address <= 0xFF03:
            yield 'hardware', 'std_ppi', 'STD ВВ55', address
        if 0xFFE0 <= address <= 0xFFE3:
            yield 'hardware', 'mx_ppi', 'MX ВВ55', address
        if 0xFFEC <= address <= 0xFFEF:
            yield 'hardware', 'mx_pit', 'MX ВИ53', address
        if address == 0xFFF8:
            yield 'hardware', 'mx_color', 'MX цвет', address
        if address >= 0xFFFC and 'write' in operation:
            yield 'hardware', 'mx_page', 'MX выбор страницы записью', address


def pointer_uses(data, load, pc, pair, target):
    """Проследить одну константу по прямой ветви до замены пары или передачи управления."""
    pos = pc + 3
    end = load + len(data)
    for _ in range(24):
        if pos >= end:
            break
        op = data[pos - load]
        size = 3 if op in THREE else 2 if op in TWO else 1
        if pos + size > end or op in UNDEFINED:
            break
        if op == 0xE9 and pair == 2:
            yield pos, op, target, 'jump'
        if op in JUMPS | CALLS or op in (0xC9, 0xE9, 0x76) or op & 0xC7 in (0xC0, 0xC7):
            break
        if pair == 2:
            if op == 0x36 or 0x70 <= op <= 0x77:
                yield pos, op, target, 'write'
            elif op in (0x34, 0x35):
                yield pos, op, target, 'read_write'
            elif 0x40 <= op <= 0xBF and op & 7 == 6:
                yield pos, op, target, 'read'
        elif op == 0x02 + pair * 16 or op == 0x0A + pair * 16:
            yield pos, op, target, 'write' if op & 8 == 0 else 'read'
        if op == 0x03 + pair * 16:
            target = (target + 1) & 0xffff
        elif op == 0x0B + pair * 16:
            target = (target - 1) & 0xffff
        elif op in (0x01 + pair * 16, 0xC1 + pair * 16):
            break
        elif (op == 0xEB and pair in (1, 2)) or (pair == 2 and (op in (0x2A, 0xE3) or op & 0xCF == 9)):
            break
        elif op >> 4 == pair and op & 7 in (4, 5, 6):
            break
        elif 0x40 <= op <= 0x7F and (op - 0x40) >> 4 == pair:
            break
        pos += size


def checksum(data):
    b = c = 0
    for i, x in enumerate(data):
        total = c + x
        c = total & 255
        if i + 1 < len(data):
            b = (b + x + (total >> 8)) & 255
    return b * 256 + c


def inventory():
    addresses = {}
    for folder in ('games', 'programs'):
        catalog = ROOT / 'assets' / folder / 'catalog.csv'
        with catalog.open(encoding='utf-8-sig', newline='') as stream:
            for row in csv.DictReader(stream):
                if row.get('LoadAddress'):
                    addresses[(Path('assets') / folder / row['File']).as_posix().lower()] = int(row['LoadAddress'], 0)
    for manifest in (ROOT / 'assets').glob('[A-Z]-*.txt'):
        for line in manifest.read_text(encoding='utf-8-sig').splitlines():
            words = line.split()
            if len(words) >= 2 and not line.lstrip().startswith('#'):
                try:
                    address = int(words[2], 0) if len(words) >= 3 else 0
                except ValueError:
                    continue
                source = (ROOT / 'assets' / words[1]).resolve()
                if source.is_relative_to(ROOT):
                    addresses[source.relative_to(ROOT).as_posix().lower()] = address
    for folder in ('games', 'programs'):
        for path in sorted((ROOT / 'assets' / folder).rglob('*')):
            if not path.is_file():
                continue
            name=path.relative_to(ROOT).as_posix()
            kind=path.suffix.lower()
            if kind == '.i80':
                if name.lower() not in addresses:
                    continue
            elif kind not in ('.rks','.exe','.com','.cpu'):
                continue
            raw=path.read_bytes()
            record={'file':name,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
            try:
                _, image=convert(path,'exe',load=addresses.get(name.lower()))
            except ValueError as error:
                yield record | {'error':str(error)}, None
                continue
            if kind=='.rks':
                record['container_sum_ok']=True
            record.update(load=image.load,end=image.load+len(image.data)-1,
                          entry=image.entry,payload_bytes=len(image.data))
            yield record,image.data


def analyse(record, data, public, *, environment=False):
    load = record['load']
    end = load + len(data)
    pending = [record['entry']]
    seen = set()
    findings = []
    evidence = {'hardware': [], 'abi': [], 'pointer': []}
    def evidence_at(group, kind, pc, op, target, operation, reason, **extra):
        evidence[group].append(dict(kind=kind, pc=pc, opcode=op, target=target,
                                    operation=operation, reason=reason, **extra))
    def memory_evidence(pc, op, target, operation, width=1, **extra):
        for group, kind, reason, address in access_points(target, width, operation):
            evidence_at(group, kind, pc, op, target, operation, reason, effective_address=address, width=width, **extra)
    def add(kind, pc, op, target=None):
        item = {'kind': kind, 'pc': pc, 'opcode': op}
        if target is not None:
            item['target'] = target
        findings.append(item)
    while pending:
        pc = pending.pop()
        while load <= pc < end and pc not in seen:
            seen.add(pc)
            op = data[pc - load]
            if op in UNDEFINED:
                add('undefined_opcode_or_data', pc, op)
                break
            length = 3 if op in THREE else 2 if op in TWO else 1
            if pc + length > end:
                add('truncated_instruction_or_data', pc, op)
                break
            target = int.from_bytes(data[pc - load + 1:pc - load + 3], 'little') if length == 3 else None
            if environment and op in JUMPS | CALLS and not load <= target < end:
                operation = 'call' if op in CALLS else 'jump'
                if target in RF_HIGH_VECTORS or target in RF_VECTORS:
                    name = 'ramfos_f800_vector' if target in RF_HIGH_VECTORS else 'ramfos_shared_vector'
                    reason = 'Таблица переходов RAMFOS F800h' if target in RF_HIGH_VECTORS else 'Вход C800h с отдельной семантикой RAMFOS'
                    evidence_at('abi', name, pc, op, target, operation, reason)
                if 0xC000 < target < 0xC800 and target in public and target != 0xC4A0:
                    evidence_at('abi', 'legacy_monitor_entry', pc, op, target, operation, 'Исторический вход Monitor-2')
            if op in JUMPS | CALLS:
                if load <= target < end:
                    pending.append(target)
                elif target in public:
                    add('mxos_public_call_requires_semantic_check', pc, op, target)
                    if 0xC000 <= target < 0xC800 and target != 0xC4A0:
                        add('legacy_monitor_entry', pc, op, target)
                elif target >= 0xC000:
                    add('external_system_control_transfer', pc, op, target)
                else:
                    add('external_low_control_transfer', pc, op, target)
                if op == 0xC3:
                    break
            if op in (0x22, 0x32, 0x2A, 0x3A):
                width = 2 if op in (0x22, 0x2A) else 1
                if environment:
                    operation = 'write' if op in (0x22, 0x32) else 'read'
                    memory_evidence(pc, op, target, operation, width)
                accessed = {(target + offset) & 0xffff for offset in range(width)}
                if accessed.intersection(range(0xFFEC, 0xFFF0)):
                    add('mx_pit_absolute_access', pc, op, target)
                if 0xFFF8 in accessed:
                    add('mx_color_absolute_access', pc, op, target)
                if op in (0x22, 0x32) and accessed.intersection(range(0xFFFC, 0x10000)):
                    add('mx_memory_page_write', pc, op, target)
                if target <= 0xF803 and target + width > 0xF800:
                    add('std_ppi_absolute_access', pc, op, target)
                if op in (0x22, 0x32) and 0xC000 <= target < 0xFFC0:
                    add('system_area_absolute_write', pc, op, target)
                if target >= 0xF800:
                    add('high_memory_absolute_access', pc, op, target)
            if op in (0x01, 0x11, 0x21) and 0xF800 <= target <= 0xF803:
                add('std_ppi_pointer_candidate', pc, op, target)
            if environment and op in (0x01, 0x11, 0x21) and (
                    target in RF_VARIABLES or target >= 0xF800 or target in RF_VECTORS or target in public):
                pair = op >> 4
                if list(access_points(target, 1, 'write')) or target in RF_VECTORS or target in public or target in RF_HIGH_VECTORS:
                    evidence_at('pointer', 'address_constant', pc, op, target, 'load_address',
                                'LXI загружает константу в регистровую пару', pair=('BC','DE','HL')[pair])
                for use_pc, use_op, addr, operation in pointer_uses(data, load, pc, pair, target):
                    if operation == 'jump':
                        if load <= addr < end:
                            continue
                        if addr in RF_HIGH_VECTORS:
                            evidence_at('abi', 'ramfos_f800_vector', use_pc, use_op, addr, operation,
                                        'PCHL по адресу таблицы RAMFOS', pointer_pc=pc)
                        elif addr in RF_VECTORS:
                            evidence_at('abi','ramfos_shared_vector',use_pc,use_op,addr,operation,
                                        'PCHL по адресу общего входа C800h',pointer_pc=pc)
                        elif addr in public and 0xC000 < addr < 0xC800 and addr!=0xC4A0:
                            evidence_at('abi','legacy_monitor_entry',use_pc,use_op,addr,operation,
                                        'PCHL по историческому входу Monitor-2',pointer_pc=pc)
                    else:
                        memory_evidence(use_pc, use_op, addr, operation, pointer_pc=pc)
            if op == 0xE9:
                add('unresolved_pchl', pc, op)
                break
            if op & 0xC7 == 0xC7:
                target = op & 0x38
                add('rst_requires_vector_check', pc, op, target)
                if load <= target < end:
                    pending.append(target)
            if op in (0xC9, 0x76):
                break
            pc += length
    record['decoded_instructions'] = len(seen)
    record['findings'] = findings
    record['finding_counts'] = dict(Counter(item['kind'] for item in findings))
    record['compatibility'] = 'unconfirmed'
    if environment:
        record['evidence'] = evidence
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--catalog', action='store_true', help='Весь игровой каталог и маршруты RUN для кампании')
    parser.add_argument('--previous-audit', type=Path, help='JSON прежнего аудита для проверки его точного охвата')
    parser.add_argument('--verify-lafan', type=Path, metavar='DIR', help='Сверить сохранённый каталог с пятью архивными ODI в DIR')
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    if args.verify_lafan:
        if lafan_directory(args.verify_lafan) != LAFAN.read_text(encoding='utf-8'):
            raise SystemExit('Извлечённые записи отличаются от ' + str(LAFAN))
        print('Все записи Lafan и нагрузки, на которые указывают их сектора, сверены с пятью ODI.')
        if not args.catalog and not args.output:
            return
    include = (ROOT / 'sdk/asm/abi/bios.inc').read_text(encoding='utf-8')
    public = {int(x, 16) for x in re.findall(r'^bios_\w+\s*=\s*0?([0-9A-Fa-f]{4})h', include, re.M)}
    if args.catalog:
        if args.previous_audit:
            previous = json.loads(args.previous_audit.read_text(encoding='utf-8'))['files']
        else:
            previous = [r for r, _ in inventory()]
        report = campaign_inventory(previous, analyse, public)
        output = args.output or ROOT / 'docs/compat/game-inventory.json'
        output.parent.mkdir(parents=True, exist_ok=True)
        output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
        print(json.dumps(report['summary'], ensure_ascii=False, indent=2))
        return
    records = []
    for record, data in inventory():
        records.append(analyse(record, data, public) if data is not None else record)
    summary = {'files': len(records), 'analysed': sum('findings' in r for r in records),
               'errors': dict(Counter(r['error'] for r in records if 'error' in r)),
               'files_with_findings': dict(Counter(k for r in records for k in r.get('finding_counts', {}))),
               'confirmed_compatible': 0}
    report = {'experiment': 'monitor-001', 'date': datetime.now(timezone.utc).date().isoformat(), 'method': 'static_recursive_8080',
              'limits': ['No execution; findings are candidates, not proven faults.',
                         'Indirect calls, runtime decoding, self-modification and input-dependent behavior remain unresolved.'],
              'summary': summary, 'files': records}
    output = args.output or ROOT / 'docs/compat/monitor-001-static.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
