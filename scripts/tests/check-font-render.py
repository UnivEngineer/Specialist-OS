"""8080: РїР°РєРµС‚РЅС‹Р№ BIOS 6x8, РїСЂРµСЂРµРЅРґРµСЂ, РєСЂР°СЏ, KOI/ESC Рё РїСѓР±Р»РёС‡РЅС‹Р№ ABI."""
from pathlib import Path
import argparse
import json
import re
import subprocess

R = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--out', type=Path, default=R/'docs/bench/bios-font/cpu')
a = p.parse_args()
O = a.out.resolve()
O.mkdir(parents=True, exist_ok=True)
S = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (R/'bin/BIOS.sym').read_text(), re.M)}
W = lambda x: x.to_bytes(2, 'little')
ticks = 'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'
font = (R/'assets/system-rom/FONT.FNT').read_bytes()
compact = bytes(font[c*8+y] for y in range(8) for c in range(256))
base = bytearray(65536)
for name, address in [('BIOS', 0xC000), ('IO', 0xF000)]:
    data = (R/f'bin/{name}.sys').read_bytes()
    base[address:address+len(data)] = data
base[S['bios_vars']:S['bios_vars']+S['BIOS_VARIABLES']] = base[S['initVars']:S['initVarsEnd']]
base[S['FONT_ADDR']:S['FONT_ADDR']+len(compact)] = compact
base[S['bios_upper.charGen']:S['bios_upper.charGen']+9] = base[S['initTextVars']:S['initTextVarsEnd']]
base[0x6000:0x6800] = font
base[0x6800:0x7000] = bytes((x ^ 0x15) & 63 for x in font)
base[0x9000:0xC000] = bytes((i*37+(i>>8)*13)&255 for i in range(12288))
base[S['GLYPH_CACHE_EPOCH']] = 9
base[S['bios_upper.escMode']] = 255

# РќРµР·Р°РІРёСЃРёРјС‹Р№ РїРѕСЃРёРјРІРѕР»СЊРЅС‹Р№ С†РёРєР»: РїРѕР»РЅС‹Р№ printChar СЃРѕ С€С‚Р°С‚РЅС‹Рј С„СЂРѕРЅС‚РµРЅРґРѕРј.
reference = bytes.fromhex('C5 F5 7E B7 CA 0F 08 4F CD')+W(S['bios_printChar'])+bytes.fromhex('23 C3 02 08 F1 C1 C9')
base[0x800:0x800+len(reference)] = reference

def execute(memory, code):
    m = bytearray(memory)
    m[0x100:0x100+len(code)] = code
    (O/'input.bin').write_bytes(m)
    result = subprocess.run([ticks, '-m8080', '-pc', '100', '-end', '2000', '-counter', '30000000', '-output', str(O/'output.bin'), str(O/'input.bin')], capture_output=True, check=True, timeout=20)
    got = (O/'output.bin').read_bytes()
    assert got[65542:65546] == W(0x2000)+W(0x8FE0), got[65542:65546].hex()
    return got, int(result.stdout.strip())

def call(memory, entry, af=0, hl=0, bc=0x1357, de=0x2468):
    code = b'\x31'+W(0x8FE0)+b'\x21'+W(af)+b'\xe5\xf1\x01'+W(bc)+b'\x11'+W(de)+b'\x21'+W(hl)+b'\xcd'+W(entry)+b'\xc3'+W(0x2000)
    return execute(memory, code)

def state(data):
    return tuple(bytes(data[S[k]:S[k]+n]) for k, n in [('bios_vars.cursorY', 2), ('bios_upper.escMode', 1), ('bios_upper.koi7', 1)])

def render(memory, text, entry, x=0, y=8, inverse=0, koi=0):
    m = bytearray(memory)
    m[0x1000:0x1000+len(text)+1] = text+b'\0'
    m[S['bios_vars.cursorY']:S['bios_vars.cursorY']+2] = bytes([y, x])
    m[S['bios_vars.inverse']:S['bios_vars.inverse']+2] = bytes([inverse])*2
    m = bytearray(call(m,S['bios_setFontEncoding'],af=koi<<8)[0][:65536])
    got, cycles = call(m, entry, af=0x5A97, hl=0x1000)
    assert got[65536:65538] == bytes([0x97, 0x5A]), got[65536:65538].hex()
    assert got[65538:65542] == W(0x1357)+W(0x1000+len(text)), got[65538:65542].hex()
    assert got[65548:65550] == W(0x2468), got[65548:65550].hex()
    return got, cycles

