"""Replay the affected FontBen profiles on four isolated, muted Emu80 runtimes."""
import base64
import http.client
from pathlib import Path
from urllib.parse import urlsplit

report = Path(__file__).resolve().parent
base_harness = report.parent/'fontben-alternating-2026-11-21/check.py'
keep_alive = report.parent/'remap32-modern-2026-11-21/check.py'
shared, scenarios = base_harness.read_text(encoding='utf-8').split('resident = (', 1)
transport = keep_alive.read_text(encoding='utf-8').split("transport = '''", 1)[1].split("'''", 1)[0]
shared = shared.replace("m = Bench(f'http://127.0.0.1:{args.port}/mcp')", transport)
boot = 'assert not owner()' + scenarios.split('assert not owner()', 1)[1]
boot = boot.split('    for name, path in resident[args.group]:', 1)[0]
body = '''
    def verify_resident():
        labels = sym('fonts/Rotate')
        begin = labels['fast6.groupNormalDown']
        end = labels['fast6.noGroup']
        base_addr = labels['FONT_ENGINE_BASE']
        engine = (root/'bin/fonts/Rotate.i80').read_bytes()
        expected = bytearray(engine[begin-base_addr:end-base_addr])
        observed = bytearray(m.read(begin,end-begin))
        for route in ('normalDownLimit','normalUpLimit','inverseDownLimit','inverseUpLimit'):
            offset = labels['fast6.'+route]+1-begin
            observed[offset] = expected[offset]
        assert observed == expected, 'Installed font does not match the four-row kernel'

    # Writable A: selects 6x8/Fast in AUTOEXEC before the NC prompt.
    install('6x8/Rotate')
    verify_resident()
    inverse = bool(args.group & 1)
    if args.group < 2:
        benchmark('', '6x8/Rotate', inverse)
        benchmark('', '6x8/Rotate', inverse, phase=0, pseudo=True)
        expected_count = 20
    else:
        benchmark('6ROTATECOL4', inverse=inverse)
        expected_count = 18
    assert len(results) == expected_count and not errors, (len(results), errors)
    assert {r['pixels'] for r in results} == {'oracle'}
    print('COMPLETE', args.group, len(results), 'results', flush=True)
finally:
    m.http.close()
    if pid and process_path(pid) == exe:
        ps(f'Stop-Process -Id {pid}')
'''
exec(compile(shared + boot + body, str(__file__), 'exec'))
