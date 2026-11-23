"""Emu80 no-build preparation: refresh C:, enable SDP, preserve edits and H:."""
from pathlib import Path
import argparse
import hashlib
import json
import os
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path,
                        default=Path(os.environ.get('SPECIALIST_EMU80_DIR', '')))
    args = parser.parse_args()
    seed = ROOT / 'images/C-Flash-512k-MMX-System-Emu80.bin'
    disk_seed = ROOT / 'images/H-Flash-2M-Games.bin'
    floppy_seed = ROOT / 'images/A-Floppy-720k-Programs.img'
    watched = [seed, disk_seed, floppy_seed, ROOT / 'bin/NC.i80']
    before = [(p.stat().st_mtime_ns, sha(p)) for p in watched]
    local = ROOT / '.local'
    local.mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='emu80-refresh-', dir=local) as name:
        runtime = Path(name).resolve()
        assert runtime.is_relative_to(local.resolve())
        for file in ('Emu80qt.exe', 'emu80.conf', 'specialist-build.json'):
            shutil.copyfile(args.runtime / file, runtime / file)
        deployment = runtime / 'specialist-os'
        system = deployment / 'system-flash.bin'
        disk = deployment / 'flash-disk.bin'
        floppy = deployment / 'floppy720.img'
        system_sdp = system.with_suffix('.bin.sdp')
        disk_sdp = disk.with_suffix('.bin.sdp')
        record = deployment / 'deployment.json'

        def prepare(*flags):
            # This agent shell can inherit a PSModulePath from PowerShell 7;
            # load the Windows PowerShell utility module explicitly, as Matrix does.
            quote = lambda value: "'" + str(value).replace("'", "''") + "'"
            script = ("Import-Module 'C:/Windows/System32/WindowsPowerShell/v1.0/Modules/"
                      "Microsoft.PowerShell.Utility/Microsoft.PowerShell.Utility.psd1'; & " +
                      quote(ROOT / 'scripts/emu80/run.ps1') + ' -EmulatorDirectory ' +
                      quote(runtime) + ' -PrepareOnly -Flash ' + ' '.join(flags))
            result = subprocess.run([
                'powershell.exe', '-NoProfile', '-Command', script],
                capture_output=True, text=True, timeout=30)
            assert result.returncode == 0, (result.stdout, result.stderr)
            return result.stdout

        def edit(path):
            data = bytearray(path.read_bytes())
            data[-1] ^= 0x55
            path.write_bytes(data)

        def backups():
            return set((deployment / 'flash-backups').glob('*'))

        prepare('-UpdateSystemFlash')
        assert sha(system) == sha(seed) and sha(disk) == sha(disk_seed)
        assert system_sdp.read_bytes() == b'\x01' and not disk_sdp.exists()
        assert json.loads(record.read_text())['systemSeedSha256'] == sha(seed)
        edit(system)
        edit(disk)
        system_sdp.write_bytes(b'\x00')
        disk_sdp.write_bytes(b'\x01\x00\x01\x00')
        system_edit, disk_edit = sha(system), sha(disk)
        prepare('-UpdateSystemFlash')
        assert sha(system) == system_edit and sha(disk) == disk_edit
        assert system_sdp.read_bytes() == b'\x00' and disk_sdp.read_bytes() == b'\x01\x00\x01\x00'
        assert not backups()
        print('Unchanged build preserves working C:/H: and explicit SDP state')

        system_sdp.unlink()
        prepare('-UpdateSystemFlash')
        assert sha(system) == system_edit and sha(disk) == disk_edit
        assert system_sdp.read_bytes() == b'\x01' and disk_sdp.read_bytes() == b'\x01\x00\x01\x00'
        print('Existing C: without an SDP sidecar is protected without replacing disk data')

        # Simulate an installed older build. C: must be replaced directly;
        # the mutable H: and its per-chip protection must stay untouched.
        data = json.loads(record.read_text())
        data['systemSeedSha256'] = '0' * 64
        record.write_text(json.dumps(data))
        prepare('-UpdateSystemFlash')
        assert not backups()
        assert sha(system) == sha(seed) and system_sdp.read_bytes() == b'\x01'
        assert sha(disk) == disk_edit and disk_sdp.read_bytes() == b'\x01\x00\x01\x00'
        print('New system seed enables SDP on C:, preserves H:/SDP, creates no backups')

        # Plain -Flash must retain both images and the remembered seed hash.
        edit(system)
        system_edit = sha(system)
        prepare()
        assert sha(system) == system_edit
        assert json.loads(record.read_text())['systemSeedSha256'] == sha(seed)
        prepare('-UpdateSystemFlash')
        assert sha(system) == system_edit

        # Old deployment records have no seed identity: refresh once.
        data = json.loads(record.read_text())
        del data['systemSeedSha256']
        record.write_text(json.dumps(data))
        prepare('-UpdateSystemFlash')
        assert not backups() and sha(system) == sha(seed)
        assert sha(disk) == disk_edit
        print('Legacy deployment migrates; plain persistent launch remains persistent')

        prepare('-ResetFlash')
        assert not backups()
        assert sha(system) == sha(seed) and sha(disk) == sha(disk_seed)
        assert system_sdp.read_bytes() == b'\x01' and not disk_sdp.exists()
        print('Explicit full reset restores both disks without backups')

        prepare('-SyncProfileImages')
        assert sha(floppy) == sha(floppy_seed)
        for image in (system, disk, floppy):
            edit(image)
        working = [sha(image) for image in (system, disk, floppy)]
        prepare('-SyncProfileImages')
        assert working == [sha(image) for image in (system, disk, floppy)]

        data = json.loads(record.read_text())
        data['diskSeedSha256'] = '0' * 64
        data['floppySeedSha256'] = '0' * 64
        record.write_text(json.dumps(data))
        prepare('-SyncProfileImages')
        assert sha(system) == working[0]
        assert sha(disk) == sha(disk_seed) and sha(floppy) == sha(floppy_seed)
        print('Profile sync refreshes H: and A: independently and preserves unchanged C:')

    assert before == [(p.stat().st_mtime_ns, sha(p)) for p in watched], 'no-build preparation rebuilt or changed source artifacts'
    workspace = (ROOT / 'Specialist-OS.code-workspace').read_text(encoding='utf-8-sig')
    tasks = {task['label']: task for task in json.loads(workspace)['tasks']['tasks']}
    assert all(task['group'] == 'build' for task in tasks.values())
    expected = {'Build all'}
    for profile in ('MMX Flash + floppy', 'MX ROM + floppy', 'RAMFOS'):
        build, run = f'Build and run Emu80 {profile}', f'Run Emu80 {profile}'
        expected.update((build, run))
        assert tasks[build]['args'] == tasks[run]['args'] + ['-Build']
        assert tasks[run]['args'][-2:] == [r'${workspaceFolder}\scripts\emu80\run-profile.ps1', profile]
        assert 'dependsOn' not in tasks[run]
    assert tasks.keys() == expected
    print('NO-BUILD SYSTEM REFRESH PASS: task has no build dependencies; source binaries/images unchanged')


if __name__ == '__main__':
    main()
