"""Shared incremental check for the Windows build entry points."""

import argparse
from collections import defaultdict
from concurrent.futures import ThreadPoolExecutor
from contextlib import contextmanager, nullcontext
import errno
from functools import cache, lru_cache
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
INCLUDE = re.compile(
    r"""^[ \t]*(?:[\w.@]+:[ \t]*)?(include|incbin)[ \t]+(?:"([^"]+)"|'([^']+)'|([^\s;,]+))""",
    re.I | re.M,
)


def z88dk_directory():
    default = 'D:/Projects/RetroPC/Specialist/MySoft/z88dk'
    return Path(os.environ.get('SPECIALIST_Z88DK_DIR', default)).absolute()


@cache
def drive_root(drive):
    return os.path.normcase(os.path.realpath(drive + '\\')).rstrip('\\')


def path_key(path):
    absolute = os.path.normcase(os.path.abspath(path))
    drive, tail = os.path.splitdrive(absolute)
    if drive.endswith(':'):
        # Resolve the drive once so mapped and UNC paths share cache entries.
        return drive_root(drive) + tail
    return absolute


def metadata(stat):
    return [stat.st_size, stat.st_mtime_ns, stat.st_ctime_ns]


@lru_cache(maxsize=512)
def read_input(path, version):
    return path.read_bytes()


def input_bytes(path, info):
    return read_input(path, tuple(metadata(info)))


def snapshot(paths, excluded=(), content=False, hashed=(), *, files_only=False, inputs=False):
    skip = {path_key(path) for path in excluded}
    skipped_subtrees = tuple(key.rstrip(os.sep) + os.sep for key in skip)
    hash_paths = {path_key(path) for path in hashed}
    records = {}

    def record(path, key, info):
        if content or key in hash_paths:
            path = Path(path)
            data = input_bytes(path, info) if inputs else path.read_bytes()
            records[key] = hashlib.sha256(data).hexdigest()
        else:
            records[key] = metadata(info)

    def visit(directory, selected=None):
        with os.scandir(directory) as entries:
            for entry in entries:
                if selected is not None and os.path.normcase(entry.name) not in selected:
                    continue
                key = path_key(entry.path)
                if key in skip or entry.name == '__pycache__':
                    continue
                if selected is None and entry.is_dir(follow_symlinks=False):
                    visit(entry.path)
                elif entry.is_file():
                    record(entry.path, key, entry.stat())

    if files_only:
        groups = defaultdict(list)
        for path in paths:
            groups[path.parent].append(path)
        paths = []
        for directory, files in groups.items():
            # On SMB, one directory listing is cheaper than many file queries.
            if len(files) < 8:
                paths.extend(files)
                continue
            try:
                visit(directory, {os.path.normcase(path.name) for path in files})
            except (FileNotFoundError, NotADirectoryError):
                pass

    for path in paths:
        key = path_key(path)
        if key in skip or key.startswith(skipped_subtrees):
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


def assembly_inputs(roots, excluded=()):
    paths = set()
    read = set()
    skip = {path_key(item) for item in excluded}
    skipped_subtrees = tuple(key.rstrip(os.sep) + os.sep for key in skip)

    def is_excluded(source):
        key = path_key(source)
        return key in skip or key.startswith(skipped_subtrees)

    @cache
    def is_file(source):
        return source.is_file()

    def visit(source, top_directory):
        context = (source, top_directory)
        if context in read:
            return
        read.add(context)
        paths.add(source)
        source_text = input_bytes(source, source.stat()).decode('utf-8-sig', errors='replace')
        for directive, quoted, single, bare in INCLUDE.findall(source_text):
            name = quoted or single or bare
            # Normalize without resolving mapped drives into UNC paths.
            dependency = Path(os.path.abspath(source.parent / name))
            if is_excluded(dependency):
                continue
            if not is_file(dependency):
                dependency = Path(os.path.abspath(top_directory / name))
            if is_excluded(dependency):
                continue
            paths.add(dependency)
            if directive.lower() == 'include' and is_file(dependency):
                visit(dependency, top_directory)

    for root in roots:
        root = root.absolute()
        sources = (p for p in root.rglob('*') if p.suffix.lower() in ('.asm', '.inc')) if root.is_dir() else (root,)
        for source in sources:
            if not is_excluded(source):
                visit(source, source.parent)
    return paths


@contextmanager
def build_lock():
    CACHE.mkdir(parents=True, exist_ok=True)
    with (CACHE / 'build.lock').open('a+b') as lock:
        wait_started = None
        while True:
            try:
                msvcrt.locking(lock.fileno(), msvcrt.LK_NBLCK, 1)
                break
            except OSError as error:
                if error.errno not in (errno.EACCES, errno.EDEADLK):
                    raise
                if wait_started is None:
                    wait_started = time.perf_counter()
                    print('[waiting] Build cache lock', flush=True)
                time.sleep(0.2)
        if wait_started is not None:
            print(f'[time] Build cache lock: {time.perf_counter() - wait_started:.3f} s', flush=True)
        yield


def read_stamp(path):
    try:
        stamp = json.loads(path.read_text(encoding='utf-8'))
    except (FileNotFoundError, ValueError):
        return {}
    return stamp if isinstance(stamp, dict) else {}


