"""Потоковые BAT: длинный файл, вложенность, границы и ошибки; свой Emu80."""
import argparse
import json
import runpy
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, default=ROOT/'.local/bat-check')
    args = p.parse_args()
    out = args.report.resolve()
    out.mkdir(parents=True, exist_ok=True)
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    h['Matrix'].__init__.__globals__.update(OUT=out, RUNTIME=ROOT/'.local/monitor2-001')
    x = h['Matrix']()
    m = x.m
    s = h['symbols'](ROOT/'bin/banked/KERNEL.sym')
    exe = runpy.run_path(str(ROOT/'scripts/images/executable.py'))
    rows = []

    def dos_read(addr, count):
        m.write(0xFFFD, b'\0')
        result = m.read(addr, count)
        m.write(0xFFFC, b'\0')
        return result

    def create(name, data):
        x.select(1)
        stem, ext = name.split('.')
        obj = bytearray(64)
        obj[:11] = (stem.ljust(8)+ext.ljust(3)).encode('ascii')
        obj[40:42] = bytes((1, 15))
        m.write(0x100, obj)
        assert not x.func(0xC8D5, af=0, hl=0x100)['flags']['C']
        for start in range(0, len(data), 8192):
            chunk = data[start:start+8192]
            m.write(0x2000, chunk)
            m.write(0x100+49, struct.pack('<HH', 0x2000, len(chunk)))
            assert not x.func(0xC8D5, af=0x0200, hl=0x100)['flags']['C']
            assert m.read(0x100+51, 2) == struct.pack('<H', len(chunk))
        assert not x.func(0xC8D5, af=0x0500, hl=0x100)['flags']['C']

    def execute(name, expected=0, count=0):
        m.write(0x8800, b'\0')
        m.write(0x8500, ('B:\\'+name+'\0').encode('ascii'))
        state = x.func(0xC85D, hl=0x8500)
        got = state['registers']['af']['dec'] >> 8
        assert state['flags']['C'] == bool(expected), (name, state)
        if expected:
            assert got == expected, (name, got)
        assert m.read(0x8800, 1) == bytes((count,)), (name, m.read(0x8800, 1))
        assert dos_read(s['v_batDepth'], 1) == b'\0'
        assert dos_read(s['v_batFrame'], 2) == struct.pack('<H', s['BAT_BUFFER']-s['BAT_FRAME_SIZE'])
        assert state['registers']['sp']['dec'] == 0x8FDF
        assert m.read(s['bankPage'], 1) == b'\0'
        for i in range(s['BAT_MAX_DEPTH']):
            assert dos_read(s['BAT_BUFFER']+i*s['BAT_FRAME_SIZE']+42, 1) != b'F'
        rows.append(dict(file=name, error=expected, executed=count, bankAndStackPreserved=True))
        print('PASS', name, flush=True)

    try:
        x.start('bat-stream')
        x.console()
        create('MARK.COM', exe['encode'](exe['Image'](bytes.fromhex('21008834C9'), 0, 0, 4), 'com'))
        create('CHILD.BAT', b'B:\\MARK.COM\nB:\\MARK.COM\r')
        create('PARENT.BAT', b'@echo off\r\nB:\\MARK.COM\r\nB:\\CHILD.BAT\r\nB:\\MARK.COM')
        execute('PARENT.BAT', count=4)
        for command in (b'MD B:/SUB\0', b'CD B:/SUB\0'):
            m.write(0x8500, command)
            assert not x.func(0xC85D, hl=0x8500)['flags']['C']
        create('LOCAL.BAT', b'B:\\MARK.COM\n')
        m.write(0x8500, b'CD B:/\0')
        assert not x.func(0xC85D, hl=0x8500)['flags']['C']
        execute('SUB/LOCAL.BAT', count=1)
        large = b'@echo off\n'+(b'REM '+b'x'*91+b'\r\n')*750+b'B:\\MARK.COM'
        assert len(large) > 65536
        create('LARGE.BAT', large)
        execute('LARGE.BAT', count=1)
        assert dos_read(s['BAT_BUFFER']+s['BAT_OFFSET'], 4) == struct.pack('<I', len(large)-len(b'B:\\MARK.COM'))
        create('EMPTY.BAT', b'')
        execute('EMPTY.BAT')
        create('EDGE.BAT', b'\n'*511+b'\r\nB:\\MARK.COM')
        execute('EDGE.BAT', count=1)
        create('MAXLINE.BAT', b'REM '+b'x'*123+b'\nB:\\MARK.COM')
        execute('MAXLINE.BAT', count=1)
        create('LONG.BAT', b'REM '+b'x'*124+b'\nB:\\MARK.COM')
        execute('LONG.BAT', expected=7)
        prefix = b'@echo off\r\nREM ok\r\n'
        create('ERROR.BAT', prefix+b'NoSuchCommand\r\nB:\\MARK.COM')
        execute('ERROR.BAT', expected=2)
        assert dos_read(s['BAT_BUFFER']+s['BAT_LINE'], 2) == struct.pack('<H', 3)
        assert dos_read(s['BAT_BUFFER']+s['BAT_OFFSET'], 4) == struct.pack('<I', len(prefix))
        m.screenshot(out/'line-error.png')
        create('NUL.BAT', b'REM x\0\nB:\\MARK.COM')
        execute('NUL.BAT', expected=7)
        create('KOI.BAT', b'REM \xFF\nB:\\MARK.COM')
        execute('KOI.BAT', count=1)
        create('SELF.BAT', b'B:\\SELF.BAT\n')
        execute('SELF.BAT', expected=7)
        execute('PARENT.BAT', count=4)
        # Последний запуск — обычным редактором NC, без прямого FILE_EXEC.
        nc = h['symbols'](ROOT/'bin/NC.sym')
        m.write(0x8500, b'A:\\NC.COM\0')
        m.write(0x8E00, bytes.fromhex('CD5DC8C3108E'))
        m.stop_at(nc['mainLoop'])
        m.call('regs_set', pc=0x8E00, sp=0x8FDF, hl=0x8500)
        m.until(nc['mainLoop'], 120, page=0)
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        m.tap('esc')
        for ch in 'B:/PARENT.BAT':
            m.tap({':':'colon', '/':'slash', '.':'period'}.get(ch, ch.lower()))
        assert m.read(nc['CMD_LINE'], 13).upper() == b'B:/PARENT.BAT'
        m.screenshot(out/'nc-command.png')
        m.write(0x8800, b'\0')
        m.stop_at(nc['mainLoop'])
        m.tap('enter')
        m.until(nc['mainLoop'], 120, page=0)
        assert m.read(0x8800, 1) == b'\4'
        rows.append(dict(case='NC-keyboard-command', executed=4))
        (out/'result.json').write_text(json.dumps(rows, indent=2)+'\n', encoding='utf-8')
    finally:
        x.stop()


if __name__ == '__main__':
    main()
