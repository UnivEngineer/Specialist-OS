"""Драйвер H: и буферизованный физический обмен в DOS-банке, реальный Emu80."""
from pathlib import Path
import argparse
import json
import re
import runpy
import subprocess
import hashlib

ROOT = Path(__file__).resolve().parents[2]


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
                                              path.read_text(), re.M)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'.local/monitor2-001')
    parser.add_argument('--report', type=Path, default=ROOT/'docs/banked/disk-001')
    parser.add_argument('--tests-only', action='store_true')
    args = parser.parse_args()
    out = args.report.resolve(); out.mkdir(parents=True, exist_ok=True)
    subprocess.run(['python', str(ROOT/'scripts/build/build-banked-transport.py')], check=True)
    base = ROOT/'bin/banked'
    ts = symbols(base/'TRANSPORT.sym'); ds = symbols(base/'DISKIO.sym'); fs = symbols(base/'FLASHDRV.sym')
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    g = h['Matrix'].__init__.__globals__; g['OUT'] = out; g['RUNTIME'] = args.runtime.resolve()
    matrix = h['Matrix'](); rows = []; timings = []
    word = lambda n: n.to_bytes(2, 'little')
    try:
        matrix.start('banked-disk')
        m = matrix.m; m.call('debug_break'); m.call('bp_clear')
        main_driver = int.from_bytes(m.read(matrix.ds['v_drives']+14, 2), 'little')
        assert main_driver == matrix.ds['DISK_DRIVER_ADDR']
        runtime = (base/'TRANSPORT.bin').read_bytes()
        window = (base/'WINDOW.bin').read_bytes()

        def select(page):
            m.write(0xFFFD if page else 0xFFFC, bytes([page-1 if page else 0]))

        for page in (0, 1, 2, 3, 16):
            select(page); m.write(0xFC00, runtime)
            m.write(ts['bankPage'], bytes([page]))
            m.write(ts['bankOwner'], bytes([7+page, page]))
            m.write(0xF7F0, b'guard-bank-tail!')
        select(1)
        m.write(ds['DOS_IO_START'], (base/'DISKIO.bin').read_bytes())
        m.write(fs['FLASH_SLOT'], (base/'FLASHDRV.bin').read_bytes())
        # Регистрация работает локально в DOS; служба не получает main-указатель драйвера.
        m.write(ds['dosIoDrivers']+7*4, word(fs['Driver'])+word(256))
        m.write(ds['dosIoDrivers']+1*4, word(ds['dosRamDriver'])+word(256))
        m.write(ds['dosIoDrivers'], word(ds['dosSystemDriver'])+word(256))
        select(0); m.write(ds['BIOS_BANK_IO_START'], (base/'MAINIO.bin').read_bytes())
        m.write(0xFFC0, window)

        def execute(caller, target, entry, bc=0, de=0, hl=0, a=0):
            select(caller)
            if target is None:
                stub = b'\xcd'+word(entry)+b'\xc3'+word(0x7000)
            else:
                stub = b'\x3e'+bytes([target])+b'\x21'+word(entry)+b'\xcd'+word(ts['bankCall'])+b'\xc3'+word(0x7000)
            m.write(0x2000, stub); m.stop_at(0x7000)
            m.call('regs_set', pc=0x2000, sp=0x7000, af=a << 8, bc=bc, de=de, hl=hl)
            ticks = m.call('cpu_state')['cpu_ticks']
            state = m.until(0x7000, 10)
            elapsed = state['cpu_ticks']-ticks
            assert state['registers']['sp']['dec'] == 0x7000
            assert m.read(ts['bankPage'], 1) == bytes([caller])
            # Пакет контекста меняется при вызове; код стандартного профиля сохраняется.
            assert m.read(0xFFC0, 8) == window[:8], 'Не восстановлен общий протокол'
            return state['registers']['af']['dec'] >> 8, bool(state['flags']['C']), elapsed

        result = execute(0, 1, fs['flashDosInit'])
        assert result[:2] == (0, False), result
        select(1)
        chips = m.read(fs['flashChipCount'], 1)[0]
        capacity = int.from_bytes(m.read(fs['FuncSize']+1, 2), 'little')
        assert 1 <= chips <= 4 and capacity == chips*2048
        rows.append(dict(case='probe-in-DOS', chips=chips, capacity=capacity))

        def io(caller, drive, op, block, dest_page, address=0x3001, expected=0, request=0x2100):
            select(caller)
            m.write(request, bytes([drive, op])+word(block)+bytes([dest_page])+word(address))
            result = execute(caller, 1, ds['dosBufferedIO'], de=request)
            assert result[:2] == (expected, bool(expected)), (caller, drive, op, block, result, expected)
            return result[2]

        original = (args.runtime.resolve()/'specialist-os/flash-disk.bin').read_bytes()
        system = (args.runtime.resolve()/'specialist-os/system-flash.bin').read_bytes()
        for block in (0, 127, 128, len(system)//256-1):
            io(3, 0, 2, block, 3)
            select(3); assert m.read(0x3001, 256) == system[block*256:(block+1)*256]
            rows.append(dict(case='system-ROM-proxy-read', block=block))
        select(3); m.write(0x3001, bytes([0xA6])*256)
        io(3, 0, 5, len(system)//256-1, 3)
        io(0, 0, 2, len(system)//256-1, 0)
        select(0); assert m.read(0x3001, 256) == bytes([0xA6])*256
        rows.append(dict(case='system-flash-proxy-write-verify'))
        # Чтение из каждой микросхемы, в том числе переход через её границу.
        for block in sorted({0, 1, 2047, 2048, capacity-1}):
            if block >= capacity: continue
            for caller, dest_page in ((0, 0), (3, 3), (0, 3)):
                select(dest_page); m.write(0x3000, b'\xa5'+bytes([0xCC])*256+b'\x5a')
                ticks = io(caller, 7, 2, block, dest_page)
                select(dest_page)
                assert m.read(0x3001, 256) == original[block*256:(block+1)*256]
                assert m.read(0x3000, 1) == b'\xa5' and m.read(0x3101, 1) == b'\x5a'
                rows.append(dict(case='read', caller=caller, page=dest_page, block=block, ticks=ticks))
        # Запись затрагивает только отдельный образ нашего тестового runtime.
        for op in (1, 5):
            data = bytes((i*19+op)&255 for i in range(256))
            select(3); m.write(0x3001, data)
            io(0, 7, op, capacity-1, 3)
            io(3, 7, 2, capacity-1, 0)
            select(0); assert m.read(0x3001, 256) == data
            rows.append(dict(case='write-read', operation=op))
        for case in [(7, 2, capacity, 0, 0x3001, 1), (6, 2, 0, 0, 0x3001, 9),
                     (8, 2, 0, 0, 0x3001, 7), (7, 3, 0, 0, 0x3001, 7),
                     (7, 2, 0, 17, 0x3001, 7), (7, 2, 0, 1, 0x3001, 7),
                     (7, 2, 0, 0, 0xF701, 7), (7, 1, 0, 0, 0xFFF0, 7)]:
            select(0); m.write(0x3001, bytes([0xCC])*256)
            io(0, *case[:4], address=case[4], expected=case[5])
            select(0); assert m.read(0x3001, 256) == bytes([0xCC])*256
            rows.append(dict(case='reject', args=case))
        io(0, 7, 2, 0, 0, request=0xF7FA, expected=7)
        # RAM-диск не затрагивает физическую страницу DOS и хвост страниц.
        payload = bytes((i*7+9)&255 for i in range(256))
        for block in (0, 254, 256, 14*256+254):
            select(0); m.write(0x3001, payload)
            io(0, 1, 5, block, 0)
            io(3, 1, 2, block, 3)
            select(3); assert m.read(0x3001, 256) == payload
            select(2+(block >> 8)); assert m.read((block & 255)*256, 256) == payload
            rows.append(dict(case='RAMD-roundtrip', block=block))
        for block in (255, 15*256):
            io(0, 1, 2, block, 0, expected=1)
        select(1)
        assert m.read(ds['DOS_IO_START'], 16) == (base/'DISKIO.bin').read_bytes()[:16]
        # Предельный допустимый буфер; копировщик не должен затронуть хвост.
        io(0, 7, 2, 0, 0, address=0xF700)
        select(0); assert m.read(0xF700, 256) == original[:256]
        # Блоки 128/512 проверяются локальным детерминированным тестовым драйвером.
        select(1)
        m.write(0xB000, bytes.fromhex('36 6d 23 0b 78 b1 c2 00 b0 b7 c9'))
        # LD (HL),6D / INC HL / DEC BC / LD A,B / OR C / JP NZ / OR A / RET.
        for length in (128, 512):
            m.write(ds['dosIoDrivers']+6*4, word(0xB000)+word(length))
            # Локальный вход получает B=операцию, поэтому обёртка задаёт BC.
            wrapper = b'\x01'+word(length)+b'\xc3'+word(0xB000)
            m.write(0xB100, wrapper)
            m.write(ds['dosIoDrivers']+6*4, word(0xB100)+word(length))
            select(0); m.write(0x3000, b'\xa5'+bytes(length)+b'\x5a')
            io(0, 6, 2, 0, 0)
            select(0)
            assert m.read(0x3001, length) == bytes([0x6D])*length
            assert m.read(0x3000, 1) == b'\xa5' and m.read(0x3001+length, 1) == b'\x5a'
            select(1)
            rows.append(dict(case='block-size', length=length))
        # При отказе драйвера клиент не получает недочитанный сектор.
        m.write(0xB200, bytes.fromhex('3e 01 37 c9'))
        m.write(ds['dosIoDrivers']+6*4, word(0xB200)+word(256))
        select(0); m.write(0x3001, bytes([0xA7])*256)
        io(0, 6, 2, 0, 0, expected=1)
        select(0); assert m.read(0x3001, 256) == bytes([0xA7])*256
        rows.append(dict(case='driver-read-error-preserves-buffer'))
        # Вложенность транспорта не разрешает повторную файловую транзакцию.
        select(1); m.write(ds['dosIoBusy'], b'\x01')
        io(0, 7, 2, 0, 0, expected=6)
        select(1); assert m.read(ds['dosIoBusy'], 1) == b'\x01'
        m.write(ds['dosIoBusy'], b'\0')
        rows.append(dict(case='reentry-rejected-preserves-busy'))
        # Разложение задержки: пустой межстраничный вызов и два размера копии.
        m.write(0xB200, b'\xc9')
        call_ticks = execute(0, 1, 0xB200)[2]
        overhead = dict(emptyCallTicks=call_ticks, copy=[])
        for length in (7, 256, 512):
            select(1)
            m.write(0x2100, b'\0'+word(0x3000)+b'\x01'+word(0x8800)+word(length))
            result = execute(1, None, ts['bankCopyFar'], hl=0x2100)
            assert result[:2] == (0, False)
            overhead['copy'].append(dict(bytes=length, ticks=result[2], ms=result[2]/2000))
        print('TRANSPORT COST', overhead, flush=True)
        # Сравнение одного и того же алгоритма ВВ55: прямой main, локальный DOS,
        # полный банковый запрос с буфером DOS. Учитываются CPU ticks, не MCP.
        for mode in (() if args.tests_only else ('main-direct', 'DOS-local', 'DOS-buffered-main', 'DOS-buffered-bank')):
            for op in (2, 1):
                ticks = 0
                for block in range(capacity-32, capacity):
                    if mode == 'main-direct':
                        select(0); m.write(0x3000, payload)
                        result = execute(0, None, main_driver, bc=op << 8, de=block, hl=0x3000)
                        assert not result[1], result
                        ticks += result[2]
                    elif mode == 'DOS-local':
                        select(1); m.write(0x8800, payload)
                        result = execute(1, None, fs['Driver'], bc=op << 8, de=block, hl=0x8800)
                        assert not result[1], result
                        ticks += result[2]
                    else:
                        page = 0 if mode.endswith('main') else 3
                        select(page); m.write(0x3000, payload)
                        ticks += io(page, 7, op, block, page, address=0x3000)
                timings.append(dict(mode=mode, operation=op, bytes=8192, ticks=ticks,
                                    ms=ticks/2000, kBps=8192/(ticks/2000000)/1000))
                print(timings[-1], flush=True)
        for page in (0, 1, 2, 3, 16):
            select(page)
            assert m.read(ts['bankDepth'], 1) == b'\0'
            assert m.read(ts['bankServiceSP'], 2) == word(0xFC00)
            assert m.read(ts['bankOwner'], 2) == bytes([7+page, page])
        report = dict(cpuHz=2000000, driverBytes=(base/'FLASHDRV.bin').stat().st_size,
                      dosServiceBytes=(base/'DISKIO.bin').stat().st_size, cases=rows, timings=timings,
                      transport=overhead, mainBridgeBytes=(base/'MAINIO.bin').stat().st_size,
                      sha256={name:hashlib.sha256((base/(name+'.bin')).read_bytes()).hexdigest()
                              for name in ('DISKIO','FLASHDRV','MAINIO','TRANSPORT','WINDOW')})
        (out/'result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
        print('BANKED DISK PASS', len(rows), flush=True)
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
