"""Shared incremental check for the Windows build entry points."""

import argparse
from contextlib import contextmanager
import errno
import hashlib
import json
import msvcrt
import os
from pathlib import Path
import re
import stat
import subprocess
import sys
import time


ROOT = Path(__file__).absolute().parents[2]
CACHE = ROOT / 'bin/.build-cache'
GENERATED_INPUTS = (
    'source/include/syscall-ids.inc', 'source/bios/syscall-tables.inc',
    'sdk/include/mxos_syscall_ids.h', 'sdk/src/mxos_syscall_ids.inc',
)
BENCH_OUTPUT_DIRS = ('source-apps/diskbench/build', 'source-apps/sortbench/build')
PROJECT_INPUTS = (
    'source', 'source-apps', 'sdk', 'scripts/build', 'scripts/images/cpu.ps1',
    'scripts/images/cpu.py', 'scripts/images/executable.py',
    'scripts/tests/check-monitor-win-build.py', 'assets/system-rom/fonts',
    'assets/programs/Test-MX.COM', 'tasm/sjasmplus.exe',
)
INCLUDE = re.compile(r"""^[ \t]*(include|incbin)[ \t]+(?:"([^"]+)"|'([^']+)'|([^\s;,]+))""", re.I | re.M)


def z88dk_directory():
    default = 'D:/Projects/RetroPC/Specialist/MySoft/z88dk'
    return Path(os.environ.get('SPECIALIST_Z88DK_DIR', default)).absolute()


def path_key(path):
    return os.path.normcase(os.path.abspath(path))


def metadata(stat):
    return [stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns]


def snapshot(paths, excluded=(), content=False, hashed=()):
    skip = {path_key(path) for path in excluded}
    hash_paths = {path_key(path) for path in hashed}
    records = {}

    def record(path, key, info):
        records[key] = hashlib.sha256(Path(path).read_bytes()).hexdigest() if content or key in hash_paths else metadata(info)

    def visit(directory):
        with os.scandir(directory) as entries:
            for entry in entries:
                key = path_key(entry.path)
                if key in skip or entry.name == '__pycache__':
                    continue
                if entry.is_dir(follow_symlinks=False):
                    visit(entry.path)
                elif entry.is_file():
                    record(entry.path, key, entry.stat())

    for path in paths:
        key = path_key(path)
        if key in skip:
            continue
        try:
            info = path.stat()
        except FileNotFoundError:
            continue
        if stat.S_ISDIR(info.st_mode):
            visit(path)
        elif stat.S_ISREG(info.st_mode):
            record(path, key, info)
    return records


def assembly_inputs(path):
    paths = set()
    read = set()

    def visit(source):
        if source in read:
            return
        read.add(source)
        paths.add(source)
        source_text = source.read_text(encoding='utf-8-sig', errors='replace')
        for directive, quoted, single, bare in INCLUDE.findall(source_text):
            name = quoted or single or bare
            dependency = (source.parent / name).absolute()
            if not dependency.is_file():
                dependency = path.parent / name
            # Normalize without resolving mapped drives into UNC paths.
            dependency = Path(os.path.abspath(dependency))
            paths.add(dependency)
            if directive.lower() == 'include' and dependency.is_file():
                visit(dependency)

    visit(path)
    return paths


@contextmanager
def build_lock():
    CACHE.mkdir(parents=True, exist_ok=True)
    with (CACHE / 'build.lock').open('a+b') as lock:
        while True:
            try:
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
                break
            except OSError as error:
                if error.errno not in (errno.EACCES, errno.EDEADLK):
                    raise
                time.sleep(0.2)
        yield


def read_stamp(path):
    try:
        stamp = json.loads(path.read_text(encoding='utf-8'))
    except (FileNotFoundError, ValueError):
        return {}
    return stamp if isinstance(stamp, dict) else {}


