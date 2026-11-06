"""CPU 8080: C254h/C25Ah и MemTop C830h/C833h против Monitor-2.

z88dk-ticks моделирует CPU и память, но не аппаратный протокол ВВ55.
Все комбинации значимых флагов, регистры и баланс стека проверяются
через машинную обвязку; эталон и MXOS используют разные адреса ВВ55.
"""
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ticks', type=Path, default=Path(
        'D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    p.add_argument('--image', type=Path, default=ROOT/'bin/BIOS.sys')
    p.add_argument('--report-directory', type=Path,
                   default=ROOT/'docs/compat/loader-abi-001/cpu')
    args = p.parse_args()
    out = args.report_directory.resolve()
    out.mkdir(parents=True, exist_ok=True)
    results = {}

    def run(m):
        inp, target = out/'input.bin', out/'output.bin'
        inp.write_bytes(m)
        subprocess.run([str(args.ticks), '-m8080', '-pc', '8200',
                        '-end', '8300', '-counter', '1000', '-output',
                        str(target), str(inp)], check=True, capture_output=True, timeout=15)
        b = target.read_bytes()
        assert int.from_bytes(b[65542:65544], 'little') == 0x8300
        assert int.from_bytes(b[65544:65546], 'little') == 0x8F00
        return b

    for name, path, port in [('MXOS', args.image, 0xFFE3),
                              ('Monitor2', ROOT/'assets/programs/Monitor2.mon', 0xFF03)]:
        code = path.read_bytes()
        count = 0
        for entry, mode in [(0xC254, 0x82), (0xC25A, 0x91)]:
            for bits in range(32):
                flags = 2 | sum((1 << bit) for i, bit in enumerate((0, 2, 4, 6, 7))
                                if bits & (1 << i))
                m = bytearray(65536)
                m[0xC000:0xC000+len(code)] = code
                m[port] = 0x55
                # LXI SP; POP PSW через HL; контрольные BC/DE/HL; CALL.
                wrapper = bytes.fromhex('31 00 8F 21') + bytes((flags, 0x5A))
                wrapper += bytes.fromhex('E5 F1 01 34 12 11 78 56 21 BC 9A CD')
                wrapper += entry.to_bytes(2, 'little')
                # Сохраняем HL, BC, DE, AF и останавливаемся по PC.
                wrapper += bytes.fromhex('22 00 87 60 69 22 02 87 EB 22 04 87 F5 E1 22 06 87 C3 00 83')
                m[0x8200:0x8200+len(wrapper)] = wrapper
                b = run(m)
                assert b[0x8700:0x8708] == bytes.fromhex('BC 9A 34 12 78 56') + bytes((flags, mode)), (name, entry, flags)
                assert b[port] == mode
                count += 1
        results[name] = dict(cases=count, sha256=hashlib.sha256(code).hexdigest(),
                             registersAndFlags=True, stackBalanced=True, port=f'{port:04X}')
        if name == 'Monitor2':
            state_address = 0x8FAB
        else:
            symbols = dict((k,int(v,16)) for k,v in re.findall(
                r'^(\w+): EQU 0x([0-9A-Fa-f]+)',path.with_suffix('.sym').read_text(),re.M))
            state_address = symbols['v_memTop']
        memtop_cases = 0
        for value in (0, 1, 0x7FFF, 0x8FDE, 0xCFFF, 0xD000, 0xFFFF):
            for flags in (2, 0xD7):
                m = bytearray(65536)
                m[0xC000:0xC000+len(code)] = code
                m[state_address:state_address+2] = b'\x55\xAA'
                # AF/BC/DE/HL до setter; все четыре пары сохраняем в снимок.
                wrapper = bytes.fromhex('31 00 8F 21')+bytes((flags,0x5A))
                wrapper += bytes.fromhex('E5 F1 01 34 12 11 78 56 21')+value.to_bytes(2,'little')
                wrapper += bytes.fromhex('CD 30 C8 E5 C5 D5 F5 E1 22 06 87 D1 C1 E1')
                wrapper += bytes.fromhex('22 00 87 60 69 22 02 87 EB 22 04 87 EB')
                # Снова контрольный HL, затем getter должен вернуть только границу.
                wrapper += bytes.fromhex('21 BC 9A CD 33 C8 22 08 87 F5 E1 22 0A 87 60 69 22 0C 87 EB 22 0E 87 C3 00 83')
                m[0x8200:0x8200+len(wrapper)] = wrapper
                b = run(m)
                expected = value.to_bytes(2,'little')+bytes.fromhex('34 12 78 56')+bytes((flags,0x5A))
                assert b[0x8700:0x8708] == expected,(name,value,flags,'setter')
                assert b[state_address:state_address+2] == value.to_bytes(2,'little')
                assert b[0x8708:0x870C] == value.to_bytes(2,'little')+bytes((flags,0x5A)),(name,value,flags,'getter')
                assert b[0x870C:0x8710] == bytes.fromhex('34 12 78 56'),(name,value,flags,'getter BC/DE')
                memtop_cases += 1
        results[name]['memTopCases'] = memtop_cases
        results[name]['memTopAllValuesAccepted'] = True
    report = dict(status='CPU-only; ВВ55 не моделируется', variants=results)
    (out/'results.json').write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, ensure_ascii=False))


if __name__ == '__main__':
    main()
