"""Measure the public scalar ABI and full Fast with all tables above the screen."""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
report = Path(__file__).resolve().parent
cpu_script = report/'check-cpu.py'
shared = {'__file__':str(cpu_script)}
exec(compile(cpu_script.read_text(encoding='utf-8').split('\nreport = {}')[0], str(cpu_script), 'exec'), shared)
model = shared['model']
symbols, word, install, encoding, run = (shared[n] for n in ('symbols','word','install','encoding','run'))
bios = shared['bios']
fonts = root/'bin/fonts'
state = json.loads((report/'legacy-size.json').read_text(encoding='utf-8'))
bitmap = (root/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
glyphs = [bitmap[i:i+8] for i in range(0,len(bitmap),8)]
glyphs[0x60:0x7F] = glyphs[0xE0:0xFF]
glyphs[0x7F] = bytes(8)
screen = slice(0x9000,0xC000)
text = 0x86FD
corpus_line = bytes(range(32,74))
fontben_corpus = (corpus_line+b'\r\n')*21
printable_corpus = (corpus_line+b'  ')*21
string_apis = ('StringZ','StringRawZ','StringRawLim')


def build_standalone_ramfos(work):
    image = work/'RamfosStandalone.i80'
    labels = work/'RamfosStandalone.sym'
    subprocess.run([str(root/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                    '-DMXOS_KERNEL=0', f'--raw={image}', f'--sym={labels}',
                    'source/ramfos/ramfos.asm'], cwd=root, capture_output=True, check=True)
    blob = image.read_bytes()
    assert hashlib.sha256(blob).hexdigest() == 'dca4ac0d8de06c90615cb0b45a68116f8844df3164e9a0b4e9a31b981b14b0b3'
    entries = re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', labels.read_text(encoding='utf-8'), re.M)
    return blob, {name: int(address, 16) for name, address in entries}


def legacy(kernel, key=None, standalone=None):
    memory = bytearray(65536)
    if kernel == 'Monitor2':
        labels = symbols(kernel)
        image = (root/'bin/Monitor2.mon').read_bytes()
        memory[0xC000:0xD000] = image
        start = labels['loader_initialState']-0xC000
        memory[0x8FDF:0x9000] = image[start:start+33]
        cursor = 0x8FFC
    else:
        if standalone is None:
            labels = symbols('RAMFOS')
            image = (root/'bin/RAMFOS.mon').read_bytes()
            memory[0xC000:0xD300] = image[:0x1300]
            memory[0xF800:0xFFE0] = image[0x1300:]
        else:
            image, labels = standalone
            memory[0xC000:0xFFE0] = image
        memory[labels['v_koi8']] = 255
        memory[labels['v_inverse']] = 0
        memory[labels['v_screenHeight']:labels['v_screenHeight']+2] = bytes([250,240])
        memory[labels['v_topLineHeight']:labels['v_topLineHeight']+2] = bytes([0,10])
        cursor = labels['v_cursorPos']
    if key is not None:
        name = kernel+'-K7'+key
        labels = symbols('fonts/'+name)
        start = state[kernel][key]['origin']
        blob = (fonts/f'{name}.i80').read_bytes()
        memory[start:start+len(blob)] = blob
        assert start>=0xC000 and start+len(blob)<=0xF800
    return memory, labels, cursor


def aggregate(mem, entry, cursor, phase, codes, work, raw=False, raster=None):
    mem = bytearray(mem)
    mem[screen] = b'\xA5'*12288
    # Reset the cursor per call so all glyphs use the same phase and row.
    start = 0x8000
    code = bytearray(b'\x31'+word(0x8F00))  # LXI SP,8F00h
    for char in codes:
        code.extend(b'\x21'+bytes([57,phase])+b'\x22'+word(cursor))
        code.extend(bytes([0x3E if raw else 0x0E,char,0xCD])+word(entry))
    code.extend(b'\xC3'+word(0x8B80))
    mem[start:start+len(code)] = code
    source = work/'input.bin'
    output = work/'output.bin'
    source.write_bytes(mem)
    result = subprocess.run([model['TICKS'],'-m8080','-pc','8000','-end','8b80',
                            '-counter','3000000','-output',str(output),str(source)],
                            capture_output=True,check=True,timeout=20)
    state_bytes = output.read_bytes()
    assert state_bytes[65542:65546] == word(0x8B80)+word(0x8F00), (hex(entry), phase, state_bytes[65542:65546].hex())
    assert state_bytes[cursor:cursor+2] == bytes([57,phase+3]), (hex(entry), phase, codes, state_bytes[cursor:cursor+2].hex())
    if raster is not None and codes in (b'A', b'a'):
        expected, _ = expected_frame(mem, codes, 57, raster == 'xor', phase)
        assert state_bytes[screen] == expected, (hex(entry), phase, codes)
    elif codes == b'A':
        assert state_bytes[screen] != mem[screen], (hex(entry), phase)
    return int(result.stdout)-20-len(codes)*50  # caller: LXI SP/JMP; LXI/SHLD/MVI/CALL


def scalar(mem, entry, cursor, work, raw=False, raster=None):
    letters = bytes(range(0x21,0x7F))
    codes = {'printable_mean':letters,'A':b'A','koi7_A':b'a','space':b' ','DEL':b'\x7F'}
    return {name:[round(aggregate(mem,entry,cursor,x,payload,work,raw,raster)/len(payload),3)
                  for x in range(4)] for name,payload in codes.items()}


def expected_frame(mem, payload, cursor_y, xor, cursor_x=0, controls=False):
    output = bytearray(mem[screen])
    px = cursor_x*2
    for code in payload:
        if controls and code in (10,13):
            px = 0
            if code == 10:
                cursor_y += 10
            continue
        for y,row in enumerate(glyphs[code]):
            for bit in range(6):
                position = px+bit
                offset = position//8*256+cursor_y-7+y
                mask = 128>>(position%8)
                set_bit = row & (1<<(5-bit))
                if xor:
                    if set_bit:
                        output[offset] ^= mask
                else:
                    output[offset] = (output[offset]&~mask) | (mask if set_bit else 0)
        px += 6
        if px==384:
            px = 0
            cursor_y += 10
    return output,bytes([cursor_y,px//2])


def batch(mem, labels, cursor, xor=False):
    cycles = {}
    for count in (4,16,64,924):
        payload = fontben_corpus[:count]
        copy = bytearray(mem)
        copy[cursor:cursor+2] = bytes([7,0])
        copy[text:text+count] = payload
        copy[screen] = b'\xA5'*12288
        expected,cursor_end = expected_frame(copy,payload,7,xor)
        actual, registers, t = run(copy,labels['printStringRawLim'],hl=text,bc=count)
        assert actual[screen]==expected, (labels['printStringRawLim'],count)
        assert actual[cursor:cursor+2]==cursor_end, (actual[cursor:cursor+2],cursor_end)
        assert registers[4:6]==word(text+len(payload))
        cycles[count] = t-98
    cycles['per_quad'] = (cycles[16]-cycles[4])//3
    return cycles


def full_string(mem, cursor, apis, *, raster=None):
    result = {}
    for corpus_name,payload in (('fontben',fontben_corpus),('printable',printable_corpus)):
        assert len(payload) == 924
        inputs = bytearray(mem)
        # A blank screen also permits comparing the XOR monitor to the opaque probes.
        inputs[screen] = bytes(0x3000)
        inputs[cursor:cursor+2] = bytes([7,0])
        inputs[text:text+len(payload)+1] = payload+b'\0'
        for api,entry in apis.items():
            is_text = api == 'StringZ'
            expected,cursor_end = expected_frame(inputs,payload,7,raster=='xor',controls=is_text)
            actual,registers,t = run(inputs,entry,hl=text,bc=len(payload))
            assert registers[4:6] == word(text+len(payload)), (api,hex(entry),corpus_name)
            assert actual[cursor:cursor+2] == cursor_end, (api,hex(entry),corpus_name,actual[cursor:cursor+2].hex(),cursor_end.hex())
            if raster:
                assert actual[screen] == expected, (api,hex(entry),corpus_name)
            else:
                assert actual[screen] != inputs[screen], (api,hex(entry),corpus_name)
            result.setdefault(corpus_name,{})[api] = t-98
    return result


results = {'char':{},'char_raw':{},'full_fast':{},'string_924':{}}
string_924 = results['string_924']
with tempfile.TemporaryDirectory(prefix='koi7-scalar-') as tmp:
    work = Path(tmp)
    model['O'] = work
    cursor = bios['bios_vars.cursorY']
    for stock,k7,lower in (('Rotate','K7Rotate',0),('Packed6K','K7Packed',4096),
                           ('Fast12K','K7Hybrid',10240)):
        stock8 = install(stock,lower)
        old,_ = encoding(stock8,1)
        for name,seed in ((stock+'-KOI8',stock8),(stock,old),(k7,install(k7))):
            mem,_ = seed
            results['char'][name] = scalar(mem,bios['bios_printChar'],cursor,work)
            results['char_raw'][name] = scalar(mem,bios['bios_printCharRaw'],cursor,work,raw=True)
            if name != stock+'-KOI8':
                apis = {api:bios['bios_print'+api] for api in string_apis}
                string_924[name] = full_string(mem,cursor,apis,raster='opaque')
        if stock=='Fast12K':
            results['full_fast'][stock] = batch(*old,cursor)
    mem,labels = install('K7Fast')
    results['char']['K7Fast-main'] = scalar(mem,labels['printChar'],cursor,work)
    results['char_raw']['K7Fast-main'] = scalar(mem,labels['printCharRaw'],cursor,work,raw=True)
    results['full_fast']['K7Fast-main'] = batch(mem,labels,cursor)
    apis = dict(zip(string_apis,(labels['FONT_ENGINE_Z'],labels['FONT_ENGINE_RAWZ'],labels['FONT_ENGINE_LIM'])))
    string_924['K7Fast-main'] = full_string(mem,cursor,apis,raster='opaque')
    memory,_,cursor = legacy('RAMFOS',standalone=build_standalone_ramfos(work))
    results['char']['RAMFOS'] = scalar(memory,0xC809,cursor,work)
    string_924['RAMFOS'] = full_string(memory,cursor,{'StringZ':0xC818})
    for kernel in ('Monitor2','RAMFOS-MXOS'):
        memory,labels,cursor = legacy(kernel)
        results['char'][kernel] = scalar(memory,0xC809,cursor,work)
        string_924[kernel] = full_string(memory,cursor,{'StringZ':0xC818})
        if kernel=='Monitor2':
            results['char']['Monitor2-C037'] = scalar(memory,labels['loader_printChar'],cursor,work)
        for key in ('Rotate','Packed','Hybrid','Fast'):
            memory,labels,cursor = legacy(kernel,key)
            name = kernel+'-K7'+key
            raster = 'xor' if kernel == 'Monitor2' else 'opaque'
            origin = state[kernel][key]['origin']
            results['char'][name] = scalar(memory,origin,cursor,work,raster=raster)
            apis = {'StringZ':origin+3,'StringRawLim':origin+6}
            string_924[name] = full_string(memory,cursor,apis,raster=raster)
            if key == 'Fast':
                results['full_fast'][name] = batch(memory,labels,cursor,xor=kernel=='Monitor2')

(report/'scalar.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
for name,result in results['char'].items():
    print(name,result['printable_mean'],result['A'],result['koi7_A'],flush=True)
for name,result in results['full_fast'].items():
    print(name,result,flush=True)