def run_cached(label, input_state, output_paths, scan_paths, command, force=False, content=False, hashed=()):
    """Run an existing builder and record its products only after success."""
    # Aliases with the same arguments share a stamp. Different variants share output
    # filenames, so the stamp also validates that their files are still installed.
    key = [label, path_key(Path.cwd()), command, list(map(path_key, output_paths)),
           list(map(path_key, scan_paths)), content, list(map(path_key, hashed))]
    digest = hashlib.sha256(json.dumps(key).encode()).hexdigest()[:24]
    stamp_path = CACHE / f'{digest}.json'
    with build_lock():
        inputs = input_state()
        stamp = read_stamp(stamp_path)
        previous = stamp.get('outputs')
        if not force and stamp.get('inputs') == inputs and isinstance(previous, dict) and previous:
            current = snapshot((Path(path) for path in previous), content=content, hashed=hashed)
            if previous == current:
                print(f'[up-to-date] {label}', flush=True)
                return 0

        stamp_path.unlink(missing_ok=True)
        before = snapshot(scan_paths, excluded=(CACHE,))
        result = subprocess.call(command)
        if result:
            return result

        products = snapshot(output_paths, content=content, hashed=hashed)
        for path in output_paths:
            if path_key(path) not in products:
                raise ValueError(f'Missing build output: {path}')
        changed = snapshot(scan_paths, excluded=(CACHE,))
        discovered = (Path(name) for name, state in changed.items() if before.get(name) != state and name not in products)
        products.update(snapshot(discovered, content=content, hashed=hashed))
        if inputs == input_state() and products:
            temporary = stamp_path.with_suffix('.tmp')
            temporary.write_text(json.dumps({'inputs': inputs, 'outputs': products}), encoding='utf-8')
            temporary.replace(stamp_path)
        return 0


def python_build(script, label, build, *, inputs, outputs=(), scan_outputs=(), exclude=(), z88dk=False):
    """Keep a standalone Python builder on the same cache as the VS Code tasks."""
    parser = argparse.ArgumentParser(description=f'Build {label}')
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--run-build', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.run_build:
        build()
        return 0
    options = [label, '--z88dk'] if z88dk else [label, '--input', str(ROOT / 'tasm/sjasmplus.exe')]
    for flag, paths in (('input', inputs), ('output', outputs), ('scan-output', scan_outputs), ('exclude', exclude)):
        for name in paths:
            options.extend(('--' + flag, str(ROOT / name)))
    if args.force:
        options.append('--force')
    return main([*options, '--', script, '--run-build'])


def main(argv):
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('label')
    for flag in ('input', 'output', 'scan-output', 'asm', 'hash', 'exclude'):
        parser.add_argument('--' + flag, type=Path, action='append', default=[])
    parser.add_argument('--inputs-from-stdin', action='store_true', help='Read additional input paths as a JSON array')
    parser.add_argument('--project-inputs', action='store_true')
    parser.add_argument('--z88dk', action='store_true')
    parser.add_argument('--content', action='store_true')
    parser.add_argument('--force', action='store_true')
    if '--' not in argv or argv[-1] == '--':
        parser.error('Pass the build script and its arguments after --')
    split = argv.index('--')
    args = parser.parse_args(argv[:split])
    if args.inputs_from_stdin:
        try:
            extra_inputs = json.load(sys.stdin)
            if not isinstance(extra_inputs, list) or not all(isinstance(name, str) for name in extra_inputs):
                raise ValueError('expected a JSON array of input paths')
            args.input.extend(map(Path, extra_inputs))
        except ValueError as error:
            parser.error(f'Invalid input list: {error}')
    script, *parameters = argv[split + 1:]
    script = Path(script).absolute()
    command = [str(script), *parameters]
    if script.suffix.lower() == '.ps1':
        command = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', *command]
    elif script.suffix.lower() == '.py':
        command = [sys.executable, *command]

    paths = [Path(__file__), Path(sys.executable), script, *args.input]
    if args.asm:
        paths.append(ROOT / 'tasm/sjasmplus.exe')
    if args.project_inputs:
        paths.extend(ROOT / name for name in PROJECT_INPUTS)
    if args.z88dk:
        z88dk = z88dk_directory()
        paths.extend(z88dk / name for name in ('bin', 'include', 'lib'))
    project_excluded = tuple(ROOT / name for name in (
        *GENERATED_INPUTS, *BENCH_OUTPUT_DIRS, 'source-apps/llm80',
        'source-apps/almaz', 'source-apps/almaz2', 'source-apps/test-scr',
        'source/ramfos/test-low.asm', 'source/ramfos/test-high.asm',
    )) if args.project_inputs else ()

    def input_state():
        assembly = {path for source in args.asm for path in assembly_inputs(source.absolute())}
        records = snapshot([*paths, *assembly], (*project_excluded, *args.exclude), content=args.content, hashed=args.hash)
        return hashlib.sha256(json.dumps(records, sort_keys=True).encode()).hexdigest()

    try:
        return run_cached(args.label, input_state, args.output, args.scan_output, command,
                          force=args.force, content=args.content, hashed=args.hash)
    except (OSError, ValueError) as error:
        parser.exit(1, f'{args.label}: {error}\n')


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
