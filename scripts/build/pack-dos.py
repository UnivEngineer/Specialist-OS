"""Библиотека измерения SjASMPlus и размещения BIOS между входами ABI.

Ассемблер разрешает все адреса заново, бинарники не патчатся. Разрывать можно
только явно объявленные цепочки INCLUDE: между их частями вставляется JP.
Копируемые драйверы и таблицы остаются неделимыми блоками.
"""
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'source/bios'
ASSEMBLER = ROOT / 'tasm/sjasmplus.exe'
MANIFEST = SOURCE / 'layout.json'


def number(value):
    return f'0{value:04X}h'


def include(path):
    return f'    INCLUDE "{path.as_posix()}"'


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text('utf-8-sig'), re.M)}


def emit_unit(unit, key, bridge=None):
    lines = [f'dos_piece_{key}_start:']
    if unit.get('condition'):
        lines.append(f'    IF {unit["condition"]}')
    lines.extend(include(SOURCE / name) for name in unit['files'])
    if unit.get('condition'):
        lines.append('    ENDIF')
    lines.append(f'dos_piece_{key}_end:')
    if bridge:
        lines.append(f'    DOS_ABI_BRIDGE dos_piece_{bridge[0]}_start, {number(bridge[1])}')
    return lines


def emit_fixed(item):
    start, size = item['address'], item['size']
    lines = [f'    ASSERT $ == {number(start)}']
    if item.get('abi'):
        lines.append(f'    ASSERT {item["abi"]} == {number(start)}')
    if item.get('target'):
        lines.append(f'    DOS_ABI_GATE {number(start)}, {item["target"]}')
    else:
        lines.extend(include(SOURCE / name) for name in item['files'])
    lines.append(f'    ASSERT $ == {number(start + size)}')
    return lines


def assemble(source, output, defines):
    command = [str(ASSEMBLER), '--i8080', '-Wno-rdlow', *['-D'+d for d in defines],
               f'--raw={output}', f'--sym={output.with_suffix(".sym")}',
               f'--lst={output.with_suffix(".lst")}', str(source)]
    subprocess.run(command, cwd=SOURCE, check=True)


def blocks(manifest, measured, split):
    result = []
    for unit in manifest['units']:
        parts = []
        for i, name in enumerate(unit['files']):
            key = f'{unit["name"]}_{i}'
            size = measured[f'dos_piece_{key}_end']-measured[f'dos_piece_{key}_start']
            parts.append(dict(key=key, files=[name], size=size, condition=unit.get('condition')))
        if split and unit.get('chain'):
            for i, part in enumerate(parts):
                link = parts[i+1]['key'] if i+1 < len(parts) else None
                result.append(dict(key=part['key'], parts=[part],
                                   size=part['size']+(3 if link else 0), link=link))
        else:
            result.append(dict(key=unit['name'], parts=parts,
                               size=sum(p['size'] for p in parts), same_page=unit.get('same_page')))
    return result


def place(fixed, candidates):
    holes = [(a['address']+a['size'], b['address']) for a,b in zip(fixed,fixed[1:])
             if a['address']+a['size'] < b['address']]
    remaining = {b['key']: b for b in candidates if b['size']}
    placed = []
    # Таблицы с младшим восьмибитным индексом не пересекают страницу.
    for block in list(remaining.values()):
        if not block.get('same_page'):
            continue
        choices = []
        for i,(start,end) in enumerate(holes):
            at = start if (start & 255)+block['size'] <= 256 else (start+255)&~255
            if at+block['size'] <= end:
                choices.append((end-start, -at, i, at))
        if not choices:
            raise ValueError(f'Нет окна для таблицы {block["key"]}')
        _,_,index,at = max(choices)
        lo,hi = holes.pop(index)
        holes += [(a,b) for a,b in [(lo,at),(at+block['size'],hi)] if a<b]
        placed.append(dict(block=block,address=at,limit=at+block['size']))
        del remaining[block['key']]
    # Точное заполнение каждого окна целыми блоками; сначала малые окна.
    for start,end in sorted(holes,key=lambda h:(h[1]-h[0],h[0])):
        capacity = end-start
        states = {0: ()}
        for key,block in sorted(remaining.items(),key=lambda p:(-p[1]['size'],p[0])):
            size = block['size']
            for used, selected in sorted(list(states.items()),reverse=True):
                if used+size <= capacity and used+size not in states:
                    states[used+size] = selected+(key,)
        for key in states[max(states)]:
            block = remaining.pop(key)
            placed.append(dict(block=block,address=start,limit=start+block['size']))
            start += block['size']
    tail = fixed[-1]['address']+fixed[-1]['size']
    for block in candidates:
        if block['key'] in remaining:
            placed.append(dict(block=block,address=tail,limit=tail+block['size']))
            tail += block['size']
    return dict(placed=placed,end=tail,bridges=sum(bool(b.get('link')) for b in candidates),
                empty=[b for b in candidates if not b['size']])


def emit_plan(plan, fixed):
    lines = ['; Сгенерировано scripts/build/pack-dos.py; исходники — source/bios/ и source/common/.',
             '    ORG 0C000h']
    for block in plan['empty']:
        for part in block['parts']:
            lines += emit_unit(part,part['key'])
    entries = [(f['address'],'fixed',f) for f in fixed]
    entries += [(p['address'],'body',p) for p in plan['placed']]
    for address,kind,item in sorted(entries,key=lambda e:e[0]):
        lines.append(f'    ORG_PAD {number(address)}')
        if kind == 'fixed':
            lines += emit_fixed(item)
            continue
        block = item['block']
        for i,part in enumerate(block['parts']):
            link = (block['link'],item['limit']) if block.get('link') else None
            lines += emit_unit(part,part['key'],link if i==len(block['parts'])-1 else None)
            lines.append(f'    ASSERT dos_piece_{part["key"]}_end - dos_piece_{part["key"]}_start == {part["size"]}')
        lines += [f'    ASSERT $ == {number(item["limit"])}']
    return '\n'.join(lines+['biosPackedEnd:',f'    ASSERT $ == {number(plan["end"])}',''])


if __name__ == '__main__':
    raise SystemExit('Используйте scripts/build/build-bios-dos.py для парной сборки BIOS/DOS.')
