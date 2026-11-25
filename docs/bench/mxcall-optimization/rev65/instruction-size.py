"""Количество байт мнемоник 8080 в установочных сегментах MEM двух снимков."""
from pathlib import Path
import re

ROOT = Path(__file__).resolve().parent
MNEMONICS = set("adc add and call ccf cp cpl daa dec di ei ex halt in inc jp ld nop or out pop push ret rla rlca rra rrca rst sbc scf sub xor".split())
LINE = re.compile(r'^\s*\d+[+ ]*([0-9A-F]{4}) ([0-9A-F]{2}(?: [0-9A-F]{2}){0,2})\s{2,}(?:[A-Za-z_$.][\w$.]*:\s*)?([\w.]+)\b')

for revision in ('rev64-bin', 'final2-bin'):
    folder = ROOT / revision / 'bin/banked'
    counts = {}
    for name, start in [('LOCALBLOCK', 0xf1c0), ('TRANSPORT', 0xfc00), ('WINDOW', 0xffc0)]:
        image = (folder / (name + '.bin')).read_bytes()
        if name == 'LOCALBLOCK':
            image += (folder / 'CLIENT.bin').read_bytes()
        covered = set()
        for line in (folder / (name + '.lst')).read_text(encoding='utf-8').splitlines():
            item = LINE.match(line)
            if not item or item[3].lower() not in MNEMONICS:
                continue
            address = int(item[1], 16)
            if start <= address < start + len(image):
                code = bytes.fromhex(item[2])
                assert image[address-start:address-start+len(code)] == code, line
                covered.update(range(address, address + len(code)))
        counts[name] = len(covered)
    print(revision, counts, 'total', sum(counts.values()))
