"""Сборка одного C-приложения с автоматической кучей для MAIN и BANK."""
from pathlib import Path
import os
import re
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'bin/tests/near-heap'
OUT.mkdir(parents=True, exist_ok=True)
z = Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk')
env = os.environ.copy()
env['ZCCCFG'] = str(z / 'lib/config')
env['PATH'] = str(z / 'bin') + os.pathsep + env['PATH']
sources = ['sdk/src/mxos_z88dk.asm', 'sdk/src/mxos_far.c', 'sdk/src/mxos_heap.c',
           'sdk/src/mxos_stdlib.c', 'sdk/tests/runtime.c']
subprocess.run([str(z / 'bin/zcc.exe'), '+cpm', '-clib=8080', '-compiler=sccz80',
                '-crt0=' + str(ROOT / 'sdk/runtime/crt0'), '-zorg=256',
                '-I' + str(ROOT / 'sdk/include'), *[str(ROOT / f) for f in sources],
                '-o', str(OUT / 'runtime.bin'), '-m'], env=env, check=True)
code = (OUT / 'runtime_code_crt_init.bin').read_bytes()
assert 0x100 + len(code) < 0x2000
e = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))
for name, flags in [('CMAIN', 4), ('CBANK', 2)]:
    (OUT / (name + '.COM')).write_bytes(e['encode'](e['Image'](code, 0x100, 0x100, flags), 'com'))
print('C runtime MAIN/BANK:', len(code), 'bytes')
