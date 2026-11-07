"""Единый реестр MXOS: номера ASM/C и таблицы диспетчера; без адресов реализаций в SDK."""
import argparse
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def generate(check=False):
    registry = json.loads((ROOT/'source/bios/syscalls.json').read_text('utf-8'))
    asm = ['; Создано scripts/build/build-syscalls.py из source/bios/syscalls.json.']
    c = ['/* Создано из source/bios/syscalls.json; номера групп и функций. */',
         '#ifndef MXOS_SYSCALL_IDS_H', '#define MXOS_SYSCALL_IDS_H']
    tables = ['; Создано из source/bios/syscalls.json; внутренние адреса, не публичный ABI.']
    z80 = ['; Создано из source/bios/syscalls.json для ассемблера z88dk.']
    legacy = ['; Создано из source/bios/syscalls.json; исторические файловые входы BIOS.',
              '; Регистры передаются через общий диспетчер, выбор main/банка един.',
              'sysLegacyStart:']
    legacy_targets = set()
    for group in registry:
        group_id = group['group']
        asm.append(f"MX_GROUP_{group['name']} = {group_id}")
        c.append(f"#define MX_GROUP_{group['name']} {group_id}")
        assert [s['id'] for s in group['services']] == list(range(len(group['services'])))
        tables.append(f"sys{group['name']}Table:")
        for service in group['services']:
            value = group_id*256+service['id']
            name = 'MX_'+group['name']+'_'+service['name']
            asm.append(f'{name} = 0{value:04X}h')
            c.append(f'#define {name} 0x{value:04X}')
            z80.append(f'defc {name} = 0{value:04X}h')
            tables.append('    DW '+service['target'])
            if group_id == 2 and service['target'].startswith('dos_') and service['target'] not in legacy_targets:
                legacy_targets.add(service['target'])
                legacy += ['sysLegacy_'+service['target'][4:]+':', f'    MXCALL {name}', '    ret']
        tables.append(f"sys{group['name']}Count = ($-sys{group['name']}Table)/2")
    c.append('#endif')
    legacy.append('sysLegacyEnd:')
    outputs = {
        ROOT/'source/include/syscall-ids.inc':asm,
        ROOT/'source/bios/syscall-tables.inc':tables,
        ROOT/'source/io/legacy-files.inc':legacy,
        ROOT/'sdk/include/mxos_syscall_ids.h':c,
        ROOT/'sdk/src/mxos_syscall_ids.inc':z80,
    }
    for path, lines in outputs.items():
        text = '\n'.join(lines)+'\n'
        if path.exists() and path.read_text('utf-8') == text:
            continue
        if check:
            raise SystemExit('Устаревший реестр: '+str(path))
        path.write_text(text,encoding='utf-8')

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--check', action='store_true')
    generate(p.parse_args().check)
