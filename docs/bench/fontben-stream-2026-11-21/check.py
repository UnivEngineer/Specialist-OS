"""Compare both kernels through FontBen on each private, muted Emu80."""
from pathlib import Path

report = Path(__file__).resolve().parent
previous = report.parent/'remap32-modern-2026-11-21/check.py'
runner = report.parent/'fontben-alternating-2026-11-21/check.py'
source = runner.read_text(encoding='utf-8')
shared, scenarios = source.split('resident = (', 1)
transport = previous.read_text(encoding='utf-8').split("transport = '''", 1)[1].split("'''", 1)[0]
import base64
import http.client
from urllib.parse import urlsplit
shared = shared.replace("m = Bench(f'http://127.0.0.1:{args.port}/mcp')", transport)
boot = 'assert not owner()' + scenarios.split('assert not owner()', 1)[1]
boot = boot.split('    for name, path in resident[args.group]:', 1)[0]
body = '''
    install('6x8/Rotate')
    groups = (
        '6ROTATEROWS,6ROTATESTREAM,6PACKEDQUADS,6PACKEDSTREAM',
        '6FULLQUADS,6FASTSTREAM',
        'CPM4,4PACKEDSTREAM,FAST4,4FASTSTREAM',
        '4COMPACT,4COMPACTSTREAM',
    )
    variants = groups[args.group]
    for inverse in (False, True):
        benchmark(variants, inverse=inverse)
    assert len(results) == len(variants.split(','))*36 and not errors, (len(results), errors)
    assert {r['pixels'] for r in results} == {'oracle'}
    print('COMPLETE', args.group, len(results), 'results', flush=True)
finally:
    m.http.close()
    if pid and process_path(pid) == exe:
        ps(f'Stop-Process -Id {pid}')
'''
exec(compile(shared + boot + body, str(__file__), 'exec'))

