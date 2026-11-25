"""Link the same KOI-7R renderers at free upper-RAM addresses for size probes."""
import argparse
import json
from pathlib import Path
import re
import subprocess
import tempfile

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
report = Path(__file__).resolve().parent
source = root/'source/font'
dest = root/'bin/fonts'


def symbols(path):
    return {s:int(n,16) for s,n in re.findall(
        r'^([\w.]+): EQU 0x([A-Fa-f0-9]+)',path.read_text(encoding='utf-8'),re.M)}


def emit(to, text):
    (source/to).write_text(text,encoding='utf-8')


def prepare(is_monitor):
    asm = (report/'legacy-embed.asm').read_text(encoding='utf-8')
    emit('K7LEGACY.asm',asm)
    replacements = {
        'bios_vars.cursorY':'K7_CURSOR', 'bios_vars.cursorX':'(K7_CURSOR+1)',
        'bios_vars.inverse':'K7_INVERSE', 'bios_upper.escMode':'legacyEsc',
        '"koi7-columns.inc"':'"legacy-7-columns.inc"',
        '"rotate-columns.inc"':'"legacy-7-rotate.inc"',
    }
    for original, short in (('packet6.inc','packet'),('scalar-position.inc','position'),
                            ('rotate-scalar.inc','scalar'),('koi7-columns.inc','columns'),
                            ('rotate-columns.inc','rotate')):
        content = (source/original).read_text(encoding='utf-8')
        for old,new in replacements.items():
            content=content.replace(old,new)
        if original=='packet6.inc':
            marker='    jp c,abortScan\n'
            content=content.replace(marker,marker+'    cp 7Fh\n    jp nc,abortScan\n')
        if is_monitor and original in ('koi7-columns.inc','rotate-columns.inc'):
            content=content.replace('    ld (de),a\n','    ex de,hl\n    xor (hl)\n    ld (hl),a\n    ex de,hl\n')
        if is_monitor and original=='rotate-scalar.inc':
            content='    DEFINE K7_SCALAR_XOR\n'+content
            content=content.replace('    and keep\n','')
            for keep in ('left','right'):
                original_merge=f'    ld a,(bc)\n    xor e\n    and {keep}\n    xor e\n'
                xor_merge=f'    ld a,e\n    and (255-({keep}))\n    ld d,a\n    ld a,(bc)\n    xor d\n'
                content=content.replace(original_merge,xor_merge)
        emit(f'legacy-7-{short}.inc',content)


def assemble(name,mode,origin,align,entry,cursor,inverse):
    sym = dest/f'{name}.sym'
    raw = dest/f'{name}.i80'
    args = [str(root/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',f'-DK7_MODE={mode}',
            f'-DK7_ORIGIN={origin}',f'-DK7_ALIGN={align}',f'-DK7_CHAR={entry}',f'-DK7_CURSOR={cursor}',
            f'-DK7_INVERSE={inverse}',f'--sym={sym}',f'--raw={raw}',str(source/'K7LEGACY.asm')]
    subprocess.run(args,cwd=source,capture_output=True,text=True,check=True)
    return symbols(sym),raw.stat().st_size


def verify(name, kernel, labels, origin, cursor, inverse_address):
    runner = root/'source-apps/fontbench/check-engines.py'
    model = {'__file__':str(runner)}
    exec(compile(runner.read_text(encoding='utf-8').split('cases=')[0],str(runner),'exec'),model)
    m = bytearray(65536)
    legacy = (root/'bin'/('Monitor2.mon' if kernel=='Monitor2' else 'RAMFOS.mon')).read_bytes()
    if kernel=='Monitor2':
        m[0xC000:0xC000+len(legacy)] = legacy
    else:
        m[0xC000:0xD300] = legacy[:0x1300]
        m[0xF800:0xFFE0] = legacy[0x1300:]
    module = (dest/f'{name}.i80').read_bytes()
    m[origin:origin+len(module)] = module
    font = (root/'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
    glyphs = [font[i:i+8] for i in range(0,len(font),8)]
    glyphs[0x60:0x7F] = glyphs[0xE0:0xFF]
    glyphs[0x7F] = bytes(8)
    count = 0
    with tempfile.TemporaryDirectory(prefix='k7-legacy-layout-') as tmp:
        model['O'] = Path(tmp)
        for x in range(4):
            for payload in (b'ABCDefgh', b'ghiJK?@ ', bytes(range(32,80))):
                frame = bytearray(b'\xA5'*12288)
                pos = x*2
                for code in payload:
                    for row,value in enumerate(glyphs[code]):
                        for bit in range(6):
                            pixel = pos+bit
                            at = pixel//8*256+row
                            mask = 128>>(pixel%8)
                            foreground = bool(value & (1<<(5-bit)))
                            if kernel=='Monitor2':
                                if foreground:
                                    frame[at] ^= mask
                            else:
                                frame[at] = (frame[at]&~mask) | (mask if foreground else 0)
                    pos += 6
                test = bytearray(m)
                test[cursor:cursor+2] = bytes([7,x])
                test[inverse_address] = 0
                test[0x9000:0xC000] = b'\xA5'*12288
                test[0x8600:0x8600+len(payload)] = payload
                got,regs,_ = model['run'](test,labels['printStringRawLim'],bc=len(payload))
                assert got[0x9000:0xC000]==frame, (name,x,payload,'pixels')
                assert got[cursor:cursor+2]==bytes([7,pos//2]), (name,x,payload,'cursor')
                assert int.from_bytes(regs[4:6],'little')==0x8600+len(payload)
                count += 1
    return count


mon = symbols(root/'bin/Monitor2.sym')
ram = symbols(root/'bin/RAMFOS.sym')
specs = [
    ('Monitor2',0xD000,mon['loader_printChar'],0x8FFC,'legacyInverse',4096,768),
    ('RAMFOS-MXOS',0xD300,ram['j_printChar'],ram['v_cursorPos'],str(ram['v_inverse']),
     6880,ram['keybMap']-ram['charGen']),
]
out = {}
for kernel,origin,entry,cursor,inverse,original,stock_font in specs:
    prepare(kernel=='Monitor2')
    out[kernel] = {}
    for mode, profile in enumerate(('Rotate','Packed','Hybrid','Fast')):
        name = kernel+'-K7'+profile
        labels,_ = assemble(name,mode,origin,3072,entry,cursor,inverse)
        code = labels['codeEnd']-origin
        aligned = (code+255)&~255
        labels, used = assemble(name,mode,origin,aligned,entry,cursor,inverse)
        assert labels['imageEnd']-origin==used and labels['imageEnd']<=0xF800
        verified = verify(name,kernel,labels,origin,cursor,
                          labels['legacyInverse'] if kernel=='Monitor2' else int(inverse))
        out[kernel][profile] = dict(origin=origin,end=labels['imageEnd'],code=code,checks=verified,
                                   padding=aligned-code,tables=used-aligned,section=used,
                                   original_image=original,original_font=stock_font,
                                   old_plus_section=original+used)
        print(kernel,profile,out[kernel][profile])
(report/'legacy-size.json').write_text(json.dumps(out,indent=2)+'\n')
