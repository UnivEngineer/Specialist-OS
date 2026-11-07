"""RAMDISK.COM и встроенный байтовый обмен; только изолированный Emu80-flash-test."""
import argparse
import hashlib
import json
import re
import runpy
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
import sys
sys.path.insert(0,str(ROOT/'scripts/images'))
from executable import Image, encode
Bench = runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))['Bench']


def symbols(path):
    return {k: int(v,16) for k,v in re.findall(
        r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',path.read_text(encoding='utf-8-sig'),re.M)}


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--report',type=Path,default=ROOT/'docs/drivers/ramd-com-001/check.json')
    args=p.parse_args()
    assert args.runtime.name=='Emu80-flash-test'
    # Порт должен принадлежать тестовой установке, прежде чем менять память.
    command='(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    listener=subprocess.run(['powershell.exe','-NoProfile','-Command',command],
        capture_output=True,text=True,check=True).stdout.strip()
    assert Path(listener).resolve()==(args.runtime/'Emu80qt-mcp.exe').resolve()
    ds=symbols(ROOT/'bin/DOS.sym');rs=symbols(ROOT/'bin/RAMDISK.sym');ns=symbols(ROOT/'bin/NC.sym')
    m=Bench('http://127.0.0.1:19781/mcp')
    m.call('debug_break');m.write(0xC000,(ROOT/'bin/DOS.sys').read_bytes())
    m.stop_at(ns['mainLoop'])
    m.call('regs_set',pc=0xC000);m.until(ns['mainLoop'],60)
    report={'checks':[],'cycles':{},'status':'emulator-only'}

    def sub(addr,**regs):
        m.stop_at(0x8100)
        return m.subroutine(addr,**regs)

    def select(drive):
        assert not sub(0xC842,af=drive<<8,de=1)['flags']['C']

    def description():
        s=sub(0xC863,bc=0xFF00)
        assert not s['flags']['C'] and s['registers']['bc']['dec']==0x5203
        return m.read(s['registers']['hl']['dec'],40).split(b'\0')[:2]

    def execute(name):
        m.write(0x7000,name.encode()+b'\0')
        assert not sub(0xC85D,hl=0x7000)['flags']['C'],name

    def check_io(label):
        select(1);report['cycles'][label]={}
        for sector in [0xFE,0x100,0xFFE]:
            for buf in [0x4001,0x40FD,0xFE00]:
                payload=bytes((i*37+sector+buf)&255 for i in range(256))
                # При FE00h следующий байт уже принадлежит коду хвоста.
                guard=m.read(buf+256,1) if buf==0xFE00 else b'\xAD'
                m.write(buf-1,b'\xDA'+payload+guard)
                for mode in [1,2]:
                    if mode==2:m.write(buf,bytes(256))
                    m.stop_at(0x8100)
                    before=m.call('cpu_state')['cpu_ticks']
                    s=m.subroutine(0xC863,af=0x5A02,bc=mode*256+0xA7,de=sector,hl=buf)
                    assert not s['flags']['C'] and s['registers']['af']['dec']>>8==0x5A
                    assert all(s['registers'][k]['dec']==v for k,v in
                        dict(bc=mode*256+0xA7,de=sector,hl=buf,sp=0x8F00).items())
                    assert m.read(buf-1,258)==b'\xDA'+payload+guard
                    if sector==0x100 and buf==0x4001:
                        report['cycles'][label][str(mode)]=s['cpu_ticks']-before-27
        for sector,buf in [(0xFF,0x4001),(0xFFF,0x4001),(0x1000,0x4001),(0x100,0xFE01)]:
            for mode in [1,2]:
                assert sub(0xC863,bc=mode*256,de=sector,hl=buf)['flags']['C']
        assert sub(0xC863,bc=0x0400,de=0x100,hl=0x4001)['flags']['C']
        report['checks'].append(label+': read/write boundaries, guards, registers, invalid arguments')
        print(label,'I/O OK',flush=True)

    select(1)
    assert description()==[b'RAMD',b'R4 banked']
    report['checks'].append('stock AUTOEXEC installs R4 before NC')
    # Для сравнения явно возвращаем встроенный R3 после обычной загрузки.
    sub(0xC860,af=0x0100,hl=ds['diskDriver'])
    sub(0xC86F,bc=0x0200,hl=ds['driverBlockStart']);sub(0xC86F,bc=0)
    assert description()==[b'RAMD',b'R3']
    assert int.from_bytes(m.read(ds['v_drives']+2,2),'little')==ds['diskDriver']
    check_io('R3')
    boot=m.sector(0)
    # Сохраняем boot/FAT/корень до установки. Секторные записи выше — только данные.
    root_start=int.from_bytes(boot[14:16],'little')+boot[16]*int.from_bytes(boot[22:24],'little')
    meta_end=root_start+(int.from_bytes(boot[17:19],'little')*32+255)//256
    metadata=b''.join(m.sector(i) for i in range(meta_end))

    # Настоящий запуск COM с диска A: через NC и стек DC00h.
    m.stop_at(ns['mainLoop']);m.call('regs_set',pc=0xC800);m.until(ns['mainLoop'],60)
    cmd=b'A:RAMDISK.COM'
    m.write(ns['vars.cmdLine'],cmd+b'\0')
    m.write(ns['vars.cmdLineEnd'],(ns['vars.cmdLine']+len(cmd)).to_bytes(2,'little'))
    m.write(0xDBFE,(0xC800).to_bytes(2,'little'))
    m.stop_at(ns['mainLoop']);m.call('regs_set',pc=ns['EnterExec'],sp=0xDBFE)
    m.until(ns['mainLoop'],60)
    select(1)
    assert description()==[b'RAMD',b'R4 banked']
    check_io('R4')
    assert b''.join(m.sector(i) for i in range(meta_end))==metadata
    report['checks'].append('real NC install from A:, boot/FAT/root unchanged')

    def tails():
        main=m.read(0xFF00,175)
        banks=[]
        for page in range(16):
            m.write(0xFFFD,bytes([page]));banks.append(m.read(0xFF00,77));m.write(0xFFFC,b'\0')
        assert all(x==main[:77] for x in banks)
        return main,banks

    expected=tails()
    # Переключение на сервисный ABI: метаданные страницы 0, за кодом и NC.
    state=sub(0xC86F,af=0x5AD7,bc=0x01A7,de=0x1234,hl=0x5678)
    assert all(state['registers'][k]['dec']==v for k,v in
        dict(af=0x5AD7,bc=0x01A7,de=0x1234,hl=0x5678,sp=0x8F00).items())
    saved=sub(0xFFC0,af=0,hl=0xFFBD)['registers']['bc']['dec']&255
    sub(0xFFD0,af=0,bc=0x5A,hl=0xFFBD)
    assert sub(0xFFC0,af=0,hl=0xFFBD)['registers']['bc']['dec']&255==0x5A
    sub(0xFFD0,af=0,bc=saved,hl=0xFFBD)
    sub(0xC86F,bc=0)
    assert tails()==expected
    report['checks'].append('service byte read/write, full register preservation, R4 restored')

    # Установка с B:, который уже активен. Проверяет немедленную регистрацию.
    module=(ROOT/'bin/RAMDISK.i80').read_bytes()
    desc=bytearray(32);desc[:11]=b'RAMDISK COM'
    packed=encode(Image(module),'com')
    desc[28:32]=len(packed).to_bytes(4,'little')
    m.write(0,packed);m.write(0x7000,desc)
    assert not sub(0xC845,hl=0x7000,de=0)['flags']['C']
    execute('B:RAMDISK.COM')
    assert int.from_bytes(m.read(0xC864,2),'little')==rs['Driver']
    assert description()==[b'RAMD',b'R4 banked']
    assert tails()==expected
    report['checks'].append('reinstall from active B: immediately updates C863h')

    # Файл проходит через штатные FAT/каталог и оба способа обмена.
    payload=bytes((i*29+7)&255 for i in range(768))
    desc[:11]=b'RAMDCHK BIN';desc[18:20]=bytes(2)
    desc[28:32]=len(payload).to_bytes(4,'little')
    m.write(0x4000,payload);m.write(0x7000,desc)
    assert not sub(0xC845,hl=0x7000,de=0x4000)['flags']['C']
    for addr in [0xC800,0xC000]:
        m.stop_at(ns['mainLoop']);m.call('regs_set',pc=addr);m.until(ns['mainLoop'],60)
        select(1);assert description()==[b'RAMD',b'R4 banked']
        m.write(0x7000,desc);m.write(0x4000,bytes(768))
        assert not sub(0xC848,hl=0x7000)['flags']['C']
        assert m.read(0x4000,768)==payload and tails()==expected
    report['checks'].append('C800h/C000h reboot, NC state, FAT file read and tails preserved')

    # Последующие flash/ROM-установки не занимают DD40h–DDFFh и хвосты.
    resident=m.read(rs['Driver'],rs['resident_end']-rs['Driver'])
    for name in ['FLASH','FLASHFST','ROM','FLASHSYS']:
        execute('A:'+name+'.COM')
        assert m.read(rs['Driver'],len(resident))==resident and tails()==expected
        select(1);assert description()==[b'RAMD',b'R4 banked']
    report['checks'].append('FLASH/FLASHFST/ROM/FLASHSYS coexist with RAMD')

    # Отказ установки не должен менять уже работающий драйвер.
    m.write(0,module)
    m.write(0x8FF8,b'\0')
    try:
        assert sub(0)['flags']['C']
        assert m.read(rs['Driver'],len(resident))==resident and tails()==expected
    finally:
        m.write(0x8FF8,b'\x10')
    flush=ds['bios_flushVolume'];old_flush=m.read(flush,3)
    try:
        m.write(flush,b'\x37\xC9\0')
        assert sub(0)['flags']['C']
        assert m.read(rs['Driver'],len(resident))==resident and tails()==expected
    finally:
        m.write(flush,old_flush)
    report['checks'].append('missing RAM / flush failure leave installed driver and tails intact')

    # Восстанавливаем встроенный драйвер и проверяем файл через байтовый путь.
    sub(0xC860,af=0x0100,hl=ds['diskDriver'])
    sub(0xC86F,bc=0x0200,hl=ds['driverBlockStart']);sub(0xC86F,bc=0)
    assert description()==[b'RAMD',b'R3']
    m.write(0x7000,desc);m.write(0x4000,bytes(768))
    assert not sub(0xC848,hl=0x7000)['flags']['C'] and m.read(0x4000,768)==payload
    report['checks'].append('same FAT file readable through restored built-in R3')
    # FORMAT только тестового B:, после проверки сохранённого файла.
    m.write(0,module);assert not sub(0)['flags']['C']
    m.write(0xF100,(ROOT/'bin/FORMAT.i80').read_bytes())
    m.write(0x7000,b'B: Y R\0')
    assert not sub(0xF100,de=0x7000)['flags']['C']
    assert tails()==expected and m.sector(0)[54:62]==b'FAT16   '
    report['checks'].append('FORMAT B: Y R preserves main and all 16 bank tails')
    # Оба ручных сценария DISKBEN: штатный ввод B + Enter, ноль записей.
    bs=symbols(ROOT/'source-apps/diskbench/build/DISKBEN.sym')
    for label in ['R3','R4 banked']:
        if label=='R3':
            sub(0xC860,af=0x0100,hl=ds['diskDriver'])
            sub(0xC86F,bc=0x0200,hl=ds['driverBlockStart']);sub(0xC86F,bc=0)
        else:
            m.write(0,module);assert not sub(0)['flags']['C']
        m.write(0,(ROOT/'bin/DISKBEN.i80').read_bytes())
        m.stop_at(bs['menu_drive']);m.call('regs_set',pc=0);m.until(bs['menu_drive'])
        m.line('b');m.line('0')
        m.stop_at(bs['run_tests']);m.line('0');m.until(bs['run_tests'],60)
        assert m.read(bs['v_budget'],1)==b'\0'
        info=int.from_bytes(m.read(bs['v_driverInfo'],2),'little')
        assert m.read(info,40).split(b'\0')[:2]==[b'RAMD',label.encode()]
        m.screenshot(args.report.parent/('diskben-'+label.split()[0].lower()+'-summary.png'))
        m.stop_at(bs['exit_wait']);m.until(bs['exit_wait'],60)
        m.screenshot(args.report.parent/('diskben-'+label.split()[0].lower()+'-results.png'))
        assert tails()==expected
    report['checks'].append('DISKBEN B + Enter, Byte/R4 descriptions, zero-write runs completed')
    m.stop_at(ns['mainLoop']);m.call('regs_set',pc=0xC800);m.until(ns['mainLoop'],60)
    report['metadata_sha256']=hashlib.sha256(metadata).hexdigest()
    args.report.parent.mkdir(parents=True,exist_ok=True)
    args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print(json.dumps(report,indent=2),flush=True)


if __name__=='__main__':
    main()
