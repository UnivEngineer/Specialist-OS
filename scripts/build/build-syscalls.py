"""Единый реестр MXOS: номера ASM/C и таблицы диспетчера; без адресов реализаций в SDK."""
import argparse
import json
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def generate(check=False):
    registry = json.loads((ROOT/'source/bios/syscalls.json').read_text('utf-8'))
    asm = ['; Создано scripts/build/build-syscalls.py из source/bios/syscalls.json.']
    c = ['/* Создано из source/bios/syscalls.json; номера групп и функций. */',
         '#ifndef MXOS_SYSCALL_IDS_H', '#define MXOS_SYSCALL_IDS_H']
    tables = ['; Создано из source/bios/syscalls.json; внутренние адреса, не публичный ABI.']
    z80 = ['; Создано из source/bios/syscalls.json для ассемблера z88dk.']
    for group in registry:
        group_id = group['group']
        asm.append(f"MXOS_GROUP_{group['name']} = {group_id}")
        c.append(f"#define MXOS_GROUP_{group['name']} {group_id}")
        assert len({s['id'] for s in group['services']}) == len(group['services'])
        tables.append(f"sys{group['name']}Table:")
        next_id = 0
        for service in group['services']:
            while next_id < service['id']:
                tables.append('    DW dos_dosUnsupported' if group_id == 2 else '    DW 0')
                next_id += 1
            next_id += 1
            value = group_id*256+service['id']
            name = 'MXOS_'+group['name']+'_'+service['name']
            asm.append(f'{name} = 0{value:04X}h')
            c.append(f'#define {name} 0x{value:04X}')
            z80.append(f'defc {name} = 0{value:04X}h')
            for alias in service.get('aliases', []):
                alias_name = 'MXOS_'+group['name']+'_'+alias
                asm.append(f'{alias_name} = {name}')
                c.append(f'#define {alias_name} {name}')
                z80.append(f'defc {alias_name} = {name}')
            tables.append('    DW '+service['target'])
        tables.append(f"sys{group['name']}Count = ($-sys{group['name']}Table)/2")
        # Байтовые команды очереди имеют отдельное пространство имён.
        for command in group.get('commands', []):
            name = f"MXOS_COMMAND_{group['name']}_{command['name']}"
            value = command['id']
            asm.append(f'{name} = 0{value:02X}h')
            c.append(f'#define {name} 0x{value:02X}')
            z80.append(f'defc {name} = 0{value:02X}h')
    # Адреса регистровых входов определены в карте MEM вместе с таблицей JMP.
    abi = (ROOT/'source/include/mxos.inc').read_text(encoding='utf-8')
    for name, addr in re.findall(r'^mx_(\w+)\s*=\s*0([0-9A-F]{4})h\b', abi, re.M):
        z80.append(f'defc mx_{name} = 0{addr}h')
    vector = re.search(r'^bios_printStringLim\s*=\s*(\w+)', abi, re.M)[1]
    z80.append(f'defc bios_printStringLim = {vector}')
    c.append('#endif')
    outputs = {
        ROOT/'source/include/syscall-ids.inc':asm,
        ROOT/'source/bios/syscall-tables.inc':tables,
        ROOT/'sdk/include/mxos_syscall_ids.h':c,
        ROOT/'sdk/src/mxos_syscall_ids.inc':z80,
    }
    changed = False
    for path, lines in outputs.items():
        if path.suffix == '.inc':
            parts = [line.partition(' = ') for line in lines]
            width = max((len(name) for name, separator, _ in parts if separator), default=0)
            lines = [f'{name:<{width}}{separator}{value}' if separator else name
                     for name, separator, value in parts]
        text = '\n'.join(lines)+'\n'
        if path.exists() and path.read_text('utf-8') == text:
            continue
        if check:
            raise SystemExit('Устаревший реестр: '+str(path))
        path.write_text(text,encoding='utf-8')
        changed = True
    return changed

if __name__ == '__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--check', action='store_true')
    print('[checking] System call registry', flush=True)
    changed = generate(p.parse_args().check)
    status = 'updated' if changed else 'up-to-date'
    print(f'[{status}] System call registry', flush=True)
