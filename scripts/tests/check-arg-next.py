"""РљРѕРЅС‚СЂР°РєС‚ bios_argNext Рё СЂР°Р·Р±РѕСЂ РїР°СЂР°РјРµС‚СЂРѕРІ СЃРёСЃС‚РµРјРЅС‹С… СѓС‚РёР»РёС‚ РЅР° CPU 8080."""
import itertools
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'docs/compat/arg-next-001/cpu'
TICKS = Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')


def symbols(name):
    path = (ROOT / 'source-apps/diskbench/build/DISKBEN.sym' if name=='DISKBEN'
            else ROOT / 'bin' / (name + '.sym'))
    return {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        path.read_text(), re.M)}


def memory(name=None):
    m = bytearray(65536)
    dos = (ROOT / 'bin/DOS.sys').read_bytes()
    m[0xC000:0xC000+len(dos)] = dos
    if name:
        sy = symbols(name)
        code = (ROOT / 'bin' / (name + '.i80')).read_bytes()
        address = 0 if name in ('CHKDSK', 'DISKBEN') else sy.get('start', 0xF100)
        m[address:address+len(code)] = code
    return m


def run(m, entry, end=0x8100, hl=0x7000, de=0x7000, bc=0xAA5A):
    wrapper = bytes.fromhex('31 00 8F 21') + hl.to_bytes(2, 'little')
    wrapper += b'\x11'+de.to_bytes(2, 'little')+b'\x01'+bc.to_bytes(2, 'little')
    wrapper += b'\xCD'+entry.to_bytes(2, 'little')
    # HL, DE, BC Рё AF РІ СЃРЅРёРјРѕРє; РѕСЃС‚Р°РЅРѕРІ РїРѕСЃР»Рµ РѕР±С‹С‡РЅРѕРіРѕ RET.
    wrapper += bytes.fromhex('22 00 72 EB 22 02 72 60 69 22 04 72 F5 E1 22 06 72 C3 00 81')
    m[0x8000:0x8000+len(wrapper)] = wrapper
    inp, target = OUT/'input.bin', OUT/'output.bin'
    inp.write_bytes(m)
    subprocess.run([str(TICKS), '-m8080', '-pc', '8000', '-end', f'{end:x}',
                    '-counter', '2000000', '-output', str(target), str(inp)],
                   capture_output=True, check=True, timeout=15)
    b = target.read_bytes()
    assert int.from_bytes(b[65542:65544], 'little') == end
    assert int.from_bytes(b[65544:65546], 'little') == (0x8F00 if end==0x8100 else 0x8EFE)
    return b


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    ds = symbols('DOS')
    assert ds['bios_argNext'] == 0xC8B7
    dos = (ROOT/'bin/DOS.sys').read_bytes()
    assert dos[0x8B7:0x8BA] == b'\xC3'+ds['argNext'].to_bytes(2, 'little')
    token_cases = 0
    for raw in (b'\0', b'   \r', b'foo bar\0', b'  -MON:Test  A:GAME.EXE  \r',
                b'A:\0', bytes((0xC1, 0xE2, 0xFF, 0)), b'x'*255+b'\0',
                b'one\ttwo\0', b'one\ntwo\0'):
        cursor = 0
        while True:
            start = cursor
            while raw[start] == 32:
                start += 1
            stop = start
            while raw[stop] > 32:
                stop += 1
            m = memory()
            m[0x7000:0x7000+len(raw)] = raw
            got = run(m, ds['bios_argNext'], hl=0x7000+cursor)
            assert got[0x7200:0x7206] == ((0x7000+stop).to_bytes(2, 'little')+
                (0x7000+start).to_bytes(2, 'little')+bytes((0x5A, stop-start)))
            assert bool(got[0x7206] & 1) == (start==stop)
            assert got[0x7000:0x7000+len(raw)] == raw
            token_cases += 1
            if start==stop:
                break
            cursor = stop
    fs = symbols('FORMAT')
    format_cases = 0
    for tokens in itertools.permutations(('B:', '-R', '-Y')):
        raw = ('  '+'   '.join(tokens).lower()+'  ').encode()+b'\r'
        m = memory('FORMAT')
        m[0x7000:0x7000+len(raw)] = raw
        got = run(m, 0xF100, end=fs['Confirmed'])
        assert got[fs['v_FormatConfirmed']] == 255
        assert got[fs['v_FormatRAMdisk']] == 255
        # BC СЃРѕС…СЂР°РЅСЏРµС‚ РІС‹Р±СЂР°РЅРЅС‹Р№ РґРёСЃРє РґРѕ РЅР°С‡Р°Р»Р° С„РѕСЂРјР°С‚РёСЂРѕРІР°РЅРёСЏ.
        assert got[65536+3] == ord('B')
        format_cases += 1
    for raw in (b'B: B: -Y\0', b'B: -RR\0', b'B: -Yjunk\0', b'-X B:\0'):
        m = memory('FORMAT')
        m[0x7000:0x7000+len(raw)] = raw
        run(m, 0xF100, end=fs['errParams'])
        format_cases += 1
    bs = symbols('DISKBEN')
    size_cases = 0
    for raw, expected in ((b'\0', 8), (b'   \0', 8), (b' 24  \0', 24),
                          (b'1\0', 1), (b'08\0', 8), (b'0\0', None),
                          (b'25\0', None), (b'8 9\0', None), (b'8x\0', None)):
        m = memory('DISKBEN')
        m[0x7000:0x7000+len(raw)] = raw
        got = run(m, bs['parse_file_kib'])
        assert bool(got[0x7206]&1) == (expected is None), raw
        if expected is not None:
            assert got[0x7207] == expected, raw
        size_cases += 1
    choice_cases = 0
    for raw, expected in ((b'\0', 0), (b'  \0', 0), (b' B  \0', ord('B')),
                          (b'2\0', ord('2')), (b'BB\0', None), (b'B C\0', None)):
        m = memory('DISKBEN')
        # РўРѕР»СЊРєРѕ CPU-С‚РµСЃС‚ Р°РЅР°Р»РёР·Р°С‚РѕСЂР°: СЂРµРґР°РєС‚РѕСЂ РІРІРѕРґР° Р·Р°РјРµРЅС‘РЅ РІРѕР·РІСЂР°С‚РѕРј Р±СѓС„РµСЂР°.
        addr = bs['read_line']
        m[addr:addr+4] = bytes.fromhex('21 00 70 C9')
        m[0x7000:0x7000+len(raw)] = raw
        got = run(m, bs['read_choice'])
        assert bool(got[0x7206]&1) == (expected is None), raw
        if expected is not None:
            assert got[0x7207] == expected, raw
        choice_cases += 1
    cs = symbols('CHKDSK')
    chkdsk_cases = 0
    for raw, target in ((b'  B:  \0', 'chooseDrive'), (b'   \r', 'askDrive')):
        m = memory('CHKDSK')
        # Только CPU-тест разбора; вывод не участвует в выборе аргумента.
        m[0xC809] = m[0xC818] = 0xC9
        m[0x7000:0x7000+len(raw)] = raw
        got = run(m, cs['entry'], end=cs[target])
        if target=='chooseDrive':
            assert got[65536+1] == ord('B')
        chkdsk_cases += 1
    result = dict(tokenCases=token_cases, formatCases=format_cases,
                  sizeCases=size_cases, choiceCases=choice_cases, chkdskCases=chkdsk_cases,
                  sourceUnchanged=True, cPreserved=True, stackBalanced=True)
    (OUT/'results.json').write_text(json.dumps(result, indent=2)+'\n')
    print(result)


if __name__ == '__main__':
    main()
