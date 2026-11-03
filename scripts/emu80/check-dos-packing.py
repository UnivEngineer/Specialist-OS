"""Сравнение старого DOS, автоматического размещения и разрывов через JP."""
import argparse
import hashlib
import json
import runpy
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
h = runpy.run_path(str(Path(__file__).with_name('check-mapped-read.py')))
Machine, symbols = h['MappedBench'], h['symbols']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--baseline',type=Path,required=True)
    p.add_argument('--split',type=Path,required=True)
    p.add_argument('--report-directory',type=Path,default=ROOT/'docs/dos/packing-001')
    args = p.parse_args()
    assert args.runtime.name == 'Emu80-flash-test'
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    listener = subprocess.run(['powershell.exe','-NoProfile','-Command',query],
                              capture_output=True,text=True,check=True).stdout.strip()
    assert Path(listener).resolve() == (args.runtime/'Emu80qt-mcp.exe').resolve()
    out = args.report_directory
    out.mkdir(parents=True,exist_ok=True)
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    ns = symbols(ROOT/'bin/NC.sym')
    system = (args.runtime/'specialist-os/system-flash.bin').read_bytes()
    results = {}
    variants = {'before':args.baseline,'packed':ROOT/'bin/DOS.sys','split':args.split}
    try:
        for name,path in variants.items():
            ds = symbols(path.with_suffix('.sym'))
            m.write(0xFFFC,b'\0')
            m.write(0xC000,path.read_bytes())
            m.stop_at(ns['mainLoop'])
            m.call('regs_set',pc=0xC009,sp=0)
            m.until(ns['mainLoop'])
            assert m.read(SYSTEM['NC_LOAD_ADDR'],32)==(ROOT/'bin/NC.i80').read_bytes()[:32]
            assert m.read(SYSTEM['SYSTEM_FLASH_ADDR'],8)==m.resident_bytes('FlashSys')[:8]
            assert m.read(SYSTEM['DISK_DRIVER_ADDR'],8)==m.resident_bytes('Flash')[:8]
            m.screenshot(out/f'{name}-boot.png')
            # Курсор без мигания; дальнейшие проверки полностью детерминированы.
            m.write(0x8FE9,b'\0')
            state,_ = m.subroutine(0xC010,bc=0x1234,de=0x5678,hl=0x9ABC)
            assert all(state['registers'][k]['dec']==v for k,v in
                       dict(bc=0x1234,de=0x5678,hl=0x9ABC,sp=0x8F00).items())
            m.write(0x8FEA,b'\0\xFF')
            m.write(0x8FFA,b'\0\0\x08\0')
            text = ('ASCII / АБВГД / абвгд\n'.encode('koi8-r')+
                    b'\tTab\x08X\n'+b'0123456789 '*8+b'\n')
            m.write(0x7000,text+b'\0')
            m.subroutine(0xC818,hl=0x7000)
            graphic = {'text':hashlib.sha256(m.read(0x9000,0x3000)).hexdigest()}
            state,_ = m.subroutine(0xC037,af=0x5A02,bc=0x1241,de=0x5678,hl=0x9ABC)
            assert all(state['registers'][k]['dec']==v for k,v in
                       dict(af=0x5A02,bc=0x1241,de=0x5678,hl=0x9ABC,sp=0x8F00).items())
            lines = b''.join(f'Line {i:02d} ABC xyz\n'.encode() for i in range(30))
            m.write(0x7000,lines+b'\0');m.subroutine(0xC818,hl=0x7000)
            graphic['scroll_up']=hashlib.sha256(m.read(0x9000,0x3000)).hexdigest()
            m.write(0x8FFC,b'\x08\0')
            m.write(0x7000,b'\x1BI\x1BI\x1BI\0');m.subroutine(0xC438,hl=0x7000)
            graphic['scroll_down']=hashlib.sha256(m.read(0x9000,0x3000)).hexdigest()
            m.write(0x8FE9,b'\x21');m.subroutine(0xC006)
            graphic['cursor']=hashlib.sha256(m.read(0x9000,0x3000)).hexdigest()
            m.screenshot(out/f'{name}-drawing.png')
            flags = []
            for left,right in [(0,0),(0,1),(1,0),(255,256),(256,255),
                               (32767,32768),(32768,32767),(65535,65535)]:
                state,_=m.subroutine(0xC427,hl=left,de=right,bc=0x1234)
                assert state['flags']['C']==(left<right) and state['flags']['Z']==(left==right)
                flags.append(state['registers']['af']['dec'])
            copies = []
            for source,destination,length in [(0x4000,0x5001,257),(0x4000,0x4001,255),
                                               (0x4001,0x4000,255),(0x7F80,0x8201,256),
                                               (0x4000,0x5000,0)]:
                m.write(0x3FFF,bytes((i*37)&255 for i in range(0x1010)))
                m.write(0x7F00,bytes((i*17)&255 for i in range(0x500)))
                expected=m.read(source,length)
                state,_=m.subroutine(0xC42D,hl=source,de=source+length,bc=destination)
                assert m.read(destination,length)==expected
                assert all(state['registers'][k]['dec']==v for k,v in dict(
                    hl=source+length,de=source+length,bc=destination+length,sp=0x8F00).items())
                copies.append(hashlib.sha256(expected).hexdigest())
            m.subroutine(0xC842,af=0x100,de=1)
            state,_=m.subroutine(0xC863,bc=0x200,de=0xFFE,hl=0x4001)
            assert not state['flags']['C']
            original=m.read(0x4001,256)
            payload=bytes((i*29+17)&255 for i in range(256))
            try:
                m.write(0x4000,b'\xDA'+payload+b'\xAD')
                state,_=m.subroutine(0xC863,bc=0x100,de=0xFFE,hl=0x4001)
                assert not state['flags']['C']
                m.write(0x4001,b'\0'*256)
                state,_=m.subroutine(0xC863,bc=0x200,de=0xFFE,hl=0x4001)
                assert not state['flags']['C'] and m.read(0x4000,258)==b'\xDA'+payload+b'\xAD'
            finally:
                m.write(0x4001,original);m.subroutine(0xC863,bc=0x100,de=0xFFE,hl=0x4001)
            m.subroutine(0xC842,af=0,de=1)
            for address in (0x4001,0x8201):
                for sector in (0,127,128,2047):
                    state,_=m.subroutine(ds['diskDriver'],bc=0x200,de=sector,hl=address)
                    assert not state['flags']['C']
                    assert m.read(address,256)==system[sector*256:(sector+1)*256]
            results[name]=dict(bytes=path.stat().st_size,graphics=graphic,compareAF=flags,
                               memcpy=copies,ramReadWrite=True,nativeRead=True,boot=True)
            if name!='before':
                for field in ('graphics','compareAF','memcpy'):
                    assert results[name][field]==results['before'][field],(name,field,results)
            print(name,'BIOS graphics/scroll, registers, memcpy, RAM/ROM OK',flush=True)
    finally:
        m.write(0xFFFC,b'\0');m.write(0xC000,(ROOT/'bin/DOS.sys').read_bytes())
        m.stop_at(ns['mainLoop']);m.call('regs_set',pc=0xC009,sp=0);m.until(ns['mainLoop'])
    assert (args.runtime/'specialist-os/system-flash.bin').read_bytes()==system
    (out/'emulator.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')


if __name__=='__main__':main()
