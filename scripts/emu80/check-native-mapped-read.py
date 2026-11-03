"""Штатный ROM-драйвер: чтение, регистры и такты в отдельном Emu80-flash-test."""
import argparse
import hashlib
import json
import runpy
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
h = runpy.run_path(str(Path(__file__).with_name('check-mapped-read.py')))
MappedBench, symbols = h['MappedBench'], h['symbols']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', required=True, type=Path)
    p.add_argument('--layout', choices=['linear', 'mx2'], default='linear')
    p.add_argument('--baseline', type=Path)
    p.add_argument('--report-directory', type=Path, default=ROOT/'docs/flash/native-read-001')
    args = p.parse_args()
    assert args.runtime.name == 'Emu80-flash-test'
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    listener = subprocess.run(['powershell.exe','-NoProfile','-Command',query],
                              capture_output=True,text=True,check=True).stdout.strip()
    assert Path(listener).resolve() == (args.runtime/'Emu80qt-mcp.exe').resolve()
    out = args.report_directory.resolve()
    out.mkdir(parents=True, exist_ok=True)
    legacy = out/'DOS-unchecked.sys'
    subprocess.run([sys.executable,str(ROOT/'scripts/build/pack-dos.py'),
                    '--define=DISK_DRIVER_CHECKED=0','--output',str(legacy)],check=True)
    variants = {'checked': ROOT/'bin/DOS.sys', 'unchecked': legacy}
    if args.baseline:
        variants = {'before': args.baseline, **variants}
    m = MappedBench('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    ns = symbols(ROOT/'bin/NC.sym')
    if m.read(SYSTEM['NC_LOAD_ADDR'],32) != (ROOT/'bin/NC.i80').read_bytes()[:32]:
        m.stop_at(SYSTEM['NC_LOAD_ADDR']);m.until(SYSTEM['NC_LOAD_ADDR'])
    # До перезаписи DOS завершаем загрузку и фиксируем главный цикл NC.
    m.stop_at(ns['mainLoop'])
    if m.call('cpu_state')['registers']['pc']['dec'] != ns['mainLoop']:
        m.call('kbd_send', keys=['ESC'], action='tap', duration_ms=100)
    m.until(ns['mainLoop'])
    m.screenshot(out/f'{args.layout}-boot.png')
    saved = m.read(0xC000,0x1A00)
    if args.layout == 'linear':
        image = args.runtime/'specialist-os/system-flash.bin'
        original = image.read_bytes()
        total = len(original)//256
        sectors = [0,1,128,256]+[bank*128+127 for bank in range(total//128)]
    else:
        image = args.runtime/'specialist-os/system.bin'
        original = image.read_bytes()
        total = 256
        sectors = [0,1,127,128,254,255]
    changed = []
    fs = symbols(ROOT/'bin/FlashSys.sym')['Driver']
    report = {'status':'emulator-only', 'cpu_hz':2000000, 'layout':args.layout,
              'physical_hardware_measured':False, 'variants':{}}
    try:
        if args.layout == 'linear':
            for sector in sectors[4:]:
                changed.append(sector)
                payload = bytes((i*37+sector*13+(sector>>7)*19)&255 for i in range(256))
                m.write(0x4000,payload)
                state,_ = m.subroutine(fs,bc=0x100,de=sector,hl=0x4000)
                assert not state['flags']['C']
            logical = image.read_bytes()
        else:
            logical = original[32768:]+original[4:32768]+b'\xFF'*4
        for name,path in variants.items():
            binary = path.read_bytes()
            ds = symbols(path.with_suffix('.sym'))
            m.write(0xC000,binary)
            m.write(ds['v_drive'],b'\0')
            m.write(ds['v_diskInfo']+10,total.to_bytes(2,'little'))
            m.write(0xC864,ds['diskDriver'].to_bytes(2,'little'))
            cases = {}
            for address in [0,1,0x2000,0x7F00,0x7F01,0x7FFF,0x8000,0x8001,0x8200,0x83FF,0x8C00,
                            0xEEFF,0xEF00,0xEF01,0xEFFF,0xF000,0xF100]:
                counts = {}
                for sector in sectors:
                    start = address-bool(address)
                    guard = (b'\xDA' if address else b'')+b'\xCC'*256+b'\xAD'
                    m.write(start,guard)
                    # Наличие этого маркера после вызова проверяет возврат RAM.
                    if not start <= 0x6000 < start+len(guard):m.write(0x6000,b'\x6A')
                    state,ticks = m.subroutine(0xC863,af=0x55D7,bc=0x025A,de=sector,hl=address)
                    assert state['flags']['C'] == (name == 'unchecked')
                    assert state['registers']['af']['dec']>>8 == 0x55
                    assert all(state['registers'][k]['dec']==v for k,v in
                               dict(bc=0x025A,de=sector,hl=address,sp=0x8F00).items())
                    n = 252 if args.layout=='mx2' and sector==255 else 256
                    assert m.read(address,n)==logical[sector*256:sector*256+n], (name,address,sector)
                    if address:assert m.read(start,1)==b'\xDA'
                    assert m.read(address+256,1)==b'\xAD'
                    assert m.read(0x6000,1)==b'\x6A'
                    counts[str(sector)]={'cpu_cycles':ticks,'kib_s':500000/ticks}
                cases[f'{address:04X}']=counts
            report['variants'][name]={'bytes':len(binary),'buffers':cases,
                                      'sha256':hashlib.sha256(binary).hexdigest()}
            print(args.layout,name,'2000=',cases['2000']['0'],'8200=',cases['8200']['0'],flush=True)
    finally:
        m.write(0xFFFC,b'\0')
        m.write(0xC000,saved)
        for sector in changed:
            m.write(0x4000,original[sector*256:(sector+1)*256])
            state,_=m.subroutine(fs,bc=0x100,de=sector,hl=0x4000)
            assert not state['flags']['C']
    assert image.read_bytes()==original
    report['image_unchanged']=True
    (out/f'{args.layout}.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':main()
