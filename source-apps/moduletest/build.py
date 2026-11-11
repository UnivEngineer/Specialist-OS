"""Автономные проверочные DLL-модули; формат тот же, что в сборщике образов."""
from pathlib import Path
import runpy,subprocess,re
R=Path(__file__).resolve().parents[2]
O=R/'bin/moduletest';O.mkdir(exist_ok=True)
e=runpy.run_path(str(R/'scripts/images/executable.py'))
for name,source,defines,origin in [('BANKMOD','MODULE',['-DMAIN_MODULE=0'],0x100),('MAINMOD','MODULE',['-DMAIN_MODULE=1'],0x6000),('DOSMOD','MODULE',['-DMAIN_MODULE=2'],None),('MRET','MRET',[],0x100),('MODTEST','MODTEST',[],0),('BANKAPP','BANKAPP',[],0x100),('MAINAPP','MAINAPP',[],0),('DOSAPP','DOSAPP',[],0x9000),('DRVPROBE','DRVPROBE',[],None)]:
    subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',*defines,f'--raw={O/name}.bin',f'--sym={O/name}.sym',str(R/f'source-apps/moduletest/{source}.asm')],check=True)
    symbols=(O/f'{name}.sym').read_text('utf-8')
    flags=re.search(r'^IMAGE_FLAGS: EQU 0x([0-9A-Fa-f]+)',symbols,re.M)
    if name in ('DOSAPP','DOSMOD','DRVPROBE'): origin=int(re.search(r'^DOS_BANK_DRIVER_ADDR: EQU 0x([0-9A-Fa-f]+)',symbols,re.M)[1],16)
    data=e['encode'](e['Image']((O/f'{name}.bin').read_bytes(),origin,origin,int(flags[1],16) if flags else 0),'com')
    ext='COM' if name in ('MODTEST','BANKAPP','MAINAPP','DOSAPP','DRVPROBE') else 'DLL'
    (O/f'{name}.{ext}').write_bytes(data)
