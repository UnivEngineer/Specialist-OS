"""Технология в панели NC и сохранность числовых полей; отдельный Emu80."""
import argparse
import json
import runpy
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
h=runpy.run_path(str(ROOT/'scripts/emu80/check-flash-allocation.py'))
Machine,symbols=h['Machine'],h['symbols']
Bench=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))['Bench']


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--flash',action='store_true')
    args=p.parse_args()
    assert args.runtime.name=='Emu80-flash-test'
    m=Machine('http://127.0.0.1:19781/mcp')
    ns=symbols(ROOT/'bin/NC.lst');ds=symbols(ROOT/'bin/DOS.lst')
    code=(ROOT/'bin/NC.i80').read_bytes()[:ns['state']-SYSTEM['NC_LOAD_ADDR']]
    m.call('debug_break');m.call('bp_clear')
    for _ in range(180):
        m.call('emu_run_for',ms=500)
        if m.read(SYSTEM['NC_LOAD_ADDR'],len(code))==code:break
    else:raise RuntimeError('NC не загружен')
    out=ROOT/'docs/nc/technology'
    out.mkdir(parents=True,exist_ok=True)
    results={}
    scenarios=[(0,'Flash'),(7,'Flash')] if args.flash else [(0,'ROM'),(1,'RAM'),(1,'n/a')]
    for index,(drive,expected) in enumerate(scenarios):
        m.call('bp_clear');m.call('bp_set',addr=0x8100,type='exec')
        assert not m.subroutine(0xC842,af=drive<<8,de=1)['flags']['C']
        total=m.subroutine(0xC863,bc=0x0300)['registers']['de']['dec']//4
        free=m.subroutine(0xC872)['registers']['de']['dec']//4
        mem=int.from_bytes(m.read(ds['v_memTop'],2),'little')+1
        extended=m.read(0x8FF8,1)[0]*64
        original=m.read(0xC863,3)
        if expected=='n/a':
            # Только запрос описания отвергается; B=2/3 остаются настоящими.
            target=original[1]|original[2]<<8
            m.write(0x7000,bytes((0x78,0xFE,0xFF,0xCA,0x09,0x70,
                                0xC3,target&255,target>>8,0x21,0,0,0x37,0xC9)))
            m.write(0xC863,bytes.fromhex('c30070'))
        texts=[];numbers=[];pads=[]
        try:
            m.write(ns['state']+1,bytes([index&1]))
            m.write(0x8000,bytes((0xCD,ns['printInfoPanel']&255,ns['printInfoPanel']>>8,0xC3,0,0x81)))
            m.call('regs_set',pc=0x8000,sp=0x8F00)
            for addr in (0xC818,0xC86C):m.call('bp_set',addr=addr,type='exec')
            for _ in range(180):
                m.call('emu_run_for',ms=500)
                state=m.call('cpu_state');pc=state['registers']['pc']['dec']
                if pc==0x8100:break
                if pc==0xC818:
                    ptr=state['registers']['hl']['dec']
                    texts.append(m.read(ptr,min(64,65536-ptr)).split(bytes([0]))[0].decode('ascii'))
                elif pc==0xC86C:
                    numbers.append(state['registers']['de']['dec'])
                    pads.append(state['registers']['bc']['dec']>>8)
                else:continue
                m.call('debug_step',mode='into')
            else:raise RuntimeError('Панель не вернула управление')
            assert texts[texts.index('Technology: ')+1]==expected,texts
            assert len(numbers)==7 and numbers[:5]==[65504,mem,extended,total,free],numbers
            assert numbers[-1]==total-free and pads==[32]*6+[0],(numbers,pads)
            key=('flash' if args.flash else 'rom')+f'-{drive}-{expected.replace("/","-")}'
            Bench(m.url).screenshot(out/(key+'.png'))
            results[key]=dict(technology=expected,numbers=numbers,padding=pads)
        finally:m.write(0xC863,original)
    (out/('flash.json' if args.flash else 'rom.json')).write_text(json.dumps(results,indent=2)+'\n')
    print(results)


if __name__=='__main__':main()
