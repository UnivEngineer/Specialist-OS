"""Build the independent MAIN SORTBEN.COM benchmark and its two kernel variants."""
from pathlib import Path
import subprocess

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]


def main():
    folder = HERE/'build'
    folder.mkdir(exist_ok=True)
    source = (HERE/'comparison.inc').read_text(encoding='utf-8-sig')
    kernels = (HERE/'kernels.inc').read_text(encoding='utf-8-sig')
    (folder/'sort-radix.inc').write_bytes((HERE/'sort-radix.inc').read_bytes())
    for name,prepared in (('raw',False),('upper',True)):
        core = source.replace('NC_COMPARE_START', name.upper()+'_COMPARE_START')
        if prepared:
            core = core.replace('    call bios_upperCase','    ; already normalized')
        (folder/f'{name}-core.inc').write_text(core,encoding='utf-8')
        fixture = kernels.replace('@TIMING@',(ROOT/'source/include/timing-pit.inc').as_posix())
        fixture = fixture.replace('@CORE@',f'{name}-core.inc')
        (folder/f'{name}-kernels.inc').write_text(fixture,encoding='utf-8')
    output = ROOT/'bin'
    output.mkdir(exist_ok=True)
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',
                    '--raw='+str(output/'SORTBEN.i80'),'--sym='+str(output/'SORTBEN.sym'),
                    '--lst='+str(output/'SORTBEN.lst'),'SORTBEN.asm'],cwd=HERE,check=True)
    (output/'SORTBEN.cpu').write_bytes(b'0000\r\n0000\r\n04\r\n')


if __name__=='__main__':
    main()
