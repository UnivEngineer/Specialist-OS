"""CPU-only 8080 comparison; not an Emu80/PIT or bank-transport measurement.

Run check-engines.py's complete ENGINEQ pixel/ABI tests first, then execute
the actual FontBen MAIN loops on identical README bytes, 46 + CRLF x 24.
Build ENGINEP, ENGINES, ENGINEQ and STANDALONE (output FONTCOM) beforehand.
"""
from pathlib import Path
import hashlib
import json
import runpy
import sys

R = Path(__file__).resolve().parents[2]
saved = sys.argv
try:
    sys.argv = [str(R/'source-apps/fontbench/check-engines.py'), 'ENGINEQ']
    q = runpy.run_path(sys.argv[0])
finally:
    sys.argv = saved
O = R/'docs/bench/triple-packed'
O.mkdir(parents=True, exist_ok=True)
g = q['s']; width = g['GRID_GLYPHS']; printable = width*g['GRID_ROWS']
source = (R/'assets/system-rom/README.TXT').read_bytes()
source = source.replace(b'\r\n', b' ').replace(b'\r', b' ').replace(b'\n', b' ')
source = bytes(b if b >= 32 else 32 for b in source)[:printable].ljust(printable, b' ')
text = b''.join(source[i:i+width]+b'\r\n' for i in range(0, printable, width))
assert len(text) == g['GRID_TEXT_BYTES']
(O/'corpus.bin').write_bytes(text)
results = []
mem = q['init']('ENGINEQ', 0, 1, 6)
s, bs = q['s'], q['bs']
packed = bytes(mem[0x3000:0x4800])
# Exhaust every code/phase with nonzero adjacent pixels, independently of
# the string-head/tail tests. Check that rendering never changes the table.
for x in range(4):
    for code in range(256):
        m = bytearray(mem)
        m[bs['mon2_vars.cursorY']:bs['mon2_vars.cursorY']+2] = bytes([7, x])
        m[0x9000:0xC000] = bytes([0xA5])*12288
        out, _, _ = q['run'](m, s['FONT_ENGINE_RAW'], af=code<<8)
        expected, cursor = q['oracle'](bytes([code]), True, 6, x, 0xA5)
        assert out[0x9000:0xC000] == expected, (code, x, 'scalar pixels')
        assert out[bs['mon2_vars.cursorY']:bs['mon2_vars.cursorY']+2] == cursor
        assert out[0x3000:0x4800] == packed, 'table modified'
print('TRIPACK all 256 scalar codes x 4 phases PASS', flush=True)
apis = ('printChar', 'printCharRaw', 'printStringZ', 'printStringRawZ', 'printStringRawLim')
for name, module, variant, pre in (('ROW6 OFF', 'ENGINEP', 0, 0),
                                    ('ROW6 ON', 'ENGINEP', 0, 1),
                                    ('COL6PRE ON', 'ENGINES', 4, 1),
                                    ('TRIPACK ON', 'ENGINEQ', 0, 1)):
    mem = q['init'](module, variant, pre, 6)
    for kind, api in enumerate(apis):
        m = bytearray(mem)
        s, bs = q['s'], q['bs']
        m[s['gridText']:s['gridText']+len(text)+1] = text+b'\0'
        m[s['gridKind']] = kind
        m[bs['mon2_vars.cursorY']:bs['mon2_vars.cursorY']+2] = bytes([7, 0])
        m[0x9000:0xC000] = bytes(12288)
        out, regs, cycles = q['run'](m, s['gridMainRun'])
        expected, cursor = q['oracle'](text, kind in (1, 3, 4), 6)
        assert out[0x9000:0xC000] == expected, (name, api, 'pixels')
        assert out[bs['mon2_vars.cursorY']:bs['mon2_vars.cursorY']+2] == cursor
        results.append(dict(engine=name, api=api, cycles=cycles, ms=cycles/2000))
        print(name, api, cycles/2000, 'ms', flush=True)
paths = ['bin/'+f+ext for f, ext in (('ENGINEP','.i80'), ('ENGINES','.i80'),
         ('ENGINEQ','.i80'), ('FONTCOM','.i80'), ('BIOS','.sys'), ('IO','.sys'))]
report = dict(method='z88dk-ticks -m8080, actual FontBen gridMainRun + public hooks',
              cpuHz=2000000, bankTested=False, emulatorTested=False,
              checks=q['checks']+1024+len(results), results=results,
              corpusSha256=hashlib.sha256(text).hexdigest(),
              sha256={p:hashlib.sha256((R/p).read_bytes()).hexdigest() for p in paths})
(O/'results.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
