"""Планы DISKBEN: автоматические буферы, RAM и зарезервированный RND механики."""
import argparse
import json
import runpy
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))
Bench,symbols=h['Bench'],h['symbols']


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--report-directory',type=Path,default=ROOT/'docs/diskbench/ui-004')
    args=p.parse_args()
    assert args.runtime.name=='Emu80-flash-test'
    m=Bench('http://127.0.0.1:19781/mcp')
    m.call('debug_break');m.stop_at(0x8100)
    s=symbols(ROOT/'source-apps/diskbench/build/DISKBEN.sym')
    binary=(ROOT/'bin/DISKBEN.com').read_bytes()
    m.write(0,binary)
    matrix=[]
    for technology,drive,size,expected in [('F',0,256,1),('O',0,256,1),
        ('F',7,256,0),('R',1,256,0),('O',7,256,0),('F',0,512,0),
        ('D',0,512,2),('D',7,256,2),('H',1,512,2),('\0',0,256,0)]:
        m.write(s['v_technology'],technology.encode())
        m.write(s['v_drive'],bytes([drive]))
        m.write(s['v_sectorSize'],size.to_bytes(2,'little'))
        m.subroutine(s['select_layout'])
        assert m.read(s['v_layout'],1)==bytes([expected])
        matrix.append(dict(technology=technology,drive=drive,sectorSize=size,layout=expected))

    # Настоящий RAM-драйвер, но метка технологии в тестовой программе
    # заменена на D для проверки заготовленного порядка RND. Записи нет.
    m.write(0,binary);m.stop_at(s['menu_drive']);m.call('regs_set',pc=0);m.until(s['menu_drive'])
    m.stop_at(s['run_tests']);m.line('b');m.line('0');m.until(s['run_tests'])
    assert m.read(s['v_technology'],1)==b'R' and m.read(s['v_layout'],1)==b'\0'
    m.write(s['v_technology'],b'D');m.write(s['v_layout'],b'\2')
    start=int.from_bytes(m.read(s['v_start'],2),'little')
    orders=[]
    for case in (0,1):
        m.stop_at(s['io_call'])
        order=[]
        for _ in range(32):
            state=m.until(s['io_call'])
            assert m.read(s['v_case'],1)==bytes([case])
            order.append(state['registers']['de']['dec']-start)
            m.call('debug_step',mode='into')
        assert sorted(order)==list(range(32))
        if case==0:assert order==list(range(1,32))+[0]
        else:
            value=0;expected=[]
            for _ in range(32):
                value=(13*value+7)&31;expected.append(value)
            assert order==expected and order!=orders[0]
        orders.append(order)
        m.stop_at(s['print_result']);m.until(s['print_result']);m.call('debug_step',mode='into')
    m.stop_at(s['exit_wait']);m.until(s['exit_wait'])
    args.report_directory.mkdir(parents=True,exist_ok=True)
    m.screenshot(args.report_directory/'mechanical-stub.png')
    report=dict(layoutMatrix=matrix,randomOrder=orders[1],sequentialOrder=orders[0],
                physicalFloppyOrHddTested=False,realBackend='RAMD',writeBudget=0)
    (args.report_directory/'layouts.json').write_text(json.dumps(report,indent=2)+'\n')
    print('10 layout cases, RAM input without extra questions, mechanical SEQ/RND stub OK')


if __name__=='__main__':main()
