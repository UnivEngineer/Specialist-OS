"""DiskBen RAW in BANK with private H: residents, including images over 512 bytes.

The installed OS driver and bank-tail layout are unchanged. Experimental code
occupies B000h..BFFFh in the worker's own bank, between its buffer and C000h DLL.
"""
from pathlib import Path
import argparse
import hashlib
import json
import runpy
import shutil
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'.local/rom-pio-19782')
    parser.add_argument('--port', type=int, default=19782)
    parser.add_argument('--report', type=Path, default=ROOT/'docs/diskbench/pio-write-2026-11-12')
    parser.add_argument('--unroll', nargs='+', type=int, default=[2, 8, 16, 256],
                        choices=[1, 2, 4, 8, 16, 32, 64, 128, 256])
    parser.add_argument('--drivers', nargs='+', choices=['AT2MX', 'AT2MXF'],
                        default=['AT2MXF', 'AT2MX'])
    parser.add_argument('--buffer-paths', nargs='+', default=['aligned'],
                        choices=['aligned', 'unaligned', 'local', 'dos'])
    parser.add_argument('--production-unroll', action='store_true',
                        help='Use production aligned/unaligned write factors (8/8 or 4/4)')
    parser.add_argument('--write-pairs',nargs='+',help='Aligned:unaligned factors, e.g. 2:1 4:4 8:1')
    parser.add_argument('--offset', type=int, default=1, choices=range(1, 256),
                        help='Low address byte of the unaligned user buffer')
    parser.add_argument('--unaligned-modes', nargs='+', type=int, choices=[0, 1, 2, 3],
                        default=[0], help='0=original, 1=unroll, 2=offset + STAX B, 3=universal compact')
    parser.add_argument('--unaligned-unroll', type=int, default=8, choices=[1, 2, 4, 8, 16])
    args = parser.parse_args()
    out = args.report.resolve(); out.mkdir(parents=True, exist_ok=True)
    frozen = out/'input'; frozen.mkdir(exist_ok=True)
    if (out/'results.json').exists():
        raise RuntimeError('Choose a new report directory; results already exist')
    for name in ['BIOS.sys', 'DOS.sys', 'DOS.sym', 'NC.i80', 'NC.sym', 'DISKBEN.i80',
                 'DISKBEN.sym', 'DISKBANK.i80', 'DISKBANK.sym']:
        source = ROOT/'bin'/name
        if not source.exists(): source = ROOT/'source-apps/diskbench/build'/name
        shutil.copyfile(source, frozen/name)
    shutil.copyfile(ROOT/'images/SystemRom_emu80_writable.bin', frozen/'system.bin')
    shutil.copyfile(ROOT/'images/FlashDisk_emu80_seed.bin', frozen/'h.bin')
    for kind in (1, 2):
        shutil.copyfile(ROOT/f'bin/banked/LOCALDRV{kind}.sym', frozen/f'LOCALDRV{kind}.sym')
    shutil.copyfile(ROOT/'bin/banked/TRANSPORT.sym', frozen/'TRANSPORT.sym')
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    C = h['Matrix']; g = C.__init__.__globals__
    g.update(OUT=out, RUNTIME=args.runtime.resolve())
    g['mcp_owner'] = lambda: h['shell'](
        f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} '
        '-ErrorAction SilentlyContinue; if($p){$p[0].OwningProcess};exit 0')
    x = C(); m = x.m; m.url = f'http://127.0.0.1:{args.port}/mcp'
    symbols = h['symbols']; ns = symbols(frozen/'NC.sym')
    cs = h['app_symbols'](frozen/'DISKBEN.sym'); ws = symbols(frozen/'DISKBANK.sym')
    word = lambda n: struct.pack('<H', n)
    assert ws['buffer']+ws['BUFFER_SECTORS']*256 <= 0x8FFF
    assert ws['entry'] == 0xC000
    # Stop after PREPARE has installed the ordinary H: copy, before BEGIN/timer.
    prepared = ws['rawCall']-2
    worker = (frozen/'DISKBANK.i80').read_bytes()
    assert worker[prepared-0xC000:prepared-0xC000+2] == b'\xaf\xc9'
    builds = out/'build'; builds.mkdir(exist_ok=True)
    variants = {}
    def pairs(driver):
        if args.write_pairs:
            result=[tuple(map(int,v.split(':'))) for v in args.write_pairs]
            assert all(len(v)==2 and all(n in (1,2,4,8,16,32,64,128,256) for n in v) for v in result)
            return result
        if args.production_unroll:return [(8,8) if driver=='AT2MX' else (4,4)]
        return [(n,1) for n in args.unroll]
    for driver in args.drivers:
        kind = {'AT2MX': 1, 'AT2MXF': 2}[driver]
        for factor, write_unaligned, unaligned_mode in [(n, w, u) for n,w in pairs(driver) for u in args.unaligned_modes]:
            stem = builds/f'{driver}-x{factor}-w{write_unaligned}-u{unaligned_mode}'
            subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                            f'-DLOCAL_KIND={kind}', '-DPIO_WRITE_BENCH=1',
                            f'-DPIO_WRITE_UNROLL={factor}', f'-DPIO_WRITE_UNALIGNED_UNROLL={write_unaligned}', f'--raw={stem}.bin',
                            f'-DPIO_UNALIGNED_MODE={unaligned_mode}',
                            f'-DPIO_UNALIGNED_UNROLL={args.unaligned_unroll}',
                            f'--sym={stem}.sym', f'--lst={stem}.lst',
                            str(ROOT/'source/mem/LOCALDRV.asm')],
                           check=True, capture_output=True)
            sy = symbols(Path(str(stem)+'.sym'))
            data = Path(str(stem)+'.bin').read_bytes()
            assert sy['Driver'] == 0xB000 and sy['localDriverEnd'] <= 0xC000
            variants[driver, factor, unaligned_mode, write_unaligned] = (sy, data)
    ts = symbols(frozen/'TRANSPORT.sym')
    wrappers = {}
    resume = ws.get('validateRawSectorDone',ws['validateRawByte']+12)
    assert worker[resume-0xC000] == 0x21
    for mode in (0, 1):
        stem = builds/f'buffers-{mode}'
        definitions = dict(BENCH_DOS=mode, WORKER_BUFFER_POINTER=ws['bufferAddress'],
                           WORKER_OPERATION=ws['operation'], WORKER_PASS=ws['pass'],
                           WORKER_MISMATCH=ws['mismatch'], WORKER_VALIDATE_RESUME=resume,
                           TRANSPORT_BANK_CALL=ts['bankCall'])
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        *[f'-D{k}={v}' for k, v in definitions.items()],
                        f'--raw={stem}.bin', f'--sym={stem}.sym', f'--lst={stem}.lst',
                        str(ROOT/'source/flash/bench/buffers.asm')],
                       check=True, capture_output=True)
        wrappers[mode] = (symbols(Path(str(stem)+'.sym')), Path(str(stem)+'.bin').read_bytes())
    rows = []
    def save():
        (out/'results.json').write_text(json.dumps(dict(
            cpuHz=2000000, pitHz=2000000, context='BANK', fileKiB=8, passes=1,
            mode='RAW', verify=False, allocation='sequential', driverAddress=0xB000,
            rows=rows), indent=2)+'\n', encoding='utf-8')
    def trap(*pcs):
        m.call('bp_clear')
        for pc in pcs: m.call('bp_set', addr=pc, type='exec')
    def wait(pc, bank=False):
        for _ in range(180):
            state = m.call('cpu_state'); current = state['registers']['pc']['dec']
            page = m.read(x.pageAddress, 1)[0]
            if current == pc and (page > 1 if bank else page == 0): return page
            if page == 0 and current in (cs['error'], cs['bank_module_error']):
                m.screenshot(out/'failure.png'); raise RuntimeError(state)
            m.call('emu_run_for', ms=1000)
        raise RuntimeError(('timeout', state))
    def command(text):
        m.call('bp_clear'); m.call('emu_run_for', ms=500)
        m.tap('esc'); m.call('emu_run_for', ms=250)
        keys = {' ': 'space', ':': 'colon', '.': 'period', '/': 'slash', '-': 'minus'}
        for char in text: m.tap(keys.get(char, char))
        assert m.read(ns['CMD_LINE'], len(text)).lower() == text.encode()
    current_driver = 'AT2MXF'
    try:
        x.start('pio-write-unroll', system=frozen/'system.bin', disk=frozen/'h.bin',
                nc_binary=frozen/'NC.i80', nc_symbols=frozen/'NC.sym')
        for driver in args.drivers:
            if driver != current_driver:
                command(f'a:/drivers/{driver.lower()}.com')
                trap(ns['mainLoop']); m.tap('enter'); m.until(ns['mainLoop'], 120, page=0)
                current_driver = driver
            kind = {'AT2MX': 1, 'AT2MXF': 2}[driver]
            installed = symbols(frozen/f'LOCALDRV{kind}.sym')
            for factor, write_unaligned, unaligned_mode, buffer_path in [(n, w, u, p) for n,w in pairs(driver)
                    for u in args.unaligned_modes for p in args.buffer_paths]:
                print('RUN', driver, 'WRITE x'+str(factor), 'unaligned write x'+str(write_unaligned), 'unaligned mode', unaligned_mode,
                      buffer_path, flush=True)
                command('a:/bench/diskben/diskben.com h: -c:bank -m:raw -b:aligned')
                trap(cs['menu_verify']); m.tap('enter'); wait(cs['menu_verify'])
                for target, value in [('menu_writes', '0'), ('menu_size', '1'),
                                      ('menu_fragment', '8')]:
                    trap(cs[target]); m.line(value); wait(cs[target])
                trap(prepared, cs['error'], cs['bank_module_error'])
                m.line('0'); page = wait(prepared, bank=True)
                assert m.read(0xC000, 32) == worker[:32]
                sy, original = variants[driver, factor, unaligned_mode, write_unaligned]; data = bytearray(original)
                limit = m.read(installed['SectorLimit']+1, 1)
                mapping = m.read(installed['v_chipMap'], 4)
                assert limit == b'\x20' and mapping == bytes([0, 8, 16, 24])
                data[sy['SectorLimit']+1-0xB000] = limit[0]
                data[sy['v_chipMap']-0xB000:sy['v_chipMap']-0xB000+4] = mapping
                m.write(0xB000, data)
                # All paths use the same full-256-byte fill/validation helpers.
                bs, blob = wrappers[1 if buffer_path == 'dos' else 0]
                m.write(0xA000, blob)
                source = ws['buffer']+(0 if buffer_path == 'aligned' else args.offset)
                m.write(ws['bufferAddress'], word(source))
                m.write(ws['prepareRawBuffer'], b'\xc3'+word(bs['fill']))
                m.write(ws['validateRawByte'], b'\xc3'+word(bs['validate']))
                m.write(bs['copyToDos'], bytes([page]))
                m.write(bs['copyFromDos']+3, bytes([page]))
                for address, value in [(source-1, 0xA5), (source+256, 0x5A),
                                       (0x8FFF, 0xC3), (0x9100, 0x69)]:
                    m.write(address, bytes([value]))
                target = bs['entry'] if buffer_path in ('local', 'dos') else 0xB000
                m.write(ws['rawCall']+1, word(target))
                if buffer_path == 'dos':
                    # No DOS DLL/installer is active: only the BANK worker runs.
                    # The six-byte thunk forces the native DOS H: driver to B600h.
                    m.write(0xFFFD, b'\0')
                    assert m.read(installed['SectorLimit']+1, 1) == limit
                    assert m.read(installed['v_chipMap'], 4) == mapping
                    m.write(cs['DOS_BANK_DRIVER_ADDR'], b'\x21'+word(cs['DOS_VERIFY_BUFFER'])+
                            b'\xc3'+word(cs['LOCAL_DRIVER_ADDR']))
                    m.write(0xFFFD, bytes([page-1]))
                lbas = list(struct.unpack('<32H', m.read(ws['sectors'], 64)))
                assert m.read(ws['config']+1, 1) == b'\x01'
                assert m.read(ws['config']+8, 1) == b'\x01'
                assert m.read(ws['bufferAddress'], 2) == word(source)
                trap(ws['result'], cs['error'], cs['bank_module_error'])
                m.call('emu_run_for', ms=1000); wait(ws['result'], bank=True)
                for address, value in [(source-1, 0xA5), (source+256, 0x5A),
                                       (0x8FFF, 0xC3), (0x9100, 0x69)]:
                    assert m.read(address, 1) == bytes([value]), (buffer_path, hex(address))
                assert m.read(source, 256) == bytes(1 ^ ((source+i) & 255) for i in range(256))
                trap(cs['exit_wait'], cs['error'], cs['bank_module_error'])
                m.call('emu_run_for', ms=1000); wait(cs['exit_wait'])
                stats = m.read(cs['resultsLow'], 40)
                wr, rd = struct.unpack_from('<I', stats, 0)[0], struct.unpack_from('<I', stats, 10)[0]
                assert wr and rd and stats[20:] == bytes(20)
                row = dict(driver=driver, unroll=factor, bufferPath=buffer_path,
                           unalignedMode=unaligned_mode, unalignedUnroll=args.unaligned_unroll,
                           alignedWriteUnroll=(4 if unaligned_mode == 3 else factor),
                           unalignedWriteUnroll=(write_unaligned if unaligned_mode == 0 else
                               4 if unaligned_mode == 3 else args.unaligned_unroll),
                           bufferAddress=source, scratchAddress=(cs['DOS_VERIFY_BUFFER']
                           if buffer_path == 'dos' else 0x9000 if buffer_path == 'local' else None),
                           wrapperBytes=len(blob), wrapperSha256=hashlib.sha256(blob).hexdigest(),
                           residentBytes=len(data),
                           ticksWrite=wr, ticksRead=rd, writeKiBs=62500/wr,
                           readKiBs=62500/rd, flushTicks=struct.unpack_from('<I', stats, 4)[0],
                           flushCount=struct.unpack_from('<H', stats, 8)[0], lbas=lbas,
                           page=page, originalSha256=hashlib.sha256(original).hexdigest(),
                           contentVerified=True, fullSectorBytesVerified=256,
                           guardsVerified=True)
                m.screenshot(out/f'{driver}-x{factor}-w{write_unaligned}-u{unaligned_mode}-{buffer_path}.png')
                trap(cs['finished']); m.call('emu_run_for', ms=500); m.tap('x'); wait(cs['finished'])
                trap(ns['mainLoop']); m.call('debug_step', mode='into')
                m.until(ns['mainLoop'], 120, page=0)
                row['returnNC'] = True; rows.append(row); save()
                print('DONE', driver, factor, write_unaligned, 'u'+str(unaligned_mode), buffer_path, 'READ', row['readKiBs'],
                      'WRITE', row['writeKiBs'], 'KiB/s', flush=True)
    finally:
        x.stop()
    for name, seed in [('system-flash.bin', 'system.bin'), ('flash-disk.bin', 'h.bin')]:
        before = (frozen/seed).read_bytes()
        after = (args.runtime/'specialist-os'/name).read_bytes()
        ss, reserved, fats, sectors = (struct.unpack_from('<H', before, 11)[0],
            struct.unpack_from('<H', before, 14)[0], before[16], struct.unpack_from('<H', before, 22)[0])
        start = reserved*ss; end = start+fats*sectors*ss
        assert before[start:end] == after[start:end], name
    for row in rows: row['fatRestored'] = True
    save()
    print('PASS: RAW-only, data checked, FAT restored, emulator closed', flush=True)


if __name__ == '__main__':
    main()
