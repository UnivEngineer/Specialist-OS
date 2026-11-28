"""8080 regression: CLI history editing and viewport with 4/6/8px fonts."""
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/cli-font-cursor-cpu'


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    symbols = {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (ROOT / 'bin/CLI.sym').read_text(encoding='utf-8'), re.M)}
    bios = {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (ROOT / 'bin/BIOS.sym').read_text(encoding='utf-8'), re.M)}
    word = lambda x: x.to_bytes(2, 'little')

    def run(font, x, text, keys):
        memory = bytearray(65536)
        for address, name in ((0, 'CLI.i80'), (0xc000, 'BIOS.sys'),
                              (0xf000, 'IO.main.bin'), (0xd800, f'fonts/{font}.i80')):
            data = (ROOT / 'bin' / name).read_bytes()
            memory[address:address + len(data)] = data
        memory[bios['mon2_vars']:bios['mon2_vars'] + bios['MON2_VARIABLES']] = \
            (ROOT/'bin/BIOS.defaults.bin').read_bytes()[:bios['MON2_VARIABLES']]
        memory[bios['bios_upper.escMode']] = 255
        memory[symbols['cliHistoryCount']] = 1
        memory[symbols['cliHistoryNext']] = 1
        at = symbols['cliHistoryEntries']
        memory[at:at + len(text) + 1] = text + b'\0'
        memory[bios['mon2_getchOld']:bios['mon2_getchOld'] + 3] = b'\xc3' + word(0x8500)
        reader = bytes.fromhex('2a 00 86 7e 23 22 00 86 b7 c9')
        memory[0x8500:0x8500 + len(reader)] = reader
        memory[0x8600:0x8602] = word(0x8700)
        memory[0x8700:0x8700 + len(keys)] = bytes(keys)
        code = (b'\x31' + word(0x8b00) + b'\xcd' + word(bios['FONT_ENGINE_INIT']) +
                b'\x21' + bytes([120, x]) + b'\x22' + word(bios['mon2_vars.cursorY']) +
                b'\xcd' + word(symbols['cliInput']) + b'\xc3' + word(0x80ff))
        memory[0x8000:0x8000 + len(code)] = code
        (OUT / 'in.bin').write_bytes(memory)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080', '-pc', '8000', '-end', '80ff', '-counter', '30000000',
                        '-output', str(OUT / 'out.bin'), str(OUT / 'in.bin')],
                       check=True, capture_output=True, timeout=20)
        result = (OUT / 'out.bin').read_bytes()
        assert result[65542:65544] == word(0x80ff), 'editor did not return'
        return result

    for font, width in [('Cpm4', 4), ('Compact', 6), ('Fast8', 8)]:
        step = width // 2
        for x in (0, 16 * step, 192 - 9 * step, 192 - 8 * step, 188):
            origin = x if x < 192 - 8 * step else 0
            y = 120 if origin == x else 130
            for text, keys, expected, position in [
                (b'fast8', [0x19, 13], b'fast8', 5),
                (b'fast8', [0x19, 127, 13], b'fast', 4),
                (b'fast8', [0x19, 127, 8, ord('X'), 13], b'fasXt', 4),
                (b'fast8', [0x19, 12, 13], b'fast8', 0),
                (b'fast8', [0x19, 12, 10, 13], b'fast8', 5),
                (b'X' * 127, [0x19, 127, 13], b'X' * 126, 126),
                (b'X' * 127, [0x19, 12, 13], b'X' * 127, 0),
                (b'fast8', [ord('Z'), 0x19, 0x1a, 13], b'Z', 1),
            ]:
                m = run(font, x, text, keys)
                value = lambda label: m[symbols[label]]
                actual = m[symbols['CMD_LINE']:symbols['CMD_LINE'] + 128].split(b'\0')[0]
                assert actual == expected, (font, x, keys, actual)
                assert value('lineEditPosition') == position
                assert value('lineEditWidth') == (192 - step - origin) // step
                assert m[bios['mon2_vars.cursorY']] == y, (font, x, 'unexpected newline')
                cursor = origin + (position - value('lineEditOffset')) * step
                assert m[bios['mon2_vars.cursorX']] == cursor, (font, x, keys, cursor,
                                                              m[bios['mon2_vars.cursorX']])
                assert cursor < 192 - step
        print(font, 'history/backspace/insertion/Home/End/draft/scroll/right edge PASS')


if __name__ == '__main__':
    main()
