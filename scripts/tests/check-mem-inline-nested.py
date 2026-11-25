"""8080: прямой TEXT из общего окна и вложенные пачки COMMAND."""
import argparse
from pathlib import Path
import re
import runpy
import struct
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def symbols(path):
    return {key: int(value, 16) for key, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(), re.M)}


def word(value):
    return struct.pack('<H', value)


def put(memory, address, data):
    memory[address:address + len(data)] = data


def trace_registers(trace):
    for registers in re.finditer(
            rb'pc=([0-9A-F]+), \[pc\]=([0-9A-F]+),\s+bc=([0-9A-F]+),'
            rb'\s+de=([0-9A-F]+),\s+hl=([0-9A-F]+),[^\r\n]*\r?\nsp=([0-9A-F]+)', trace):
        yield tuple(int(value, 16) for value in registers.groups())


def source_reads(trace, first, last):
    reads = []
    for pc, opcode, bc, de, hl, sp in trace:
        if (opcode & 0xc7) in (0x46, 0x86) or opcode in (0x34, 0x35):
            addresses = (hl,)
        elif opcode in (0x0a, 0x1a):
            addresses = (bc if opcode == 0x0a else de,)
        elif opcode in (0xc1, 0xd1, 0xe1, 0xf1, 0xe3):
            addresses = (sp, (sp + 1) & 0xffff)
        else:
            continue
        reads.extend((pc, address) for address in addresses if first <= address < last)
    return reads


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--bin', type=Path, default=ROOT / 'bin')
    parser.add_argument('--ticks', type=Path, default=Path(
        'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    args = parser.parse_args()
    bios = symbols(args.bin / 'BIOS.sym')
    mem = symbols(args.bin / 'banked/LOCALBLOCK.sym')
    io = symbols(args.bin / 'banked/MAINBIOS.sym')
    image = bytearray(65536)
    runpy.run_path(str(ROOT / 'scripts/tests/mem-image.py'))['install'](image, bios, args.bin)
    # Стенд начинает выполнение в банке DOS, где используется банковый диспетчер.
    put(image, mem['bankMxRegsRouteCall'] + 1, word(mem['bankMxRegsRoute']))
    image[mem['bankPage']] = 1
    image[mem['bankPageCount']] = 16
    put(image, mem['bankServiceSP'], word(0xe000))

    source, output, cursor = 0x60ef, 0x6200, 0x6500
    write_char, overwrite, print_z, print_char = 0x8000, 0x8020, 0x8060, 0x8080
    inner, copy_request = 0x80a0, 0x80b0
    print_raw_z, print_raw_lim, print_pseudo = 0x80c0, 0x80e0, 0x8100
    print_text_lim = 0x8120
    alternate_z, alternate_raw_z = 0x8160, 0x8180
    copy_source, copy_destination = 0x6700, 0x6800
    result, entry, done = 0x7500, 0x8800, 0x88ff
    window = mem['BANK_WINDOW']

    def vector(name, target):
        put(image, bios[name], b'\xc3' + word(target))

    put(image, cursor, word(output))
    for name, target in (
            ('bios_printChar', print_char),
            ('FONT_ENGINE_Z', print_z), ('bios_printStringZ', print_z),
            ('FONT_ENGINE_RAWZ', print_raw_z), ('bios_printStringRawZ', print_raw_z),
            ('FONT_ENGINE_TEXT_LIM', print_text_lim), ('FONT_ENGINE_LIM', print_raw_lim),
            ('bios_printStringRawLim', print_raw_lim), ('bios_printStringRawPseudo', print_pseudo)):
        vector(name, target)
    put(image, bios['FONT_ENGINE_TEXT_EXT'], word(bios['FONT_ENGINE_TEXT_LIM']))
    # Добавить A к выводу, сохранив позицию печати в HL.
    put(image, write_char, b'\xe5\x2a' + word(cursor) + b'\x77\x23\x22'
        + word(cursor) + b'\xe1\xc9')
    # Плоский стенд игнорирует маппер. Установить логическую страницу MAIN.
    # Адреса источника и назначения в стенде не перекрываются.
    # После вложенного вызова отдельно испортить все 32 байта общего окна.
    page = word(mem['bankPage'])
    put(image, overwrite, b'\xe5\x3a' + page + b'\xf5\xaf\x32' + page
        + b'\x21' + word(copy_request) + b'\xcd' + word(mem['mx_memcpyFar'])
        + b'\xda' + word(0x80ff) + b'\xf1\x32' + page
        + b'\xc5\x21' + word(mem['BANK_WINDOW']) + b'\x06\x20'
        + b'\x36\xe7\x23\x05\xc2' + word(overwrite + 28) + b'\xc1\xe1\xc9')
    put(image, 0x80ff, b'\x76')
    put(image, copy_source, b'\xe7' * 32)
    put(image, copy_request, struct.pack('<BHBHH', 2, copy_source, 0, copy_destination, 32))
    # Отдельная точка входа отличает очередной фрагмент от оборота цикла.
    # Строковый движок оставляет общее окно доступным для чтения до возврата.
    def print_loop(address, *, limited=False, terminated=False):
        loop = address + 3
        code = b'\xc3' + word(loop)
        if limited:
            code += b'\x78\xb1\xc8'
        code += b'\x7e'
        if terminated:
            code += b'\xb7\xc8'
        code += b'\xcd' + word(write_char) + b'\x23'
        if limited:
            code += b'\x0b'
        put(image, address, code + b'\xc3' + word(loop))

    for address in (print_z, print_raw_z, alternate_z, alternate_raw_z):
        print_loop(address, terminated=True)
    for address in (print_raw_lim, print_pseudo, print_text_lim):
        print_loop(address, limited=True)
    # Первая внешняя запись запускает вторую пачку из локальной памяти MAIN.
    put(image, print_char, b'\x79\xcd' + word(write_char) + b'\xfeA\xf5\xcd'
        + word(overwrite) + b'\xf1\xc0\x21' + word(inner) + b'\x01\x05\0\xc3'
        + word(io['bbBatchLocal']))
    record = lambda char: b'\x02\x05\x04' + bytes([char, 0])
    put(image, inner, record(ord('!')))
    save = b'\x22' + word(result) + b'\x21\0\0\x39\x22' + word(result + 2)
    save += b'\x60\x69\x22' + word(result + 4) + b'\xeb\x22' + word(result + 6)
    save += b'\xc3' + word(done)

    def run_case(folder, payload, call, expected, expected_hl=None, *, read_count=None,
                 expected_bc=0x5a24, profile=None, printer=None):
        ram = (profile if profile is not None else image).copy()
        put(ram, source, payload)
        put(ram, entry, b'\x31\0\x70\x01\x24\x5a\x11\x34\x12\x21' + word(source) + call + save)
        initial, final = folder / 'in.bin', folder / 'out.bin'
        initial.write_bytes(ram)
        command = [str(args.ticks), '-m8080', '-pc', f'{entry:x}',
                   '-end', f'{done:x}', '-counter', '3000000', '-output', str(final)]
        if read_count is not None:
            command.append('-trace')
        run = subprocess.run(command + [str(initial)], check=True, capture_output=True, timeout=15)
        actual = final.read_bytes()
        assert struct.unpack_from('<H', actual, 65542)[0] == done, (
            call.hex(), len(expected), run.stdout[-350:].decode('ascii', errors='replace'))
        end = struct.unpack_from('<H', actual, cursor)[0]
        assert end == output + len(expected), (expected, actual[output:end])
        assert actual[output:output + len(expected)] == expected
        if read_count is None:
            assert actual[copy_destination:copy_destination + 32] == b'\xe7' * 32
        else:
            assert actual[copy_destination:copy_destination + 32] == ram[copy_destination:copy_destination + 32]
            assert not actual[65536] & 1, (call.hex(), actual[65536:65538].hex())
        hl, sp, bc, de = struct.unpack_from('<HHHH', actual, result)
        assert sp == 0x7000 and (expected_hl is None or hl == expected_hl), (hl, sp)
        assert (actual[mem['bankPage']], actual[mem['bankDepth']]) == (1, 0)
        assert struct.unpack_from('<H', actual, mem['bankServiceSP'])[0] == 0xe000
        if read_count is not None:
            assert (bc, de) == (expected_bc, 0x1234), (call.hex(), bc, de)
            trace = list(trace_registers(run.stdout))
            reads = source_reads(trace, source, source + len(payload))
            assert [address for _, address in reads] == list(range(source, source + read_count)), (
                call.hex(), read_count, reads)
            printers = {print_z, print_raw_z, print_raw_lim, print_pseudo, print_text_lim,
                        alternate_z, alternate_raw_z}
            calls = [(pc, bc, hl, sp) for pc, _, bc, _, hl, sp in trace if pc in printers]
            if expected:
                assert calls, (call.hex(), 'печатный вектор не вызван')
            assert all(pc == printer for pc, _, _, _ in calls), (call.hex(), calls)
            expected_counts = [min(32, len(expected) - i) for i in range(0, len(expected), 32)]
            if printer in (print_text_lim, print_raw_lim, print_pseudo):
                assert [count for _, count, _, _ in calls] == expected_counts, (calls, expected_counts)
                starts = [window] * len(expected_counts)
            else:
                # Старый ASCIIZ-вектор получает полную порцию как 31+1 из того же окна.
                starts = [start for count in expected_counts
                          for start in ([window, window + 30] if count == 32 else [window])]
            assert [hl for _, _, hl, _ in calls] == starts, (call.hex(), calls, starts)
            return calls

    with tempfile.TemporaryDirectory() as temporary:
        folder = Path(temporary)
        lengths = (0, 1, 19, 20, 21, 30, 31, 32, 33, 43, 63, 64, 65, 127, 128, 255, 256, 257)
        print_command = b'\x3e\x05\xcd' + word(mem['mx_memory'])

        def print_bios(kind, count):
            return (b'\x01' + word(count) + b'\x3e\x01\xcd' + word(mem['mx_callRegs'])
                    + word(mem['MXOS_BIOS_PRINT_STRING_' + kind]))

        for count in lengths:
            line = (b'Abc123! ' * 33)[:count]
            run_case(folder, line + b'\0poison', print_command, line, source + count,
                     read_count=count + 1, printer=print_text_lim)
            for suffix, printer in (('RAW_Z', print_raw_lim), ('RAW_LIM', print_raw_lim),
                                    ('RAW_PSEUDO', print_pseudo)):
                terminated = suffix == 'RAW_Z'
                raw = line if terminated else bytes(index & 0xff for index in range(count))
                run_case(folder, raw + b'\0poison', print_bios(suffix, count), raw, source + count,
                         read_count=count + int(terminated), expected_bc=count,
                         printer=printer)

        no_exports = image.copy()
        put(no_exports, bios['FONT_ENGINE_TEXT_EXT'], b'\0\0')
        other_vectors = image.copy()
        put(other_vectors, bios['bios_printStringZ'], b'\xc3' + word(alternate_z))
        put(other_vectors, bios['bios_printStringRawZ'], b'\xc3' + word(alternate_raw_z))
        for profile, z, raw_z in ((no_exports, print_z, print_raw_lim),
                                  (other_vectors, alternate_z, alternate_raw_z)):
            for count in (0, 31, 32, 33, 64, 65):
                line = b'Q' * count
                run_case(folder, line + b'\0poison', print_command, line, source + count,
                         read_count=count + 1, profile=profile, printer=z)
                run_case(folder, line + b'\0poison', print_bios('RAW_Z', count), line, source + count,
                         read_count=count + 1, expected_bc=count, profile=profile, printer=raw_z)
        for size in (10, 19, 20, 21, 30, 31, 32, 33):
            tail = size // 5 - 1
            batch = record(ord('A')) + record(ord('B')) * tail
            batch += bytes(size - len(batch))
            run_case(folder, batch, b'\x01' + word(size) + b'\x3e\x01\xcd'
                     + word(mem['mx_callRegs']) + word(mem['MXOS_BIOS_COMMAND_BATCH']),
                     b'A!' + b'B' * tail)
    print('MEM INLINE NESTED PASS: TEXT из FFC0h, совместимость Z, границы чтения, '
          'вложенные пачки 10–33 байта, регистры и SP')


if __name__ == '__main__':
    main()
