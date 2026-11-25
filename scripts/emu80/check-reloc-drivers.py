"""Проверка замены службы ВГ93, вложенных BAT и драйвера H: на тестовых образах."""
import argparse
import json
import re
import runpy
import shutil
import struct
import subprocess
import tempfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
EMU = ROOT / 'scripts/emu80'
pack = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))
bench = runpy.run_path(str(ROOT / 'source-apps/diskbench/check-emulator.py'))
path_io = runpy.run_path(str(EMU / 'file-api.py'))['path_io']
W = struct.Struct('<H')


def verify(machine, floppy, out):
    k = runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))['symbols'](ROOT / 'bin/banked/KERNEL.sym')
    core = bench['symbols'](ROOT / 'bin/NC_CORE.sym')
    cli = bench['symbols'](ROOT / 'bin/CLI.sym')
    m = machine
    poll = core['ncBankInputPoll']
    m.stop_at(poll)
    for _ in range(120):
        m.until(poll, 25)
        page = m.read(k['bankPage'], 1)[0]
        if page > 1:
            break
        m.call('debug_step', mode='into')
    else:
        raise AssertionError('NC EXEC')
    m.call('disk_attach', drive='A', file=str(floppy))
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.tap('esc')
    m.call('emu_run_for', ms=250)
    command = 'C:/DRIVERS/FLOPPY.COM'
    for char in command.lower():
        m.tap({':':'colon', '/':'slash', '.':'period'}.get(char, char))
    m.stop_at(poll)
    m.until(poll, page=page)
    assert m.read(core['CMD_LINE'], len(command)).upper() == command.encode()
    m.screenshot(out / 'nc-install.png')
    m.stop_at(k['diCopyFloppy'])
    m.tap('enter')
    m.until(k['diCopyFloppy'], 120, page=1)
    m.stop_at(poll)
    m.until(poll, 120, page=page)
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.stop_at(cli['cliPrompt'])
    m.tap('f10')
    m.until(cli['cliPrompt'], 120, page=0)

    def call(op, hl=0, de=0, bc=0, a=0, error=None):
        m.write(0x2800, bytes.fromhex('CD 00 F8 C3 00 8B'))
        m.write(0x2f80, struct.pack('<4H', hl, de, bc, (a << 8) | 2))
        m.stop_at(0x8b00)
        m.call('regs_set', pc=0x2800, sp=0x8fdf, bc=op, de=0x2f80)
        r = m.until(0x8b00, 120, page=0)['registers']
        af = r['af']['dec']
        assert r['sp']['dec'] == 0x8fdf
        assert bool(af & 1) == (error is not None), (hex(op), af >> 8, error)
        if error is not None:
            assert af >> 8 == error, (hex(op), af >> 8, error)
        return r['hl']['dec']

    def word(name):
        m.write(0xfffd, bytes(1))
        value = W.unpack(m.read(k[name], 2))[0]
        m.write(0xfffc, bytes(1))
        return value

    def file(name, data=None, capacity=512):
        count = len(data) if data is not None else capacity
        m.write(0x3400, name.encode() + bytes(1))
        if data is not None:
            m.write(0x4000, data)
        m.write(0x3200, struct.pack('<3H', 0x3400, 0x4000, count) + bytes(34))
        path_io(m, call, k, 0x3200, data is not None)
        return m.read(0x4000, W.unpack(m.read(0x3206, 2))[0])

    def execute(name, error=None):
        m.write(0x3400, name.encode() + bytes(1))
        call(k['MXOS_DOS_FILE_EXEC'], hl=0x3400, error=error)

    def sector(drive, size, block):
        call(k['MXOS_DOS_FILE_SELECT_DRIVE'], a=drive)
        def raw(mode):
            call(k['MXOS_DOS_RAW_IO'], hl=0x5000, de=block, bc=mode << 8)
        raw(2)
        original = m.read(0x5000, size)
        changed = bytes(x ^ 0xa5 for x in original)
        for data in (changed, original):
            m.write(0x5000, data)
            raw(1)
            raw(4)
            m.write(0x5000, bytes(size))
            raw(2)
            assert m.read(0x5000, size) == data, (drive, block)
        return original

    first = word('diFloppyStart')
    content = sector(0, 512, 1439)
    execute(command)
    second, end = word('diFloppyStart'), word('diFloppyEnd')
    assert first != second and end > second
    assert sector(0, 512, 1439) == content
    size = word('diEnd') - word('diStart')
    # Всё свободное место занимает фиксированная DLL, кроме ровно одного
    # непрерывного интервала для установщика. Живая служба остаётся в реестре.
    low, top = k['DOS_ARENA_START'], k['DOS_BANK_DRIVER_END']
    gaps = [(low, second), (end, top)]
    keep = max(range(2), key=lambda i: gaps[i][1] - gaps[i][0])
    handles = []
    for index, (start, limit) in enumerate(gaps):
        if index == keep:
            limit -= size
        if limit <= start:
            continue
        name = f'B:/FULL{index}.DLL'
        image = pack['Image'](bytes([0xc9]), start, start, 1, bss=limit-start-1)
        file(name, pack['encode'](image, 'dll'))
        m.write(0x3400, name.encode() + bytes(1))
        handles.append(call(k['MXOS_DOS_MODULE_LOAD'], hl=0x3400, a=2))
    execute(command, k['MEM_ERROR_NOMEM'])
    assert word('diFloppyStart') == second and word('diFloppyEnd') == end
    assert sector(0, 512, 1439) == content
    for handle in reversed(handles):
        call(k['MXOS_DOS_MODULE_UNLOAD'], hl=handle)
    execute('C:/DRIVERS/AT2MXF.COM')
    sector(7, 256, 16)
    file('B:/RELOC.TXT', b'copy from nested BAT')
    file('B:/CHILD.BAT', b'COPY B:/RELOC.TXT B:/CHILD.TXT\r\n')
    file('B:/PARENT.BAT', b'@ECHO OFF\r\n' + b'REM fill window\r\n'*40 +
         b'B:/CHILD.BAT\r\nDIR B:/C*.TXT\r\nCOPY B:/RELOC.TXT B:/FINAL.TXT')
    execute('B:/PARENT.BAT')
    for name in ('CHILD', 'FINAL'):
        assert file('B:/'+name+'.TXT') == b'copy from nested BAT'
    # Удержать два обработчика одновременно и проверить конфликт по фактической базе.
    enum_base, copy_base = word('enumOverlay'), word('fileopsOverlay')
    m.write(0x3400, b'B:/C*.TXT'+bytes(1))
    m.write(0x3600, struct.pack('<5H2B', 0, 0x3400, 0x5000, 1, 0, 0, 0))
    call(k['MXOS_DOS_ENUMERATE'], hl=0x3600)
    job = bytearray(k['FILEOPS_JOB'])
    offset = k['FILEOPS_JOB.request']
    job[offset+4:offset+6] = bytes([1, 1])
    job[offset+6:offset+17] = b'RELOC   TXT'
    job[offset+38:offset+49] = b'LOCKCOPYTXT'
    m.write(0x3700, job)
    call(k['MXOS_DOS_FILEOPS'], hl=0x3700)
    for number, base in enumerate((enum_base, copy_base)):
        name = f'B:/BUSY{number}.DLL'
        file(name, pack['encode'](pack['Image'](bytes([0xc9]), base, base, 1),'dll'))
        m.write(0x3400, name.encode()+bytes(1))
        call(k['MXOS_DOS_MODULE_LOAD'], hl=0x3400, a=2, error=k['MEM_ERROR_ACTIVE'])
    call(k['MXOS_DOS_FILEOPS'], hl=0x3700, a=4)
    call(k['MXOS_DOS_ENUMERATE'], hl=0x3600, a=1)
    assert W.unpack(m.read(0x3608, 2))[0] == 1
    call(k['MXOS_DOS_ENUMERATE'], hl=0x3600, a=2)
    # Такты одного опубликованного перехода: один шаг эмулятора в странице DOS.
    m.write(0xfffd, bytes(1))
    ticks = {}
    for facade, target in [('enumInvoke', enum_base), ('fileopsInvoke', copy_base)]:
        m.call('bp_clear')
        m.call('regs_set', pc=k[facade], sp=0xef00)
        before = m.call('cpu_state')['cpu_ticks']
        m.call('debug_step',mode='into')
        after = m.call('cpu_state')
        assert after['registers']['pc']['dec'] == target
        ticks[facade] = after['cpu_ticks'] - before
        assert ticks[facade] == 10, ticks
    report = dict(floppyBases=[first, second], retainedOnError=True, sectorBytes=[512, 256],
                  enumBase=enum_base, fileopsBase=copy_base, pinnedOverlays=2, jumpTicks=ticks)
    (out/'result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf8')
    print('PASS', report)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, required=True, choices=range(19781,19785))
    args = parser.parse_args()
    out = ROOT / 'docs/dos/reloc-tests/drivers'
    out.mkdir(parents=True, exist_ok=True)
    m = bench['Bench'](f'http://127.0.0.1:{args.port}/mcp')
    with tempfile.TemporaryDirectory() as temporary:
        floppy = Path(temporary) / 'drive-a.img'
        shutil.copyfile(ROOT/'images/A-Floppy-720k-Programs.img', floppy)
        launch = subprocess.run(['powershell.exe', '-NoProfile', '-File', str(EMU/'run-mcp-slot.ps1'),
                                 '-Port', str(args.port), '-TestFlashSeed'], capture_output=True)
        if launch.returncode:
            raise RuntimeError(launch.stderr.decode(errors='replace'))
        pid = int(re.search(rb'\(PID (\d+)\)', launch.stdout)[1])
        try:
            verify(m, floppy, out)
        finally:
            subprocess.run(['taskkill', '/PID', str(pid), '/F'], check=True, capture_output=True)
            subprocess.run(['powershell.exe','-NoProfile','-Command',
                            f'Wait-Process -Id {pid} -ErrorAction SilentlyContinue; exit 0'], check=True)


if __name__ == '__main__':
    main()
