"""Измерить исходный и текущий перенос DOS на CPU Intel 8080."""
import argparse
import json
import re
import struct
import subprocess
import sys
import tempfile
from pathlib import Path


HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[3]
sys.path.insert(0, str(ROOT / 'scripts/tests'))
from dos_state import initialize_dos_state, install_dos_mem


def word(value):
    return struct.pack('<H', value)


def load_bundle(bundle):
    symbols = {
        name: int(value, 16)
        for name, value in re.findall(
            r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
            (bundle / 'banked/KERNEL.sym').read_text(encoding='utf-8'),
            re.M,
        )
    }
    binary = (bundle / 'DOS.sys').read_bytes()
    memory = bytearray(65536)
    memory[0x100:0x100 + len(binary)] = binary
    initialize_dos_state(memory, symbols, bundle)
    install_dos_mem(memory, bundle)
    memory[symbols['bankPage']] = symbols['DOS_IO_PAGE']
    memory[symbols['bankPageCount']] = 16
    memory[0xDE00] = 0xC9
    return memory, symbols


def run(bundle, page, operation, payload, ticks, directory):
    original, symbols = bundle
    memory = bytearray(original)
    pointer = 0xA333
    descriptor = operation in ('kdFetch', 'kdStore')
    destination = 0xA500 if descriptor else symbols['KD_PATH']
    memory[symbols['kdPage']] = page
    memory[pointer:pointer + len(payload)] = payload
    memory[destination:destination + 128] = b'\xA5' * 128
    if operation == 'return':
        entry = 0xDE00
    elif operation == 'old-open-path':
        # LD DE,KD_PATH; LD A,MAX_PATH; затем исходный цикл OPEN_PATH.
        entry = symbols['kdHandlePathScan'] - 5
        assert memory[entry:entry + 5] == b'\x11' + word(destination) + b'\x3E\x80'
        memory[symbols['kdHandlePathReady']] = 0xC9
    elif operation == 'old-full-path':
        # Пропускаем чтение и сохранение исходного HL из пакета регистров.
        entry = symbols['kdPath'] + 6
        assert memory[entry:entry + 6] == b'\x11' + word(destination) + b'\x01\x80\x00'
        memory[symbols['kdPathReady']] = 0xC9
    else:
        entry = symbols[operation]
    stub = (
        bytes.fromhex('31 00 ED 21') + word(pointer) +
        b'\x11' + word(destination) + b'\x01' + word(len(payload)) +
        b'\xCD' + word(entry) + bytes.fromhex('F5 E1 22 00 E1 60 69 22 02 E1 C3 00 E0')
    )
    memory[0xDF00:0xDF00 + len(stub)] = stub
    source = directory / 'in.bin'
    result = directory / 'out.bin'
    source.write_bytes(memory)
    elapsed = subprocess.check_output([
        str(ticks), '-m8080', '-pc', 'df00', '-end', 'e000', '-counter', '1000000',
        '-output', str(result), str(source),
    ])
    final = result.read_bytes()
    assert final[65542:65544] == word(0xE000), operation
    if operation != 'return':
        assert not final[0xE100] & 1, (page, operation, final[0xE100:0xE102])
        assert final[destination:destination + len(payload)] == payload, (page, operation)
    if operation != 'old-full-path':
        rest = 128 - len(payload)
        assert final[destination + len(payload):destination + 128] == b'\xA5' * rest, operation
    if descriptor:
        assert final[0xE102:0xE104] == word(len(payload)), (page, operation, 'BC')
    return int(elapsed.strip())


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--before', type=Path, default=HERE.parent / 'before/bin')
    parser.add_argument('--after', type=Path, default=ROOT / 'bin')
    parser.add_argument('--ticks', type=Path, default=Path(
        'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    parser.add_argument('--output', type=Path, default=HERE / 'timings.json')
    args = parser.parse_args()
    before = load_bundle(args.before)
    after = load_bundle(args.after)
    payloads = [
        b'\0', b'B:/\0', b'B:/DATA.BIN\0', b'B:/TREE/SUB/DEEP.BIN\0',
        *(b'x' * (size - 1) + b'\0' for size in (32, 33, 64, 128)),
    ]
    rows = []
    with tempfile.TemporaryDirectory(prefix='mxos-dos-chunks-') as temporary:
        directory = Path(temporary)
        overhead = run(after, 0, 'return', b'', args.ticks, directory) - 10

        def measure(bundle, page, operation, payload):
            return run(bundle, page, operation, payload, args.ticks, directory) - overhead

        print(f'Общая заглушка исключена: {overhead} T')
        for label, page in (('MAIN', 0), ('EXEC', 2)):
            for payload in payloads:
                row = dict(
                    profile=label,
                    operation='path',
                    size=len(payload),
                    open_path_before=measure(before, page, 'old-open-path', payload),
                    full_path_before=measure(before, page, 'old-full-path', payload),
                    after=measure(after, page, 'kdFetchPath', payload),
                )
                rows.append(row)
                print(*row.values())
            for size in (11, 16, 32, 33, 48, 63, 64):
                payload = bytes(range(size))
                for operation in ('kdFetch', 'kdStore'):
                    row = dict(
                        profile=label,
                        operation=operation,
                        size=size,
                        before=measure(before, page, operation, payload),
                        after=measure(after, page, operation, payload),
                    )
                    rows.append(row)
                    print(*row.values())
    args.output.write_text(json.dumps(rows, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
