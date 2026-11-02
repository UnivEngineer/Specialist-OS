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
    image = args.image.read_bytes()
    ds = symbols(args.image.with_suffix('.sym'))
    layout = json.loads(args.image.with_suffix('.layout.json').read_text('utf-8'))
    spans = [(v['address'],v['address']+v['size']) for v in layout['windows']+layout['blocks']]
    spans.sort()
    assert all(a[1] <= b[0] for a,b in zip(spans,spans[1:]))
    assert len(image) == layout['bytes'] == layout['end']-0xC000
    assert ds['dosImageEnd'] == layout['end'] <= 0xDA00
    assert ds['delay_l'] == 0xC18F and ds['delay_b'] == 0xC190
    assert image[0x18F:0x195] == bytes.fromhex('45 05 C2 90 C1 C9')
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
        assert abi == {k:v for k,v in before.items() if k in declared}

    negative = out/'negative'
    negative.mkdir(exist_ok=True)
    include = h['include']
    cases = {
        'bridge_no_room': include(ROOT/'source/dos/layoutMacros.inc')+'\n ORG 0C000h\n DOS_ABI_BRIDGE 0C100h, $+2\n END\n',
        'bridge_wrong_target': include(ROOT/'source/dos/layoutMacros.inc')+'\n ORG 0C000h\n DOS_ABI_BRIDGE 0BFFFh, $+3\n END\n',
        'gate_self_loop': include(ROOT/'source/dos/layoutMacros.inc')+'\n ORG 0C000h\n DOS_ABI_GATE 0C000h, 0C000h\n END\n',
        'key_table_crosses_page': ' ORG 0C0D0h\n'+include(ROOT/'source/dos/keyTable.inc')+'\n END\n',
    }
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
