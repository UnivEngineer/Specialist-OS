"""8080: вывод точного числа байтов на штатных резидентных движках."""
import argparse
from pathlib import Path
import re
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
ENGINES = (
    'Rotate', 'Packed6K', 'Fast12K', 'Compact',
    'Cpm4', 'Cpm4Compact', 'Cpm4Fast',
    'Fast8', 'Fast8Row', 'Graph8', 'Graph8x2', 'Graph8x4', 'GraphP8K', 'Graph14K',
    'K7Rotate', 'K7Packed', 'K7Hybrid',
)
AF, DE, SP = 0x5a97, 0x2468, 0x8fe0
STUB, REFERENCE, DONE = 0x0100, 0x1000, 0x2000
REGISTER_LINE = re.compile(
    rb'pc=([0-9A-F]+), \[pc\]=([0-9A-F]+),\s+bc=([0-9A-F]+),'
    rb'\s+de=([0-9A-F]+),\s+hl=([0-9A-F]+),[^\r\n]*\r?\nsp=([0-9A-F]+)'
)


def symbols(path):
    return {name: int(value, 16) for name, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(encoding='utf-8'), re.M)}


def word(value):
    return value.to_bytes(2, 'little')


def put(memory, address, data):
    memory[address:address + len(data)] = data


def watched_reads(trace, memory, start, end):
    for record in REGISTER_LINE.finditer(trace):
        pc, op, bc, de, hl, sp = (int(value, 16) for value in record.groups())
        if op != 0x76 and ((op & 0xc7) in (0x46, 0x86) or op in (0x34, 0x35)):
            addresses = (hl,)
        elif op in (0x0a, 0x1a):
            addresses = (bc if op == 0x0a else de,)
        elif op in (0xc1, 0xd1, 0xe1, 0xf1, 0xe3):
            addresses = (sp, (sp + 1) & 0xffff)
        elif op in (0x2a, 0x3a):
            address = int.from_bytes(memory[pc + 1:pc + 3], 'little')
            addresses = (address, (address + 1) & 0xffff) if op == 0x2a else (address,)
        else:
            continue
        yield from ((pc, address) for address in addresses if start <= address < end)


def scalar_code(payload, raw, bios):
    # Независимый эталон не читает строку из памяти и не использует пакетный цикл.
    if raw:
        instruction = b'\x3e'
        entry = bios['bios_printCharRaw']
    else:
        instruction = b'\x0e'
        entry = bios['bios_printChar']
    return b''.join(instruction + bytes([value]) + b'\xcd' + word(entry)
                    for value in payload) + b'\xc9'


