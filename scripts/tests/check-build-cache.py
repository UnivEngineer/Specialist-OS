"""Exercise invalidation and failed builds without touching the project outputs."""

import hashlib
import os
from pathlib import Path
import sys
import tempfile
from unittest.mock import patch


sys.path.insert(0, str(Path(__file__).absolute().parents[1] / 'build'))
import build_cache


def check_assembly_dependencies(root, worker, output):
    source = root / 'main.asm'
    header = root / 'shared.inc'
    binary = root / 'data.bin'
    source.write_text('    INCLUDE "shared.inc"\n', encoding='utf-8')
    header.write_text('data: INCBIN "data.bin"\n', encoding='utf-8')
    binary.write_bytes(b'first')
    command = ['assembly', '--input-content', '--asm', str(source), '--output', str(output),
               '--', str(worker), str(binary), str(output), 'A']

    assert build_cache.main(command) == 0 and output.read_bytes() == b'firstA'
    created = output.stat().st_mtime_ns
    mtime = source.stat().st_mtime_ns
    os.utime(source, ns=(mtime, mtime + 1_000_000_000))
    assert build_cache.main(command) == 0 and output.stat().st_mtime_ns == created
    binary.write_bytes(b'changed')
    assert build_cache.main(command) == 0 and output.read_bytes() == b'changedA'


def main():
    with tempfile.TemporaryDirectory() as directory:
        root = Path(directory)
        source, output, worker = (root / name for name in ('source', 'output', 'worker.py'))
        source.write_bytes(b'first')
        worker.write_text(
            'from pathlib import Path\n'
            'import sys\n'
            'source, output, variant = sys.argv[1:]\n'
            'data = Path(source).read_bytes()\n'
            'Path(output).write_bytes(data + variant.encode())\n'
            'sys.exit(1 if data == b"fail" else 0)\n',
            encoding='utf-8',
        )

        def build(variant='A', force=False):
            return build_cache.run_cached(
                'sample', lambda: hashlib.sha256(source.read_bytes()).hexdigest(),
                (output,), (), [sys.executable, str(worker), str(source), str(output), variant],
                force=force,
            )

        with patch.object(build_cache, 'CACHE', root / 'cache'):
            assert build() == 0 and output.read_bytes() == b'firstA'
            created = output.stat().st_mtime_ns
            assert build() == 0 and output.stat().st_mtime_ns == created
            assert build('B') == 0 and output.read_bytes() == b'firstB'
            assert build() == 0 and output.read_bytes() == b'firstA'
            output.unlink()
            assert build() == 0 and output.read_bytes() == b'firstA'
            source.write_bytes(b'next')
            assert build() == 0 and output.read_bytes() == b'nextA'
            source.write_bytes(b'fail')
            assert build() != 0 and output.read_bytes() == b'failA'
            assert build() != 0
            source.write_bytes(b'recovered')
            assert build() == 0 and output.read_bytes() == b'recoveredA'
            assert build(force=True) == 0
            check_assembly_dependencies(root, worker, output)
    print('Build cache: hits, configuration, assembly dependencies, invalidation, failure and recovery OK')


if __name__ == '__main__':
    main()
