"""Скопировать игру и выполнить DISKBEN на отдельной копии дискеты через NC."""
import argparse
from pathlib import Path
import re
import runpy
import shutil
import struct
import subprocess
from tempfile import TemporaryDirectory

ROOT = Path(__file__).resolve().parents[2]
KEYS = {' ': 'space', ':': 'colon', '.': 'period', '/': 'slash', '-': 'minus'}


def check_copy(path):
    # Независимая проверка файла по каталогу, обеим FAT и цепочке на носителе.
    data = path.read_bytes()
    assert len(data) == 720 * 1024
    sector, spc, reserved, copies, entries = struct.unpack_from('<HBHBH', data, 11)
    fat_bytes = sector * struct.unpack_from('<H', data, 22)[0]
    cluster_bytes = sector * spc
    first = reserved * sector
    fat = data[first:first + fat_bytes]
    assert copies == 2 and fat == data[first + fat_bytes:first + 2 * fat_bytes]
    root = first + copies * fat_bytes
    start = root + ((32 * entries + sector - 1) // sector) * sector
    original = (ROOT/'images/A-Floppy-720k-Programs.img').read_bytes()
    for number in range(2, 2 + (len(data) - start) // cluster_bytes):
        at = first + number + number // 2
        pair = struct.unpack_from('<H', original, at)[0]
        if (pair >> (4 if number & 1 else 0)) & 0xfff:
            at = start + (number - 2) * cluster_bytes
            assert data[at:at + cluster_bytes] == original[at:at + cluster_bytes], number
    for offset in range(root, root + entries * 32, 32):
        if data[offset:offset + 11] == b'FDCOPY  RKS':
            break
    else:
        raise AssertionError('FDCOPY.RKS missing from the root')
    cluster, size = struct.unpack_from('<HI', data, offset + 26)
    result = bytearray()
    seen = set()
    while cluster < 0xff8:
        assert 2 <= cluster < 2 + (len(data) - start) // cluster_bytes and cluster not in seen
        seen.add(cluster)
        at = start + (cluster - 2) * cluster_bytes
        result.extend(data[at:at + cluster_bytes])
        pair = struct.unpack_from('<H', fat, cluster + cluster // 2)[0]
        cluster = (pair >> 4) if cluster & 1 else (pair & 0xfff)
    assert len(seen) == (size + cluster_bytes - 1) // cluster_bytes
    assert result[:size] == (ROOT/'assets/games/families/adskok/repaired/Adskok.rks').read_bytes()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', required=True, type=int, choices=range(19781, 19785))
    parser.add_argument('--caller', choices=('MAIN', 'DOS', 'ALL'), default='MAIN')
    parser.add_argument('--access', choices=('RAW', 'FILE', 'ALL'), default='FILE')
    parser.add_argument('--drive', choices=('B','A'), default='A')
    parser.add_argument('--allocation', choices=(0, 1), type=int, default=0)
    parser.add_argument('--interactive-access', action='store_true')
    args = parser.parse_args()
    out = ROOT/'.local/floppy-check'/f'{args.drive}-{args.caller}-{args.access}-f{args.allocation}'.lower()
    out.mkdir(parents=True, exist_ok=True)
    bench = runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))
    m = bench['Bench'](f'http://127.0.0.1:{args.port}/mcp')
    sy = bench['symbols']
    nc = sy(ROOT/'bin/NC_CORE.sym')
    dos = sy(ROOT/'bin/banked/KERNEL.sym')
    controller = sy(ROOT/'source-apps/diskbench/build/DISKBEN.sym')
    poll = nc['ncBankInputPoll']

    def command(text, screenshot, target, page):
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        m.tap('esc')
        m.call('emu_run_for', ms=250)
        for ch in text.lower():
            if ch == '=':
                m.call('kbd_send', keys=['shift'], action='press')
                try:
                    m.call('emu_run_for', ms=250)
                    m.tap('minus')
                finally:
                    m.call('kbd_send', keys=['shift'], action='release')
                m.call('emu_run_for', ms=250)
            else:
                m.tap(KEYS.get(ch, ch))
        m.stop_at(poll)
        m.until(poll, page=nc_page)
        actual = m.read(nc['CMD_LINE'], len(text))
        assert actual.upper() == text.encode('ascii'), (text, actual)
        m.screenshot(out/screenshot)
        # Дождаться обработки Enter и следующего входа в NC или программу.
        m.call('bp_clear')
        if target != poll:
            m.stop_at(target)
        m.tap('enter')
        if target == poll:
            m.call('emu_run_for', ms=2000)
            m.stop_at(poll)
        m.until(target, 120, page=page)

    with TemporaryDirectory() as folder:
        floppy = Path(folder)/'test.img'
        shutil.copyfile(ROOT/'images/A-Floppy-720k-Programs.img', floppy)
        launch = subprocess.run(['powershell.exe', '-NoProfile', '-File',
            str(ROOT/'scripts/emu80/run-mcp-slot.ps1'), '-Port', str(args.port), '-TestFlashSeed'],
            capture_output=True)
        if launch.returncode:
            raise RuntimeError(launch.stderr.decode(errors='replace') or launch.stdout.decode(errors='replace'))
        pid = int(re.search(rb'\(PID (\d+)\)', launch.stdout)[1])
        mounted = False
        try:
            m.stop_at(poll)
            m.until(poll, 120)
            nc_page = m.read(dos['bankPage'], 1)[0]
            assert nc_page > 1
            # В контроллере физический привод A (0) соответствует A: в MXOS.
            m.call('disk_attach', drive='A', file=str(floppy))
            mounted = True
            command('COPY A:/GAMES/ADSKOK.RKS A:/FDCOPY.RKS', 'copy-command.png', poll, nc_page)
            verify = 2 if args.caller == 'DOS' else 1
            program = f'H:/BENCH/DISKBEN/DISKBEN.COM {args.drive}: -C:{args.caller}'
            if args.interactive_access:
                command('A:', 'drive-command.png', poll, nc_page)
                m.call('bp_clear')
                m.call('emu_run_for', ms=500)
                # F3 меняет состояние второй панели. Если она уже открыта, открыть снова.
                for _ in range(2):
                    m.stop_at(nc['pip_driverType'])
                    m.tap('f3')
                    state = m.call('cpu_state')['registers']
                    if state['pc']['dec'] != nc['pip_driverType']:
                        m.call('emu_run_for', ms=1500)
                        state = m.call('cpu_state')['registers']
                    if state['pc']['dec'] == nc['pip_driverType']:
                        assert state['de']['dec'] == nc['aTypeFloppy'], state
                        break
                else:
                    m.screenshot(out/'nc-f3.png')
                    raise AssertionError('F3 did not display floppy information')
                m.call('bp_clear')
                m.call('emu_run_for', ms=2000)
                m.screenshot(out/'nc-floppy.png')
                command(f'{program} -S=4 -F={args.allocation}', 'bench-command.png',
                        controller['menu_access'], 0)
                for key, target in ((('RAW','FILE','ALL').index(args.access), 'menu_verify'),
                                    (verify, 'menu_writes'), (1, 'exit_wait')):
                    m.stop_at(controller[target])
                    m.line(str(key))
                    m.until(controller[target], 120, page=0)
            else:
                command(f'{program} -M:{args.access} -S=4 -W=1 -V={verify} -F={args.allocation}',
                        'bench-command.png', controller['exit_wait'], 0)
            m.screenshot(out/'diskbench.png')
            if args.drive == 'A' and args.caller in ('MAIN','ALL') and args.access != 'FILE':
                worker = sy(ROOT/'source-apps/diskbench/build/DISKMAIN.sym')
                sectors = struct.unpack('<8H', m.read(worker['sectors'], 16))
                assert len(set(sectors)) == 8 and all(14 <= lba < 1440 for lba in sectors), sectors
                assert all(sectors[i+1] == sectors[i]+1 for i in range(0,8,2)), sectors
                if args.allocation:
                    assert any(sectors[i+1] != sectors[i]+1 for i in (1,3,5)), sectors
                print('DISKBEN A: MAIN physical LBAs', *sectors)

            batch = args.caller == 'ALL'
            for i in range(3 if batch else 1):
                addr = controller['batchStats'] + 88*i if batch else controller['resultsLow']
                stats = m.read(addr, 40)
                for offset, mode in ((0, 'RAW'), (20, 'FILE')):
                    for operation in (offset, offset+10):
                        ticks = struct.unpack_from('<I', stats, operation)[0]
                        assert bool(ticks) == (args.access in (mode, 'ALL')), (
                            args.caller, i, mode, operation, ticks)
            m.call('bp_clear')
            m.call('emu_run_for', ms=250)
            m.tap('space')
            m.stop_at(poll)
            m.until(poll, 120, page=nc_page)
            if args.caller == 'MAIN':
                game = sy(ROOT/'bin/Almaz2-mx-optimized.sym')
                command('A:/GAMES/ALMAZ2MX.EXE', 'game-command.png', game['game_frame'], 0)
                game_file = ROOT/'assets/games/families/almaz-2/builds/Almaz2-mx-optimized.exe'
                assert m.read(0, 32) == game_file.read_bytes()[32:64]
                m.call('bp_clear')
                m.call('emu_run_for', ms=500)
                m.screenshot(out/'game.png')
                m.stop_at(game['audio_game_exit'])
                m.call('kbd_send', keys=['clear'], action='press')
                try:
                    m.until(game['audio_game_exit'], 60, page=0)
                finally:
                    m.call('kbd_send', keys=['clear'], action='release')
                m.stop_at(poll)
                m.until(poll, 120, page=nc_page)
                print('PASS: ALMAZ2MX.EXE loaded from A:, reached game loop and returned to NC.')
            command('A:', 'leave-drive.png', poll, nc_page)
            m.call('disk_detach', drive='A')
            mounted = False
            check_copy(floppy)
            print('PASS: NC COPY, both FAT12 copies, file bytes, DISKBEN', args.caller, args.access, 'write/read/verify and return to NC.')
        finally:
            # Освободить образ до удаления временного каталога на Windows.
            try:
                if mounted:
                    m.call('disk_detach', drive='A')
            finally:
                stop_owned(pid, args.port)


def stop_owned(pid, port):
    subprocess.run(['powershell.exe', '-NoProfile', '-Command',
        f"$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; "
        f"if ($p -and $p.Path -eq (Join-Path $env:LOCALAPPDATA 'Specialist-OS/emu80/{port}/Emu80qt-mcp.exe')) "
        "{ Stop-Process -InputObject $p; $p.WaitForExit() }"], check=True)


if __name__ == '__main__':
    main()
