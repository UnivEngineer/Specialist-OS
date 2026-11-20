"""Автономные проверочные DLL-модули; формат тот же, что в сборщике образов."""
from pathlib import Path
import re
import runpy
import subprocess
import sys


HERE = Path(__file__).absolute().parent
ROOT = HERE.parents[1]
OUTPUT = ROOT / 'bin/moduletest'


def build():
    OUTPUT.mkdir(parents=True, exist_ok=True)
    executable = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))
    modules = (
        ('BANKMOD', 'MODULE', ['-DMAIN_MODULE=0'], 0x100),
        ('MAINMOD', 'MODULE', ['-DMAIN_MODULE=1'], 0x6000),
        ('DOSMOD', 'MODULE', ['-DMAIN_MODULE=2'], None),
        ('MRET', 'MRET', [], 0x100), ('MODTEST', 'MODTEST', [], 0),
        ('BANKAPP', 'BANKAPP', [], 0x100), ('MAINAPP', 'MAINAPP', [], 0),
        ('DOSAPP', 'DOSAPP', [], None), ('DRVPROBE', 'DRVPROBE', [], None),
    )
    for name, source, defines, origin in modules:
        subprocess.run([
            str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow', *defines,
            f'--raw={OUTPUT / name}.bin', f'--sym={OUTPUT / name}.sym', str(HERE / f'{source}.asm'),
        ], check=True)
        symbols = (OUTPUT / f'{name}.sym').read_text('utf-8')
        flags = re.search(r'^IMAGE_FLAGS: EQU 0x([0-9A-Fa-f]+)', symbols, re.M)
        if origin is None:
            origin = int(re.search(r'^DOS_BANK_DRIVER_ADDR: EQU 0x([0-9A-Fa-f]+)', symbols, re.M)[1], 16)
        ext = 'COM' if name in ('MODTEST', 'BANKAPP', 'MAINAPP', 'DOSAPP', 'DRVPROBE') else 'DLL'
        image = executable['Image']((OUTPUT / f'{name}.bin').read_bytes(), origin, origin, int(flags[1], 16) if flags else 0)
        (OUTPUT / f'{name}.{ext}').write_bytes(executable['encode'](image, ext.lower()))


if __name__ == '__main__':
    python_build = runpy.run_path(str(ROOT / 'scripts/build/build_cache.py'))['python_build']
    sys.exit(python_build(__file__, 'Module tests', build,
                          inputs=(HERE, 'source/include', 'scripts/images/executable.py', 'scripts/images/cpu.py'),
                          scan_outputs=(OUTPUT,)))
