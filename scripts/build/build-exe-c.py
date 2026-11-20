"""Build the C reference for comparison with EXE.COM."""

import os
import re
import runpy
import subprocess
import sys

from build_cache import ROOT, python_build, z88dk_directory


def build():
    # z88dk's assembler include lookup needs the drive letter on mapped shares.
    out = ROOT / 'bin'
    z88dk = z88dk_directory()
    env = os.environ.copy()
    env['ZCCCFG'] = str(z88dk / 'lib/config')
    env['PATH'] = str(z88dk / 'bin') + os.pathsep + env['PATH']
    files = (
        'sdk/src/mxos_z88dk.asm', 'sdk/src/mxos.c', 'sdk/src/mxos_file_z88dk.asm',
        'sdk/src/mxos_file.c', 'sdk/src/mxos_far.c', 'sdk/src/mxos_heap.c', 'source/exe/EXE.c',
    )
    subprocess.run([
        str(z88dk / 'bin/zcc.exe'), '+cpm', '-SO3', '-clib=8080', '-compiler=sccz80',
        '-crt0=' + str(ROOT / 'sdk/runtime/crt0'), '-zorg=256', '-I' + str(ROOT / 'sdk/include'),
        *(str(ROOT / name) for name in files), '-o', str(out / 'EXE_C.bin'), '-m',
    ], env=env, cwd=ROOT, check=True)
    code = (out / 'EXE_C_code_crt_init.bin').read_bytes()
    map_text = (out / 'EXE_C.map').read_text('utf-8')
    head = int(re.search(r'^__BSS_head\s*=\s*\$([0-9A-Fa-f]+)', map_text, re.M)[1], 16)
    assert 0x100 <= head <= 0x100 + len(code)
    crt_limit = int(re.search(r'^crtLimit\s*=\s*\$([0-9A-Fa-f]+)', map_text, re.M)[1], 16)
    assert code[crt_limit - 0x100] == 0x11, 'CRT heap must use LXI D, not a memory load'
    code = code[:head - 0x100]
    (out / 'EXE_C.i80').write_bytes(code)
    cpu = runpy.run_path(str(ROOT / 'scripts/images/cpu.py'))
    fields = cpu['fields_from_c_map'](map_text, 0x100, len(code))
    (out / 'EXE_C.cpu').write_text(cpu['format_cpu'](0x100, flags=4, fields=fields), encoding='ascii')
    print('EXE C reference:', len(code), 'body bytes, BSS', fields['BSS'])


if __name__ == '__main__':
    sys.exit(python_build(__file__, 'EXE_C', build, z88dk=True,
                          inputs=('sdk', 'source/exe/EXE.c', 'scripts/images/cpu.py'),
                          outputs=('bin/EXE_C.cpu', 'bin/EXE_C.i80', 'bin/EXE_C.map', 'bin/EXE_C_code_crt_init.bin'),
                          scan_outputs=('bin',)))
