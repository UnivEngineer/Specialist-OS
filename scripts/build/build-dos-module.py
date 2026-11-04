"""Сборка DOSMOD по таблице импортов готового резидентного DOS."""
from pathlib import Path
import re,subprocess,struct
ROOT=Path(__file__).resolve().parents[2]

def build(kernel='DOS'):
    symbols={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin'/f'{kernel}.sym').read_text(encoding='utf-8'),re.M)}
    names=re.findall(r'^\s+DW\s+([\w.]+)',(ROOT/'source/dos/moduleImports.inc').read_text(encoding='utf-8'),re.M)
    data=b''.join(struct.pack('<H',symbols[n]) for n in names)
    total=sum(data);prefix=total-data[-1]
    checksum=(((prefix+(prefix>>8))&255)<<8)|(total&255)
    (ROOT/'bin/DOS.module.inc').write_text('; Сгенерировано build-dos-module.py; адреса для парной сборки DOS.\n'+f'DOS_MODULE_IMPORT_SUM EQU 0{checksum:04X}h\n'+''.join(f'{n} EQU 0{symbols[n]:04X}h\n' for n in names),encoding='utf-8')
    stem='DOSMOD' if kernel=='DOS' else 'DOSMOD4000'
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',f'--raw={ROOT}/bin/{stem}.i80',f'--lst={ROOT}/bin/{stem}.lst',f'--sym={ROOT}/bin/{stem}.sym',str(ROOT/'source/dosmod/DOSMOD.asm')],cwd=ROOT/'source/dosmod',check=True)
    (ROOT/'bin'/f'{stem}.cpu').write_text('8000 8000',encoding='ascii')
    print(f'{stem}: {(ROOT/"bin"/(stem+".i80")).stat().st_size} bytes; imports {checksum:04X}')

if __name__=='__main__':
    import sys
    build(sys.argv[1] if len(sys.argv)>1 else 'DOS')
