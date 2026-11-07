"""Парная сборка BIOS C000h и плотной банковой DOS с 0100h."""
import json
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def component_groups():
    """Раздельные составы BIOS/DOS; общие примитивы находятся в common."""
    manifests={region:json.loads((ROOT/f'source/{region}/layout.json').read_text('utf-8'))
               for region in ('bios','dos')}
    def resolve(unit,region):
        return {**unit,'files':[str((ROOT/f'source/{region}'/f).resolve()) for f in unit['files']]}
    manifest=manifests['bios']
    manifest['fixed']=[resolve(f,'bios') if 'files' in f else f for f in manifest['fixed']]
    groups={region:[resolve(u,region) for u in manifests[region]['units']]
            for region in ('bios','dos')}
    return manifest, groups

def main():
    import subprocess
    subprocess.run(['python',str(ROOT/'scripts/build/build-compact-font.py')],check=True)
    runpy.run_path(str(ROOT/'scripts/build/build-syscalls.py'))['generate']()
    runpy.run_path(str(ROOT/'scripts/build/build-banked-transport.py'))['main']()
    h = runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    source = ROOT/'source/dos'
    out = ROOT/'bin'
    out.mkdir(exist_ok=True)
    manifest, groups = component_groups()
    inc = h['include']
    lines = ['    DEFINE DOS_REFERENCE_BUILD 1',inc(ROOT/'source/include/mxos.inc'),inc(ROOT/'source/common/layoutMacros.inc'),inc(out/'banked/transport-imports.inc'),'DOS_IMAGE_SUM = 0','    ORG DOS_CODE_START','dosCoreStart:']
    for u in groups['dos']:
        for i,f in enumerate(u['files']):
            lines += h['emit_unit']({**u,'files':[f]},f'{u["name"]}_{i}')
    lines += ['dosCoreEnd:','dosImageEnd = dosCoreEnd','    ASSERT $ <= DOS_CODE_END_MAX','    ORG 0C000h']
    for f in manifest['fixed']:
        lines += [f'    ORG_PAD {h["number"](f["address"])}',*h['emit_fixed'](f)]
    for u in groups['bios']:
        if u.get('same_page'): lines += ['    ALIGN 256,0FFh']
        for i,f in enumerate(u['files']):
            lines += h['emit_unit']({**u,'files':[f]},f'{u["name"]}_{i}')
    lines += [inc(ROOT/'source/bios'/f) for f in ('getch.inc','printString1.inc','cmp_hl_de_2.inc')]
    lines += [inc(ROOT/'source/io/legacy-files.inc'), inc(ROOT/'source/bios/banked-start.inc'), inc(ROOT/'source/io/raw-local.inc'), inc(ROOT/'source/io/fileBody.inc'), inc(ROOT/'source/bios/syscall-tables.inc'), inc(ROOT/'source/io/glyphCache.inc')]
    probe=out/'System.measure.asm'
    probe.write_text('\n'.join(lines+['    END','']),encoding='utf-8')
    h['assemble'](probe,out/'System.measure.bin',[])
    measured=h['symbols'](out/'System.measure.sym')
    bios_plan=h['place'](manifest['fixed'],h['blocks']({'units':groups['bios']},measured,False))
    lines=['    ORG DOS_CODE_START','dosCoreStart:']
    for u in groups['dos']:
        lines += h['emit_unit'](u,u['name'])
    lines += ['dosCoreEnd:','dosImageEnd = dosCoreEnd','    ASSERT $ <= DOS_CODE_END_MAX']
    lines += [h['emit_plan'](bios_plan,manifest['fixed']).replace('    ORG 0C000h','    ORG_PAD 0C000h',1),
              inc(ROOT/'source/bios/banked-start.inc'),
              'ioTablesStart:', inc(ROOT/'source/bios/syscall-tables.inc'), 'ioTablesEnd:',
              'biosLowEnd:', '    ASSERT $ <= FONT_ADDR',
              '    ORG_PAD BIOS_IO_ADDR',
              'bankBiosImageStart:',
              '    INCBIN "'+(out/'banked/MAINBIOS.bin').as_posix()+'"',
              'bankBiosImageEnd:',
              '    ASSERT $ <= BIOS_BANK_GATE_END',
              '    ORG_PAD BIOS_BANK_IO_ADDR',
              'bankIoImageStart:',
              '    INCBIN "'+(out/'banked/MAINIO.bin').as_posix()+'"',
              'bankIoImageEnd:',
              inc(ROOT/'source/io/legacy-files.inc'),
              inc(ROOT/'source/io/fileBody.inc'),
              inc(ROOT/'source/io/raw-local.inc'),
              inc(ROOT/'source/io/glyphCache.inc'),
              'ioImageEnd:', '    ASSERT $ <= LOCAL_BLOCK_ENTRY',
              '    ASSERT BIOS_BANK_IO_BUFFER+261 <= MAIN_COMMAND_ADDR',
              '    ASSERT BIOS_BANK_STACK_TOP <= BIOS_BANK_IO_BUFFER']
    layout='\n'.join(lines)+'\n'
    (out/'System.layout.inc').write_text('DOS_IMAGE_SUM = 0\n'+layout,encoding='utf-8')
    h['assemble'](ROOT/'source/bios/BIOS.asm',out/'System.linked.bin',[])
    sy=h['symbols'](out/'System.linked.sym')
    raw=(out/'System.linked.bin').read_bytes()
    dos_start=sy['DOS_CODE_START']
    dos=raw[:sy['dosCoreEnd']-dos_start]
    total=sum(dos); prefix=total-dos[-1]
    checksum=(((prefix+(prefix>>8))&255)<<8)|(total&255)
    (out/'System.layout.inc').write_text(f'DOS_IMAGE_SUM = 0{checksum:04X}h\n'+layout,encoding='utf-8')
    h['assemble'](ROOT/'source/bios/BIOS.asm',out/'System.linked.bin',[])
    raw=(out/'System.linked.bin').read_bytes()
    assert raw[:len(dos)]==dos, 'Контрольная сумма не должна менять DOS'
    bios=raw[0xC000-dos_start:sy['biosLowEnd']-dos_start]
    io=raw[sy['BIOS_IO_ADDR']-dos_start:sy['ioImageEnd']-dos_start]
    (out/'IO.sys').write_bytes(io)
    (out/'DOS.native.sys').write_bytes(dos)
    (out/'BIOS.sys').write_bytes(bios)
    for stem in ('DOS','BIOS','IO'):
        (out/f'{stem}.sym').write_bytes((out/'System.linked.sym').read_bytes())
        (out/f'{stem}.lst').write_bytes((out/'System.linked.lst').read_bytes())
    report=dict(bios=dict(start=0xC000,end=0xC000+len(bios),bytes=len(bios)),
                io=dict(start=sy['BIOS_IO_ADDR'],end=sy['ioImageEnd'],bytes=len(io)),
                glyphCache=dict(start=sy['FONT_ADDR']+(ROOT/'assets/system-rom/FONTKOI.FNT').stat().st_size,end=sy['GLYPH_CACHE_END']),
                dos=dict(start=dos_start,end=sy['dosCoreEnd'],bytes=len(dos)),
                units={k:[u['name'] for u in v] for k,v in groups.items()},windows=manifest['fixed'],
                blocks=[dict(name=b['block']['key'],address=b['address'],size=b['block']['size'])
                        for b in sorted(bios_plan['placed'],key=lambda b:b['address'])]
                       + [dict(name='bankBiosGateway', address=sy['bankBiosImageStart'],
                               size=sy['bankBiosImageEnd']-sy['bankBiosImageStart']),
                          dict(name='bankDiskBridge', address=sy['bankIoImageStart'],
                               size=sy['bankIoImageEnd']-sy['bankIoImageStart']),
                          dict(name='legacyFileAdapters', address=sy['sysLegacyStart'],
                               size=sy['sysLegacyEnd']-sy['sysLegacyStart']),
                          dict(name='fileBodyLoader', address=sy['fileBodyLoad'],
                               size=sy['fileBodyEnd']-sy['fileBodyLoad']),
                          dict(name='bankedBoot',address=sy['biosBankCold'],size=sy['ioTablesStart']-sy['biosBankCold']),
                          dict(name='rawMain',address=sy['sysRawRoute'],size=sy['sysRawEnd']-sy['sysRawRoute']),
                          dict(name='glyphCache',address=sy['glyphCacheService'],size=sy['glyphCacheEnd']-sy['glyphCacheService']),
                          dict(name='ioInitialize',address=sy['ioInitialize'],size=sy['ioInitializeEnd']-sy['ioInitialize']),
                          dict(name='syscallTables',address=sy['ioTablesStart'],size=sy['ioTablesEnd']-sy['ioTablesStart'])])
    report['nativeDosReference'] = report['dos']
    runpy.run_path(str(ROOT/'scripts/build/build-banked-dos.py'))['main']()
    bank=bytearray((out/'banked/KERNEL.bin').read_bytes())
    # Активный слот заполняется при инициализации DOS из общего шаблона.
    (out/'DOS.sys').write_bytes(bank)
    bank_layout=json.loads((out/'banked/KERNEL.layout.json').read_text(encoding='utf-8'))
    report['dos']=dict(start=bank_layout['dosCoreStart'],end=bank_layout['kernelEnd'],bytes=len(bank),page=1,
                       coreEnd=bank_layout['dosCoreEnd'],extensionStart=bank_layout['dosCoreEnd'],
                       dataStart=bank_layout['DOS_BANK_DATA_ADDR'])
    (out/'System.layout.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f'BIOS: {len(bios)} bytes; IO: {len(io)} bytes; bank DOS: {len(bank)} bytes, end {bank_layout['kernelEnd']:04X}h')

if __name__=='__main__': main()
