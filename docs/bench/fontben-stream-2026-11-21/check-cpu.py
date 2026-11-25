"""Compare both 8080 kernels of each standalone DLL, including public ABI."""
import argparse
import json
from pathlib import Path
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
runner = root/'source-apps/fontbench/check-engines.py'
model = {'__file__': str(runner)}
exec(compile(runner.read_text(encoding='utf-8').split('cases=')[0], str(runner), 'exec'), model)
base, run, symbols, word = (model[name] for name in ('base', 'run', 'symbols', 'w'))
bios = model['bs']
cursor = bios['bios_vars.cursorY']
polarity = bios['bios_vars.inverse']
text = 0x8600
screen = slice(0x9000, 0xC000)
apis = ('printStringZ', 'printStringRawZ', 'printStringRawLim')
modules = (('RotRows', 6), ('FontPack', 6), ('FontFull', 6),
           ('Font4', 4), ('Font4Fst', 4), ('Font4Cmp', 4))


def init(name, stream, encoding):
    mem = bytearray(base)
    resident = (root/'bin/fonts/Rotate.i80').read_bytes()
    mem[0xD800:0xD800+len(resident)] = resident
    data = (root/f'bin/fonts/{name}.i80').read_bytes()
    mem[0x2000:0x2000+len(data)] = data
    labels = symbols('fonts/'+name)
    mem, _, _ = run(mem, 0x2000, af=0x8000*stream)
    target = 'rowStream.tryGroup' if stream else 'fast6.tryGroup'
    hook = labels['stringTryGroup']
    assert mem[hook:hook+3] == b'\xc3'+word(labels[target])
    if name == 'Font4Cmp' and stream:
        start = labels['FONT4_REVERSE_ADDR']
        assert not any(mem[start:start+1024])
    if encoding:
        mem, _, _ = run(mem, labels['setEncoding'], af=0x100)
    scalar = bios['FONT_ENGINE_SCALAR']
    mem[scalar:scalar+3] = b'\xc3'+word(labels['scalar'])
    mem[bios['FONT_ENGINE_HEADER']+5] = mem[labels['benchDescriptor']]
    return mem, labels


def measure(seed, api, codes, x, y, inverse):
    mem, labels = seed
    mem = bytearray(mem)
    mem[cursor:cursor+2] = bytes([y,x])
    mem[polarity:polarity+2] = bytes([255*inverse])*2
    mem[screen] = bytes([0xA5])*12288
    mem[text:text+len(codes)+1] = codes+b'\0'
    try:
        result, registers, cycles = run(mem, labels[api], hl=text, bc=len(codes))
    except AssertionError as error:
        raise AssertionError((api, len(codes), x, y, inverse)) from error
    assert registers[:6] == word(0xA597)+word(len(codes))+word(text+len(codes)), registers.hex()
    assert registers[12:14] == word(0x2468) and registers[8:10] == word(0x8F00), registers.hex()
    assert result[text:text+len(codes)+1] == codes+b'\0'
    return (result[screen], result[cursor:cursor+2]), cycles


checks = 0
timing = {}
with tempfile.TemporaryDirectory(prefix='font-row-stream-') as work:
    model['O'] = Path(work)
    for name, width in modules:
        for encoding in (0,1):
            before = init(name, False, encoding)
            after = init(name, True, encoding)
            cases = [(apis[2], bytes(range(n))) for n in (0,1,2,3,4,5,19,20,21,64,96,256)]
            cases += [(apis[1], bytes(range(1,256))), (apis[0], b'ABCDE\r\n'+bytes(range(100,256)))]
            cases += [(apis[2], bytes([0x02,0x04,0x03,0x05,0x04,0x03,0x05,0x02,0x7f,0xff]))]
            for inverse in (False,True):
                for x,y in ((0,7),(1,7),(2,7),(3,7),(180,7),(184,7),(188,7),
                            (190,7),(0,0),(188,6),(184,255)):
                    for api,codes in cases:
                        old, _ = measure(before, api, codes, x, y, inverse)
                        new, _ = measure(after, api, codes, x, y, inverse)
                        if old != new:
                            raise AssertionError((name, encoding, inverse, x, y, api, len(codes),
                                                  old[1], new[1],
                                                  [(i,a,b) for i,(a,b) in enumerate(zip(old[0],new[0])) if a!=b][:10]))
                        checks += 1
            if encoding:
                continue
            timing[name] = {}
            for kind in apis:
                timing[name][kind] = {}
                for count in (4,16,64 if width==6 else 96):
                    codes = bytes(range(32,32+count))
                    _, stock = measure(before, kind, codes, 0, 7, False)
                    _, stream = measure(after, kind, codes, 0, 7, False)
                    timing[name][kind][count] = [stock,stream]
        print(name, 'PASS', checks, flush=True)
Path(__file__).with_name('cpu-timing.json').write_text(json.dumps(timing, indent=2)+'\n')

