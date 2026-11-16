"""Owned visible/muted Emu80: CLI history cursor through real keyboard input."""
from pathlib import Path
import re
import runpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/cli-font-cursor-ui'


def main():
    OUT.mkdir(parents=True, exist_ok=True)
    helpers = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
    helpers['Matrix'].__init__.__globals__.update(
        RUNTIME=ROOT / '.local/nc-sort-benchmark-runtime', OUT=OUT)
    machine = helpers['Matrix']()
    symbols = {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (ROOT / 'bin/CLI.sym').read_text(encoding='utf-8'), re.M)}
    try:
        machine.start('cli-font-cursor', disk=ROOT / 'images/FlashDisk_emu80_seed.bin')
        m = machine.m

        def byte(label):
            return m.read(symbols[label], 1)[0]

        def text():
            return m.read(symbols['CMD_LINE'], 128).split(b'\0')[0].upper()

        def type_text(value):
            keys = {' ': 'space', ':': 'colon', '.': 'period', '/': 'slash'}
            for char in value:
                m.tap(keys.get(char, char.lower()))

        def wait_cli():
            expected = (ROOT / 'bin/CLI.i80').read_bytes()[:32]
            for _ in range(120):
                m.until(symbols['cliInput'], 120, page=0)
                if m.read(0, 32) == expected:
                    break
                m.call('debug_step', mode='into')
            else:
                raise AssertionError('CLI did not return to input')
            m.call('bp_clear')
            m.call('emu_run_for', ms=500)

        def command(value):
            m.tap('esc')
            type_text(value)
            assert text() == value.upper().encode(), (value, text())
            m.stop_at(symbols['cliInput'])
            m.tap('enter')
            wait_cli()

        def check_cursor(width, position, expected):
            assert text() == expected, (text(), expected)
            assert byte('lineEditPosition') == position
            origin = m.read(symbols['lineEditOrigin'] + 1, 1)[0]
            x = origin + (position - byte('lineEditOffset')) * width // 2
            assert byte('bios_vars.cursorX') == x, (width, x, byte('bios_vars.cursorX'))

        m.stop_at(symbols['cliInput'])
        m.tap('f10')
        wait_cli()
        for font, width in [('Cpm4', 4), ('Compact', 6), ('Fast8', 8)]:
            command('A:/FONT.COM ' + font)
            assert m.read(symbols['FONT_ENGINE_HEADER'] + 5, 1)[0] == width
            command('CD A:/FONTS/8X8')
            command('fast8')
            m.tap('up')
            check_cursor(width, 5, b'FAST8')
            m.tap('backspace')
            check_cursor(width, 4, b'FAST')
            m.screenshot(OUT / (font + '-history-backspace.png'))
            m.tap('left')
            m.tap('x')
            check_cursor(width, 4, b'FASXT')
            m.tap('home')
            check_cursor(width, 0, b'FASXT')
            m.tap('lf')  # Specialist LF sends 0Ah (editor End); EK_END is not wired.
            check_cursor(width, 5, b'FASXT')
            m.tap('esc')
            long_text = 'echo ' + 'abcdefghi' * 9
            type_text(long_text)
            check_cursor(width, len(long_text), long_text.upper().encode())
            m.tap('home')
            check_cursor(width, 0, long_text.upper().encode())
            m.tap('lf')
            m.tap('backspace')
            check_cursor(width, len(long_text) - 1, long_text[:-1].upper().encode())
            m.screenshot(OUT / (font + '-long-line.png'))
            m.tap('esc')
            print(font, 'CLI keyboard history/backspace/insertion/Home/End/scroll PASS', flush=True)
    finally:
        try:
            machine.m.screenshot(OUT / 'last.png')
        except Exception:
            pass
        machine.stop()


if __name__ == '__main__':
    main()
