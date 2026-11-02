"""Измерение SjASMPlus и размещение целых блоков DOS между входами ABI.

Ассемблер разрешает все адреса заново, бинарники не патчатся. Разрывать можно
только явно объявленные цепочки INCLUDE: между их частями вставляется JP.
Копируемые драйверы и таблицы остаются неделимыми блоками.
"""
import argparse
import json
import re
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / 'source/dos'
ASSEMBLER = ROOT / 'tasm/sjasmplus.exe'
MANIFEST = SOURCE / 'layout.json'


def number(value):
    return f'0{value:04X}h'


def include(path):
    return f'    INCLUDE "{path.as_posix()}"'


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text('utf-8-sig'), re.M)}


def dependencies(paths):
    found = set()
    def visit(path):
        path = path.resolve()
        if path in found:
            return
        found.add(path)
        for name in re.findall(r'^\s*(?:INCLUDE|INCBIN)\s+"([^"]+)"',
                               path.read_text('utf-8-sig'), re.M | re.I):
            # DOS.asm имеет только резервный путь к генерируемому INCLUDE.
            if path.name == 'DOS.asm' and name.endswith('DOS.layout.inc'):
                continue
            nested = path.parent / name
            if nested.suffix.lower() in ('.asm', '.inc'):
                visit(nested)
            else:
                found.add(nested.resolve())
    for path in paths:
        visit(path)
    return found


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


def probe_source(manifest):
    lines = [include(ROOT/'source/include/mxos.inc'), include(SOURCE/'layoutMacros.inc'),
             '    ORG 0C000h']
    for item in manifest['fixed']:
        lines += [f'    ORG_PAD {number(item["address"])}', *emit_fixed(item)]
    for unit in manifest['units']:
        if unit.get('same_page'):
            lines.append('    ALIGN 256, 0FFh')
        for i, name in enumerate(unit['files']):
            part = {**unit, 'files': [name]}
            lines += emit_unit(part, f'{unit["name"]}_{i}')
    lines += [include(SOURCE/name) for name in ['getch.inc','printString1.inc','cmp_hl_de_2.inc']]
    return '\n'.join(lines + ['    END', ''])


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
    lines = ['; Сгенерировано scripts/build/pack-dos.py; исходники — source/dos/.',
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
    return '\n'.join(lines+['dosImageEnd:',f'    ASSERT $ == {number(plan["end"])}',''])


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--output',type=Path,default=ROOT/'bin/DOS.sys')
    p.add_argument('--define',action='append',default=[])
    p.add_argument('--configuration',default='Configured')
    p.add_argument('--force',action='store_true')
    p.add_argument('--chains',choices=['auto','whole','split'],default='auto')
    args = p.parse_args()
    output = args.output.resolve()
    output.parent.mkdir(parents=True,exist_ok=True)
    layout = output.with_suffix('.layout.inc')
    report = output.with_suffix('.layout.json')
    stamp = output.with_suffix(output.suffix+'.build-config')
    manifest = json.loads(MANIFEST.read_text('utf-8'))
    paths = [SOURCE/'DOS.asm',ROOT/'source/include/mxos.inc',SOURCE/'layoutMacros.inc']
    paths += [SOURCE/name for u in manifest['units']+manifest['fixed'] for name in u.get('files',[])]
    inputs = dependencies(paths)|{MANIFEST,Path(__file__).resolve(),ASSEMBLER}
    signature = json.dumps(dict(configuration=args.configuration,defines=args.define,chains=args.chains),sort_keys=True)
    outputs = [output,output.with_suffix('.lst'),output.with_suffix('.sym'),layout,report,stamp]
    if not args.force and all(p.exists() for p in outputs) and stamp.read_text('utf-8') == signature:
        if min(p.stat().st_mtime_ns for p in outputs) >= max(p.stat().st_mtime_ns for p in inputs):
            print(f'[up-to-date] {output}')
            return
    print(f'[pack DOS] {output}',flush=True)
    stamp.unlink(missing_ok=True)
    probe = output.with_suffix('.measure.asm')
    probe.write_text(probe_source(manifest),encoding='utf-8')
    assemble(probe,output.with_suffix('.measure.bin'),args.define)
    measured = symbols(output.with_suffix('.measure.sym'))
    plans = []
    for split in [False,True]:
        if args.chains != 'auto' and split != (args.chains == 'split'):
            continue
        plans.append(place(manifest['fixed'],blocks(manifest,measured,split)))
    plan = min(plans,key=lambda v:(v['end'],v['bridges']))
    layout.write_text(emit_plan(plan,manifest['fixed']),encoding='utf-8')
    master = output.with_suffix('.build.asm')
    master.write_text(f'    DEFINE DOS_LAYOUT "{layout.as_posix()}"\n'+include(SOURCE/'DOS.asm')+'\n',encoding='utf-8')
    assemble(master,output,args.define)
    if len(output.read_bytes()) != plan['end']-0xC000:
        raise ValueError('Размер raw не совпадает с картой размещения')
    metadata = dict(bytes=plan['end']-0xC000,end=plan['end'],bridges=plan['bridges'],
                    defines=args.define,windows=manifest['fixed'],blocks=[dict(
                        name=b['block']['key'],address=b['address'],size=b['block']['size'],
                        link=b['block'].get('link')) for b in sorted(plan['placed'],key=lambda b:b['address'])],
                    alternatives=[dict(end=v['end'],bridges=v['bridges']) for v in plans])
    report.write_text(json.dumps(metadata,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    stamp.write_text(signature,encoding='utf-8')
    print(f'DOS: {metadata["bytes"]} bytes, end {metadata["end"]:04X}h, bridges {metadata["bridges"]}')


if __name__ == '__main__':
    main()
