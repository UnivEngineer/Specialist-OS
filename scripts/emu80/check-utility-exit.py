"""Финальная строка и выход произвольной клавишей DISKBEN/CHKDSK в тестовом Emu80."""
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
    m.call('debug_break')
    args.report_directory.mkdir(parents=True,exist_ok=True)
    results={}
    for name,prompt,wait,sympath in [
        ('DISKBEN','done','exit_wait',ROOT/'source-apps/diskbench/build/DISKBEN.sym'),
        ('CHKDSK','waitPrompt','waitKey',ROOT/'bin/CHKDSK.sym')]:
        s=symbols(sympath)
        binary=(ROOT/f'bin/{name}.com').read_bytes()
        m.write(0,binary)
        assert m.read(s['txtDone']+1,22)==b'Press any key to exit\0'
        m.stop_at(s[wait]);m.call('regs_set',pc=s[prompt],sp=0x8FDF)
        m.until(s[wait])
        m.screenshot(args.report_directory/(name.lower()+'-exit.png'))
        m.stop_at(0xC800);m.call('emu_run_for',ms=100)
        assert m.call('cpu_state')['registers']['pc']['dec']!=0xC800
        m.tap('x');m.until(0xC800)
        results[name]=dict(prompt='Press any key to exit',waited=True,nonEnterKeyExited=True)
    (args.report_directory/'exit.json').write_text(json.dumps(results,indent=2)+'\n')
    print(results)


if __name__=='__main__':main()
