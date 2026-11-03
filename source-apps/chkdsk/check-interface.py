"""Штатный запуск CHKDSK и вставка символа в середину строки MXOS."""
import argparse,json,runpy,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
h=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--report-directory',type=Path,default=ROOT/'docs/chkdsk');a=p.parse_args()
    assert a.runtime.name=='Emu80-flash-test'
    q='(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    exe=subprocess.run(['powershell.exe','-NoProfile','-Command',q],capture_output=True,text=True,check=True).stdout.strip()
    assert Path(exe).resolve()==(a.runtime/'Emu80qt-mcp.exe').resolve()
    m=h['Bench']('http://127.0.0.1:19781/mcp');m.call('debug_break');m.write(0xFFFC,b'\0')
    s=h['symbols'](ROOT/'bin/CHKDSK.sym');out=a.report_directory;out.mkdir(parents=True,exist_ok=True)
    for command in [b'A:CHKDSK.COM A:\0',b'A:CHKDSK.COM\0']:
        m.write(0x7000,command);m.write(0x8000,bytes.fromhex('cd5dc8c30081'))
        no_arg=command==b'A:CHKDSK.COM\0'
        m.stop_at(s['askDrive'] if no_arg else s['waitKey'])
        m.call('regs_set',pc=0x8000,sp=0x8F00,hl=0x7000)
        if no_arg:
            m.until(s['askDrive'],30);m.stop_at(s['waitKey'])
            # Даём getch принять отпущенные клавиши перед следующим нажатием.
            m.call('emu_run_for',ms=200);m.tap('a')
            assert m.read(s['driveInput'],1).upper()==b'A'
            assert m.call('cpu_state')['registers']['pc']['dec']!=s['waitKey']
            m.screenshot(out/'drive-input.png')
            m.tap('ENTER')
        m.until(s['waitKey'],60)
        result={k:int.from_bytes(m.read(s[k],1 if k=='result' else 2),'little') for k in ['result','errors','checked','skipped']}
        assert result['result']==0 and result['errors']==0,result
        (out/'launch.json').write_text(json.dumps(result,indent=2)+'\n')
        m.screenshot(out/('no-argument.png' if no_arg else 'system-scan.png'))
        m.call('bp_clear');m.tap('SPACE');m.call('emu_run_for',ms=500);m.call('debug_break');m.write(0xFFFC,b'\0')
    m.stop_at(0x8100);m.write(0x3000,b'\xa6'+bytes(17)+b'\xc9')
    m.write(0x8000,bytes.fromhex('cd0fc8c30081'));m.call('regs_set',pc=0x8000,sp=0x8F00,hl=0x3001,de=0x3011)
    m.call('emu_run_for',ms=300)
    for key in ['a','b','c','LEFT','x','ENTER']:m.tap(key)
    m.until(0x8100,30)
    assert m.read(0x3001,6).upper()==b'ABXC\r\0'
    assert m.read(0x3000,1)==b'\xa6' and m.read(0x3012,1)==b'\xc9'
    (out/'interface.json').write_text(json.dumps({'native_fileExec':True,'no_argument_prompt_and_keyboard':True,'drive_input_echo_and_enter':True,'input_middle_insert_and_guards':True})+'\n')
    m.stop_at(SYSTEM['NC_LOAD_ADDR']);m.call('regs_set',pc=0xC000);m.until(SYSTEM['NC_LOAD_ADDR'],30);m.call('bp_clear');m.call('emu_run_for',ms=500)
    print('CHKDSK fileExec/keyboard + MXOS middle insertion: OK')

if __name__=='__main__':main()
