"""Четыре скорости встроенного R3: файл 8 КиБ, три прохода DISKBEN."""
import argparse
import hashlib
import json
import runpy
from datetime import datetime, timezone
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--report-directory',type=Path,
                   default=ROOT/'docs/diskbench/ramd-r3-001')
    args=p.parse_args()
    assert args.runtime.name=='Emu80-flash-test'
    out=args.report_directory.resolve()
    if (out/'raw.json').exists():
        p.error('Отчёт уже существует; выберите новый --report-directory')
    out.mkdir(parents=True,exist_ok=True)
    helper=runpy.run_path(str(Path(__file__).with_name('benchmark-disks.py')))
    Matrix=helper['Matrix']
    # Используем те же функции DISKBEN и процедуру измерения, что в матрице.
    context=Matrix.__init__.__globals__
    context['OUT']=out
    context['RUNTIME']=args.runtime.resolve()
    matrix=Matrix()
    try:
        matrix.start('ramd-r3')
        matrix.func(0xC860,af=0x0100,hl=matrix.ds['diskDriver'])
        matrix.func(0xC86F,bc=0x0200,hl=matrix.ds['driverBlockStart'])
        matrix.func(0xC86F,bc=0)
        matrix.select(1)
        state=matrix.func(0xC863,bc=0xFF00)
        assert not state['flags']['C']
        ptr=state['registers']['hl']['dec']
        assert matrix.m.read(ptr,40).split(b'\0')[:2]==[b'RAMD',b'R3']
        matrix.format_ram()  # Только тестовый B:, не пользовательский носитель.
        matrix.writable(1,'RAMD',1,'R3')
        matrix.report['completedUtc']=datetime.now(timezone.utc).isoformat()
        matrix.report['binaries']={f:hashlib.sha256((ROOT/'bin'/f).read_bytes()).hexdigest()
                                  for f in ['DOS.sys','DISKBEN.com','RAMD.com']}
        matrix.save()
        tick_ms=256000/matrix.report['pitHz']
        volume=matrix.report['fileBytes']*matrix.report['passes']
        row=matrix.report['rows'][0]
        values={key:(volume/1024)/(row['metrics'][key][0]*tick_ms/1000)
                for key in ['rawRead','rawWrite','fileRead','fileWrite']}
        (out/'speeds.json').write_text(json.dumps({'units':'KiB/s','values':values},indent=2)+'\n',encoding='utf-8')
        print(json.dumps(values,indent=2),flush=True)
    finally:
        matrix.stop()


if __name__=='__main__':
    main()
