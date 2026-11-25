"""8080 timings, API equivalence and memory accounting for fixed KOI-7R."""
import argparse
import json
from pathlib import Path
import re
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
out = Path(__file__).resolve().parent
runner = root/'source-apps/fontbench/check-engines.py'
model = {'__file__':str(runner)}
exec(compile(runner.read_text(encoding='utf-8').split('cases=')[0],str(runner),'exec'),model)
bios, symbols, base, run, word = (model[n] for n in ('bs','symbols','base','run','w'))
fonts = root/'bin/fonts'
screen = slice(0x9000,0xC000)
cursor = bios['bios_vars.cursorY']
polarity = bios['bios_vars.inverse']
text = 0x86FD
width = 6
profiles = (
    ('Rotate','K7Rotate',0,1024),
    ('Packed6K','K7Packed',4096,3072),
    ('Fast12K','K7Hybrid',10240,3072),
)
apis = ('printStringZ','printStringRawZ','printStringRawLim')


def install(name, lower=0):
    mem = bytearray(base)
    labels = symbols('fonts/'+name)
    arena = labels['FONT_ENGINE_BASE']
    data = (fonts/f'{name}.i80').read_bytes()
    if arena != 0xD800:
        resident = (fonts/'Rotate.i80').read_bytes()
        mem[0xD800:0xD800+len(resident)] = resident
    mem[arena:arena+len(data)] = data
    if lower:
        mem[0x5000:0x5000+lower] = (fonts/f'{name}.pre').read_bytes()
    mem, _, _ = run(mem,arena,af=0,de=0x5000 if lower else 0)
    if arena != 0xD800:
        addr = bios['FONT_ENGINE_SCALAR']
        mem[addr:addr+3] = b'\xC3'+word(labels['scalar'])
        mem[bios['FONT_ENGINE_HEADER']+5] = width
    return mem, labels


def encoding(seed, value):
    mem, labels = seed
    mem, registers, elapsed = run(mem,labels['setEncoding'],af=(value<<8)|0x97)
    assert registers[:2] == word((value<<8)|0x97)
    return (mem,labels), elapsed


def draw(seed, api, payload, *, x=0,y=7,inverse=False):
    mem, labels = seed
    mem = bytearray(mem)
    mem[cursor:cursor+2] = bytes([y,x])
    mem[polarity:polarity+2] = bytes([255*inverse])*2
    mem[screen] = bytes([0xA5])*12288
    mem[text:text+len(payload)+1] = payload+b'\0'
    length = len(payload)
    if api == 'printCharRaw':
        input_a = (payload[0]<<8)|0x97
        length = 0x1357
        end = text
    else:
        input_a = 0xA597
        end = text+length
    state, regs, elapsed = run(mem,labels[api],af=input_a,hl=text,bc=length)
    assert regs[:6] == word(input_a)+word(length)+word(end), (api,regs.hex())
    assert regs[12:14]==word(0x2468) and regs[8:10]==word(0x8F00)
    assert state[text:text+len(payload)] == payload
    return (state[screen],state[cursor:cursor+2]), elapsed


def stock_size(name):
    listing = (fonts/f'{name}.lst').read_text(encoding='utf-8')
    end = int(re.search(r'\b([A-Fa-f0-9]{4})\s+ASSERT \$ <= FONT_ADDR',listing)[1],16)
    return end-0xD800


def compare(old,new):
    n = 0
    for inverse in (False,True):
        for x,y in ((0,7),(1,7),(2,7),(3,7),(176,17),(184,255),(189,7),(0,6)):
            params = dict(x=x,y=y,inverse=inverse)
            for api, data in ((apis[2], bytes(range(128))), (apis[2], b''),
                              (apis[2],b'ABCD~'), (apis[1], bytes(range(1,128))),
                              (apis[0], b'ABCD~\r\n\x1b?'+bytes(range(32,128))),
                              ('printCharRaw',b'\x7f'), ('printCharRaw',b'\x00')):
                expected,_ = draw(old,api,data,**params)
                actual,_ = draw(new,api,data,**params)
                assert actual == expected, (new[1]['FONT_ENGINE_BASE'],api,x,y,inverse,len(data))
                n += 1
            data = bytes(range(256))
            expected,_ = draw(old,apis[2],bytes(c&127 for c in data),**params)
            actual,_ = draw(new,apis[2],data,**params)
            assert actual == expected, ('high bit alias',x,y,inverse)
            n += 1
    return n


def timing(old,new):
    result = {}
    sample = bytes(range(32,128))
    for inverse in (False,True):
        for size in (4,16,64,924):
            row = sample[:42]
            for api in apis:
                data = sample[:size] if size<=64 else (row+b'\r\n')*21
                expected,before = draw(old,api,data,inverse=inverse)
                actual,after = draw(new,api,data,inverse=inverse)
                assert actual==expected
                result.setdefault(api+('Inverse' if inverse else ''),{})[size] = [before,after]
    return result


report = {}
with tempfile.TemporaryDirectory(prefix='font-koi7-') as temp:
    model['O'] = Path(temp)
    empty = bytearray(base)
    empty[0x8500] = 0xC9
    _,_,harness = run(empty,0x8500,af=0x97)
    for old_name,new_name,lower,table_size in profiles:
        old = install(old_name,lower)
        old,forward = encoding(old,1)
        _,reverse = encoding(old,0)
        new = install(new_name)
        new_mem,new_symbols = new
        assert new_mem[bios['bios_upper.koi7']] == 255
        de = new_symbols['FONT_ENGINE_HEADER']+12
        assert new_mem[de:de+2] == b'\0\0'
        count = compare(old,new)
        timed = timing(old,new)
        tail = new_symbols['imageEnd']-new_symbols['K7_TABLES']-table_size
        report[new_name] = {
            'comparison':old_name, 'checks':count, 'timing':timed,
            'switch_cycles':{'to7':forward-harness,'to8':reverse-harness},
            'old':{'code':stock_size(old_name),'tables':2048+lower,
                   'body':(fonts/f'{old_name}.i80').stat().st_size,'main':lower},
            'new':{'code':new_symbols['codeEnd']-0xD800+tail,'tables':table_size,
                   'body':(fonts/f'{new_name}.i80').stat().st_size,'main':0},
        }
        print(new_name,report[new_name]['new'],timed[apis[2]][924],count,flush=True)
    old,_ = encoding(install('Fast12K',10240),1)
    full = install('K7Fast')
    report['K7Fast'] = {'comparison':'Fast12K','checks':compare(old,full),
                        'timing':timing(old,full),
                        'code':full[1]['codeEnd']-0x2000,'tables':6144}
    print('K7Fast',report['K7Fast']['timing'][apis[2]][924],flush=True)
(out/'cpu.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
