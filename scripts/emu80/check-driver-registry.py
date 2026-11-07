"""Реестр, загрузка инициализатора в DOS и прямые серии RAW в Emu80."""
from pathlib import Path
import argparse
import json
import runpy

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    args = parser.parse_args()
    out = args.report.resolve()
    out.mkdir(parents=True, exist_ok=True)
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    h['Matrix'].__init__.__globals__.update(OUT=out, RUNTIME=ROOT/'.local/monitor2-001')
    x = h['Matrix']()
    m = x.m
    ks = h['symbols'](ROOT/'bin/banked/KERNEL.sym')
    bios = h['symbols'](ROOT/'bin/BIOS.sym')
    ts = h['symbols'](ROOT/'bin/banked/TRANSPORT.sym')
    word = lambda n: n.to_bytes(2, 'little')
    exe = runpy.run_path(str(ROOT/'scripts/images/executable.py'))
    rows = []

    def page(p):
        m.write(0xFFFD if p else 0xFFFC, bytes([p-1 if p else 0]))

    def dos_read(address, size):
        page(1)
        result = m.read(address, size)
        page(0)
        return result

    def syscall(op, **regs):
        m.write(0x8000, b'\xcd\x06\xf8'+word(op)+b'\xc9')
        return x.func(0x8000, **regs)

    def exec_file(name, error=0):
        m.write(0x7E00, name.encode('ascii')+b'\0')
        state = x.func(0xC85D, hl=0x7E00)
        assert state['flags']['C'] == bool(error), (name, state)
        if error:
            assert state['registers']['af']['dec'] >> 8 == error, (name, state)

    def save(name, payload):
        x.select(1)
        stem, ext = name.split('.')
        desc = bytearray(stem.ljust(8).encode()+ext.ljust(3).encode()+bytes(21))
        desc[28:32] = len(payload).to_bytes(4, 'little')
        m.write(0x7800, desc)
        m.write(0x6000, payload)
        assert not x.func(0xC845, hl=0x7800, de=0x6000)['flags']['C']

    def record():
        return dos_read(ks['DOS_DRIVER_REGISTRY']+7*ks['DRIVER_RECORD_SIZE'], 80)

    try:
        x.start('driver-registry')
        x.console()
        for drive, capacity, name in ((0,2048,b'FLASHSYS'), (1,3840,b'Bank RAM'), (7,8192,b'FLASH')):
            x.select(drive)
            size = x.func(0xC863, bc=0x0300)
            assert size['registers']['de']['dec'] == capacity, size
            info = x.func(0xC863, bc=0xFF00)
            text = m.read(info['registers']['hl']['dec'],64).split(b'\0')[0]
            assert text == name, text
            rows.append(dict(case='registry-metadata',drive=drive,capacity=capacity,name=text.decode()))

        # Повторное RAW не входит в DOS и не упаковывает регистры заново.
        for drive in (0,1,7):
            x.select(drive)
            assert not x.func(0xC863,bc=0x0200,de=0,hl=0x2000)['flags']['C']
            assert m.read(ks['MAIN_RAW_LEASE'],1) == bytes([drive+1])
            m.write(0x8E00,b'\xcd\x63\xc8\xc3\x10\x8e')
            m.stop_at(0x8E10)
            m.call('bp_set',addr=ks['kernelDispatch'],type='exec')
            m.call('bp_set',addr=h['symbols'](ROOT/'bin/banked/CLIENT.sym')['bankMxCallRegs'],type='exec')
            m.call('regs_set',pc=0x8E00,sp=0x8FDF,bc=0x0200,de=1,hl=0x2200)
            state = m.until(0x8E10,30)
            assert not state['flags']['C']
            rows.append(dict(case='second-RAW-without-DOS-or-register-packet',drive=drive))

        # Ошибка локального пути возвращается и через GETERROR; смена диска
        # завершает прямую серию. Проверяем обе границы сектора/буфера.
        x.select(1)
        assert not x.func(0xC863,bc=0x0200,de=0,hl=0x2000)['flags']['C']
        for block, buf in ((255,0x2000),(3840,0x2000),(0,0xFB01),(0,0xFF00)):
            state = x.func(0xC863,bc=0x0200,de=block,hl=buf)
            assert state['flags']['C'] and state['registers']['af']['dec']>>8 == 7
            state = x.func(0xC8C9)
            assert state['flags']['C'] and state['registers']['af']['dec']>>8 == 7
        rows.append(dict(case='local-RAW-range-and-GETERROR'))

        # Реальный COM выполняется в DOS с 9000h; main не затрагивается.
        for name, kind in (('FlashFst',2),('Flash',1),('Rom',0),('Flash',1)):
            print('INSTALL',name,flush=True)
            lower = m.read(0,0x2000)
            x.install(name)
            assert m.read(0,0x2000) == lower
            r = record()
            assert r[7] == kind, (name,r.hex())
            page(1)
            m.write(0x9000,bytes([0xCC])*0x3000)
            page(0)
            if kind == 0:
                x.func(0xC842,af=0x0700,de=1)  # текущий 2-МБ BPB больше ROM
            else:
                x.select(7)
            info = x.func(0xC863,bc=0xFF00)
            text = m.read(info['registers']['hl']['dec'],64)
            assert text.startswith(name.upper().encode()+b'\0'), text
            assert not x.func(0xC863,bc=0x0200,de=0,hl=0x2000)['flags']['C']
            if kind == 0:
                assert x.func(0xC863,bc=0x0200,de=256,hl=0x2000)['flags']['C']
            rows.append(dict(case='initializer-reusable-arena-owned-metadata',driver=name))

        # Маркер страницы исполнения и передаваемого DE с аргументами.
        marker=ks['DOS_BANK_CMD_ADDR']+256
        body = b'\x3a'+word(ts['bankPage'])+b'\x32'+word(marker)+b'\xeb\x22'+word(marker+1)+b'\xaf\xc9'
        save('INIT.COM',exe['encode'](exe['Image'](body,0x9000,0x9000,1),'com'))
        m.write(0x3700,b'main marker')
        exec_file('B:\\INIT.COM hello')
        result = dos_read(marker,3)
        assert result[0] == 1 and ks['DOS_BANK_CMD_ADDR'] <= int.from_bytes(result[1:],'little') < ks['DOS_BANK_CMD_ADDR']+128
        assert m.read(0x3700,11) == b'main marker'
        rows.append(dict(case='driver-exec-page-and-local-arguments'))

        # Некорректный дескриптор, размер/указатели или заголовок не меняют
        # прежний драйвер. Инициализатор намеренно пытается занять чужую область.
        desc_addr=0x9010
        prefix=b'\x21'+word(desc_addr)+b'\xcd\x06\xf8'+word(ks['MX_DOS_DRIVERINSTALL'])+b'\xc9'
        valid=bytes([1,7,1,8])+word(8192)+word(0)+bytes([7,ord('F')])+word(0x9020)+word(1)+word(0x9021)
        candidates=[]
        for offset, value in ((0,2),(1,0),(2,3),(3,9)):
            d=bytearray(valid);d[offset]=value;candidates.append(bytes(d))
        for offset, value in ((10,0x2000),(10,0x9025),(12,577),(12,0),(14,0x3000),(14,0x9025)):
            d=bytearray(valid);d[offset:offset+2]=word(value);candidates.append(bytes(d))
        for i,desc in enumerate(candidates):
            payload=prefix.ljust(16,b'\0')+desc+b'\xc9X\0Y\0'
            save(f'BAD{i}.COM',exe['encode'](exe['Image'](payload,0x9000,0x9000,1),'com'))
            old=record()
            exec_file(f'B:\\BAD{i}.COM',7)
            assert record() == old
        for name, address, flags, error in (('LOW',0,1,7),('HIGH',0xBFFF,1,7),('PLAIN',0x9000,0,7)):
            save(name+'.COM',exe['encode'](exe['Image'](b'\xaf\xc9',address,address,flags),'com'))
            exec_file('B:\\'+name+'.COM',error)
        rows.append(dict(case='invalid-install-preserves-previous-driver',cases=len(candidates)+3))
        state=syscall(ks['MX_DOS_DRIVERINSTALL'],hl=0x9000)
        assert state['flags']['C'] and state['registers']['af']['dec']>>8 == ks['DOS_ERR_UNSUPPORTED']
        rows.append(dict(case='main-cannot-register-local-DOS-pointers'))

        # Прямые записи, границы банков B: и чипов H:, восстановление данных.
        for drive, blocks in ((1,(254,256,3838)),(0,(127,128,2047)),(7,(2047,2048,8191))):
            x.select(drive)
            for block in blocks:
                assert not x.func(0xC863,bc=0x0200,de=block,hl=0x2000)['flags']['C']
                old=m.read(0x2000,256)
                payload=bytes((i^block)&255 for i in range(256))
                try:
                    m.write(0x2000,payload)
                    assert not x.func(0xC863,bc=0x0100,de=block,hl=0x2000)['flags']['C']
                    assert not x.func(0xC863,bc=0x0200,de=block,hl=0x2200)['flags']['C']
                    assert m.read(0x2200,256)==payload
                finally:
                    m.write(0x2000,old)
                    assert not x.func(0xC863,bc=0x0100,de=block,hl=0x2000)['flags']['C']
                rows.append(dict(case='write-read-restore-boundary',drive=drive,block=block))
        (out/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
        print('DRIVER REGISTRY PASS',len(rows),flush=True)
    finally:
        (out/'progress.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
        x.stop()


if __name__ == '__main__':
    main()
