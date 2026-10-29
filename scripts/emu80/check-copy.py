"""Проверка создания файла и копирования NC на отдельном Emu80-flash-test."""
import argparse
import hashlib
import json
import re
import runpy
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(Path(__file__).with_name('check-flash.py')))['Machine']


class FileMachine(Machine):
    def subroutine(self, address, **registers):
        self.write(0x8000, bytes((0xCD, address & 255, address >> 8, 0xC3, 0, 0x81)))
        self.call('regs_set', pc=0x8000, sp=0x8F00, **registers)
        for _ in range(120):
            self.call('emu_run_for', ms=500)
            state = self.call('cpu_state')
            if state['registers']['pc']['dec'] == 0x8100:
                return state
        raise RuntimeError(f'Файловая операция не вернулась: {state}')


def get_file(image, name):
    word = lambda pos: struct.unpack_from('<H', image, pos)[0]
    sector_size = word(11)
    fat_start = word(14)
    root_start = fat_start + image[16] * word(22)
    root_size = (word(17) * 32 + sector_size - 1) // sector_size
    root = image[root_start * sector_size:(root_start + root_size) * sector_size]
    entry = next(root[i:i + 32] for i in range(0, len(root), 32) if root[i:i + 11] == name)
    cluster = struct.unpack_from('<H', entry, 26)[0]
    length = struct.unpack_from('<I', entry, 28)[0] + 1  # Исторический формат MXOS: N-1.
    data = bytearray()
    visited = set()
    while len(data) < length:
        assert cluster not in visited and cluster >= 2, 'Повреждённая цепочка'
        visited.add(cluster)
        sector = root_start + root_size + (cluster - 2) * image[13]
        data += image[sector * sector_size:(sector + image[13]) * sector_size]
        cluster = word(fat_start * sector_size + cluster * 2)
    return entry, bytes(data[:length])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--verify-only', action='store_true')
    parser.add_argument('--report', type=Path, default=ROOT / '.codex-tmp/emu80-copy-report.json')
    args = parser.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужна отдельная установка Emu80-flash-test')
    m = FileMachine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    m.call('bp_set', addr=0x8100, type='exec')
    image_path = args.runtime / 'specialist-os/flash-disk.bin'
    expected = bytes(range(256)) * 2
    source = (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
    names = [(b'COPYCHK TXT', expected), (b'NCCOPY  FNT', source)]
    if not args.verify_only:
        # Предыдущие секторные тесты могли оставить другой драйвер и активный H:.
        m.subroutine(0xC842, af=0x0000, de=1)
        m.write(0xDE00, (ROOT / 'bin/FlashChk.com').read_bytes())
        m.subroutine(0xDE00)
        m.subroutine(0xC842, af=0x0700, de=1)
        descriptor = bytearray(32)
        descriptor[:11] = names[0][0]
        struct.pack_into('<H', descriptor, 18, 0x4000)
        struct.pack_into('<I', descriptor, 28, len(expected) - 1)
        m.write(0x7000, descriptor)
        m.write(0x4000, expected)
        m.write(0x8FF9, b'\0')
        state = m.subroutine(0xC845, hl=0x7000)
        assert not state['flags']['C'] and m.read(0x8FF9, 1) == b'\0'
        assert get_file(image_path.read_bytes(), names[0][0])[1] == expected
        # Удаляем только результат предыдущего прогона в тестовой установке.
        m.write(0x7000, names[1][0] + bytes(21))
        m.subroutine(0xC84B, hl=0x7000)

        # Исполняем процедуру, которую вызывает F5 в NC, с подготовленным выбором FONT.FNT.
        listing = (ROOT / 'bin/NC.lst').read_text(encoding='utf-8-sig')
        def label(name):
            return int(re.search(r'^\s*\d+\+?\s+([0-9A-F]{4})\s+(?:[0-9A-F]{2}\s+)*' + name + r'[: ]', listing, re.M)[1], 16)
        copy = label('copyFileInt')
        state_addr = label('state')
        operand = re.search(r'21 ([0-9A-F]{2}) ([0-9A-F]{2})\s+ld\s+hl, vars.input', listing)
        input_addr = int(operand[2] + operand[1], 16)
        buffer_addr = input_addr + 64  # input[32], FILE_DESCRIPTOR[32], затем FILE_LIST_BUFFER.
        system_path = args.runtime / 'specialist-os/system-flash.bin'
        if system_path.exists():
            logical = system_path.read_bytes()  # Линейный образ системной AT29.
        else:
            system_image = (args.runtime / 'specialist-os/system.bin').read_bytes()
            # Старый 64-КБ MX2-образ имеет отдельный четырёхбайтный вход STD.
            logical = system_image[32768:] + system_image[4:32768]
        entry, payload = get_file(logical, b'FONT    FNT')
        assert payload == source
        m.write(state_addr + 1, b'\0')
        m.write(state_addr + 4, b'\0')
        m.write(state_addr + 8, b'\0')
        m.write(state_addr + 10, b'\0')
        m.write(buffer_addr, entry[:12] + entry[18:20] + entry[28:30])
        # Источник FONT.FNT отличается от COPYCHK.TXT по размеру и содержимому.
        # Отказ должен происходить до любой записи, включая FAT и каталог.
        for destination in [b'H:COPYCHK.TXT\0', b'H:copychk.txt\0', b'A:FONT.FNT\0']:
            before = image_path.read_bytes()
            system_path = args.runtime / 'specialist-os/system-flash.bin'
            if not system_path.exists():
                system_path = args.runtime / 'specialist-os/system.bin'
            before_system = system_path.read_bytes()
            m.write(input_addr, destination)
            m.call('bp_clear')
            error = label('copyFileShowError')
            m.call('bp_set', addr=error, type='exec')
            m.write(0x8000, bytes([0xCD, copy & 255, copy >> 8, 0xC3, 0, 0x81]))
            m.call('regs_set', pc=0x8000, sp=0x8F00)
            for _ in range(120):
                m.call('emu_run_for', ms=500)
                cpu = m.call('cpu_state')['registers']
                if cpu['pc']['dec'] == error:
                    assert m.read(cpu['hl']['dec'], 19) == b'File already exists'
                    break
            else:
                raise AssertionError('NC не сообщил о существующем файле')
            assert image_path.read_bytes() == before
            assert system_path.read_bytes() == before_system
            m.call('bp_clear')
            m.call('bp_set', addr=0x8100, type='exec')
        m.write(input_addr, b'H:NCCOPY.FNT\0')
        m.subroutine(copy)
        assert m.read(0x8FF9, 1) == b'\0'
    results = []
    m.subroutine(0xC842, af=0x0000, de=1)
    m.subroutine(0xC842, af=0x0700, de=1)  # Сбрасываем кэш и читаем результат через DOS.
    for name, payload in names:
        entry, actual = get_file(image_path.read_bytes(), name)
        assert actual == payload
        m.write(0x7000, entry)
        result = m.subroutine(0xC866, hl=0x7000, de=0x4000)
        assert not result['flags']['C']
        assert m.read(0x4000, len(payload)) == payload
        results.append({'name': name.decode(), 'size': len(payload), 'sha256': hashlib.sha256(payload).hexdigest()})
    report = json.loads(args.report.read_text()) if args.verify_only and args.report.exists() else {}
    report.update({'files': results, 'ncCopyProcedureTested': True, 'verifiedAfterRestart': args.verify_only})
    if not args.verify_only:
        report['existingCopiesRejectedWithoutDiskChanges'] = ['same name', 'different case', 'source itself']
    args.report.write_text(json.dumps(report, indent=2) + '\n')
    print('NC copy: existing names rejected without changes; new file and DOS readback OK')


if __name__ == '__main__':
    main()