def cases():
    for count in (0, 1, 31, 32, 33, 255, 256, 257):
        yield count, None
    for count, zero_offsets in ((1, (0,)), (33, (0, 1, 2, 3, 4, 30, 31, 32)),
                                (256, (0,)), (257, (0, 1, 31, 32, 254, 255, 256))):
        for at in zero_offsets:
            yield count, at


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin', type=Path, default=ROOT / 'bin')
    parser.add_argument('--ticks', type=Path, default=Path(
        'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    parser.add_argument('engines', nargs='*', default=ENGINES)
    args = parser.parse_args()
    bios = symbols(args.bin / 'BIOS.sym')
    cursor = bios['bios_vars.cursorY']
    inverse = bios['bios_vars.inverse']
    esc, koi = bios['bios_upper.escMode'], bios['bios_upper.koi7']
    font_bin = args.bin / 'fonts'
    template = bytearray(65536)
    for name, address in (('BIOS', 0xc000), ('IO', 0xf000)):
        put(template, address, (args.bin / f'{name}.sys').read_bytes())
    for name, start, end in (('bios_vars', 'initVars', 'initVarsEnd'),
                             ('bios_upper.charGen', 'initTextVars', 'initTextVarsEnd')):
        put(template, bios[name], template[bios[start]:bios[end]])

    video = bytes((index * 37 + (index >> 8) * 13) & 255 for index in range(0x3000))
    text = (b'A\tB\rC\nD\x1b(E\x1b)FG' + bytes([0x60, 0x7f, 0x80, 0xe0, 0xff])
            + bytes(range(32, 127)) * 3)
    checked = 0
    with tempfile.TemporaryDirectory(prefix='font-text-lim-') as temporary:
        initial = Path(temporary) / 'input.bin'
        final = Path(temporary) / 'output.bin'

        def call(memory, entry, hl=0, bc=0, de=DE, af=AF, trace=False):
            image = bytearray(memory)
            code = (b'\x31' + word(SP) + b'\x21' + word(af) + b'\xe5\xf1\x01' + word(bc)
                    + b'\x11' + word(de) + b'\x21' + word(hl) + b'\xcd' + word(entry)
                    + b'\xc3' + word(DONE))
            put(image, STUB, code)
            initial.write_bytes(image)
            command = [str(args.ticks), '-m8080', '-pc', f'{STUB:x}', '-end', f'{DONE:x}',
                       '-counter', '3000000', '-output', str(final)]
            if trace:
                command.append('-trace')
            result = subprocess.run(command + [str(initial)], check=True, capture_output=True, timeout=20)
            data = final.read_bytes()
            assert data[65542:65546] == word(DONE) + word(SP), (entry, hl, bc, 'PC/SP')
            return data[:65536], data[65536:], result.stdout if trace else b''

        for name in args.engines:
            body = (font_bin / f'{name}.i80').read_bytes()
            sym = symbols(font_bin / f'{name}.sym')
            seed = bytearray(template)
            put(seed, bios['FONT_ENGINE_BASE'], body)
            header = bios['FONT_ENGINE_HEADER'] - bios['FONT_ENGINE_BASE']
            assert body[header:header + 4] == b'FNT1', name
            lower_size = int.from_bytes(body[header + 12:header + 14], 'little')
            lower_base = 0x5000 if lower_size else 0
            if lower_size:
                lower = (font_bin / f'{name}.pre').read_bytes()
                assert len(lower) == lower_size, name
                put(seed, lower_base, lower)
            installed, registers, _ = call(seed, bios['FONT_ENGINE_INIT'], de=lower_base)
            assert not registers[0] & 1, (name, 'init')
            feature = sym['FONT_ENGINE_TEXT_EXT']
            assert installed[feature:feature + 2] == word(sym['FONT_ENGINE_TEXT_LIM']), (name, 'capability')

            for api, vector, raw in (('printStringLim', 'FONT_ENGINE_TEXT_LIM', False),
                                     ('printStringRawLim', 'FONT_ENGINE_LIM', True)):
                entry = sym[vector]
                assert installed[entry] == 0xc3, (name, api, 'vector')
                assert installed[entry + 1:entry + 3] == word(sym[api]), (name, api, 'target')
                for case, (count, zero) in enumerate(cases()):
                    memory = bytearray(installed)
                    source = 0xffe0 - count if count <= 33 else 0x37fd
                    payload = bytearray(text[:count])
                    if zero is not None:
                        payload[zero] = 0
                    # Сторожевые байты с обеих сторон заданного диапазона.
                    guard = b'\xa5\x5a\xa5\x5a\xa5\x5a\xa5\x5a'
                    first, last = source - len(guard), source + count + len(guard)
                    guarded = guard + payload + guard
                    put(memory, first, guarded)
                    put(memory, 0x9000, video)
                    put(memory, cursor, bytes([7, case % 4 * 2]))
                    put(memory, inverse, bytes([255 * (case % 2)]) * 2)
                    put(memory, REFERENCE, scalar_code(payload, raw, bios))
                    actual, regs, trace = call(memory, entry, source, count, trace=True)
                    expected, _, _ = call(memory, REFERENCE)
                    key = (name, api, count, zero)
                    assert regs[:6] == word(AF) + word(count) + word(source + count), (key, regs.hex())
                    assert regs[12:14] == word(DE), (key, regs.hex())
                    assert actual[0x9000:0xc000] == expected[0x9000:0xc000], (key, 'pixels')
                    assert actual[cursor:cursor + 2] == expected[cursor:cursor + 2], (key, 'cursor')
                    assert (actual[esc], actual[koi], actual[inverse:inverse + 2]) == (
                        expected[esc], expected[koi], expected[inverse:inverse + 2]), (key, 'font state')
                    reads = watched_reads(trace, memory, first, last)
                    visited = {address for _, address in reads}
                    expected_reads = set(range(source, source + count))
                    assert visited == expected_reads, (key, 'reads', visited ^ expected_reads)
                    assert actual[first:last] == guarded, (key, 'source')
                    checked += 1
            print(name, 'PASS', flush=True)
    print('FONT TEXT LIM PASS', checked, 'cases')


if __name__ == '__main__':
    main()
