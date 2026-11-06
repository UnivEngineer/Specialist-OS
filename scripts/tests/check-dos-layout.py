"""Проверка размещения DOS, заглушек ABI и отказа статических ASSERT."""
import argparse
import hashlib
import json
import re
import runpy
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
h = runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))
symbols = h['symbols']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--image', type=Path, default=ROOT/'bin/DOS.sys')
    p.add_argument('--baseline', type=Path)
    p.add_argument('--report-directory', type=Path, default=ROOT/'docs/dos/packing-001')
    args = p.parse_args()
    args.image = args.image.resolve()
    out = args.report_directory.resolve()
    out.mkdir(parents=True,exist_ok=True)
    if args.image.name == 'DOS.sys' and (ROOT/'bin/System.layout.json').exists():
        layout=json.loads((ROOT/'bin/System.layout.json').read_text('utf-8'))
        spans=sorted((v['address'],v['address']+v['size']) for v in layout['windows']+layout['blocks'])
        assert all(a[1]<=b[0] for a,b in zip(spans,spans[1:]))
        ds=symbols(ROOT/'bin/BIOS.sym')
        for name in ['BIOS','DOS']:
            region=layout[name.lower()]
            assert len((ROOT/f'bin/{name}.sys').read_bytes())==region['bytes']==region['end']-region['start']
        assert layout['bios']['end']<=ds['DOS_END_MAX']<=ds['DISK_DRIVER_ADDR']
        assert ds['DISK_DRIVER_ADDR']+512<=ds['FONT_ADDR']
        assert ds['mx_call']==0xF800 and ds['RAMD_RESIDENT_ADDR']+192<=ds['mx_call']
        ks=symbols(ROOT/'bin/banked/KERNEL.sym')
        assert layout['dos']['page']==1
        assert ks['dosCoreEnd']<=ks['DOS_BANK_COMPAT_ADDR']
        assert layout['dos']['end']<=0xD800<=ks['DOS_BANK_STACK_BOTTOM']
        assert ks['BANK_KERNEL_CACHE']==0xC000 and ks['bankKernelCommand']==0xC400
        assert ds['APP_MEMORY_END']==0x8C00
        assert ds['FONT_ADDR']+1280<=ds['STD_IO_ADDR']
        runpy.run_path(str(ROOT/'scripts/tests/check-bios-dos-split.py'),run_name='__main__')
        print('BIOS/DOS layout PASS: no overlaps; font below STD IO; DOS below cache')
        return
    image = args.image.read_bytes()
    ds = symbols(args.image.with_suffix('.sym'))
    layout = json.loads(args.image.with_suffix('.layout.json').read_text('utf-8'))
    spans = [(v['address'],v['address']+v['size']) for v in layout['windows']+layout['blocks']]
    spans.sort()
    assert all(a[1] <= b[0] for a,b in zip(spans,spans[1:]))
    assert len(image) == layout['bytes'] == layout['end']-0xC000
    assert ds['dosImageEnd'] == layout['end'] <= min(ds['DOS_END_MAX'], ds['CMD_LINE'])
    assert ds['STD_IO_ADDR'] == 0xF000 and ds['STD_KEYB_ADDR'] == 0xF800
    assert ds['CMD_LINE'] == 0xFA00 and ds['CMD_LINE_SIZE'] == 130
    assert ds['APP_MEMORY_END'] <= ds['STACK_BOTTOM'] < ds['STACK_ADDR'] == ds['bios_vars']
    assert ds['STACK_ADDR']-ds['STACK_BOTTOM'] == 479
    assert ds['dosImageEnd'] <= ds['DOS_END_MAX'] == ds['FONT_ADDR']
    assert ds['FONT_ADDR']+1280 == ds['STD_IO_ADDR']
    assert ds['CMD_LINE'] >= ds['STD_IO_ADDR']
    assert ds['delay_l'] == 0xC18F and ds['delay_b'] == 0xC190
    assert image[0x18F:0x195] == bytes.fromhex('45 05 C2 90 C1 C9')
    assert ds['setKeybMode82'] == ds['bios_setKeybMode82'] == 0xC254
    assert ds['setKeybMode91'] == ds['bios_setKeybMode91'] == 0xC25A
    assert ds['v_keybTbl'] == ds['bios_keybTable'] == 0xC4A0
    assert image[0x254:0x260] == bytes.fromhex('3E 82 32 E3 FF C9 3E 91 32 E3 FF C9')
    monitor = (ROOT/'assets/programs/Monitor2.mon').read_bytes()
    table = monitor[0x4A0:0x500]
    assert len(table) == 96 and image[0x4A0:0x500] == table
    assert image.count(table) == 1
    assert ds['bios_reboot'] == 0xC800 and ds['bios_div_hl_de'] == 0xC887
    assert ds['bios_setMemTop'] == 0xC830 and ds['bios_getMemTop'] == 0xC833
    for offset, label in [(0x830, 'setMemTop'), (0x833, 'getMemTop')]:
        assert image[offset:offset+3] == b'\xC3'+ds[label].to_bytes(2,'little')
    address = ds['v_memTop'].to_bytes(2,'little')
    assert image[ds['setMemTop']-0xC000:ds['setMemTop']-0xC000+4] == b'\x22'+address+b'\xC9'
    assert image[ds['getMemTop']-0xC000:ds['getMemTop']-0xC000+4] == b'\x2A'+address+b'\xC9'
    assert image[0x800] == image[0x887] == 0xC3
    addresses = [int(v,16) for v in re.findall(r'^bios_\w+\s*=\s*0([0-9A-Fa-f]+)h',
                  (ROOT/'source/include/mxos.inc').read_text('utf-8'),re.M)]
    assert addresses == sorted(addresses)
    assert ds['keybScan3'] and ds['v_keybTbl']>>8 == (ds['v_keybTbl']+95)>>8
    for v in layout['windows']:
        if v.get('target'):
            offset = v['address']-0xC000
            assert image[offset:offset+3] == b'\xC3'+ds[v['target']].to_bytes(2,'little')
    for v in layout['blocks']:
        if v.get('link'):
            offset = v['address']+v['size']-3-0xC000
            target = ds['dos_piece_'+v['link']+'_start']
            assert image[offset:offset+3] == b'\xC3'+target.to_bytes(2,'little')
    declared = set(re.findall(r'^\s*(bios_\w+)\s+(?:=|BIOS_VARIABLES)',
                             (ROOT/'source/include/mxos.inc').read_text('utf-8'),re.M))
    abi = {k:v for k,v in ds.items() if k in declared}
    if args.baseline:
        before = symbols(args.baseline.with_suffix('.sym'))
        expected = {k:v for k,v in before.items() if k in declared}
        # Единственная намеренная смена прежних адресов: порядок Monitor-2.
        expected.update(bios_setMemTop=0xC830, bios_getMemTop=0xC833)
        assert {k:v for k,v in abi.items() if k in before} == expected

    negative = out/'negative'
    negative.mkdir(exist_ok=True)
    include = h['include']
    cases = {
        'bridge_no_room': include(ROOT/'source/dos/layoutMacros.inc')+'\n ORG 0C000h\n DOS_ABI_BRIDGE 0C100h, $+2\n END\n',
        'bridge_wrong_target': include(ROOT/'source/dos/layoutMacros.inc')+'\n ORG 0C000h\n DOS_ABI_BRIDGE 0BFFFh, $+3\n END\n',
        'gate_self_loop': include(ROOT/'source/dos/layoutMacros.inc')+'\n ORG 0C000h\n DOS_ABI_GATE 0C000h, 0C000h\n END\n',
        'key_table_crosses_page': ' ORG 0C0D0h\n'+include(ROOT/'source/dos/keyTable.inc')+'\n END\n',
    }
    memory_map = (ROOT/'source/include/mxos.inc').read_text('utf-8')
    cases['std_stack_in_io'] = re.sub(r'(?m)^(STACK_ADDR\s*=\s*)0[0-9A-Fa-f]+h',r'\g<1>0FFC0h',memory_map)+'\n END\n'
    cases['std_font_in_io'] = memory_map.replace('FONT_ADDR = 0EB00h','FONT_ADDR = 0F800h')+'\n END\n'
    source = args.image.with_suffix('.layout.inc').read_text('utf-8')
    # Измерение изменилось, а размещение осталось прежним.
    stale = re.sub(r'(ASSERT dos_piece_\w+_end - dos_piece_\w+_start == )(\d+)',
                   lambda m:m[1]+str(int(m[2])+1),source,count=1)
    # Первый блок после входа C010h намеренно смещён поверх его JP.
    overlap = source.replace('    ORG_PAD 0C013h','    ORG_PAD 0C012h',1)
    for name,content in [('stale_size',stale),('overlap_abi',overlap)]:
        altered = negative/(name+'.inc')
        altered.write_text(content,encoding='utf-8')
        cases[name] = f' DEFINE DOS_LAYOUT "{altered.as_posix()}"\n'+include(ROOT/'source/dos/DOS.asm')+'\n'
    rejected = []
    for name,content in cases.items():
        path = negative/(name+'.asm')
        path.write_text(content,encoding='utf-8')
        result = subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',
                                 f'--raw={negative/(name+".bin")}',str(path)],
                                cwd=ROOT/'source/dos',capture_output=True,text=True,encoding='utf-8')
        log = result.stdout+result.stderr
        assert result.returncode != 0 and ('ASSERT' in log or 'ORG_PAD' in log), (name,log)
        (negative/(name+'.log')).write_text(log,encoding='utf-8')
        rejected.append(name)
    report = dict(bytes=len(image),sha256=hashlib.sha256(image).hexdigest(),abiEntries=len(abi),
                  bridges=layout['bridges'],nonoverlapping=True,negativeCasesRejected=rejected)
    (out/(args.image.stem+'-static.json')).write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(report)


if __name__ == '__main__':main()
