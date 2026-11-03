"""Строковое меню, сводка и запись DISKBEN; только тестовый Emu80."""
import argparse
import json
import runpy
import struct
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
helpers=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))
Bench,symbols=helpers['Bench'],helpers['symbols']


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--rom-only',action='store_true')
    p.add_argument('--report-directory',type=Path,default=ROOT/'docs/diskbench/ui-004')
    args=p.parse_args()
    assert args.runtime.name=='Emu80-flash-test'
    m=Bench('http://127.0.0.1:19781/mcp')
    m.call('debug_break');m.stop_at(SYSTEM['NC_LOAD_ADDR'])
    if m.read(SYSTEM['NC_LOAD_ADDR'],32)!=(ROOT/'bin/NC.i80').read_bytes()[:32]:m.until(SYSTEM['NC_LOAD_ADDR'])
    m.stop_at(0x8100)
    if not args.rom_only:
        m.subroutine(0xC842,af=0,de=1)
        m.install_flash('FlashFst')
    s=symbols(ROOT/'source-apps/diskbench/build/DISKBEN.sym')
    out=args.report_directory
    out.mkdir(parents=True,exist_ok=True)
    runs={}
    scenarios=[('rom','a',None,None),('h-rom','h',None,None)] if args.rom_only else [
        ('h-write','h','0','1'),('h-verify','h','1','1'),
        ('a-write','a','0','1'),('a-verify','','1',''),
        ('h-zero','h','0','0')]
    for name,drive,choice,budget in scenarios:
        system=args.runtime/'specialist-os'/(
            ('rom-disk.bin' if drive=='h' else 'system.bin') if args.rom_only else
            'system-flash.bin' if drive in ('a','') else 'flash-disk.bin')
        initial=system.read_bytes()
        if args.rom_only and drive!='h':
            # Штатная перестановка половин 64-КБ MX2 ROM.
            logical=initial[32768:]+initial[4:32768]+bytes([255])*4
        else:logical=initial
        word=lambda n:struct.unpack_from('<H',logical,n)[0]
        data=word(14)+logical[16]*word(22)+(word(17)*32+255)//256
        m.write(0,(ROOT/'bin/DISKBEN.i80').read_bytes())
        m.stop_at(s['menu_drive']);m.call('regs_set',pc=0);m.until(s['menu_drive'])
        m.stop_at(s['run_tests'])
        m.line(drive)
        if choice is not None:
            m.line(choice)
            # Enter режима не должен сам подтвердить следующий запрос.
            assert m.read(s['v_budget'],1)==bytes([0])
            assert m.call('cpu_state')['registers']['pc']['dec']!=s['run_tests']
            if name=='h-write':
                m.line('12')  # Лишний текст отклоняется целиком, а не становится 1.
                assert m.call('cpu_state')['registers']['pc']['dec']!=s['run_tests']
            m.line(budget)
        m.until(s['run_tests'])
        count=0 if budget=='0' or args.rom_only else 1
        mode=5 if choice=='1' else 1
        assert m.read(s['v_budget'],1)==bytes([count])
        assert m.read(s['v_writeMode'],1)==bytes([mode])
        assert m.read(s['v_technology'],1)==(b'O' if args.rom_only else b'F')
        layout=1 if drive in ('a','') else 0
        assert m.read(s['v_layout'],1)==bytes([layout])
        pointer=int.from_bytes(m.read(s['v_driverInfo'],2),'little')
        driver,description=m.read(pointer,64).split(bytes([0]))[:2]
        assert driver==(b'ROM' if args.rom_only else b'FLASHSYS' if drive in ('a','') else b'FLASHFST')
        m.screenshot(out/(name+'-summary.png'))
        m.stop_at(s['print_result'])
        record={}
        cases=list(range(4) if count else range(2)) if layout else ([0,2] if count else [0])
        for case in cases:
            m.until(s['print_result'])
            assert m.read(s['v_case'],1)==bytes([case])
            ticks=int.from_bytes(m.read(s['v_elapsed'],4),'little')
            assert ticks>0
            expected_buffer=0x8200 if layout and case&1 else 0x2000
            assert int.from_bytes(m.read(s['io_buffer']+1,2),'little')==expected_buffer
            record[str(case)]=ticks
            print(name,'case=',case,'ticks=',ticks,flush=True)
            if case==cases[-1]:
                m.stop_at(s['exit_wait']);m.until(s['exit_wait'])
            else:m.call('debug_step',mode='into')
        after=system.read_bytes()
        if args.rom_only:assert after==initial
        else:assert after[:data*256]==initial[:data*256]
        m.screenshot(out/(name+'-results.png'))
        runs[name]=dict(driver=driver.decode(),description=description.decode(),
            type='ROM' if args.rom_only else 'Flash',writeMode=mode,passes=count,
            metadataUnchanged=True,layout=layout,ticks=record)
        m.stop_at(0x8100)
    if not args.rom_only:
        for drive in ('a','h'):
            assert runs[drive+'-verify']['ticks']['2']>runs[drive+'-write']['ticks']['2']
            if drive=='a':
                assert runs[drive+'-verify']['ticks']['3']>runs[drive+'-write']['ticks']['3']
    (out/('rom.json' if args.rom_only else 'flash.json')).write_text(json.dumps(runs,indent=2)+'\n')


if __name__=='__main__':main()
