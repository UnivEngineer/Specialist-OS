"""Link each DOS overlay against the current kernel addresses."""
import argparse
import os
from pathlib import Path
import re
import runpy
import sys

from build_cache import main as cached_build

ROOT = Path(__file__).absolute().parents[2]
OVERLAYS = {
    'ENUM': 'enumerate',
    'FILEOPS': 'fileops',
}
KERNEL = ROOT / 'bin/banked/KERNEL.sym'


def build(name, output_dir=ROOT / 'bin', kernel=KERNEL, defines=()):
    component = OVERLAYS[name]
    pack = runpy.run_path(str(ROOT / 'scripts/build/pack-dos.py'))
    cpu = runpy.run_path(str(ROOT / 'scripts/images/cpu.py'))
    symbols = pack['symbols'](kernel)
    host = ROOT / f'source/include/{component}-host.inc'
    names = re.findall(r'^\s*DW (\S+)', host.read_text(encoding='utf-8'), re.M)
    imports = output_dir / f'{component}-imports.inc'
    imports.write_text(''.join(f'{symbol} = 0{symbols[symbol]:04X}h\n' for symbol in names), encoding='utf-8')

    output = output_dir / f'{name}.i80'
    pack['assemble'](ROOT / f'source/{component}/{name}.asm', output, defines)
    listing = output.with_suffix('.sym')
    meta = pack['symbols'](listing)
    origin = 0
    if meta['IMAGE_MEM_END'] > symbols['DOS_BANK_DRIVER_END'] - symbols['DOS_ARENA_START']:
        raise ValueError(f'{name} exceeds the DOS arena')
    fields = cpu['fields_from_symbols'](listing.read_text(encoding='utf-8'))
    output.with_suffix('.cpu').write_text(cpu['format_cpu'](origin, flags=1, fields=fields), encoding='ascii')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--force', action='store_true')
    parser.add_argument('--run-build', choices=OVERLAYS, help=argparse.SUPPRESS)
    args = parser.parse_args()
    if args.run_build:
        build(args.run_build)
        return 0

    os.chdir(ROOT)
    for name, component in OVERLAYS.items():
        imports = ROOT / f'bin/{component}-imports.inc'
        options = [name, '--input-content', '--asm', str(ROOT / f'source/{component}/{name}.asm'),
                   '--exclude', str(imports), '--exclude', str(ROOT / 'bin/mx'), '--output', str(imports)]
        for path in (KERNEL, ROOT / 'scripts/build/pack-dos.py', ROOT / 'scripts/images/cpu.py'):
            options.extend(('--input', str(path)))
        for extension in ('i80', 'cpu', 'lst', 'sym'):
            options.extend(('--output', str(ROOT / f'bin/{name}.{extension}')))
        if args.force:
            options.append('--force')
        result = cached_build([*options, '--', __file__, '--run-build', name])
        if result:
            return result
    return 0


if __name__ == '__main__':
    sys.exit(main())
