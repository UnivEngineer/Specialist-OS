"""8080 regression: changing panel capacity keeps the cursor and fills the tail."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/nc-viewport-check'
W = lambda value: int(value).to_bytes(2, 'little')


def main():
    n = {key: int(value, 16) for key, value in re.findall(
        r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',
        (ROOT / 'bin/NC.sym').read_text('utf-8-sig'), re.M)}
    memory = bytearray(65536)
    binary = (ROOT / 'bin/NC.i80').read_bytes()
    memory[:len(binary)] = binary
    # Run the real viewport calculation, stopping before index-list lookup.
    memory[n['lf_ViewportReady']] = 0xc9
    code = bytearray(b'\x31' + W(0xff00))
    cases = []

    def store(address, value):
        code.extend(b'\x3e' + bytes([value]) + b'\x32' + W(address))

    for panel in (0, 1):
        for count in (0, 12, 32, 64, 128, 250, 255):
            for old_visible in (20, 40, 60):
                for visible in (20, 40, 60):
                    for cursor in sorted({0, count // 2, max(0, count - 1)}):
                        first = max(0, count - old_visible)
                        store(n['state.activePanel'], panel)
                        store(n['state.panelA_filesCnt'] + panel, count)
                        store(n['state.panelA_firstFile'] + panel, first)
                        store(n['state.panelA_curFile'] + panel, cursor)
                        store(n['ncVisibleCount'], visible)
                        code.extend(b'\x3e' + bytes([cursor]) + b'\xcd' + W(n['lf_CursorReady']))
                        code.extend(b'\x3a' + W(n['state.panelA_firstFile'] + panel))
                        code.extend(b'\x32' + W(0xb000 + len(cases)))
                        expected = min(first, cursor)
                        expected = max(expected, cursor - visible + 1)
                        expected = min(expected, max(0, count - visible))
                        cases.append((panel, count, old_visible, visible, cursor, expected))
    # Stream cases in chunks: the large matrix must not overwrite NC or stack.
    # Each case has a fixed instruction length after the initial LD SP.
    stride = (len(code) - 3) // len(cases)
    OUT.mkdir(parents=True, exist_ok=True)
    for start in range(0, len(cases), 80):
        batch = code[3 + start * stride:3 + min(start + 80, len(cases)) * stride]
        program = b'\x31' + W(0xff00) + batch + b'\xc3' + W(0xef00)
        memory[0xc000:0xc000 + len(program)] = program
        (OUT / 'input.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', 'c000', '-end', 'ef00', '-counter', '30000000',
                        '-output', str(OUT / 'output.bin'), str(OUT / 'input.bin')],
                       check=True, capture_output=True, timeout=20)
        result = (OUT / 'output.bin').read_bytes()
        assert result[65542:65544] == W(0xef00), 'CPU did not finish'
        for index in range(start, min(start + 80, len(cases))):
            assert result[0xb000 + index] == cases[index][-1], cases[index]
    print(f'NC viewport PASS: {len(cases)} cases, both panels, all font-width transitions')


if __name__ == '__main__':
    main()
