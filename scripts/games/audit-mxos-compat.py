"""Статические кандидаты несовместимости библиотеки с ядром MXOS.

Обход прямого потока 8080 от точки входа; данные не сканируются как код подряд.
Косвенные переходы, распаковка и самомодификация делают результат неполным.
Отсутствие замечаний не является доказательством совместимости.
"""
import argparse
from collections import Counter
import csv
import hashlib
import json
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parents[2]
import sys
sys.path.insert(0,str(ROOT/'scripts/images'))
from executable import convert
UNDEFINED = {0x08, 0x10, 0x18, 0x20, 0x28, 0x30, 0x38,
             0xCB, 0xD9, 0xDD, 0xED, 0xFD}
JUMPS = {0xC3} | set(range(0xC2, 0x100, 8))
CALLS = {0xCD} | set(range(0xC4, 0x100, 8))
THREE = JUMPS | CALLS | {0x01, 0x11, 0x21, 0x31, 0x22, 0x2A, 0x32, 0x3A}
TWO = {0x06, 0x0E, 0x16, 0x1E, 0x26, 0x2E, 0x36, 0x3E,
       0xC6, 0xCE, 0xD6, 0xDE, 0xE6, 0xEE, 0xF6, 0xFE, 0xD3, 0xDB}


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
    for manifest in (ROOT / 'scripts/images/manifests').glob('*.txt'):
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
            if not path.is_file() or path.suffix.lower() not in ('.rks','.exe','.com','.cpu'):
                continue
            name=path.relative_to(ROOT).as_posix()
            raw=path.read_bytes()
            record={'file':name,'bytes':len(raw),'sha256':hashlib.sha256(raw).hexdigest()}
            try:
                _, image=convert(path,'exe',load=addresses.get(name.lower()))
            except ValueError as error:
                yield record | {'error':str(error)}, None
                continue
            if path.suffix.lower()=='.rks':
                record['container_sum_ok']=True
            record.update(load=image.load,end=image.load+len(image.data)-1,
                          entry=image.entry,payload_bytes=len(image.data))
            yield record,image.data


def analyse(record, data, public):
    load = record['load']
    end = load + len(data)
    pending = [record['entry']]
    seen = set()
    findings = []
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
            if op in JUMPS | CALLS:
                if load <= target < end:
                    pending.append(target)
                elif target in public:
                    add('mxos_public_call_requires_semantic_check', pc, op, target)
                elif target >= 0xC000:
                    add('external_system_control_transfer', pc, op, target)
                else:
                    add('external_low_control_transfer', pc, op, target)
                if op == 0xC3:
                    break
            if op in (0x22, 0x32, 0x2A, 0x3A):
                width = 2 if op in (0x22, 0x2A) else 1
                if target <= 0xF803 and target + width > 0xF800:
                    add('std_ppi_absolute_access', pc, op, target)
                if op in (0x22, 0x32) and 0xC000 <= target < 0xFFC0:
                    add('system_area_absolute_write', pc, op, target)
                if target >= 0xF800:
                    add('high_memory_absolute_access', pc, op, target)
            if op in (0x01, 0x11, 0x21) and 0xF800 <= target <= 0xF803:
                add('std_ppi_pointer_candidate', pc, op, target)
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
    return record


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, default=ROOT / 'docs/compat/monitor-001-static.json')
    args = parser.parse_args()
    include = (ROOT / 'source/include/mxos.inc').read_text(encoding='utf-8')
    public = {int(x, 16) for x in re.findall(r'^bios_\w+\s*=\s*0?([0-9A-Fa-f]{4})h', include, re.M)}
    records = []
    for record, data in inventory():
        records.append(analyse(record, data, public) if data is not None else record)
    summary = {'files': len(records), 'analysed': sum('findings' in r for r in records),
               'errors': dict(Counter(r['error'] for r in records if 'error' in r)),
               'files_with_findings': dict(Counter(k for r in records for k in r.get('finding_counts', {}))),
               'confirmed_compatible': 0}
    report = {'experiment': 'monitor-001', 'date': '2026-10-31', 'method': 'static_recursive_8080',
              'limits': ['No execution; findings are candidates, not proven faults.',
                         'Indirect calls, runtime decoding, self-modification and input-dependent behavior remain unresolved.'],
              'summary': summary, 'files': records}
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(json.dumps(report, ensure_ascii=False, indent=2) + '\n', encoding='utf-8')
    print(json.dumps(summary, ensure_ascii=False, indent=2))


if __name__ == '__main__':
    main()