prepared = {}
checks = 0
for linear in (False,):
    m = bytearray(base)
    m[S['bios_upper.charGen']:S['bios_upper.charGen']+2] = W(0x6000 if linear else S['FONT_ADDR'])
    built, _ = call(m, S['fast6.service'], af=0x100, hl=0x3000)
    assert built[65537] == 1 and not built[65536]&1
    assert built[S['fast6.preBase']:S['fast6.preBase']+2] == W(0x3000)
    assert built[0x2FFF] == m[0x2FFF] and built[0x5800] == m[0x5800]
    transforms = [lambda b:b<<2,lambda b:b>>2,lambda b:(b<<6)&255,lambda b:b>>4,lambda b:(b<<4)&255]
    expected = b''.join(bytes(t(font[c*8+y]&63) for y in range(8) for c in range(256)) for t in transforms)
    assert built[0x3000:0x5800] == expected, 'phase data'
    prepared[linear] = (m, built[:65536])
    samples = [(bytes(range(32, 256)), x, 8, inverse, koi)
               for x in range(4) for inverse in (0, 1, 255) for koi in (0, 255)]
    samples += [(b'Abcdefghi'[:n], x, 8, inv, koi) for n in range(10) for x in range(4) for inv in (0,1,255) for koi in (0,255)]
    samples += [(b'Abcd\tEFGH\rIJKL\nmnop\x1b(abcdef\x1b)qrst', x, 8, inv, 0)
                for x in range(4) for inv in (0, 255)]
    # Controls in every slot of a quad must leave the unrendered prefix
    # for scalar dispatch and commit only already rendered packet glyphs.
    samples += [(b'ABCDEFG'[:n] + control + b'xyz', x, 8, inv, 0)
                for n in range(8) for x in range(4) for inv in (0, 255)
                for control in (b'\r', b'\n', b'\t', b'\x1b(', b'\x1b)')]
    # COLPRE has an inline ordinary cursor and a rare pre-draw wrap helper.
    # Exercise both sides of the boundary, every inverse representation, and
    # legacy out-of-range X values, including a wrap on the bottom scanlines.
    samples += [(b'ABCDEFGHI', x, y, inv, 0)
                for x in (180, 184, 188, 189, 190, 191, 192, 255)
                for y in (0, 6, 7, 248, 255) for inv in (0, 1, 255)]
    for text, x, y, inverse, koi in samples:
        expected, _ = render(m, text, 0x800, x, y, inverse, koi)
        for label, memory in [('rotate', m), ('prerender', built[:65536])]:
            actual, _ = render(memory, text, S['bios_printString'], x, y, inverse, koi)
            assert actual[0x9000:0xC000] == expected[0x9000:0xC000], (label, linear, x, y, inverse, koi, text[:12])
            assert state(actual) == state(expected), (label, 'state', linear, x, y)
            checks += 1

