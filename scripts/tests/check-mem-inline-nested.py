"""8080: стековый TEXT, границы чтения и вложенные пачки COMMAND."""
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


def source_reads(trace, memory, first, last):
    reads = []
    for pc, opcode, bc, de, hl, sp in trace:
        if opcode != 0x76 and ((opcode & 0xc7) in (0x46, 0x86) or opcode in (0x34, 0x35)):
            addresses = (hl,)
        elif opcode in (0x0a, 0x1a):
            addresses = (bc if opcode == 0x0a else de,)
        elif opcode in (0xc1, 0xd1, 0xe1, 0xf1, 0xe3):
            addresses = (sp, (sp + 1) & 0xffff)
        elif opcode in (0x2a, 0x3a):
            address = int.from_bytes(memory[pc + 1:pc + 3], 'little')
            addresses = (address, (address + 1) & 0xffff) if opcode == 0x2a else (address,)
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
    image[mem['bankOwner']] = 13
    image[mem['bankHome']] = 1
    put(image, mem['bankServiceSP'], word(0xe000))

    source, output, cursor = 0x60ef, 0x6250, 0x6500
    write_char, overwrite, print_z, print_char = 0x8000, 0x8020, 0x8060, 0x8080
    inner, copy_request = 0x80a0, 0x80b0
    print_raw_lim, print_pseudo, print_text_lim = 0x80e0, 0x8100, 0x8120
    print_raw_z = 0x8140
    alternate_z, alternate_raw_z = 0x8160, 0x8180
    alternate_lim, scalar_hook = 0x81c0, 0x81e0
    copy_source, copy_destination = 0x6700, 0x6800
    result, entry, done = 0x7500, 0x8800, 0x88ff
    window = mem['BANK_WINDOW']
    queue_callback, queue_count = 0x81a0, 0x7400
    packet_addr = 0x7420
    caller_af = 0xa597

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
    # Строковый движок сохраняет AF/BC/DE и останавливается на NUL или счётчике.
    def print_loop(address, *, limited=False, terminated=False):
        loop = address + 5
        code = b'\xc3' + word(address + 3) + b'\xf5\xc5'
        exits = []
        if limited:
            code += b'\x78\xb1\xca'
            exits.append(len(code))
            code += b'\0\0'
        code += b'\x7e'
        if terminated:
            code += b'\xb7\xca'
            exits.append(len(code))
            code += b'\0\0'
        code += b'\xcd' + word(write_char) + b'\x23'
        if limited:
            code += b'\x0b'
        code = bytearray(code + b'\xc3' + word(loop))
        for offset in exits:
            put(code, offset, word(address + len(code)))
        put(image, address, code + b'\xc1\xf1\xc9')

    for address in (print_z, print_raw_z, alternate_z, alternate_raw_z):
        print_loop(address, terminated=True)
    for address in (print_raw_lim, print_pseudo, print_text_lim, alternate_lim):
        print_loop(address, limited=True)
    put(image, scalar_hook, b'\xf5\x79\xcd' + word(write_char) + b'\xf1\xc9')
    # Первая внешняя запись запускает вторую пачку из локальной памяти MAIN.
    put(image, print_char, b'\x79\xcd' + word(write_char) + b'\xfeA\xf5\xcd'
        + word(overwrite) + b'\xf1\xc0\x21' + word(inner) + b'\x01\x05\0\xc3'
        + word(io['bbBatchLocal']))
    record = lambda char: b'\x02\x05\x04' + bytes([char, 0])
    put(image, inner, record(ord('!')))
    save = b'\x22' + word(result) + b'\xf5\xe1\x22' + word(result + 8)
    save += b'\x21\0\0\x39\x22' + word(result + 2)
    save += b'\x60\x69\x22' + word(result + 4) + b'\xeb\x22' + word(result + 6)
    save += b'\xc3' + word(done)

    def run_case(folder, payload, call, expected, *, source=source, read_count,
                 terminated=False, expected_bc=0x5a24, expected_af=None,
                 profile=None, printers=(), batch=False, queued=False, counted=False,
                 api='f806', native=False, scalar=False):
        ram = (profile if profile is not None else image).copy()
        put(ram, source, payload)
        af_in = caller_af if expected_af is None else expected_af
        start = b'\x31\0\x70\x21' + word(af_in) + b'\xe5\xf1\x01\x24\x5a\x11\x34\x12\x21'
        put(ram, entry, start + word(source) + call + save)
        initial, final = folder / 'in.bin', folder / 'out.bin'
        initial.write_bytes(ram)
        command = [str(args.ticks), '-m8080', '-pc', f'{entry:x}',
                   '-end', f'{done:x}', '-counter', '3000000', '-output', str(final), '-trace']
        run = subprocess.run(command + [str(initial)], check=True, capture_output=True, timeout=15)
        actual = final.read_bytes()
        assert struct.unpack_from('<H', actual, 65542)[0] == done, (
            call.hex(), len(expected), run.stdout[-350:].decode('ascii', errors='replace'))
        end = struct.unpack_from('<H', actual, cursor)[0]
        assert end == output + len(expected), (call.hex(), hex(source), expected, actual[output:end])
        assert actual[output:output + len(expected)] == expected, (call.hex(), hex(source), expected, actual[output:end])
        if batch:
            assert actual[copy_destination:copy_destination + 32] == b'\xe7' * 32
        else:
            assert actual[copy_destination:copy_destination + 32] == ram[copy_destination:copy_destination + 32]
        hl, sp, bc, de, af = struct.unpack_from('<HHHHH', actual, result)
        assert sp == 0x7000, (call.hex(), sp)
        context = mem['bankPage']
        assert actual[context:context + 7] == ram[context:context + 7], (call.hex(), 'контекст банка')
        if source < window:
            assert actual[source:source + len(payload)] == payload, (call.hex(), 'источник изменён')
        trace = list(trace_registers(run.stdout))
        if expected_af is not None and api == 'f806':
            pcs = [pc for pc, *_ in trace]
            continuation = entry + len(start) + 2 + len(call)
            assert pcs.count(continuation) == 1 and not any(
                pc in (continuation - 2, continuation - 1) for pc in pcs), (call.hex(), 'возврат за DW')
            if not native:
                assert mem['bankFastCheckText'] in pcs and mem['bankMxCallRegsSaved'] not in pcs, (
                    call.hex(), 'прямой F806h')
        if queued:
            assert actual[queue_count] == 1 and actual[mem['bankQueueBefore']] == 0xc3, call.hex()
        hardware = bios['IO_KEYB_A']
        # Плоская память совпадает по адресам с окном MAIN для источника на хвосте DOS.
        # Сам движок находится в коде стенда и его чтения проверяются отдельно по портам.
        transport = (state for state in trace if native or not 0x8000 <= state[0] < 0x8200)
        # В плоском окне кадр маршрута может читаться прямо перед источником.
        guard_start = source if source >= window else source - 8
        reads = source_reads(transport, ram, guard_start, min(hardware, source + len(payload)))
        addresses = [address for _, address in reads]
        assert not source_reads(trace, ram, hardware, 0x10000), (call.hex(), 'чтение порта')
        required = range(source, source + read_count)
        block = mem['BANK_TEXT_BYTES']
        bulk = [hl - source for pc, _, _, _, hl, _ in trace if pc == mem['bankTextCopy32']]
        if terminated:
            assert set(addresses) == set(required), (call.hex(), hex(source), 'чтения Z', reads)
            checked = [hl - source for pc, _, _, _, hl, _ in trace if pc == mem['bankTextChecked']]
            assert not bulk and checked == list(range(0, read_count, block)), (call.hex(), 'маршрут Z', bulk, checked)
        else:
            assert addresses == list(required), (call.hex(), read_count, reads)
        if counted:
            blocks, tail = divmod(read_count, block)
            tail_start = blocks * block
            assert bulk == list(range(0, tail_start, block)), (call.hex(), 'стековый LIM', bulk)
            partial = [hl - source for pc, _, _, _, hl, _ in trace if pc == mem['bankTextCopyPartial']]
            uses_partial = tail >= mem['BANK_TEXT_STACK_MIN']
            assert partial == ([tail_start] if uses_partial else []), (call.hex(), 'неполный стековый LIM', partial)
            expected_stack_reads = list(range(0, tail_start, 2))
            if uses_partial:
                expected_stack_reads += list(range(tail_start + tail % 2, read_count, 2))
            stack_reads = [sp - source for _, op, _, _, _, sp in trace
                           if op in (0xd1, 0xe1) and source <= sp < source + read_count]
            assert stack_reads == expected_stack_reads, (call.hex(), 'чтения со стека LIM', stack_reads)
            short_tail = 0 if uses_partial else tail
            copy_entries = {mem['bankCopyQuads'], mem['bankCopyBytes']}
            copy_reads = [(pc, hl - source) for pc, _, _, _, hl, _ in trace
                          if pc in copy_entries and source <= hl < source + read_count]
            quads = [offset for pc, offset in copy_reads if pc == mem['bankCopyQuads']]
            tail_bytes = [offset for pc, offset in copy_reads if pc == mem['bankCopyBytes']]
            tail_bytes_start = tail_start + short_tail // 4 * 4
            assert quads == list(range(tail_start, tail_bytes_start, 4)), (call.hex(), 'четвёрки LIM', quads)
            assert tail_bytes == list(range(tail_bytes_start, tail_start + short_tail)), (
                call.hex(), 'байтовый остаток LIM', tail_bytes)
        if batch:
            return
        assert (hl, bc, de) == (source + len(expected), expected_bc, 0x1234), (call.hex(), hl, bc, de)
        if expected_af is not None:
            assert af == expected_af, (call.hex(), hex(af))
        else:
            assert not af & 1, (call.hex(), hex(af))
        if api == 'f800':
            assert actual[packet_addr:packet_addr + 8] == struct.pack('<HHHH', hl, de, bc, af), (
                call.hex(), 'возврат MX_REGS')

        entries = {print_z, print_raw_z, print_raw_lim, print_pseudo, print_text_lim,
                   alternate_z, alternate_raw_z, alternate_lim, scalar_hook}
        calls = [(pc, bc, hl, sp) for pc, _, bc, _, hl, sp in trace if pc in entries]
        if expected:
            assert calls, (call.hex(), 'печатный вектор не вызван')
        assert all(pc in printers for pc, _, _, _ in calls), (call.hex(), calls, printers)
        if not native:
            assert all(window <= hl < hardware and 0xdf00 <= sp < 0xe000 for _, _, hl, sp in calls), (
                call.hex(), calls, 'буфер и стек MAIN')
        if scalar:
            assert len(calls) == len(expected), (call.hex(), 'посимвольный LIM', calls)
            return
        fragments = []
        for pc, _, bc, _, _, _ in trace:
            if pc in entries:
                fragments.append([pc, bc, 0])
            elif pc == write_char:
                assert fragments, (call.hex(), 'вывод в обход печатного вектора')
                fragments[-1][2] += 1
        limited = {print_text_lim, print_raw_lim, print_pseudo, alternate_lim}
        counts = []
        for pc, count, printed in fragments:
            if pc in limited:
                assert printed == count, (call.hex(), fragments)
            if printed:
                counts.append(printed)
        expected_counts = ([len(expected)] if expected else []) if native else [
            min(block, len(expected) - i) for i in range(0, len(expected), block)]
        for count in expected_counts:
            if terminated and count == 32 and counts[:2] == [31, 1]:
                del counts[:2]
            else:
                assert counts and counts.pop(0) == count, (call.hex(), fragments, expected_counts)
        assert not counts, (call.hex(), fragments, expected_counts)

    with tempfile.TemporaryDirectory() as temporary:
        folder = Path(temporary)
        lengths = (0, 1, 19, 20, 21, 28, 29, 30, 31, 32, 33, 43, 60, 61, 62, 63, 64, 65,
                   127, 128, 255, 256, 257)
        print_command = b'\x3e\x05\xcd' + word(mem['mx_memory'])

        def print_bios(kind, count):
            return (b'\x01' + word(count) + b'\xcd' + word(mem['mx_callRegs'])
                    + word(mem['MXOS_BIOS_PRINT_STRING_' + kind]))

        def z_case(line, call, printers, *, profile=None, bc=0x5a24, af=None):
            # Ненулевой хвост выявляет печать за NUL и чтение лишней порции.
            payload = line + b'\0' + b'P' * 40
            run_case(folder, payload, call, line, read_count=len(line) + 1,
                     terminated=True, expected_bc=bc, expected_af=af, profile=profile,
                     printers=printers)

        def line_bytes(count):
            return bytes(0x21 + (index * 37) % 94 for index in range(count))

        lim_vector = bios['bios_printStringLim']
        font_char = bios['FONT_ENGINE_CHAR']
        public_char = bios['bios_printChar']
        normal_lim = image.copy()
        for vector_addr in (font_char, public_char):
            put(normal_lim, vector_addr, b'\xc3' + word(scalar_hook))
        old_font_lim = normal_lim.copy()
        put(old_font_lim, bios['FONT_ENGINE_TEXT_EXT'], b'\0\0')
        char_hook_lim = normal_lim.copy()
        put(char_hook_lim, public_char, b'\xcd' + word(scalar_hook) + b'\xc9')
        vector_hook_lim = normal_lim.copy()
        put(vector_hook_lim, lim_vector, b'\xc3' + word(alternate_lim))
        for base, printer in ((normal_lim, print_text_lim), (old_font_lim, scalar_hook),
                              (char_hook_lim, scalar_hook), (vector_hook_lim, alternate_lim)):
            for count in (0, 1, 7, 29, 32, 33, 65):
                line = (b'\0A\x1b(B\x1b)C\r\n' * 7)[:count]
                if count >= 33:
                    line = line[:31] + b'\x1b(' + line[33:]
                for api, native in (('f800', False), ('f806', False), ('f800', True),
                                    ('f806', True), ('main', True)):
                    profile = base.copy()
                    flags = caller_af ^ (count & 1)
                    if native:
                        profile[mem['bankPage']] = 0
                    if api == 'f800':
                        op = mem['MXOS_BIOS_PRINT_STRING_LIM']
                        put(profile, packet_addr, struct.pack('<HHHH', source, 0x1234, count, flags))
                        call = b'\x01' + word(op) + b'\x11' + word(packet_addr) + b'\xcd' + word(mem['mx_callPacket'])
                    elif api == 'f806':
                        call = print_bios('LIM', count)
                    else:
                        call = b'\x01' + word(count) + b'\xcd' + word(lim_vector)
                    run_case(folder, line + b'poison', call, line, read_count=count,
                             expected_bc=count, expected_af=flags, profile=profile,
                             printers=(printer,), counted=not native,
                             api=api, native=native, scalar=printer == scalar_hook)

        for count in lengths:
            line = line_bytes(count)
            z_case(line, print_command, (print_text_lim,))
            z_case(line, print_bios('RAW_Z', count), (print_raw_lim,), bc=count, af=caller_af)
            for suffix, printer in (('RAW_LIM', print_raw_lim), ('RAW_PSEUDO', print_pseudo)):
                raw = bytes(index & 0xff for index in range(count))
                run_case(folder, raw + b'poison', print_bios(suffix, count), raw,
                         read_count=count, expected_bc=count, expected_af=caller_af & ~1,
                         printers=(printer,), counted=True)

        bound_queue = normal_lim.copy()
        counter = word(queue_count)
        # INC (счётчик), затем испортить AF, BC, DE и HL перед RET.
        put(bound_queue, queue_callback, b'\x21' + counter + b'\x34\x01\x9a\xbc\x11\xfa\xdc\x21\x57\x13\xaf\xc9')
        put(bound_queue, mem['MEM_QUEUE_HOOK'], word(queue_callback))
        bound_queue[mem['bankQueueBefore']] = 0xc3
        for suffix, printer in (('Z', print_text_lim), ('RAW_Z', print_raw_lim),
                                ('LIM', print_text_lim),
                                ('RAW_LIM', print_raw_lim), ('RAW_PSEUDO', print_pseudo)):
            line = line_bytes(10)
            terminated = suffix in ('Z', 'RAW_Z')
            run_case(folder, line + b'\0' + b'P' * 40, print_bios(suffix, len(line)), line,
                     read_count=len(line) + terminated, terminated=terminated,
                     expected_bc=len(line), expected_af=caller_af, profile=bound_queue,
                     printers=(printer,), queued=True, counted=not terminated)

        no_exports = image.copy()
        put(no_exports, bios['FONT_ENGINE_TEXT_EXT'], b'\0\0')
        other_vectors = image.copy()
        put(other_vectors, bios['bios_printStringZ'], b'\xc3' + word(alternate_z))
        put(other_vectors, bios['bios_printStringRawZ'], b'\xc3' + word(alternate_raw_z))
        # CALL/RET с тем же операндом, что у штатного JMP, остаётся перехватчиком.
        other_opcodes = image.copy()
        put(other_opcodes, bios['bios_printStringZ'], b'\xcd' + word(print_z) + b'\xc9')
        put(other_opcodes, bios['bios_printStringRawZ'], b'\xcd' + word(print_raw_z) + b'\xc9')
        profiles = ((no_exports, (print_z,), (print_raw_lim,)),
                    (other_vectors, (alternate_z,), (alternate_raw_z,)),
                    (other_opcodes, (print_z,), (print_raw_z,)))
        for profile, z, raw_z in profiles:
            for count in (*range(32), 32, 33, 64, 65):
                line = line_bytes(count)
                z_case(line, print_command, z, profile=profile)
                z_case(line, print_bios('RAW_Z', count), raw_z, profile=profile, bc=count, af=caller_af)
        for count in range(192, 224):
            line = line_bytes(count)
            for kind, printer in (('Z', print_text_lim), ('RAW_Z', print_raw_lim)):
                z_case(line, print_bios(kind, count), (printer,), bc=count, af=caller_af)
        line = line_bytes(193)
        z_case(line, print_command, (print_z,), profile=no_exports)
        z_case(line, print_bios('RAW_Z', len(line)), (alternate_raw_z,), profile=other_vectors,
               bc=len(line), af=caller_af)
        limit = bios['IO_KEYB_A']
        for count in range(32):
            line = line_bytes(count)
            for at, payload in ((source, line + b'\0' + b'P' * 40),
                                (window, line + b'\0' + b'P' * (31 - count)),
                                (limit - count - 1, line + b'\0')):
                for kind, printers in (('Z', (print_text_lim,)), ('RAW_Z', (print_raw_lim,))):
                    run_case(folder, payload, print_bios(kind, count), line,
                             source=at, read_count=count + 1, terminated=True,
                             expected_bc=count, expected_af=caller_af, printers=printers)
        for size in (10, 19, 20, 21, 30, 31, 32, 33):
            tail = size // 5 - 1
            batch = record(ord('A')) + record(ord('B')) * tail
            batch += bytes(size - len(batch))
            run_case(folder, batch + b'poison', b'\x01' + word(size) + b'\x3e\x01\xcd'
                     + word(mem['mx_callRegs']) + word(mem['MXOS_BIOS_COMMAND_BATCH']),
                     b'A!' + b'B' * tail, batch=True, read_count=size)
    print('MEM INLINE NESTED PASS: F806h, очередь, точное чтение Z/LIM, граница FFE0h '
          'и вложенные пачки 10–33 байта')


if __name__ == '__main__':
    main()
