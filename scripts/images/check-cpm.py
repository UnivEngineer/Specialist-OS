"""Статическая проверка образов и модель загрузчика до входа в CPM.RKS."""
import hashlib
import importlib.util
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('builder', Path(__file__).with_name('build-cpm.py'))
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


def main():
    report = json.loads((ROOT / 'images/CPM-images.json').read_text(encoding='utf-8'))
    for name, entry in report['files'].items():
        data = (ROOT / 'images' / name).read_bytes()
        assert len(data) == entry['bytes'] and hashlib.sha256(data).hexdigest() == entry['sha256']
    physical = (ROOT / 'images/CPM-system.bin').read_bytes()
    rom = physical[32768:]
    ram = bytearray(65536)
    enabled = True
    pc = hl = de = a = b = c = 0
    zero = False

    def read(address):
        return rom[address] if enabled and address < 0x8000 else ram[address]

    for _ in range(300000):
        if not enabled and pc == 0x100:
            original = (ROOT / 'images/CPM-original.rks').read_bytes()
            assert ram[0x100:report['end_address'] + 1] == original[4:-2]
            assert builder.checksum(original[4:-2]) == int.from_bytes(original[-2:], 'little')
            print('OK: unchanged CP/M payload, image hashes, ROM -> upper RAM -> main RAM, no stack')
            return
        op = read(pc)
        pc += 1
        if op in (0x01, 0x11, 0x21, 0x32, 0xC2, 0xC3):
            value = read(pc) | read(pc + 1) << 8
            pc += 2
            if op == 0x01:
                b, c = value >> 8, value & 255
            elif op == 0x11:
                de = value
            elif op == 0x21:
                hl = value
            elif op == 0x32:
                assert value == 0xFFFC and 0x8000 <= pc < 0x8100
                enabled = False
            elif op == 0xC3 or not zero:
                pc = value
        elif op == 0x7E:
            a = read(hl)
        elif op == 0x12:
            assert de < 0xFFC0 and (not enabled or de >= 0x8000)
            ram[de] = a
        elif op == 0x23:
            hl = (hl + 1) & 65535
        elif op == 0x13:
            de = (de + 1) & 65535
        elif op == 0x0B:
            value = ((b << 8 | c) - 1) & 65535
            b, c = value >> 8, value & 255
        elif op == 0x78:
            a = b
        elif op == 0xB1:
            a |= c
            zero = a == 0
        elif op == 0x06:
            b = read(pc)
            pc += 1
        elif op == 0x05:
            b = (b - 1) & 255
            zero = b == 0
        elif op == 0xF3:
            pass
        else:
            raise AssertionError(f'Unexpected opcode {op:02X} at {pc - 1:04X}')
    raise AssertionError('Boot timeout')


if __name__ == '__main__':
    main()
