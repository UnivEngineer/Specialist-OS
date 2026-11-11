"""Настоящий sccz80/8080: stdlib malloc/calloc/realloc/free и объединение блоков."""
from pathlib import Path
import os
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'bin/tests/near-heap'
OUT.mkdir(parents=True, exist_ok=True)
Z88 = Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk')
env = os.environ.copy()
env['ZCCCFG'] = str(Z88 / 'lib/config')
env['PATH'] = str(Z88 / 'bin') + os.pathsep + env['PATH']
subprocess.run([str(Z88 / 'bin/zcc.exe'), '+cpm', '-clib=8080', '-compiler=sccz80',
                '-crt0=' + str(ROOT / 'sdk/tests/file-crt'), '-zorg=8192',
                '-I' + str(ROOT / 'sdk/include'), str(ROOT / 'sdk/src/mxos_heap.c'),
                str(ROOT / 'sdk/src/mxos_stdlib.c'), str(ROOT / 'sdk/tests/heap.c'),
                '-o', str(OUT / 'heap.bin'), '-m'], env=env, check=True)
code = (OUT / 'heap_code_crt_init.bin').read_bytes()
assert 0x2000 + len(code) < 0x5e80
memory = bytearray(65536)
memory[0x2000:0x2000 + len(code)] = code
(OUT / 'input.bin').write_bytes(memory)
subprocess.run([str(Z88 / 'bin/z88dk-ticks.exe'), '-m8080', '-pc', '2000', '-end', '6100',
                '-counter', '10000000', '-output', str(OUT / 'out.bin'), str(OUT / 'input.bin')], check=True, capture_output=True)
result = (OUT / 'out.bin').read_bytes()
assert int.from_bytes(result[65542:65544], 'little') == 0x6100
status = int.from_bytes(result[0x5e80:0x5e82], 'little')
assert status == 0, status
print('PASS: NEAR stdlib, overflow, realloc rollback, coalescing;', len(code), 'bytes')
