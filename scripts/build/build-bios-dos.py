"""Парная сборка BIOS C000h и DOS 4000h; общий линкер, раздельные образы."""
import json
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

def component_groups():
    """Единый состав ядра для основной и банковой сборок."""
    source = ROOT/'source/dos'
    manifest = json.loads((source/'layout.json').read_text('utf-8'))
    manifest['fixed'][-1]['size'] = 237
    profile = json.loads((source/'layout4000.json').read_text('utf-8'))
    bios_names = set(profile['bios_units'])
    omitted = {'reboot1','reboot2','initFont','dosModule'}
    groups = {region:[u for u in manifest['units'] if u['name'] not in omitted
                     and (u['name'] in bios_names) == (region == 'bios')]
              for region in ('dos','bios')}
    groups['dos'].insert(0,dict(name='entries',files=['entries.inc']))
    def add(region, name, *files):
        groups[region].append(dict(name=name, files=list(files)))
    add('bios','startup','../bios/startup.inc')
    add('bios','blockIO','../bios/blockIO.inc')
    add('bios','syscall','../bios/syscall.inc','../bios/bdos.inc')
    add('bios','ui','ui/draw.inc','ui/inverse.inc','ui/toolbar.inc','ui/copyRect.inc')
    add('dos','services','services.inc')
    add('dos','pathIO','filePathIO.inc')
    add('dos','cli','../cli/command.inc','../cli/directories.inc','../cli/attrib.inc',
        '../cli/move.inc','../cli/tree.inc','../cli/data.inc','../cli/moveDirectories.inc',
        '../cli/copyConfirm.inc','../cli/changes.inc')
    add('dos','advanced','fileAssociation.inc','fileCopy.inc','fileHandle.inc',
        'directoryCwd.inc','../cli/directoryExtend.inc')
    return manifest, groups

