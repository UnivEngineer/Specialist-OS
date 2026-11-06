"""Автономная сборка исторических копировщиков, без пересборки DOS."""
from pathlib import Path
import subprocess
import re

HERE = Path(__file__).resolve().parent
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
                        f'--lst={OUT/name}.lst', str(source)], check=True)

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

if __name__ == '__main__':
    main()
