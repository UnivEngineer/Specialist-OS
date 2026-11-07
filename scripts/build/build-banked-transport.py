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
        f'{name} = 0{sy[name]:04X}h\n' for name in ('bankCall','bankCallPacket','bankCallback','bankCopyFar','bankCopySector','bankPageCount','bankPage','bankOwner','bankHome','bankDepth','bankServiceSP','bankStackFloor')),encoding='utf-8')
    with (OUT/'transport-imports.inc').open('a',encoding='utf-8') as stream:
        stream.write(f'BANK_TRANSPORT_BYTES = {(OUT/"TRANSPORT.bin").stat().st_size}\n')
    for name in ('DISKIO','FLASHDRV','FASTDRV','ROMDRV','MAINIO','MAINBIOS','CLIENT'):
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        f'--lst={OUT/name}.lst', f'--sym={OUT/name}.sym',
                        f'--raw={OUT/name}.bin', str(ROOT/f'source/banked/{name}.asm')], check=True)
    for kind in range(3):
        name = f'LOCALDRV{kind}'
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                        f'-DLOCAL_KIND={kind}', f'--lst={OUT/name}.lst',
                        f'--sym={OUT/name}.sym', f'--raw={OUT/name}.bin',
                        str(ROOT/'source/banked/LOCALDRV.asm')], check=True)

    def symbols(path):
        return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(),re.M)}
    with (OUT/'flash-imports.inc').open('w',encoding='utf-8') as stream:
        for kind in (1,2):
            ls=symbols(OUT/f'LOCALDRV{kind}.sym')
            for key in ('SectorLimit','v_chipMap'):
                stream.write(f'local{kind}{key} = {ls[key]-ls["Driver"]}\n')
        cs=symbols(OUT/'CLIENT.sym')
        stream.write(f'LOCAL_DRIVER_KIND = 0{cs["localDriverKind"]:04X}h\n')

if __name__ == '__main__':
    main()