def main():
    runpy.run_path(str(ROOT/'scripts/build/build-syscalls.py'))['generate']()
    runpy.run_path(str(ROOT/'scripts/build/build-banked-transport.py'))['main']()
    h = runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
    source = ROOT/'source/dos'
    out = ROOT/'bin'
    out.mkdir(exist_ok=True)
    manifest, groups = component_groups()
    inc = h['include']
    lines = [inc(ROOT/'source/include/mxos.inc'),inc(source/'layoutMacros.inc'),inc(out/'banked/transport-imports.inc'),'DOS_IMAGE_SUM = 0','    ORG 04000h','dosCoreStart:']
    for u in groups['dos']:
        for i,f in enumerate(u['files']):
            lines += h['emit_unit']({**u,'files':[f]},f'{u["name"]}_{i}')
    lines += ['dosCoreEnd:','    ASSERT $ <= FAT_CACHE_ADDR','    ORG 0C000h']
    for f in manifest['fixed']:
        lines += [f'    ORG_PAD {h["number"](f["address"])}',*h['emit_fixed'](f)]
    for u in groups['bios']:
        if u.get('same_page'): lines += ['    ALIGN 256,0FFh']
        for i,f in enumerate(u['files']):
            lines += h['emit_unit']({**u,'files':[f]},f'{u["name"]}_{i}')
    lines += [inc(source/f) for f in ('getch.inc','printString1.inc','cmp_hl_de_2.inc')]
    lines += [inc(ROOT/'source/bios/legacy-files.inc'), inc(ROOT/'source/bios/banked-start.inc')]
    probe=out/'System.measure.asm'
    probe.write_text('\n'.join(lines+['    END','']),encoding='utf-8')
    h['assemble'](probe,out/'System.measure.bin',[])
    measured=h['symbols'](out/'System.measure.sym')
    bios_plan=h['place'](manifest['fixed'],h['blocks']({'units':groups['bios']},measured,False))
    lines=['    ORG 04000h','dosCoreStart:']
    for u in groups['dos']:
        lines += h['emit_unit'](u,u['name'])
    lines += ['dosCoreEnd:','    ASSERT $ <= FAT_CACHE_ADDR']
    lines += [h['emit_plan'](bios_plan,manifest['fixed']).replace('    ORG 0C000h','    ORG_PAD 0C000h',1),
              '    ASSERT $ <= BIOS_BANK_GATE_ADDR',
              '    ORG_PAD BIOS_BANK_GATE_ADDR',
              'bankBiosImageStart:',
              '    INCBIN "'+(out/'banked/MAINBIOS.bin').as_posix()+'"',
              'bankBiosImageEnd:',
              inc(ROOT/'source/bios/banked-start.inc'),
              '    ASSERT $ <= BIOS_BANK_GATE_END',
              '    ASSERT BIOS_BANK_GATE_END <= BIOS_BANK_IO_ADDR',
              '    ORG_PAD BIOS_BANK_IO_ADDR',
              'bankIoImageStart:',
              '    INCBIN "'+(out/'banked/MAINIO.bin').as_posix()+'"',
              'bankIoImageEnd:',
              inc(ROOT/'source/bios/legacy-files.inc'),
              '    ASSERT $ <= BIOS_BANK_IO_BUFFER',
              '    ASSERT BIOS_BANK_IO_BUFFER+512 <= BIOS_BANK_STACK_BOTTOM',
              '    ASSERT BIOS_BANK_STACK_TOP <= DOS_END_MAX']
    layout='\n'.join(lines)+'\n'
    (out/'System.layout.inc').write_text('DOS_IMAGE_SUM = 0\n'+layout,encoding='utf-8')
    h['assemble'](ROOT/'source/bios/BIOS.asm',out/'System.linked.bin',[])
    sy=h['symbols'](out/'System.linked.sym')
    raw=(out/'System.linked.bin').read_bytes()
    dos=raw[:sy['dosCoreEnd']-0x4000]
    total=sum(dos); prefix=total-dos[-1]
    checksum=(((prefix+(prefix>>8))&255)<<8)|(total&255)
    (out/'System.layout.inc').write_text(f'DOS_IMAGE_SUM = 0{checksum:04X}h\n'+layout,encoding='utf-8')
    h['assemble'](ROOT/'source/bios/BIOS.asm',out/'System.linked.bin',[])
    raw=(out/'System.linked.bin').read_bytes()
    assert raw[:len(dos)]==dos, 'Контрольная сумма не должна менять DOS'
    bios=raw[0x8000:]
    (out/'DOS.native.sys').write_bytes(dos)
    (out/'BIOS.sys').write_bytes(bios)
    for stem in ('DOS','BIOS'):
        (out/f'{stem}.sym').write_bytes((out/'System.linked.sym').read_bytes())
        (out/f'{stem}.lst').write_bytes((out/'System.linked.lst').read_bytes())
    report=dict(bios=dict(start=0xC000,end=0xC000+len(bios),bytes=len(bios)),
                dos=dict(start=0x4000,end=sy['dosCoreEnd'],bytes=len(dos)),
                units={k:[u['name'] for u in v] for k,v in groups.items()},windows=manifest['fixed'],
                blocks=[dict(name=b['block']['key'],address=b['address'],size=b['block']['size'])
                        for b in sorted(bios_plan['placed'],key=lambda b:b['address'])]
                       + [dict(name='bankBiosGateway', address=sy['bankBiosImageStart'],
                               size=sy['bankBiosImageEnd']-sy['bankBiosImageStart']),
                          dict(name='bankDiskBridge', address=sy['bankIoImageStart'],
                               size=sy['bankIoImageEnd']-sy['bankIoImageStart']),
                          dict(name='legacyFileAdapters', address=sy['sysLegacyStart'],
                               size=sy['sysLegacyEnd']-sy['sysLegacyStart'])])
    report['nativeDosReference'] = report['dos']
    runpy.run_path(str(ROOT/'scripts/build/build-banked-dos.py'))['main']()
    bank=bytearray((out/'banked/KERNEL.bin').read_bytes())
    driver=(out/'banked/FLASHDRV.bin').read_bytes()
    bank[0x5000:0x5000+len(driver)]=driver
    rom_driver=(out/'banked/ROMDRV.bin').read_bytes()
    bank[0x5400:0x5400+len(rom_driver)]=rom_driver
    fast_driver=(out/'banked/FASTDRV.bin').read_bytes()
    bank[0x5600:0x5600+len(fast_driver)]=fast_driver
    (out/'DOS.sys').write_bytes(bank)
    bank_layout=json.loads((out/'banked/KERNEL.layout.json').read_text(encoding='utf-8'))
    report['dos']=dict(start=0x4000,end=bank_layout['kernelEnd'],bytes=len(bank),page=1,
                       coreEnd=bank_layout['dosCoreEnd'],extensionStart=0xC500)
    (out/'System.layout.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    transport=(out/'banked/TRANSPORT.bin').read_bytes()
    assert len(transport)<=0x3C0
    (out/'TAIL.sys').write_bytes(transport+bytes(1024-len(transport))+(out/'banked/WINDOW.bin').read_bytes())
    print(f'BIOS: {len(bios)} bytes; bank DOS: {len(bank)} bytes, end {bank_layout['kernelEnd']:04X}h')

if __name__=='__main__': main()
