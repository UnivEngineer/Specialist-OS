"""Проверка ROM-каталога и исполнения загрузчика до входа F800h (модель 8080)."""
import importlib.util
import json
from pathlib import Path
import struct

ROOT = Path(__file__).resolve().parents[2]
spec = importlib.util.spec_from_file_location('builder', Path(__file__).with_name('build-ramfos.py'))
builder = importlib.util.module_from_spec(spec)
spec.loader.exec_module(builder)


def boot(image):
    ram = bytearray(65536)
    rom = image[0x8000:]
    enabled = True
    pc = a = b = c = de = hl = 0
    zero = False

    def read(address):
        return rom[address] if enabled and address < 0x8000 else ram[address]

    for _ in range(100000):
        if pc == 0xF800:
            assert not enabled
            kernel = (ROOT / 'bin/RAMFOS-ROM-ramfos.bin').read_bytes()
            assert ram[0xC000:0xD300] == kernel[:0x1300]
            assert ram[0xF800:0xFFE0] == kernel[0x3800:]
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
                assert value == 0xFFFC and 0x8000 <= pc < 0x9000
                enabled = False
            elif op == 0xC3 or not zero:
                pc = value
        elif op == 0x7E:
            a = read(hl)
        elif op == 0x12:
            assert 0xC000 <= de < 0xD300 or 0xF800 <= de < 0xFFE0 or 0x8000 <= de < 0x8006
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
        else:
            raise AssertionError(f'Unexpected opcode {op:02X} at {pc - 1:04X}')
    raise AssertionError('Boot timeout')


def main():
    report = json.loads((ROOT / 'images/RAMFOS-games.json').read_text(encoding='utf-8'))
    ids = []
    physical = (ROOT / 'images' / report['system_image']).read_bytes()
    assert len(physical) == 65536
    assert builder.hashlib.sha256(physical).hexdigest() == report['system_sha256']
    boot(physical)
    logical = physical[0x8000:] + physical[4:0x8000] + bytes(4)
    seed = (ROOT / 'images' / report['ram_image']).read_bytes()
    assert len(seed) == 16 * 65536
    assert builder.hashlib.sha256(seed).hexdigest() == report['ram_sha256']
    disks = [(logical, builder.START, [('Monitor2.SYS', None)])]
    for page in range(16):
        data = seed[page * 65536:(page + 1) * 65536]
        # checkARAM пишет номер страницы+1 в FFBBh; каталог этого не касается.
        assert data[0xFFBB:] == bytes(65536 - 0xFFBB)
        disks.append((data, 0, [(item['name'], item) for item in report['files'] if item['page'] == page]))
    for logical, offset, expected in disks:
        for name, item in expected:
            header = logical[offset:offset + 30]
            assert header[:3] == b'\xd3' * 3
            actual = header[3:11].decode('ascii').rstrip() + '.' + header[12:15].decode('ascii').rstrip()
            assert actual == name
            start, end, crc = struct.unpack('<HHH', header[24:30])
            data = logical[offset + 30:offset + 31 + end - start]
            assert builder.checksum(data) == crc
            assert struct.unpack_from('<H', logical, offset + 30 + len(data))[0] == offset
            if item:
                assert item['offset'] == offset and item['load_address'] == start
                assert builder.hashlib.sha256(data).hexdigest() == item['payload_sha256']
                ids.append(item['id'])
            offset += 32 + len(data)
        assert logical[offset:offset + 3] != b'\xd3' * 3
    registry = json.loads((ROOT / 'assets/games/ramfos-candidates.json').read_text(encoding='utf-8'))
    assert sorted(ids) == sorted(item['id'] for item in registry['entries'])
    assert len(set(ids)) == len(ids)
    print(f"OK: 1 ROM, 16 RAM-страниц, {len(ids)} кандидата; заголовки, суммы, обратные ссылки, резерв проверки памяти, переход ROM -> RAMFOS")


if __name__ == '__main__':
    main()
