"""Изолированные сборки для исследования; bin/, images/ и настройки не меняются."""
import re
import subprocess
from pathlib import Path

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[2]
OUT=ROOT/'.codex-tmp/ramd-002'
DOS=ROOT/'source/dos'


def absolute_includes(text):
    return re.sub(r'(?im)^(\s*INCLUDE\s+)"([^"]+)"',
        lambda m:m[1]+'"'+(DOS/m[2]).resolve().as_posix()+'"',text)


def main():
    original=(HERE/'stack.inc').read_text(encoding='utf-8')
    installer=original[original.index('installRAMDTails:'):original.index('; Подготовка выполняется')]
    for variant,buffered in [('Legacy',0),('Stack',1),('Loop',1),('Unrolled',1),('Direct',1)]:
        out=OUT/variant;out.mkdir(parents=True,exist_ok=True)
        source=DOS/'DOS.asm'
        if variant in ('Stack','Loop','Unrolled','Direct'):
            body=original if variant=='Stack' else (HERE/(variant.lower()+'.inc')).read_text(encoding='utf-8')
            body=body.replace('; Здесь сборочный сценарий добавляет общий установщик из driverBuffered.inc.',installer)
            (out/'driver.inc').write_text(body,encoding='utf-8')
            gate=absolute_includes((DOS/'driverFFC0.inc').read_text(encoding='utf-8'))
            gate=gate.replace((DOS/'driverBuffered.inc').as_posix(),(out/'driver.inc').as_posix())
            (out/'gate.inc').write_text(gate,encoding='utf-8')
            dos=absolute_includes(source.read_text(encoding='utf-8'))
            dos=dos.replace((DOS/'driverFFC0.inc').as_posix(),(out/'gate.inc').as_posix())
            source=out/'DOS.asm';source.write_text(dos,encoding='utf-8')
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',
            '-DRAMD_BUFFERED='+str(buffered),'--lst='+str(out/'DOS.lst'),
            '--sym='+str(out/'DOS.sym'),'--raw='+str(out/'DOS.sys'),str(source)],cwd=DOS,check=True)
        syms={k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)',(out/'DOS.sym').read_text(),re.M)}
        print(variant,'DOS bytes',(out/'DOS.sys').stat().st_size,
              'tail bytes',syms.get('ramdTailEnd',0xFF00)-0xFF00,flush=True)


if __name__=='__main__':main()