# РќРµР·Р°РІРёСЃРёРјС‹Р№ РїРёРєСЃРµР»СЊРЅС‹Р№ СЌС‚Р°Р»РѕРЅ РІСЃРµС… 256 РєРѕРґРѕРІ РґР»СЏ РїСЂСЏРјРѕРіРѕ СЃРёРјРІРѕР»Р°.
# РљР°Р¶РґС‹Р№ РіР»РёС„ РёРјРµРµС‚ РѕС‚РґРµР»СЊРЅРѕРµ РјРµСЃС‚Рѕ: РїРѕР·РґРЅРёР№ РІС‹РІРѕРґ РЅРµ СЃРєСЂС‹РІР°РµС‚ РѕС€РёР±РєСѓ СЂР°РЅРЅРµРіРѕ.
for xphase in range(4):
    for inv in (0,1,255):
        for koi in (0,255):
            for memory in (m,built[:65536]):
                mem=bytearray(memory)
                mem[S['bios_vars.inverse']:S['bios_vars.inverse']+2]=bytes([inv])*2
                mem=bytearray(call(mem,S['bios_setFontEncoding'],af=koi<<8)[0][:65536])
                expected=bytearray(mem)
                code=b'\x31'+W(0x8FE0)
                positions=[]
                for c in range(256):
                    x=(c%24)*8+xphase
                    y=7+(c//24)*8
                    code+=b'\x21'+W((x<<8)|y)+b'\x22'+W(S['bios_vars.cursorY'])
                    code+=bytes([0x3E,c,0xCD])+W(S['bios_printCharReal'])
                    code+=b'\x2A'+W(S['bios_vars.cursorY'])+b'\x22'+W(0x7800+c*2)
                    accepted=True
                    positions+=list(bytes([y,x+3 if accepted else x]))
                    if not accepted: continue
                    mapped=c^128 if koi and (0x60<=c<0x80 or c>=0xE0) else c
                    for row in range(8):
                        bits=font[mapped*8+row]&63
                        if inv: bits^=63
                        for pixel in range(6):
                            sx=x*2+pixel
                            at=0x9000+(sx//8)*256+y-7+row
                            mask=0x80>>(sx%8)
                            expected[at]=(expected[at]&~mask)|(mask if bits&(0x20>>pixel) else 0)
                code+=b'\xC3'+W(0x2000)
                got,_=execute(mem,code)
                assert got[0x9000:0xC000]==expected[0x9000:0xC000],('all glyphs',xphase,inv,koi)
                assert got[0x7800:0x7A00]==bytes(positions),('cursor immediate',xphase,inv,koi)
                checks+=256

# РџСЂСЏРјС‹Рµ СЃС‚СЂРѕРєРё РёРіРЅРѕСЂРёСЂСѓСЋС‚ СѓРїСЂР°РІР»СЏСЋС‰РёРµ Р±Р°Р№С‚С‹, РЅРµ РјРµРЅСЏСЏ СЃРѕСЃС‚РѕСЏРЅРёРµ ESC.
for koi in (0,255):
    for inverse in (0,1,255):
        for memory in (m,built[:65536]):
            text = b'AB\x01\x09CD\x1b(EF\rGH\nIJKL'
            clean = text
            # Independent scalar Raw loop, including control-code glyphs.
            rawref = bytearray(memory)
            routine = bytearray.fromhex('C5 F5 D5 7E B7 CA 00 00 E5 C5 D5 7E CD')+W(S['bios_printCharRaw'])+bytes.fromhex('D1 C1 E1 23 C3 03 08 D1 F1 C1 C9')
            routine[6:8] = W(0x800+len(routine)-4)
            rawref[0x800:0x800+len(routine)] = routine
            reference,_ = render(rawref,clean,0x800,inverse=inverse,koi=koi)
            actual,_ = render(memory,text,S['bios_printStringReal'],inverse=inverse,koi=koi)
            assert actual[0x9000:0xC000] == reference[0x9000:0xC000], 'real controls'
            assert actual[S['bios_upper.escMode']] == 255 and actual[S['fast6.realMode']] == 0
            checks += 1
# РџРµСЂРµСЃС‚Р°РЅРѕРІРєР° KOI РїСЂРѕРІРµСЂСЏРµС‚СЃСЏ РЅРµР·Р°РІРёСЃРёРјРѕ РѕС‚ РѕР±РѕРёС… СЂРµРЅРґРµСЂРѕРІ.
for memory in (m,built[:65536]):
    koi = bytearray(memory)
    after,_ = call(koi,S['bios_setFontEncoding'],af=0xFF00)
    source_font = memory[S['FONT_ADDR']:S['FONT_ADDR']+2048]
    expected_font = bytearray(source_font)
    for y in range(8):
        for c in range(32):
            expected_font[y*256+0x60+c] = source_font[y*256+0xE0+c]
            expected_font[y*256+0xE0+c] = source_font[y*256+0x60+c]
    assert after[S['FONT_ADDR']:S['FONT_ADDR']+2048] == expected_font
    after = bytearray(after[:65536])
    restored,_ = call(after,S['bios_setFontEncoding'],af=0)
    assert restored[S['FONT_ADDR']:S['FONT_ADDR']+2048] == source_font
    assert restored[0x3000:0x5800] == memory[0x3000:0x5800], 'KOI round trip prerender'
    checks += 1
esc_memory = bytearray(m)
esc_memory[S['bios_upper.escMode']] = 1
actual,_ = render(esc_memory,b'ABCDEFGH',S['bios_printStringReal'])
reference,_ = render(m,b'ABCDEFGH',0x800)
assert actual[0x9000:0xC000] == reference[0x9000:0xC000]
assert actual[S['bios_upper.escMode']] == 1
checks += 1

# Р›РёРЅРµР№РЅС‹Р№ С€СЂРёС„С‚ СЃС‚РѕСЂРѕРЅРЅРµРіРѕ РєР»РёРµРЅС‚Р°: СЃРѕРІРјРµСЃС‚РёРјРѕСЃС‚СЊ РёСЃС…РѕРґРЅРёРєРѕРІ РїСЂРѕРµРєС‚Р°.
linear_memory = bytearray(base)
linear_memory[S['bios_upper.charGen']:S['bios_upper.charGen']+2] = W(0x6000)
for text in (b'abcDEF', bytes(range(32,256))):
    expected,_ = render(linear_memory,text,0x800)
    actual,_ = render(linear_memory,text,S['bios_printString'])
    assert actual[0x9000:0xC000] == expected[0x9000:0xC000], 'linear fallback'
    checks += 1

# РќРµРґРѕРїСѓСЃС‚РёРјС‹Р№ Р±СѓС„РµСЂ РЅРµ РѕС‚РєР»СЋС‡Р°РµС‚ Рё РЅРµ РїРѕСЂС‚РёС‚ РїСЂРµР¶РЅРёР№ РїСЂРµСЂРµРЅРґРµСЂ.
m, built = prepared[False]
for address in (0, 0x3001, 0x6500, 0x8000, 0xFF00):
    got, _ = call(built, S['fast6.service'], af=0x100, hl=address)
    assert got[65536]&1 and got[S['fast6.preBase']:S['fast6.preBase']+2] == W(0x3000)
    assert got[0x3000:0x5800] == built[0x3000:0x5800]
    checks += 1
for change in ('font', 'epoch'):
    stale = bytearray(built)
    if change == 'font':
        stale[S['bios_upper.charGen']:S['bios_upper.charGen']+2] = W(0x6800)
    else:
        stale[S['GLYPH_CACHE_EPOCH']] += 1
    got, _ = call(stale, S['fast6.service'])
    assert got[65537] == 0 and not got[65536]&1
    expected, _ = render(stale, b'abcdefgh', 0x800)
    actual, _ = render(stale, b'abcdefgh', S['bios_printString'])
    assert actual[0x9000:0xC000] == expected[0x9000:0xC000], change
    checks += 1
# РћРґРёРЅРѕС‡РЅС‹Рµ РіРѕР»РѕРІС‹/С…РІРѕСЃС‚С‹ РЅРµ С‡РёС‚Р°СЋС‚ РёСЃРїРѕСЂС‡РµРЅРЅС‹Рµ С„Р°Р·С‹ РїРѕСЃР»Рµ РёРЅРІР°Р»РёРґРёСЂРѕРІР°РЅРёСЏ.
stale=bytearray(built)
stale[S['GLYPH_CACHE_EPOCH']]+=1
stale[0x3000:0x5800]=bytes([255])*0x2800
for x in range(4):
    for inverse in (0,1,255):
        for n in (1,2,3):
            text=b'ABC'[:n]
            expected,_=render(m,text,0x800,x=x,inverse=inverse)
            actual,_=render(stale,text,S['bios_printString'],x=x,inverse=inverse)
            assert actual[0x9000:0xC000]==expected[0x9000:0xC000],('stale scalar',x,inverse,n)
            checks+=1

disabled, _ = call(built, S['fast6.service'], af=0x200)
assert disabled[S['fast6.preBase']:S['fast6.preBase']+2] == bytes(2)
assert disabled[0x3000:0x5800] == built[0x3000:0x5800]
rom = bytearray(m)
rom[S['bios_upper.charGen']:S['bios_upper.charGen']+2] = W(0xFFFF)
got, _ = call(rom, S['fast6.service'], af=0x100, hl=0x3000)
assert got[65536]&1

# РћРґРёРЅР°РєРѕРІС‹Р№ РєРѕСЂРїСѓСЃ 48x24: РїРѕРґРіРѕС‚РѕРІРєР° РїСЂРµСЂРµРЅРґРµСЂР° РёСЃРєР»СЋС‡РµРЅР° РёР· С‡РёСЃР»Р° С‚Р°РєС‚РѕРІ.
timings = {}
corpus = (R/'assets/system-rom/README.TXT').read_bytes().replace(b'\r\n', b' ').replace(b'\r', b' ').replace(b'\n', b' ')
corpus = bytes(c if c >= 32 else 32 for c in corpus)[:1152].ljust(1152, b' ')
for linear, (plain, pre) in prepared.items():
    images = []
    for label, memory, entry in [('char', plain, 0x800), ('char-pre',pre,0x800), ('rotate', plain, S['bios_printString']), ('prerender', pre, S['bios_printString']), ('real-rotate',plain,S['bios_printStringReal']), ('real-prerender',pre,S['bios_printStringReal'])]:
        mem = bytearray(memory)
        code = b'\x31'+W(0x8FE0)
        for row in range(24):
            address = 0x1000+row*49
            mem[address:address+49] = corpus[row*48:row*48+48]+b'\0'
            code += b'\x21'+W(7+row*8)+b'\x22'+W(S['bios_vars.cursorY'])+b'\x21'+W(address)+b'\xcd'+W(entry)
        code += b'\xc3'+W(0x2000)
        got, cycles = execute(mem, code)
        images.append(got[0x9000:0xC000])
        timings[('linear' if linear else 'paged')+'-'+label] = dict(cycles=cycles, ms=cycles/2000)
    assert all(image == images[0] for image in images)
# RawLim: exact counts around the 256-byte packet boundary and multiple packets.
# A small independent scalar loop avoids overlapping the corpus with generated
# unrolled reference code when testing lengths above 256.
for memory in prepared[False]:
    for n in (0,1,2,3,4,5,19,20,21,255,256,257,511,512,513,1023,1024,1152):
        for phase in range(4):
            raw=bytearray(memory)
            raw[0x1000:0x1000+n+4]=bytes(c&255 for c in range(n+4))
            raw[S['bios_vars.cursorY']:S['bios_vars.cursorY']+2]=bytes([7,phase])
            routine=bytearray.fromhex('78 B1 CA 00 00 C5 E5 7E CD')+W(S['bios_printCharRaw'])+bytes.fromhex('E1 C1 23 0B C3 00 08 C9')
            routine[3:5]=W(0x800+len(routine)-1)
            raw[0x800:0x800+len(routine)]=routine
            ref,_=call(raw,0x800,hl=0x1000,bc=n)
            got,_=call(raw,S['bios_printStringRawLim'],af=0x5A97,hl=0x1000,bc=n)
            assert got[65536:65538]==W(0x5A97),('RawLim AF',n,phase)
            assert got[65538:65540]==W(n),('RawLim BC',n,phase)
            assert got[65548:65550]==W(0x2468),('RawLim DE',n,phase)
            assert got[65540:65542]==W(0x1000+n),('RawLim pointer',n,phase)
            assert got[0x9000:0xC000]==ref[0x9000:0xC000],('RawLim bitmap',n,phase)
            assert state(got)==state(ref),('RawLim cursor',n,phase)
            assert got[0x1000:0x1000+n+4]==raw[0x1000:0x1000+n+4],'RawLim source'
            checks+=1
# Linear font fallback must preserve the 16-bit count across every scalar call.
for n in (0,1,3,4,20,256,257):
    raw=bytearray(prepared[False][0])
    raw[S['bios_upper.charGen']:S['bios_upper.charGen']+2]=W(0x6000)
    raw[S['bios_vars.cursorY']:S['bios_vars.cursorY']+2]=bytes([7,1])
    raw[0x1000:0x1000+n+4]=bytes(c&255 for c in range(n+4))
    raw[0x800:0x800+len(routine)]=routine
    ref,_=call(raw,0x800,hl=0x1000,bc=n)
    got,_=call(raw,S['bios_printStringRawLim'],hl=0x1000,bc=n)
    assert got[0x9000:0xC000]==ref[0x9000:0xC000],('RawLim linear',n)
    assert got[65540:65542]==W(0x1000+n)
    assert state(got)==state(ref)
    checks+=1
(O/'results.json').write_text(json.dumps(dict(checks=checks, bitmap='PASS', abi='PASS', timings=timings), indent=2)+'\n')
print('BIOS FONT RENDER PASS', checks, timings)
