"""Проверка загрузчика v3 в коде банковой DOS на CPU 8080 с потоком в памяти."""
import argparse
from dataclasses import replace
from pathlib import Path
import re
import struct
import subprocess
import sys
from tempfile import TemporaryDirectory

from dos_state import initialize_dos_state

ROOT = Path(__file__).resolve().parents[2]
sys.path.insert(0, str(ROOT / 'scripts/images'))
from executable import HIGH8, WORD16, Image, checksum, encode, relocated_body

TICKS = Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')
FILE = 0xD000
FIXTURE = 0xE000
STOP = 0xE400
STACK = 0xE700


def word(value):
    return struct.pack('<H', value)


def symbols(path):
    return {name: int(value, 16) for name, value in re.findall(
        r'^([\w.]+): EQU 0x([\da-fA-F]+)', path.read_text('utf-8'), re.M)}


def fixture(directory, kernel):
    names = ('imageOpen', 'imageOpenModule', 'imagePlaceDos', 'imageLoad', 'v_streamOffset')
    imports = '\n'.join(f'{name} = 0{kernel[name]:04X}h' for name in names)
    source = directory / 'read.asm'
    source.write_text(imports + f'''
    ORG 0{FIXTURE:X}h
load:
    ld sp,0{STACK:X}h
    call imageOpenModule
body:
    jp c,0{STOP:X}h
    call imagePlaceDos
    jp c,0{STOP:X}h
    call imageLoad
    jp 0{STOP:X}h
fixedOnly:
    ld sp,0{STACK:X}h
    call imageOpen
    jp body
stream:
    push hl
    ld hl,(v_streamOffset)
    ld de,0{FILE:X}h
    add hl,de
    pop de
copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,copy
    ex de,hl
    push hl
    ld hl,-0{FILE:X}h
    add hl,de
    ld (v_streamOffset),hl
    pop hl
    ld de,2
    xor a
    ret
''', encoding='utf-8')
    subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080',
                    f'--raw={source.with_suffix(".bin")}', f'--sym={source.with_suffix(".sym")}',
                    str(source)], capture_output=True, check=True)
    return source.with_suffix('.bin').read_bytes(), symbols(source.with_suffix('.sym'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--kernel', type=Path, default=ROOT / 'bin/banked/KERNEL.bin')
    args = parser.parse_args()
    s = symbols(args.kernel.with_suffix('.sym'))
    initial = bytearray(65536)
    code = args.kernel.read_bytes()
    initial[0x100:0x100 + len(code)] = code
    initialize_dos_state(initial, s)
    with TemporaryDirectory() as temporary:
        directory = Path(temporary)
        binary, labels = fixture(directory, s)
        initial[FIXTURE:FIXTURE + len(binary)] = binary
        initial[s['fatStream']:s['fatStream'] + 3] = b'\xc3' + word(labels['stream'])
        for name in ('fatStreamFinish', 'fileLoadInfo'):
            initial[s[name]:s[name] + 2] = b'\xaf\xc9'
        data = bytes((i * 13 + 17) & 255 for i in range(513))
        offsets = tuple(range(0, 270, 2)) + (511,)
        image = Image(data, load=0x20, entry=0x21, flags=1, bss=19, reloc_kind=WORD16,
                      align_log2=0, relocations=offsets)

        def run(name, packed, model=image, error=False, fixed=False):
            memory = bytearray(initial)
            memory[FILE:FILE + len(packed)] = packed
            descriptor = s['v_tmpFileDescr']
            memory[descriptor + 26:descriptor + 32] = word(2) + struct.pack('<I', len(packed))
            base = 0 if model.reloc_kind is None else (
                (s['DOS_BANK_MODULE_END'] - model.memory()[2]) & -(1 << model.align_log2))
            first = base + model.load
            arena = s['DOS_ARENA_START']
            end = s['DOS_BANK_MODULE_END']
            memory[arena:end + 1] = b'\x5a' * (end + 1 - arena)
            seed, output = directory / 'seed.bin', directory / 'out.bin'
            seed.write_bytes(memory)
            routine = labels['fixedOnly' if fixed else 'load']
            subprocess.run([str(TICKS), '-m8080', '-pc', f'{routine:x}', '-end', f'{STOP:x}',
                            '-counter', '30000000', '-output', str(output), str(seed)],
                           check=True, capture_output=True, timeout=15)
            actual = output.read_bytes()
            assert actual[65542:65546] == word(STOP) + word(STACK), (name, 'PC/SP')
            assert bool(actual[65536] & 1) == error, (name, actual[65536:65542].hex())
            assert actual[arena:first] == memory[arena:first], (name, 'до тела')
            assert actual[first + len(model.data):end + 1] == memory[first + len(model.data):end + 1], (name, 'после тела')
            if error:
                assert actual[65537] == s['DOS_ERR_BAD_IMAGE'], (name, 'код ошибки')
            else:
                assert actual[65540:65542] == word(base + model.entry), (name, 'вход')
                body = model.data if model.reloc_kind is None else relocated_body(model, base)
                assert actual[first:first + len(model.data)] == body, name

        high = replace(image, reloc_kind=HIGH8, align_log2=9, relocations=(0, 255, 256, 512))
        empty = replace(image, relocations=())
        for name, model in [('WORD16', image), ('HIGH8', high), ('Пустая таблица', empty)]:
            run(name, encode(model, 'dll'), model)
        fixed_image = Image(data, load=0xa120, entry=0xa141, flags=1, bss=17)
        run('Обычное чтение v2', encode(fixed_image, 'com'), fixed_image, fixed=True)
        run('Модуль v2', encode(fixed_image, 'dll'), fixed_image)
        good = encode(image, 'dll')
        run('Обычный вход v2', good, error=True, fixed=True)
        run('Обрезанная таблица', good[:-1], error=True)
        run('Лишний байт', good + b'\0', error=True)
        damaged = bytearray(good)
        damaged[40] ^= 1
        run('Сумма тела', damaged, error=True)
        damaged = bytearray(good)
        damaged[36] ^= 1
        repair_header(damaged)
        run('Сумма таблицы', damaged, error=True)
        for name, offset in [('Дубликат', offsets[-2]), ('Перекрытие', offsets[-2] + 1),
                             ('Неверный порядок', 3), ('Последнее слово вне тела', len(data) - 1)]:
            damaged = bytearray(good)
            damaged[-2:] = word(offset)
            damaged[36:38] = word(checksum(damaged[40 + len(data):]))
            repair_header(damaged)
            run(name, damaged, error=True)
        for field, value in [(32, 2), (33, 16), (38, 1), (16, 1), (29, 1), (27, 32), (22, 3)]:
            damaged = bytearray(good)
            damaged[field] = value
            repair_header(damaged)
            run(f'Байт заголовка {field}', damaged, error=True)
        damaged = bytearray(encode(high, 'dll'))
        damaged[33] = 7
        repair_header(damaged)
        run('HIGH8 без выравнивания страницы', damaged, high, error=True)
        damaged = bytearray(good)
        damaged[34:36] = word(0x8000)
        repair_header(damaged)
        run('Переполнение удвоенного числа записей', damaged, error=True)
        damaged = bytearray(good)
        damaged[30] ^= 1
        run('Сумма заголовка', damaged, error=True)
    print('RELOC STREAM PASS: WORD16/HIGH8, multi-chunk checksums, bounds, damaged files, PC/SP')


def repair_header(packed):
    packed[30:32] = b'\0\0'
    packed[30:32] = word(checksum(packed[:40]))


if __name__ == '__main__':
    main()
