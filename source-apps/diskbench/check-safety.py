"""Проверка BPB, бюджета flash и сохранности хвостов после FORMAT в Emu80."""
import argparse
import hashlib
import json
import runpy
import struct
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
helpers = runpy.run_path(str(HERE/'check-emulator.py'))


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--report-directory',type=Path,default=HERE/'build')
    args=p.parse_args()
    out=args.report_directory
    out.mkdir(parents=True,exist_ok=True)
    if args.runtime.name!='Emu80-flash-test':
        raise ValueError('Use isolated Emu80-flash-test')
    m=helpers['Bench']('http://127.0.0.1:19781/mcp')
    s=helpers['symbols'](HERE/'build/DISKBEN.sym')
    ds=helpers['symbols'](ROOT/'bin/DOS.sym')
    rs=helpers['symbols'](ROOT/'bin/RAMD.sym')
    m.call('debug_break');m.stop_at(0xE800)
    if m.read(0xE800,32)!=(ROOT/'bin/NC.com').read_bytes()[:32]:
        m.until(0xE800,30)
    m.stop_at(0x8100)
    m.write(0x7000,b'A:RAMD.COM\0')
    assert not m.subroutine(0xC85D,hl=0x7000)['flags']['C']
    m.write(0,(HERE/'build/DISKBEN.com').read_bytes())
    m.subroutine(0xC842,af=0x100,de=1)
    original=m.sector(0)
    checks=[]
    # Чистые проверки BPB выполняют только арифметику DOS и память программы.
    for size in [256,512,1024,2048,4096]:
        for copies in [1,2]:
            boot=bytearray(original)
            struct.pack_into('<H',boot,11,size)
            boot[16]=copies
            struct.pack_into('<H',boot,17,65)
            m.write(s['sectorBuffer'],boot)
            m.write(s['v_capacity'],struct.pack('<H',4096))
            state=m.subroutine(s['geometry'])
            assert not state['flags']['C']
            expected=1+copies*32+(65*32+size-1)//size
            assert int.from_bytes(m.read(s['v_dataStart'],2),'little')==expected
    checks.append('BPB: 256..4096 bytes, 1/2 FAT, root rounding at 65 entries')
    for label,offset,data in [('FAT12',54,b'FAT12   '),('sector-size',11,b'\0\3'),
            ('zero-reserved',14,b'\0\0'),('FAT-multiply-overflow',22,b'\0\x80'),
            ('zero-total',19,b'\0\0'),('cluster-power',13,b'\3')]:
        boot=bytearray(original);boot[offset:offset+len(data)]=data
        m.write(s['sectorBuffer'],boot);m.write(s['v_capacity'],struct.pack('<H',4096))
        assert m.subroutine(s['geometry'])['flags']['C'],label
    checks.append('invalid BPB/FAT12 rejected before writes')
    # Синтетический FAT-кэш: 17-битное смещение кластера 32768.
    m.write(s['v_candidate'],struct.pack('<H',33022))
    for name,value in [('v_dataStart',256),('v_fatSize',512),('v_sectorSize',256),
                       ('v_reserved',1),('v_cachedFat',257)]:
        m.write(s[name],struct.pack('<H',value))
    m.write(s['v_spc'],b'\1');m.write(s['fatBuffer'],b'\0\0')
    state=m.subroutine(s['cluster_is_free'])
    assert not state['flags']['C'] and state['flags']['Z']
    m.write(s['fatBuffer'],b'\xF7\xFF')
    state=m.subroutine(s['cluster_is_free'])
    assert not state['flags']['C'] and not state['flags']['Z']
    checks.append('FAT16 cluster 32768: free/bad entries without 16-bit offset wrap')
    print('BPB and FAT16 boundary checks passed',flush=True)
    # Настоящий строковый интерфейс с нулём проходов не пишет на RAM B:.
    metadata=b''.join(m.sector(i) for i in range(41))
    m.write(0,(HERE/'build/DISKBEN.com').read_bytes())
    m.stop_at(s['menu_drive']);m.call('regs_set',pc=0);m.until(s['menu_drive'])
    m.line('b')
    m.line('0')  # Обычная запись; следующий запрос — бюджет проходов.
    m.stop_at(s['run_tests']);m.line('0');m.until(s['run_tests'])
    assert m.read(s['v_budget'],1)==b'\0' and m.read(s['v_technology'],1)==b'R'
    m.screenshot(out/'zero-budget.png')
    m.stop_at(s['exit_wait']);m.until(s['exit_wait'],60)
    assert m.read(s['v_case'],1)==b'\4'
    m.stop_at(0x8100)
    assert b''.join(m.sector(i) for i in range(41))==metadata
    checks.append('bios_input: explicit budget 0, reads completed, writes visibly skipped, metadata unchanged')
    worst={}
    payload=bytes((i*17+1)&255 for i in range(256))
    m.write(0xFE00,payload)
    for mode in [1,2]:
        before=m.call('cpu_state')['cpu_ticks']
        state=m.subroutine(0xC863,bc=mode*256,de=0x1FE,hl=0xFE00)
        assert not state['flags']['C']
        worst[str(mode)]=state['cpu_ticks']-before-27
    assert m.read(0xFE00,256)==payload
    checks.append('last valid buffer FE00h: read/write and data preserved')
    m.write(0x4000,payload)
    # Обёртка вызывается только при объявленном B=4; отказ барьера не скрывается.
    jump=m.read(0xC863,3)
    m.write(0x7000,bytes.fromhex('78fe01c837c9'))  # B=1: CF=0; B=4: CF=1.
    try:
        m.write(0xC863,bytes.fromhex('c30070'))
        assert m.subroutine(s['write_with_flush'],bc=0x100,de=0x1FE,hl=0x4000)['flags']['C']
    finally:m.write(0xC863,jump)
    checks.append('declared async flush failure is propagated')
    # FORMAT пишет только метаданные B:; хвосты не должны меняться.
    n=rs['ramdBankTailEnd']-0xFF00
    replicas=[]
    for page in range(16):
        m.write(0xFFFD,bytes([page]));replicas.append(m.read(0xFF00,n));m.write(0xFFFC,b'\0')
    m.write(0xF100,(ROOT/'bin/FORMAT.com').read_bytes())
    m.write(0x7000,b'B: Y R\0')
    m.write(0x8000,b'\xCD\0\xF1\xC3\0\x81')
    m.call('regs_set',pc=0x8000,sp=0x8F00,de=0x7000)
    m.until(0x8100,10)
    for page in range(16):
        m.write(0xFFFD,bytes([page]));assert m.read(0xFF00,n)==replicas[page];m.write(0xFFFC,b'\0')
    assert m.sector(0)[54:62]==b'FAT16   '
    checks.append('FORMAT B: Y R preserves all 16 tail replicas')
    # Обычный запуск из нового ROM, а не инъекция COM: проверяем загрузку по адресу 0.
    m.write(0x7000,b'A:DISKBEN.COM\0')
    m.stop_at(s['entry'])
    m.write(0x8000,b'\xCD\x5D\xC8\xC3\0\x81')
    m.call('regs_set',pc=0x8000,sp=0x8F00,hl=0x7000)
    m.until(s['entry'],10)
    assert m.read(0,32)==(HERE/'build/DISKBEN.com').read_bytes()[:32]
    checks.append('A:DISKBEN.COM loaded by MXOS fileExec to 0000h')
    report={'status':'emulator-only','checks':checks,'flash_hardware_written':False,
        'worst_buffer_driver_cycles':worst,
        'metadata_sha256_before_format':hashlib.sha256(metadata).hexdigest()}
    (out/'safety.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report,indent=2),flush=True)


if __name__=='__main__':main()
