"""Owned visible/muted Emu80: CLI history cursor through real keyboard input."""
from pathlib import Path
import argparse
import re
import runpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/cli-font-cursor-ui'


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT / '.local/nc-sort-benchmark-runtime')
    parser.add_argument('--port', type=int, choices=range(19781, 19797), default=19781)
    args = parser.parse_args()
    OUT.mkdir(parents=True, exist_ok=True)
    helpers = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
    helpers['Matrix'].__init__.__globals__.update(
        RUNTIME=args.runtime.resolve(), OUT=OUT,
        mcp_owner=lambda: helpers['shell'](
            f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; '
            'if ($p) { $p[0].OwningProcess }; exit 0'))
    machine = helpers['Matrix']()
    machine.m.url = f'http://127.0.0.1:{args.port}/mcp'
    symbols = {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
        (ROOT / 'bin/CLI.sym').read_text(encoding='utf-8'), re.M)}
    try:
        machine.start('cli-font-cursor', disk=ROOT / 'images/H-Flash-2M-Games.bin')
        m = machine.m

        def byte(label):
            return m.read(symbols[label], 1)[0]

        def text():
            return m.read(symbols['CMD_LINE'], 128).split(b'\0')[0].upper()

        def type_text(value):
            keys = {' ': 'space', ':': 'colon', '.': 'period', '/': 'slash', '-': 'minus'}
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

        font_symbols = helpers['symbols'](ROOT / 'bin/FONT.sym')
        font_start = (ROOT / 'bin/FONT.i80').read_bytes()[:32]

        def command(value, font_result=None):
            m.tap('esc')
            type_text(value)
            assert text() == value.upper().encode(), (value, text())
            target = symbols['cliInput']
            if font_result is not None:
                target = font_symbols['error' if font_result else 'fontExportReady']
            m.stop_at(target)
            m.tap('enter')
            if font_result is not None:
                for _ in range(120):
                    state = m.until(target, 120, page=0)
                    if m.read(0, 32) == font_start:
                        break
                    m.call('debug_step', mode='into')
                else:
                    raise AssertionError(('FONT.COM', value))
                actual = (state['registers']['af']['dec'] >> 8 if font_result
                          else m.read(font_symbols['preparedError'], 1)[0])
                assert actual == font_result, (value, actual, font_result)
                m.stop_at(symbols['cliInput'])
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
        assert byte('cliHistoryOnDisk') == 1, 'CLI must use the RAM-disk history'
        command('PWD')
        m.tap('up')
        assert text() == b'PWD'
        ring = symbols['cliHistoryEntries']
        assert m.read(ring, 4096) == bytes(4096), 'Disk history copied the ring into MAIN'
        m.tap('esc')
        print('PASS RAM disk holds history and Up reads a single record', flush=True)
        def engine(line, width, kind):
            command(line, font_result=0)
            actual = m.read(symbols['FONT_ENGINE_HEADER'] + 5, 3)
            assert actual == bytes([width, 8, kind]), (line, actual)

        command('CD C:/FONTS/6X8')
        engine('FONT Rotate', 6, 0)
        engine('Fast.fnt', 6, 2)
        m.screenshot(OUT / 'cli-fast-fnt.png')
        engine('FONT 6x8/Packed', 6, 1)
        engine('FONT Fast.fnt', 6, 2)
        engine('FONT ../4x8/Compact', 4, 9)
        engine('FONT /FONTS/6X8/Rotate.fnt', 6, 0)
        command('COPY C:/FONTS/6X8/FAST.FNT B:/LOCAL.FNT')
        command('COPY C:/FONTS/6X8/INIT6.DLL B:/INIT6.DLL')
        engine('FONT B:LOCAL.FNT', 6, 2)
        command('MD C:/FONTS/EXTRA')
        command('COPY C:/FONTS/6X8/FAST.FNT C:/FONTS/EXTRA/SAMPLE.FNT')
        command('COPY C:/FONTS/6X8/INIT6.DLL C:/FONTS/EXTRA/INIT6.DLL')
        command('B:')
        engine('FONT EXTRA/SAMPLE', 6, 2)
        engine('FONT EXTRA/SAMPLE.FNT', 6, 2)
        command('MD B:/EXTRA')
        command('COPY C:/FONTS/6X8/PACKED.FNT B:/EXTRA/SAMPLE.FNT')
        # Если локальный .fnt найден, отсутствие его инициализатора должно вернуть 02.
        command('FONT EXTRA/SAMPLE.FNT', font_result=2)
        command('COPY C:/FONTS/6X8/INIT6.DLL B:/EXTRA/INIT6.DLL')
        engine('FONT EXTRA/SAMPLE.FNT', 6, 1)
        command('MD B:/FONTSET')
        command('CD C:/FONTS/6X8')
        engine('Fast.fnt -IMPORT KOI8.PF -OUT B:/FONTSET/NEW.FNT', 6, 2)
        engine('FONT Rotate', 6, 0)
        command('CD B:/FONTSET')
        engine('NEW.FNT -FILE', 6, 2)
        print('PASS CLI.EXT Fast.fnt, local, geometry, root and drive-relative names', flush=True)
        for font, width in [('Cpm4', 4), ('Rotate', 6), ('Fast8', 8)]:
            engine('FONT ' + font, width, {'Cpm4':5, 'Rotate':0, 'Fast8':8}[font])
            command('CD C:/FONTS/8X8')
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
        command('CD B:/FONTSET')
        engine('NEW.FNT -FILE', 6, 2)
        command('CD C:/DOS')
        bios = helpers['symbols'](ROOT / 'bin/BIOS.sym')
        nc_keyboard = helpers['symbols'](ROOT / 'bin/NC_CORE.sym')['ncBankInputPoll']
        m.stop_at(nc_keyboard)
        m.call('emu_reset')
        for _ in range(120):
            m.until(nc_keyboard, 120)
            if m.read(bios['bankPage'], 1)[0] > 1:
                break
            m.call('debug_step', mode='into')
        else:
            raise AssertionError('Banked NC did not start after Reset')
        m.stop_at(symbols['cliInput'])
        m.tap('f10')
        wait_cli()
        assert m.read(symbols['FONT_ENGINE_HEADER'] + 7, 1) == b'\2'
        assert m.read(symbols['io_vars.fontName'], 8).upper() == b'NEW     '
        print('PASS warm Reset restores NEW.FNT and PRE from the saved B: directory', flush=True)
    finally:
        if machine.pid:
            try:
                machine.m.screenshot(OUT / 'last.png')
            except Exception:
                pass
        machine.stop()


if __name__ == '__main__':
    main()
