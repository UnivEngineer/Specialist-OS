"""Build matched MAIN and EXEC NC profiles from the shared implementation."""
from pathlib import Path
import subprocess
import sys
import runpy

ROOT = Path(__file__).resolve().parents[2]
cpu_helpers = runpy.run_path(str(ROOT/'scripts/images/cpu.py'))
exe = runpy.run_path(str(ROOT/'scripts/images/executable.py'))
cpu_format = cpu_helpers['format_cpu']
def cpu_fields(path): return cpu_helpers['fields_from_symbols'](path.read_text('utf-8'))


def build():
    output = ROOT / 'bin'
    for name, source, origin, flags, defines in (
        ('NC', 'NC.asm', 0, 4, []),
        ('NC_CORE', 'NC.asm', 0x100, 2, ['-DNC_BANKED=1']),
    ):
        subprocess.run([str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        *defines, f'--raw={output / name}.i80',
                        f'--sym={output / name}.sym', f'--lst={output / name}.lst',
                        source], cwd=ROOT / 'source/nc', check=True)
        (output / f'{name}.cpu').write_text(
            cpu_format(origin,flags=flags,fields=cpu_fields(output/f'{name}.sym')), encoding='ascii')

    subprocess.run([sys.executable, str(ROOT/'scripts/build/build-driver.py'),
                    str(ROOT/'source/nc/NC_DOS.asm')], check=True)
    for stem, filename in [('NC_DOS', 'NC.com'), ('NC', 'NC_MN.com')]:
        descriptor = output / f'{stem}.cpu'
        image = exe['decode'](descriptor.read_bytes(), 'cpu', path=descriptor)
        (output / filename).write_bytes(exe['encode'](image, 'com'))


if __name__ == '__main__':
    build()
