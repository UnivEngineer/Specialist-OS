"""Автономная сборка исторических копировщиков, без пересборки DOS."""
from pathlib import Path
import runpy
import subprocess
import re
import sys

HERE = Path(__file__).absolute().parent
ROOT = HERE.parents[1]
OUT = ROOT / 'bin/copybench'
VARIANTS = ('stack', 'loop', 'unrolled', 'direct', 'regs2', 'regs4', 'regs6')

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for name in (*VARIANTS, 'builtin_byte', 'builtin_block', 'CPMCOPY'):
        source = HERE / 'CPMCOPY.asm'
        if name != 'CPMCOPY':
            source = OUT / (name + '.asm')
            source.write_text('    ORG 02000h\nIO_PAGE_RAM=0FFFCh\n'
                              'IO_PAGE_RAMD=0FFFDh\nNC_STATE_ADDR=0FFC0h\n'
                              f'    INCLUDE "{(HERE/(name+".inc")).as_posix()}"\n',
                              encoding='utf-8')
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        f'--raw={OUT/name}.bin', f'--sym={OUT/name}.sym',
                        f'--lst={OUT/name}.lst', source.as_posix()], check=True)

    lines = ['; Генерируется build.py: локальные снимки копировщиков.']
    for name in VARIANTS:
        sym = {k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)', (OUT/(name+'.sym')).read_text(), re.M)}
        offset = sym['ramdTailImage']-0x2000
        size = sym['ramdTailImageEnd']-sym['ramdTailImage']
        lines += [f'{name}Blob: INCBIN "copybench/{name}.bin"',
                  f'{name}Desc: DW {name}Blob,{(OUT/(name+".bin")).stat().st_size},{name}Blob+{offset},{size},{name}Name']
    lines += ['cpmBlob: INCBIN "copybench/CPMCOPY.bin"',
              'variantTable: DW '+','.join(n+'Desc' for n in VARIANTS)+',0']
    (OUT.parent/'COPYBEN.variants.inc').write_text('\n'.join(lines)+'\n',encoding='utf-8')

    # Private comparison candidates; not added to COPYBEN.COM or system drivers.
    for name, source, defines in [
        ('r4-reference', 'r4-reference.asm', []),
        ('near-engines', 'near-engines.asm', []),
        ('cpm-local16', 'cpm-local.asm', ['CHUNK=16']),
        ('cpm-local32', 'cpm-local.asm', ['CHUNK=32']),
        ('cpm-local32-aligned256', 'cpm-local.asm', ['CHUNK=32', 'ALIGNED=1']),
        ('cpm-local32-aligned-batch', 'cpm-local.asm', ['CHUNK=32', 'ALIGNED=2']),
    ]:
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        *['-D'+value for value in defines], f'--raw={OUT/name}.bin',
                        f'--sym={OUT/name}.sym', f'--lst={OUT/name}.lst',
                        (HERE/source).as_posix()], check=True)

if __name__ == '__main__':
    python_build = runpy.run_path(str(ROOT / 'scripts/build/build_cache.py'))['python_build']
    sys.exit(python_build(__file__, 'COPYBEN variants', main,
                          asm=(HERE / name for name in (
                              *(name + '.inc' for name in (*VARIANTS, 'builtin_byte', 'builtin_block')),
                              'CPMCOPY.asm', 'r4-reference.asm', 'near-engines.asm', 'cpm-local.asm')),
                          inputs=(), input_content=True,
                          outputs=('bin/COPYBEN.variants.inc',), scan_outputs=(OUT,)))
