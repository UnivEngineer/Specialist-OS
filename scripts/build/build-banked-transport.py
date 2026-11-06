"""Отдельная сборка банкового транспорта; штатные DOS/ROM не заменяет."""
from pathlib import Path
import subprocess
import re

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'bin/banked'

def main():
    OUT.mkdir(parents=True, exist_ok=True)
    for name in ('WINDOW', 'TRANSPORT'):
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        f'--lst={OUT/name}.lst', f'--sym={OUT/name}.sym',
                        f'--raw={OUT/name}.bin', str(ROOT/f'source/banked/{name}.asm')], check=True)
    sy = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
                                          (OUT/'TRANSPORT.sym').read_text(),re.M)}
    (OUT/'transport-imports.inc').write_text('; Генерируемые адреса внутреннего транспорта.\n'+''.join(
        f'{name} = 0{sy[name]:04X}h\n' for name in ('bankCall','bankCallPacket','bankCallback','bankCopyFar','bankPageCount','bankPage','bankOwner','bankHome','bankDepth','bankServiceSP','bankStackFloor')),encoding='utf-8')
    with (OUT/'transport-imports.inc').open('a',encoding='utf-8') as stream:
        stream.write(f'BANK_TRANSPORT_BYTES = {(OUT/"TRANSPORT.bin").stat().st_size}\n')
    for name in ('DISKIO','FLASHDRV','FASTDRV','ROMDRV','MAINIO','MAINBIOS','CLIENT'):
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        f'--lst={OUT/name}.lst', f'--sym={OUT/name}.sym',
                        f'--raw={OUT/name}.bin', str(ROOT/f'source/banked/{name}.asm')], check=True)

if __name__ == '__main__':
    main()
