"""B=FFh, B=5, панель NC; только отдельный Emu80-flash-test."""
import argparse
import json
import runpy
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
helpers = runpy.run_path(str(ROOT/'scripts/emu80/check-flash-allocation.py'))
Machine, symbols = helpers['Machine'], helpers['symbols']
Bench = runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))['Bench']


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', type=Path, required=True)
    p.add_argument('--ram-only',action='store_true',help='Проверить B=FFh RAMD Per-byte и R4 без записи')
    p.add_argument('--report-directory',type=Path,default=ROOT/'docs/flash/driver-api')
    p.add_argument('--locked', action='store_true', help='Проверить отказ B=5 при запрете J4')
    args = p.parse_args()
    assert args.runtime.name == 'Emu80-flash-test'
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('debug_break'); m.call('bp_clear')
    ncsyms = symbols(ROOT/'bin/NC.lst')
    ds = symbols(ROOT/'bin/DOS.lst')
    code = (ROOT/'bin/NC.i80').read_bytes()[:ncsyms['state']-SYSTEM['NC_LOAD_ADDR']]
    for _ in range(180):
        m.call('emu_run_for', ms=500)
        if m.read(SYSTEM['NC_LOAD_ADDR'],len(code)) == code:
            break
    else:
        raise RuntimeError('NC не загружен')
    m.call('bp_set', addr=0x8100, type='exec')
    report = {'descriptors': {}, 'writes': {}, 'mismatchRejected': False,
              'readFailureRejected': False,
              'systemImageBytes': (args.runtime/'specialist-os/system-flash.bin').stat().st_size}
    out = args.report_directory
    out.mkdir(parents=True,exist_ok=True)

    def select(drive):
        assert not m.subroutine(0xC842, af=drive<<8, de=1)['flags']['C']

    def info(expected, entry=0xC863):
        state = m.subroutine(entry, bc=0xFF00, hl=0)
        assert not state['flags']['C']
        addr = state['registers']['hl']['dec']
        name, description = m.read(addr,64).split(b'\0')[:2]
        assert name.decode() == expected, (name,expected)
        assert len(name)<=22 and 0<len(description)<=30
        bc=state['registers']['bc']['dec']
        technology,capabilities=bc>>8,bc&255
        assert technology==ord('R' if expected=='RAMD' else 'O' if expected=='ROM' else 'F')
        assert capabilities==dict(RAMD=3,ROM=1,FLASH=7,FLASHFST=15,FLASHSYS=7)[expected]
        report['descriptors'][expected+'/'+description.decode()] = dict(address=addr,
            technology=chr(technology),capabilities=capabilities)

    def install(name):
        select(0)
        m.install_flash(name)
        select(7)

    if args.ram_only:
        select(1)
        # Штатный AUTOEXEC уже установил R4; явно выбираем встроенный Per-byte.
        m.subroutine(0xC860,af=0x0100,hl=ds['diskDriver'])
        m.subroutine(0xC86F,bc=0x0200,hl=ds['driverByteStart'])
        m.subroutine(0xC86F,bc=0)
        info('RAMD')
        assert 'RAMD/Per-byte' in report['descriptors']
        m.write(0x7000,b'A:RAMD.COM\0')
        assert not m.subroutine(0xC85D,hl=0x7000)['flags']['C']
        select(1)
        info('RAMD')
        assert 'RAMD/R4 banked' in report['descriptors']
        (out/'ramd.json').write_text(json.dumps(report['descriptors'],indent=2)+'\n')
        print('RAMD Per-byte/R4 B=FFh: name, description, technology R, capabilities 03h OK')
        return

    if args.locked:
        image=args.runtime/'specialist-os/system-flash.bin'
        before=image.read_bytes()
        select(0)
        m.write(0x4000,bytes(b^255 for b in before[-256:]))
        m.write(0x8FF9,b'\0')
        state=m.subroutine(0xC863,bc=0x0500,de=len(before)//256-1,hl=0x4000)
        assert state['flags']['C'] and m.read(0x8FF9,1)==b'\1'
        assert state['registers']['sp']['dec']==0x8F00 and image.read_bytes()==before
        (out/'locked.json').write_text(json.dumps(dict(checkedWriteRejected=True,
            imageUnchanged=True,stackRestored=True),indent=2)+'\n')
        print('B=5: J4 rejection, unchanged image and restored stack OK')
        return

    absent=m.subroutine(ds['diskDriverDummy'],bc=0xFF00,hl=0xBEEF)
    assert absent['flags']['C'] and absent['registers']['hl']['dec']==0
    select(1); info('RAMD')
    m.subroutine(0xC860, af=0, hl=ds['diskDriver'])
    select(0); info('ROM')
    m.install_flash('FlashSys')
    select(1); select(0); info('FLASHSYS')

    # Полная запись и повторное чтение для трёх flash-драйверов.
    # Используется только последний сектор тестовых образов, FAT не изменяется.
    for name in ('Flash','FlashFst','FlashSys'):
        if name == 'FlashSys':
            select(0); sector=(args.runtime/'specialist-os/system-flash.bin').stat().st_size//256-1
        else:
            install(name); sector=8191
        info(name.upper())
        ticks={}
        for mode in (1,5):
            payload=bytes((i*31+mode*17)&255 for i in range(256))
            m.write(0x4000,b'\xDA'+payload+b'\xAD')
            m.write(0x8DC0,bytes([0xCD])*32)
            before=m.call('cpu_state')['cpu_ticks']
            state=m.subroutine(0xC863,bc=mode<<8|0xA7,de=sector,hl=0x4001)
            ticks[str(mode)]=state['cpu_ticks']-before
            assert not state['flags']['C'], (name,mode)
            assert all(state['registers'][k]['dec']==v for k,v in
                       dict(bc=mode<<8|0xA7,de=sector,hl=0x4001,sp=0x8F00).items())
            assert m.read(0x8DC0,32)==bytes([0xCD])*32
            assert m.read(0x4000,258)==b'\xDA'+payload+b'\xAD'
            assert not m.subroutine(0xC863,bc=0x0200,de=sector,hl=0x5000)['flags']['C']
            assert m.read(0x5000,256)==payload
        report['writes'][name]=ticks

    install('FlashFst')
    fs=symbols(ROOT/'bin/FlashFst.lst')
    # Ошибка B=2 должна вернуться из B=5, а не потеряться после освобождения стека.
    saved=m.read(fs['FuncRead'],4)
    m.write(fs['FuncRead'],bytes((0x37,0xC3,fs['Exit']&255,fs['Exit']>>8)))
    m.write(0x8FF9,b'\0')
    state=m.subroutine(0xC863,bc=0x0500,de=8191,hl=0x4001)
    assert state['flags']['C'] and m.read(0x8FF9,1)==b'\1'
    m.write(fs['FuncRead'],saved)
    report['readFailureRejected']=True

    # После настоящего чтения меняем байт временного буфера до сравнения.
    m.call('bp_set',addr=ds['dwcCompare'],type='exec')
    m.write(0x8000,b'\xCD\x63\xC8\xC3\x00\x81')
    m.write(0x8FF9,b'\0')
    m.call('regs_set',pc=0x8000,sp=0x8F00,bc=0x0500,de=8191,hl=0x4001)
    for _ in range(120):
        m.call('emu_run_for',ms=500)
        state=m.call('cpu_state')
        if state['registers']['pc']['dec']==ds['dwcCompare']: break
    else: raise RuntimeError('Сравнение не достигнуто')
    scratch=state['registers']['hl']['dec']
    m.write(scratch,bytes((m.read(scratch,1)[0]^1,)))
    m.call('bp_clear');m.call('bp_set',addr=0x8100,type='exec')
    m.call('emu_run_for',ms=100)
    state=m.call('cpu_state')
    assert state['registers']['pc']['dec']==0x8100
    assert state['flags']['C'] and m.read(0x8FF9,1)==b'\1'
    assert state['registers']['sp']['dec']==0x8F00
    report['mismatchRejected']=True

    # Панель читает дескриптор после каждой замены драйвера без перезапуска NC.
    for name in ('Flash','FlashFst'):
        install(name)
        m.write(ncsyms['state']+1,b'\0')  # NC_STATE.activePanel
        m.subroutine(ncsyms['printInfoPanel'])
        Bench(m.url).screenshot(out/f'info-{name}.png')
    select(0)
    m.write(SYSTEM['DISK_DRIVER_ADDR'],(ROOT/'bin/Rom.i80').read_bytes())
    m.subroutine(SYSTEM['DISK_DRIVER_ADDR'])
    info('ROM',symbols(ROOT/'bin/Rom.lst')['Driver'])
    (out/'results.json').write_text(json.dumps(report,indent=2)+'\n')
    (out/f"results-{report['systemImageBytes']//1024}.json").write_text(json.dumps(report,indent=2)+'\n')
    print(report)


if __name__=='__main__':
    main()
