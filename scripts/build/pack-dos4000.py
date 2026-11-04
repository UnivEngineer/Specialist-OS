"""Отдельная сборка DOS с 4000h и монитор-совместимого BIOS с C000h.

Использует измерение и размещатель основного DOS. Не меняет DOS.sys и образы
NC; переключение страниц и загрузка двух частей в ОС сюда не входят.
"""
import json
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT/'source/dos'


def main():
    h = runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    manifest = json.loads((SOURCE/'layout.json').read_text('utf-8'))
    profile = json.loads((SOURCE/'layout4000.json').read_text('utf-8'))
    bios_names = set(profile['bios_units'])
    names = {u['name'] for u in manifest['units']}
    assert bios_names <= names, bios_names-names
    groups = {kind:[u for u in manifest['units'] if (u['name'] in bios_names)==(kind=='bios')]
              for kind in ['dos','bios']}
    out = ROOT/'bin/DOS4000.sys'
    out.parent.mkdir(parents=True,exist_ok=True)
    defines = ['DOS_SPLIT_BUILD=1']
    inc = h['include']
    lines = [inc(ROOT/'source/include/mxos.inc'),inc(SOURCE/'layoutMacros.inc'),
             '    ORG 04000h']
    for unit in groups['dos']:
        for i,name in enumerate(unit['files']):
            lines += h['emit_unit']({**unit,'files':[name]},f'{unit["name"]}_{i}')
    lines += ['dosCoreEnd:', '    ORG 0C000h']
    for fixed in manifest['fixed']:
        lines += [f'    ORG_PAD {h["number"](fixed["address"])}',*h['emit_fixed'](fixed)]
    for unit in groups['bios']:
        if unit.get('same_page'): lines += ['    ALIGN 256,0FFh']
        for i,name in enumerate(unit['files']):
            lines += h['emit_unit']({**unit,'files':[name]},f'{unit["name"]}_{i}')
    lines += [inc(SOURCE/name) for name in ['getch.inc','printString1.inc','cmp_hl_de_2.inc']]
    probe = out.with_suffix('.measure.asm')
    probe.write_text('\n'.join(lines+['    END','']),encoding='utf-8')
    h['assemble'](probe,out.with_suffix('.measure.bin'),defines)
    measured = h['symbols'](out.with_suffix('.measure.sym'))
    bios_manifest = {**manifest,'units':groups['bios']}
    bios_plan = h['place'](manifest['fixed'],h['blocks'](bios_manifest,measured,False))
    assert bios_plan['end'] <= profile['bios_limit'], 'BIOS не помещается в системную область'
    core_blocks = h['blocks']({**manifest,'units':groups['dos']},measured,False)
    lines = ['; Отдельное размещение DOS 4000h / BIOS C000h.', '    ORG 04000h',
             'dosCoreStart:']
    core_end = profile['dos_start']
    blocks = []
    for block in core_blocks:
        begin = core_end
        if block.get('same_page') and (begin&255)+block['size']>256:
            begin = (begin+255)&~255
            lines += [f'    ORG_PAD {h["number"](begin)}']
        for part in block['parts']:
            lines += h['emit_unit'](part,part['key'])
            lines += [f'    ASSERT dos_piece_{part["key"]}_end - dos_piece_{part["key"]}_start == {part["size"]}']
        core_end = begin+block['size']
        lines += [f'    ASSERT $ == {h["number"](core_end)}']
        blocks.append(dict(name=block['key'],region='dos',address=begin,size=block['size']))
    lines += ['dosCoreEnd:','    ASSERT $ <= DOS_CODE_END_MAX']
    assert core_end <= profile['dos_limit'], 'DOS не помещается до общих переменных BIOS'
    bios_layout = h['emit_plan'](bios_plan,manifest['fixed']).replace(
        '    ORG 0C000h','    ORG_PAD 0C000h',1)
    lines += [bios_layout,'biosCoreEnd:']
    for b in bios_plan['placed']:
        blocks.append(dict(name=b['block']['key'],region='bios',address=b['address'],size=b['block']['size']))
    layout = out.with_suffix('.layout.inc')
    layout.write_text('\n'.join(lines)+'\n',encoding='utf-8')
    master = out.with_suffix('.build.asm')
    master.write_text(f'    DEFINE DOS_LAYOUT "{layout.as_posix()}"\n'+inc(SOURCE/'DOS.asm')+'\n',encoding='utf-8')
    linked = out.with_suffix('.linked.bin')
    h['assemble'](master,linked,defines)
    raw = linked.read_bytes()
    assert len(raw) == bios_plan['end']-profile['dos_start']
    sy = h['symbols'](linked.with_suffix('.sym'))
    assert sy['DOS_CODE_START']==profile['dos_start']
    assert sy['DOS_CODE_END_MAX']==profile['dos_limit']
    assert sy['DOS_END_MAX']==profile['bios_limit']
    out.with_suffix('.sym').write_bytes(linked.with_suffix('.sym').read_bytes())
    out.with_suffix('.lst').write_bytes(linked.with_suffix('.lst').read_bytes())
    core = raw[:core_end-profile['dos_start']]
    bios = raw[profile['bios_start']-profile['dos_start']:]
    out.write_bytes(core)
    bios_out = out.with_name('DOS4000.bios.sys'); bios_out.write_bytes(bios)
    memory = bytearray(65536)
    memory[profile['dos_start']:core_end] = core
    memory[profile['bios_start']:bios_plan['end']] = bios
    # Снимок предназначен для CPU-проверок: добавляем штатный шрифт и
    # начальное состояние BIOS без вызова незавершённого загрузчика DOS.
    font=(ROOT/'assets/system-rom/FONTKOI.FNT').read_bytes()
    assert len(font)==1280
    memory[sy['FONT_ADDR']:sy['FONT_ADDR']+len(font)]=font
    memory[sy['bios_vars']:sy['bios_vars']+sy['BIOS_VARIABLES']]=memory[sy['initVars']:sy['initVarsEnd']]
    out.with_suffix('.memory.bin').write_bytes(memory)
    assert sy['dosCoreStart']==0x4000 and sy['dosCoreEnd']==core_end
    for name in bios_names:
        for part in next(u for u in manifest['units'] if u['name']==name)['files']:
            assert part not in [p for u in groups['dos'] for p in u['files']]
    report = dict(profile='DOS4000',futureDosBudget=profile['future_dos_budget'],
                  integratedWithNC=False,bankSwitching=False,
                  regions=[dict(name='dos',start=0x4000,end=core_end,bytes=len(core),file=out.name),
                           dict(name='bios',start=0xC000,end=bios_plan['end'],bytes=len(bios),file=bios_out.name)],
                  windows=manifest['fixed'],blocks=blocks)
    out.with_suffix('.layout.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f'DOS4000: {len(core)} bytes at 4000h; BIOS: {len(bios)} bytes at C000h; NC integration deferred')


if __name__ == '__main__': main()
