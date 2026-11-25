"""Assemble local test DLLs, then compare their public 6x8 string paths."""
import argparse
import json
from pathlib import Path
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path, help='isolated local checkout with OS built')
parser.add_argument('--rotate-columns', action='store_true', help='compare the three Rotate kernels only')
args = parser.parse_args()
root = args.checkout.resolve()
report = Path(__file__).resolve().parent
source = root/'source/font'
font = root/'bin/fonts'
runner = root/'source-apps/fontbench/check-engines.py'
model = {'__file__':str(runner)}
exec(compile(runner.read_text(encoding='utf-8').split('cases=')[0], str(runner), 'exec'), model)
base, run, symbols, word = (model[name] for name in ('base','run','symbols','w'))
bios = model['bs']
cursor = bios['bios_vars.cursorY']
polarity = bios['bios_vars.inverse']
text = 0x86FD
screen = slice(0x9000,0xC000)
apis = ('printStringZ','printStringRawZ','printStringRawLim')
stock_modules = ('RotRows','FontPack','FontFull')
probe_order = ('Sweep','Snake','Reset')


def assemble(source_name, output, defines):
    target = font/output
    result = subprocess.run(
        [str(root/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',*defines,
         f'--raw={target.with_suffix(".i80")}',f'--sym={target.with_suffix(".sym")}',
         f'--lst={target.with_suffix(".lst")}',str(source/source_name)],
        cwd=source, capture_output=True, text=True)
    if result.returncode:
        raise RuntimeError(result.stdout+result.stderr)


def make_modules():
    engine = (source/'ENGINE.asm').read_text(encoding='utf-8')
    engine = engine.replace('INCLUDE "packet6.inc"','INCLUDE "quad-probe-packet.inc"')
    engine = engine.replace('INCLUDE "install.inc"','INCLUDE "quad-probe-install.inc"')
    (source/'QUAD_PROBE.asm').write_text(engine,encoding='utf-8')
    packet = (source/'packet6.inc').read_text(encoding='utf-8')
    first = packet.index('    IFDEF FONT_ROTATE_ROWS')
    last = packet.index('noGroup:',first)
    (source/'quad-probe-packet.inc').write_text(
        packet[:first]+'    INCLUDE "quad-probe-rows.inc"\n'+packet[last:],encoding='utf-8')
    (source/'quad-probe-rows.inc').write_bytes((report/'quad-rows.inc').read_bytes())
    install = (source/'install.inc').read_text(encoding='utf-8')
    install = ''.join(line for line in install.splitlines(keepends=True)
                      if not line.lstrip().startswith('COLUMN_PAGES '))
    (source/'quad-probe-install.inc').write_text(install,encoding='utf-8')
    for mode in range(3):
        for name in probe_order:
            assemble('QUAD_PROBE.asm',f'{name}{mode}',[
                '-DFONT_BENCH_MAIN','-DFONT_BENCH_STREAM',f'-DFONT_MODE={mode}',
                f'-DSNAKE_HORIZONTAL={int(name != "Sweep")}',f'-DQUAD_RESET_Y={int(name == "Reset")}'])


def make_column_modules():
    engine = (source/'ENGINE.asm').read_text(encoding='utf-8')
    (source/'COL_PROBE.asm').write_text(
        engine.replace('INCLUDE "packet6.inc"','INCLUDE "column-probe-packet.inc"'),
        encoding='utf-8')
    packet = (source/'packet6.inc').read_text(encoding='utf-8')
    (source/'column-probe-packet.inc').write_text(
        packet.replace('INCLUDE "packed-columns.inc"','INCLUDE "column-probe.inc"'),
        encoding='utf-8')
    columns = (source/'packed-columns.inc').read_text(encoding='utf-8')
    (source/'column-probe.inc').write_text(
        (report/'rotate-columns.inc').read_text(encoding='utf-8')
        + columns[columns.index('groupNormalDown:'):], encoding='utf-8')
    for rows in (2,4,8):
        assemble('COL_PROBE.asm',f'RotCol{rows}',
                 ['-DFONT_BENCH_MAIN','-DFONT_BENCH_STREAM',f'-DROTATE_COLUMN_ROWS={rows}'])
    assemble('COL_PROBE.asm','RotColResident',['-DROTATE_COLUMN_ROWS=4'])


def initialize(name, encoding):
    mem = bytearray(base)
    resident = (font/'Rotate.i80').read_bytes()
    mem[0xD800:0xD800+len(resident)] = resident
    data = (font/f'{name}.i80').read_bytes()
    mem[0x2000:0x2000+len(data)] = data
    labels = symbols('fonts/'+name)
    mem, registers, _ = run(mem,0x2000,af=0)
    assert not registers[0] & 1
    if 'stringTryGroup' in labels:
        hook = labels['stringTryGroup']
        assert mem[hook:hook+3] == b'\xc3'+word(labels['fast6.tryGroup'])
    if encoding:
        mem, _, _ = run(mem,labels['setEncoding'],af=0x100)
    address = bios['FONT_ENGINE_SCALAR']
    mem[address:address+3] = b'\xc3'+word(labels['scalar'])
    mem[bios['FONT_ENGINE_HEADER']+5] = 6
    return mem, labels


def draw(seed, api, codes, x, y, inverse):
    mem, labels = seed
    mem = bytearray(mem)
    mem[cursor:cursor+2] = bytes([y,x])
    mem[polarity:polarity+2] = bytes([255*inverse])*2
    mem[screen] = bytes([0xA5])*12288
    mem[text:text+len(codes)+1] = codes+b'\0'
    result, regs, cycles = run(mem,labels[api],hl=text,bc=len(codes))
    assert regs[:6] == word(0xA597)+word(len(codes))+word(text+len(codes)), regs.hex()
    assert regs[12:14] == word(0x2468) and regs[8:10] == word(0x8F00), regs.hex()
    assert result[text:text+len(codes)+1] == codes+b'\0'
    return (result[screen],result[cursor:cursor+2]), cycles


def initialize_resident(name, encoding):
    mem = bytearray(base)
    data = (font/f'{name}.i80').read_bytes()
    mem[0xD800:0xD800+len(data)] = data
    labels = symbols('fonts/'+name)
    mem, _, _ = run(mem,labels['attach'],af=0,de=0)
    if encoding:
        mem, _, _ = run(mem,labels['setEncoding'],af=0x100)
    return mem, labels


def compare_rotate_columns():
    names = ('FontRot','RotRows','Snake0','RotCol2','RotCol4','RotCol8')
    stats = {}
    checks = 0
    with tempfile.TemporaryDirectory(prefix='font-rotate-by-column-') as work:
        model['O'] = Path(work)
        seeds = [initialize(name, 0) for name in names]
        for name, (_, labels) in zip(names, seeds):
            stats[name] = {'cycles_per_quad': {}, 'full_calls': {},
                           'code_bytes': labels['fast6.noGroup']-labels['fast6.groupNormalDown']}
        for inverse in (False, True):
            key = 'inverse' if inverse else 'normal'
            trials = {name:{} for name in names}
            for length in (4,16,64):
                codes = bytes(range(32,32+length))
                runs = [draw(seed,apis[2],codes,0,7,inverse) for seed in seeds]
                assert all(result[0] == runs[0][0] for result in runs)
                checks += len(runs)-1
                for name, (_, ticks) in zip(names, runs):
                    trials[name][length] = ticks
            for name, times in trials.items():
                difference = times[16]-times[4]
                assert difference % 3 == 0
                stats[name]['cycles_per_quad'][key] = difference//3
                stats[name]['full_calls'][key] = times
        for encoding in (0,1):
            reference = initialize('RotRows',encoding)
            candidates = [initialize(name,encoding) for name in names[3:]]
            old_resident = initialize_resident('Rotate',encoding)
            new_resident = initialize_resident('RotColResident',encoding)
            cases = ((apis[0],b'ABCD\r\n'+bytes(range(33,256))),
                     (apis[1],bytes(range(1,256))),
                     (apis[2],bytes(range(256))), (apis[2],b'ABCDE'))
            for inverse in (False,True):
                for x,y in ((0,7),(2,17),(184,7),(189,255)):
                    for api,codes in cases:
                        expected,_ = draw(reference,api,codes,x,y,inverse)
                        for seed in candidates:
                            actual,_ = draw(seed,api,codes,x,y,inverse)
                            assert actual == expected
                            checks += 1
                        original,_ = draw(old_resident,api,codes,x,y,inverse)
                        replacement,_ = draw(new_resident,api,codes,x,y,inverse)
                        assert replacement == original
                        checks += 1
    report_data = {'pixel_checks':checks, 'modes':stats}
    (report/'rotate-columns.json').write_text(json.dumps(report_data,indent=2)+'\n')
    for name, data in stats.items():
        print(name, data['cycles_per_quad'], data['code_bytes'])


make_modules()
if args.rotate_columns:
    make_column_modules()
    compare_rotate_columns()
    raise SystemExit(0)
checks = 0
timing = {}
with tempfile.TemporaryDirectory(prefix='font-quad-snake-') as tmp:
    model['O'] = Path(tmp)
    for mode, stock in enumerate(stock_modules):
        modules = (stock,*(f'{name}{mode}' for name in probe_order))
        trial = {'modules':modules, 'code_bytes':{}, 'cycles':{}}
        timing[stock] = trial
        for encoding in (0,1):
            seeds = [initialize(name,encoding) for name in modules]
            cases = [
                (apis[2], b''), (apis[2], b'Q'), (apis[2], b'MNOPS'),
                (apis[2], bytes(range(256))), (apis[1], bytes(range(1,256))),
                (apis[0], b'IJKLN\r\n'+bytes(range(126,256))),
            ]
            for inverse in (False,True):
                for x,y in ((0,7),(1,7),(2,7),(3,7),(176,17),(184,255),(189,7),(0,6)):
                    for api,codes in cases:
                        expected, _ = draw(seeds[0],api,codes,x,y,inverse)
                        for name, seed in zip(modules[1:],seeds[1:]):
                            pixels, _ = draw(seed,api,codes,x,y,inverse)
                            assert pixels == expected, (name,api,len(codes),x,y,inverse,encoding)
                            checks += 1
            if encoding:
                continue
            for name, (_,labels) in zip(modules,seeds):
                trial['code_bytes'][name] = labels['fast6.noGroup']-labels['fast6.groupNormalDown']
            for api in apis:
                for inverse in (False,True):
                    key = api+('Inverse' if inverse else '')
                    trial['cycles'][key] = {}
                    for size in (4,16,64):
                        codes = bytes(range(32,32+size))
                        runs = [draw(seed,api,codes,0,7,inverse) for seed in seeds]
                        assert all(result[0]==runs[0][0] for result in runs)
                        trial['cycles'][key][size] = [cycles for _,cycles in runs]
            four = trial['cycles'][apis[2]][4]
            sixteen = trial['cycles'][apis[2]][16]
            delta = [(second-first)//3 for first,second in zip(four,sixteen)]
            assert all(second-first == 3*cost for first,second,cost in zip(four,sixteen,delta))
            assert delta[1]-delta[2] == 80
            assert delta[3]-delta[2] == (12 if mode==0 else 17)
            trial['cycles_per_quad'] = dict(zip(modules,delta))
        print(stock,trial['code_bytes'],trial['cycles'][apis[2]],checks,flush=True)
(report/'results.json').write_text(json.dumps({'checks':checks,'timing':timing},indent=2)+'\n')
