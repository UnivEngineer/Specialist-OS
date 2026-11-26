"""РџР°СЂРЅР°СЏ СЃР±РѕСЂРєР° BIOS C000h Рё РїР»РѕС‚РЅРѕР№ Р±Р°РЅРєРѕРІРѕР№ DOS СЃ 0100h."""
import os
import json
import runpy
from pathlib import Path

ROOT = Path(__file__).absolute().parents[2]

def component_groups():
    """Р Р°Р·РґРµР»СЊРЅС‹Рµ СЃРѕСЃС‚Р°РІС‹ BIOS/DOS; РѕР±С‰РёРµ РїСЂРёРјРёС‚РёРІС‹ РЅР°С…РѕРґСЏС‚СЃСЏ РІ common."""
    manifests={region:json.loads((ROOT/f'source/{region}/layout.json').read_text('utf-8'))
               for region in ('bios','dos')}
    def resolve(unit,region):
        return {**unit,'files':[os.path.normpath(ROOT/f'source/{region}'/f) for f in unit['files']]}
    manifest=manifests['bios']
    start, size = runpy.run_path(str(ROOT/'scripts/build/build-syscalls.py'))['bios_vector_window'](manifest)
    manifest['fixed'].append(dict(address=start, size=size, files=['vectors.inc']))
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
    main_io = ['bankBiosImageStart:',
               inc(ROOT/'source/io/main-bios-gate.inc'),
               'bankBiosImageEnd:',
               inc(ROOT/'source/io/main-ppi.inc'),
               inc(ROOT/'source/io/key-scan.inc'),
               inc(ROOT/'source/io/beep-timer.inc'),
               inc(ROOT/'source/io/input-clock.inc'),
               'mainPpiImageEnd:',
               inc(ROOT/'source/io/pit-tail.inc')]
    lines = ['    DEFINE DOS_REFERENCE_BUILD 1',inc(ROOT/'source/common/system.inc'),
             inc(ROOT/'source/common/version.inc'),inc(ROOT/'source/common/layoutMacros.inc'),
             inc(out/'banked/transport-imports.inc'),'DOS_IMAGE_SUM = 0','    ORG DOS_CODE_START','dosCoreStart:']
    for u in groups['dos']:
        for i,f in enumerate(u['files']):
            lines += h['emit_unit']({**u,'files':[f]},f'{u["name"]}_{i}')
    lines += [h['include'](source/'variables.inc'),'dosCoreEnd:','dosImageEnd = dosCoreEnd','    ASSERT $ <= DOS_CODE_END_MAX','    ORG 0C000h']
    for f in manifest['fixed']:
        lines += [f'    ORG_PAD {h["number"](f["address"])}',*h['emit_fixed'](f)]
    for u in groups['bios']:
        if u.get('same_page'): lines += ['    ALIGN 256,0FFh']
        for i,f in enumerate(u['files']):
            lines += h['emit_unit']({**u,'files':[f]},f'{u["name"]}_{i}')
    lines += [inc(ROOT/'source/bios'/f) for f in ('getch.inc','cmp_hl_de_2.inc')]
    lines += [inc(ROOT/'source/bios/banked-start.inc'), inc(ROOT/'source/bios/syscall-tables.inc'),
              '    ORG BIOS_IO_ADDR', *main_io]
    probe=out/'System.measure.asm'
    probe.write_text('\n'.join(lines+['    END','']),encoding='utf-8')
    h['assemble'](probe,out/'System.measure.bin',[])
    measured=h['symbols'](out/'System.measure.sym')
    bios_plan=h['place'](manifest['fixed'],h['blocks']({'units':groups['bios']},measured,False))
    lines=['    ORG DOS_CODE_START','dosCoreStart:']
    for u in groups['dos']:
        lines += h['emit_unit'](u,u['name'])
    lines += [h['include'](source/'variables.inc'),'dosCoreEnd:','dosImageEnd = dosCoreEnd','    ASSERT $ <= DOS_CODE_END_MAX']
    lines += [h['emit_plan'](bios_plan,manifest['fixed']).replace('    ORG 0C000h','    ORG_PAD 0C000h',1),
              inc(ROOT/'source/bios/banked-start.inc'),
              'ioTablesStart:', inc(ROOT/'source/bios/syscall-tables.inc'), 'ioTablesEnd:',
              'biosLowEnd:', '    ASSERT $ <= FONT_ENGINE_BASE',
              '    ORG_PAD BIOS_IO_ADDR', *main_io,
              '    ASSERT $ <= BIOS_BANK_GATE_END',
              'bankIoImageStart = BIOS_BANK_IO_ADDR',
              'bankIoImageEnd = BIOS_BANK_IO_ADDR+3',
              'ioImageEnd:', '    ASSERT $ <= MEM_IMAGE_LOCAL_START',
              '    ASSERT BIOS_BANK_IO_BUFFER+261 <= MAIN_COMMAND_ADDR',
              '    ASSERT BIOS_BANK_STACK_TOP <= BIOS_BANK_IO_BUFFER']
    layout='\n'.join(lines)+'\n'
    (out/'System.layout.inc').write_text('DOS_IMAGE_SUM = 0\n'+layout,encoding='utf-8')
    h['assemble'](ROOT/'source/bios/BIOS.asm',out/'System.linked.bin',[])
    sy=h['symbols'](out/'System.linked.sym')
    assert sy['FONT_ADDR']+sy['FONT_BYTES'] <= sy['FONT_ARENA_END'], 'РЁСЂРёС„С‚ РїРµСЂРµСЃРµРєР°РµС‚ СЃРёСЃС‚РµРјРЅС‹Р№ СЃС‚РµРє'
    raw=(out/'System.linked.bin').read_bytes()
    dos_start=sy['DOS_CODE_START']
    dos=raw[:sy['dosCoreEnd']-dos_start]
    total=sum(dos); prefix=total-dos[-1]
    checksum=(((prefix+(prefix>>8))&255)<<8)|(total&255)
    (out/'System.layout.inc').write_text(f'DOS_IMAGE_SUM = 0{checksum:04X}h\n'+layout,encoding='utf-8')
    h['assemble'](ROOT/'source/bios/BIOS.asm',out/'System.linked.bin',[])
    raw=(out/'System.linked.bin').read_bytes()
    assert raw[:len(dos)]==dos, 'РљРѕРЅС‚СЂРѕР»СЊРЅР°СЏ СЃСѓРјРјР° РЅРµ РґРѕР»Р¶РЅР° РјРµРЅСЏС‚СЊ DOS'
    bios=raw[0xC000-dos_start:sy['biosLowEnd']-dos_start]
    io=raw[sy['BIOS_IO_ADDR']-dos_start:sy['ioImageEnd']-dos_start]
    key_imports = ('bankKeyEvent', 'bankCursorSet')
    (out/'banked/key-imports.inc').write_text(''.join(
        f'{name} = 0{sy[name]:04X}h\n' for name in key_imports), encoding='utf-8')
    h['assemble'](ROOT/'source/io/KEYBANK.asm',out/'banked/KEYBANK.bin',[])
    keybank=(out/'banked/KEYBANK.bin').read_bytes()
    (out/'IO.main.bin').write_bytes(io)  # Flat MAIN segment for standalone CPU fixtures.
    (out/'IO.sys').write_bytes(io+keybank)
    (out/'banked/MAINBIOS.bin').write_bytes(io[:sy['bankBiosImageEnd']-sy['bankBiosImageStart']])
    for suffix in ('sym','lst'):
        (out/f'banked/MAINBIOS.{suffix}').write_bytes((out/f'System.linked.{suffix}').read_bytes())
    (out/'DOS.native.sys').write_bytes(dos)
    (out/'BIOS.sys').write_bytes(bios)
    (out/'banked/MAINIO.bin').write_bytes(bytes([0xC3,sy['mainBlockIO']&255,sy['mainBlockIO']>>8]))
    (out/'banked/MAINIO.sym').write_bytes((out/'System.linked.sym').read_bytes())
    # Internal linked entry: the flash installer is rebuilt with this BIOS.
    (out/'bios-copy-imports.inc').write_text(
        '; Generated internal BIOS copy entry; not a fixed public ABI.\n'
        f'bios_copyForward = 0{sy["moveBytesForward"]:04X}h\n', encoding='utf-8')
    (out/'bios-move-switch.inc').write_text(f'biosMoveUseLow = 0{sy["biosMoveUseLow"]:04X}h\n', encoding='utf-8')
    for stem in ('DOS','BIOS','IO'):
        (out/f'{stem}.sym').write_bytes((out/'System.linked.sym').read_bytes())
        (out/f'{stem}.lst').write_bytes((out/'System.linked.lst').read_bytes())
    report=dict(bios=dict(start=0xC000,end=0xC000+len(bios),bytes=len(bios)),
                fontEngine=dict(start=sy['FONT_ENGINE_BASE'],end=sy['FONT_ENGINE_END']),
                io=dict(start=sy['BIOS_IO_ADDR'],end=sy['ioImageEnd'],bytes=len(io),bankAddress=sy['IO_BANK_KEYS'],bankBytes=len(keybank)),
                diskBuffer=dict(start=sy['BIOS_DISK_BUFFER'],end=sy['BIOS_DISK_BUFFER_END']),
                temporaryBuffer=dict(start=sy['BIOS_TEMP_BUFFER'],end=sy['BIOS_TEMP_BUFFER_END']),
                upperReserve=dict(start=sy['FONT_ADDR']+sy['FONT_BYTES'],end=sy['BIOS_BANK_STACK_BOTTOM']),
                glyphCache=dict(start=sy['FONT_ADDR']+sy['FONT_BYTES'],end=sy['GLYPH_CACHE_END']),
                dos=dict(start=dos_start,end=sy['dosCoreEnd'],bytes=len(dos)),
                units={k:[u['name'] for u in v] for k,v in groups.items()},windows=manifest['fixed'],
                blocks=[dict(name=b['block']['key'],address=b['address'],size=b['block']['size'])
                        for b in sorted(bios_plan['placed'],key=lambda b:b['address'])
                        if b['block']['key'] not in ('fileBodyAdapter','rawLocalAdapter','glyphCacheAdapter')]
                       + [dict(name='bankBiosGateway', address=sy['bankBiosImageStart'],
                               size=sy['bankBiosImageEnd']-sy['bankBiosImageStart']),
                          dict(name='mainPpi', address=sy['bankBiosImageEnd'],
                               size=sy['mainPpiImageEnd']-sy['bankBiosImageEnd']),
                          dict(name='ioTimer', address=sy['io_pitRead'],
                               size=sy['ioImageEnd']-sy['io_pitRead']),
                          dict(name='bankDiskBridge', address=sy['bankIoImageStart'],
                               size=sy['bankIoImageEnd']-sy['bankIoImageStart']),
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
    # РђРєС‚РёРІРЅС‹Р№ СЃР»РѕС‚ Р·Р°РїРѕР»РЅСЏРµС‚СЃСЏ РїСЂРё РёРЅРёС†РёР°Р»РёР·Р°С†РёРё DOS РёР· РѕР±С‰РµРіРѕ С€Р°Р±Р»РѕРЅР°.
    (out/'DOS.sys').write_bytes(bank)
    bank_layout=json.loads((out/'banked/KERNEL.layout.json').read_text(encoding='utf-8'))
    report['dos']=dict(start=bank_layout['dosCoreStart'],end=bank_layout['kernelEnd'],bytes=len(bank),page=1,
                       coreEnd=bank_layout['dosCoreEnd'],extensionStart=bank_layout['dosCoreEnd'],
                       variablesStart=bank_layout['dosWorkStart'],variablesEnd=bank_layout['dosWorkEnd'],
                       dataStart=bank_layout['DOS_BANK_DATA_ADDR'])
    (out/'System.layout.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(f'BIOS: {len(bios)} bytes; IO: {len(io)} bytes; bank DOS: {len(bank)} bytes, end {bank_layout['kernelEnd']:04X}h')

if __name__=='__main__': main()