def run_cached(label, input_state, output_paths, scan_paths, command, force=False, content=False, hashed=(),
               input_content=False, *, cwd=None, batch_locked=False):
    """Run an existing builder and record its products only after success."""
    started = time.perf_counter()
    print(f'[checking] {label}', flush=True)
    # Aliases with the same arguments share a stamp. Different variants share output
    # filenames, so the stamp also validates that their files are still installed.
    command_key = [path_key(arg) if os.path.isabs(arg) else arg for arg in command]
    key = [label, path_key(cwd or Path.cwd()), command_key, list(map(path_key, output_paths)),
           list(map(path_key, scan_paths)), content, list(map(path_key, hashed))]
    if input_content:
        key.append('input-content')
    digest = hashlib.sha256(json.dumps(key).encode()).hexdigest()[:24]
    stamp_path = CACHE / f'{digest}.json'
    with (nullcontext() if batch_locked else build_lock()):
        inputs = input_state()
        stamp = read_stamp(stamp_path)
        previous = stamp.get('outputs')
        if not force and stamp.get('inputs') == inputs and isinstance(previous, dict) and previous:
            current = snapshot((Path(path) for path in previous), content=content, hashed=hashed, files_only=True)
            if previous == current:
                print(f'[up-to-date] {label} ({time.perf_counter() - started:.3f} s)', flush=True)
                return 0

        stamp_path.unlink(missing_ok=True)
        print(f'[build] {label} (checked in {time.perf_counter() - started:.3f} s)', flush=True)
        started = time.perf_counter()
        before = snapshot(scan_paths, excluded=(CACHE,))
        result = subprocess.call(command, cwd=cwd)
        read_input.cache_clear()
        if result:
            print(f'[failed] {label} (exit code {result}, {time.perf_counter() - started:.3f} s)',
                  file=sys.stderr, flush=True)
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
        print(f'[time] {label}: {time.perf_counter() - started:.3f} s', flush=True)
        return 0


def python_build(script, label, build, *, inputs, asm=(), outputs=(), scan_outputs=(), exclude=(),
                 z88dk=False, input_content=False):
    """Keep a standalone Python builder on the same cache as the VS Code tasks."""
    parser = argparse.ArgumentParser(description=f'Build {label}')
    parser.add_argument('--force', action='store_true')
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument('--run-build', action='store_true', help=argparse.SUPPRESS)
    mode.add_argument('--describe', action='store_true', help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.run_build:
        build()
        return 0
    options = [label, '--z88dk'] if z88dk else [label, '--input', str(ROOT / 'tasm/sjasmplus.exe')]
    for flag, paths in (('input', inputs), ('asm', asm), ('output', outputs),
                        ('scan-output', scan_outputs), ('exclude', exclude)):
        for name in paths:
            options.extend(('--' + flag, str(ROOT / name)))
    if args.force:
        options.append('--force')
    if input_content:
        options.append('--input-content')
    command = [*options, '--', script, '--run-build']
    if args.describe:
        print(json.dumps({'cwd': str(Path.cwd()), 'args': command}))
        return 0
    return main(command)


def run_batch(jobs):
    # Одна блокировка на весь пакет. Сканирование результатов по умолчанию
    # последовательно. Для непересекающихся каталогов вызывающий может разрешить параллельную работу.
    with build_lock(), ThreadPoolExecutor(max_workers=32) as pool:
        pending = []

        def run(job):
            if 'describe' in job:
                result = subprocess.run([sys.executable, *job['describe'], '--describe'], cwd=job['cwd'],
                                        stdout=subprocess.PIPE, encoding='utf-8')
                if result.returncode:
                    return result.returncode
                job = json.loads(result.stdout)
            return main(job['args'], cwd=Path(job['cwd']).absolute(), batch_locked=True)

        def finish():
            results = [future.result() for future in pending]
            pending.clear()
            return next((status for status in results if status), 0)

        for job in jobs:
            args = job.get('args')
            options = args[:args.index('--')] if args is not None else ()
            if job.get('serial', args is None or '--scan-output' in options):
                status = finish() or run(job)
                if status:
                    return status
            else:
                pending.append(pool.submit(run, job))
        return finish()


def main(argv, *, cwd=None, batch_locked=False):
    if argv == ['--batch']:
        return run_batch(json.load(sys.stdin))

    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('label')
    for flag in ('input', 'output', 'scan-output', 'asm', 'hash', 'exclude'):
        parser.add_argument('--' + flag, type=Path, action='append', default=[])
    parser.add_argument('--inputs-from-stdin', action='store_true', help='Read additional input paths as a JSON array')
    parser.add_argument('--z88dk', action='store_true')
    parser.add_argument('--content', action='store_true')
    parser.add_argument('--input-content', action='store_true', help='Compare input bytes instead of timestamps')
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
    cwd = cwd or Path.cwd()
    script = cwd / script
    command = [str(script), *parameters]
    if script.suffix.lower() == '.ps1':
        command = ['powershell.exe', '-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', *command]
    elif script.suffix.lower() == '.py':
        command = [sys.executable, *command]

    for flag in ('input', 'output', 'scan_output', 'asm', 'hash', 'exclude'):
        setattr(args, flag, [cwd / path for path in getattr(args, flag)])
    paths = [Path(__file__), Path(sys.executable), script, *args.input]
    if args.asm:
        paths.append(ROOT / 'tasm/sjasmplus.exe')
    if args.z88dk:
        z88dk = z88dk_directory()
        paths.extend(z88dk / name for name in ('bin', 'include', 'lib'))
    def input_state():
        assembly = assembly_inputs(args.asm, args.exclude)
        options = dict(content=args.content or args.input_content, hashed=args.hash, inputs=True)
        records = snapshot(paths, args.exclude, **options)
        records.update(snapshot(assembly, args.exclude, files_only=True, **options))
        return hashlib.sha256(json.dumps(records, sort_keys=True).encode()).hexdigest()

    try:
        return run_cached(args.label, input_state, args.output, args.scan_output, command,
                          force=args.force, content=args.content, hashed=args.hash, input_content=args.input_content,
                          cwd=cwd, batch_locked=batch_locked)
    except (OSError, ValueError) as error:
        parser.exit(1, f'{args.label}: {error}\n')


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
