"""Выпустить все профили из основной ветки рабочей копии владельца."""

import argparse
import os
import subprocess
import sys

from build_cache import RELEASE_BUILD, ROOT, build_lock


BUILD_DIR = ROOT / 'scripts/build'
COUNTER = BUILD_DIR / 'release-build.txt'
MAX_BUILD = 2**32 - 1


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--force', action='store_true')
    args = parser.parse_args()
    if RELEASE_BUILD in os.environ:
        parser.error('A release build is already running in this process tree')

    enabled = subprocess.run(['git', 'config', '--local', '--type=bool', '--get', 'mxos.release'],
                             cwd=ROOT, capture_output=True, text=True, check=False)
    if enabled.returncode or enabled.stdout.strip() != 'true':
        parser.error('Release builds are disabled in this checkout')

    with build_lock():
        branch = subprocess.run(['git', 'symbolic-ref', '--quiet', '--short', 'HEAD'],
                                cwd=ROOT, capture_output=True, text=True, check=False)
        if branch.returncode or branch.stdout.strip() != 'main':
            parser.error('Release builds require the main branch')

        last = int(COUNTER.read_text(encoding='ascii'))
        if not 0 <= last < MAX_BUILD:
            parser.error(f'Invalid or exhausted release build counter: {last}')
        build = str(last + 1)
        command = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass',
                   '-File', str(BUILD_DIR / 'build-all.ps1')]
        if args.force:
            command.append('-Force')
        print(f'[release] BUILD {build}', flush=True)
        result = subprocess.call(command, cwd=ROOT, env={**os.environ, RELEASE_BUILD: build})
        if result:
            return result

        temporary = COUNTER.with_suffix('.tmp')
        temporary.write_text(build + '\n', encoding='ascii', newline='\n')
        temporary.replace(COUNTER)
        print(f'[release] Saved BUILD {build}', flush=True)
        return 0


if __name__ == '__main__':
    sys.exit(main())
