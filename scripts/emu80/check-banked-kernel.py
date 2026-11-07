"""Полное файловое ядро в ДОЗУ 0: ABI из main и чужого банка, реальный Emu80."""
from pathlib import Path
import argparse
import hashlib
import json
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'.local/monitor2-001')
    parser.add_argument('--report', type=Path, default=ROOT/'docs/banked/kernel-001')
    parser.add_argument('--images-only', action='store_true')
    parser.add_argument('--copies-only', action='store_true')
    args = parser.parse_args()
    out = args.report.resolve(); out.mkdir(parents=True, exist_ok=True)
    subprocess.run(['python', str(ROOT/'scripts/build/build-banked-dos.py')], check=True)
    symbols = runpy.run_path(str(ROOT/'scripts/emu80/check-banked-bios.py'))['symbols']
    base = ROOT/'bin/banked'
    ks = symbols(base/'KERNEL.sym'); ts = symbols(base/'TRANSPORT.sym')
    bios = symbols(ROOT/'bin/BIOS.sym')
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    g = h['Matrix'].__init__.__globals__; g['OUT'] = out; g['RUNTIME'] = args.runtime.resolve()
    matrix = h['Matrix'](); rows = []; calls = []
    word = lambda n: n.to_bytes(2, 'little')
    regs = lambda hl=0, de=0, bc=0, a=0: word(hl)+word(de)+word(bc)+word((a << 8)|2)
    def finish_report():
        report = dict(cases=rows, requests=calls, cpuHz=2000000, kernelBytes=(base/'KERNEL.bin').stat().st_size,
                      kernelEnd=ks['kernelEnd'], sha256={name:hashlib.sha256((base/(name+'.bin')).read_bytes()).hexdigest()
                      for name in ('KERNEL','CLIENT','TRANSPORT','MAINIO','MAINBIOS','FLASHDRV')})
        (out/'result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
        print('BANKED KERNEL PASS', len(rows), flush=True)

    try:
        matrix.start('banked-kernel')
        m = matrix.m; m.call('debug_break'); m.call('bp_clear')

        def select(page):
            m.write(0xFFFD if page else 0xFFFC, bytes([page-1 if page else 0]))

        # Штатный B: уже начинается с физической страницы 1.
        select(2)
        assert int.from_bytes(m.read(11, 2), 'little') == 256
        ram_seed = bytearray(m.read(0, 0xFF00))
        for page in (0, 1, 3):
            select(page); m.write(0xFC00, (base/'TRANSPORT.bin').read_bytes())
            m.write(ts['bankPage'], bytes([page]))
            m.write(ts['bankOwner'], bytes([7+page, page]))
            top, low = ((ks['BIOS_BANK_STACK_TOP'], ks['BIOS_BANK_STACK_BOTTOM']) if page == 0
                        else (ks['DOS_BANK_STACK_TOP'], ks['DOS_BANK_STACK_BOTTOM']))
            m.write(ts['bankServiceSP'], word(top)); m.write(ts['bankStackFloor'], word(low+64))
            m.write(low-16, b'guard-DOS-stack!')
            if page: m.write(0xF800, (base/'CLIENT.bin').read_bytes())
        select(1)
        m.write(0x4000, (base/'KERNEL.bin').read_bytes())
        select(0)
        io_symbols=symbols(base/'MAINIO.sym')
        io_code=io_symbols['mainIoRequest']-ks['BIOS_BANK_IO_START']
        assert m.read(ks['BIOS_BANK_IO_START'], io_code) == (base/'MAINIO.bin').read_bytes()[:io_code]
        m.write(0xFFC0, (base/'WINDOW.bin').read_bytes())

        def invoke(page, entry, packet=None, operation=0, inline=False, legacy=None):
            select(page)
            call = ts['bankCallPacket'] if packet is not None else ts['bankCall']
            packet_address = 0x3800 if page == 1 else 0x3000
            if packet is not None: m.write(packet_address, packet)
            if legacy is not None:
                code = b'\xcd'+word(legacy)
            elif inline:
                code = b'\xcd'+word(0xF806)+word(operation)
            elif packet is not None:
                code = b'\xcd'+word(0xF800)
            else:
                code = b'\x3e\x01\x21'+word(entry)+b'\xcd'+word(call)
            stop = 0x3E00
            stack = 0x3F00
            m.write(0x2800, code+b'\xc3'+word(stop))
            m.stop_at(stop)
            args = dict(bc=operation, de=packet_address)
            if inline or legacy is not None:
                args = {key:int.from_bytes(packet[i:i+2], 'little')
                        for key,i in (('hl',0),('de',2),('bc',4),('af',6))}
            m.call('regs_set', pc=0x2800, sp=stack, **args)
            ticks = m.call('cpu_state')['cpu_ticks']
            state = m.until(stop, 30)
            assert state['registers']['sp']['dec'] == stack
            assert m.read(ts['bankPage'], 1) == bytes([page])
            result = state['registers']['af']['dec']
            calls.append(dict(caller=page, operation=operation if packet is not None else 'boot',
                              suboperation=packet[7] if packet is not None else 0,
                              inline=inline, legacy=legacy, ticks=state['cpu_ticks']-ticks, error=result >> 8, carry=bool(result & 1)))
            if packet is not None and not inline and legacy is None and m.read(0xFFDB, 1) == b'\0':
                returned = b''.join(word(state['registers'][key]['dec']) for key in ('hl','de','bc','af'))
                assert m.read(packet_address, 8) == returned, 'MX_REGS результата'
            for p in (0, 1, 3):
                select(p)
                assert m.read(ts['bankDepth'], 1) == b'\0'
                assert m.read(ts['bankOwner'], 2) == bytes([7+p, p])
                low = ks['BIOS_BANK_STACK_BOTTOM'] if p == 0 else ks['DOS_BANK_STACK_BOTTOM']
                assert m.read(low-16, 16) == b'guard-DOS-stack!'
            select(page)
            return result >> 8, bool(result & 1)

        def request(page, fn, packet, expected=0, inline=False):
            result = invoke(page, ks['BANK_DOS_GATE'], packet, fn, inline)
            assert result == (expected, bool(expected)), (hex(fn), page, result, expected)
            return result

        result = invoke(0, ks['dosBoot'])
        assert result == (0, False), ('boot', result)
        rows.append(dict(case='boot-DOS-in-bank-with-nested-BIOS-ROM'))
        print('BANK DOS BOOT PASS', flush=True)
        m.screenshot(out/'bank-dos-boot.png')
        select(0); m.write(bios['v_dosBanked'], b'\1')
        m.write(0x3200, b'A:\\FLASH.COM\0')
        request(0, ks['MX_DOS_FILEEXEC'], regs(hl=0x3200))
        rows.append(dict(case='load-driver-COM-in-DOS-page'))
        image_checks = runpy.run_path(str(ROOT/'scripts/emu80/banked-image-cases.py'))['check_images']
        def check_images():
            image_checks(m, ks, bios, select, invoke, request, regs, rows)
        copy_checks = runpy.run_path(str(ROOT/'scripts/emu80/banked-copy-cases.py'))['check_copies']
        def check_copies():
            copy_checks(ROOT, out, m, ks, bios, select, invoke, request, regs, rows)
        if args.copies_only:
            check_copies()
            finish_report()
            return
        if args.images_only:
            check_images()
            finish_report()
            return
        # Публичные номера неизменны: только маршрут уходит в другой банк.
        for page in (0, 3):
            request(page, ks['MX_DOS_FILESELECTDRIVE'], regs(a=7))
            request(page, ks['MX_DOS_DIRECTORY'], regs(hl=0x3200, a=4))
            assert m.read(0x3200, 4) == b'H:\\\0'
            rows.append(dict(case='select-H-getcwd', caller=page))
        # A: обслуживается основным BIOS; H: — драйвером в DOS-странице.
        for page in (0, 3):
            select(page)
            handle = bytearray(64); handle[:11] = b'AUTOEXECBAT'
            handle[ks['FH_DRIVE']] = 0; handle[ks['FH_MODE']] = 1
            m.write(0x3100, handle)
            request(page, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100))
            handle = bytearray(m.read(0x3100, 64))
            size = int.from_bytes(handle[28:32], 'little')
            handle[49:53] = word(0x3400)+word(2048)
            m.write(0x3100, handle)
            request(page, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=1))
            actual = m.read(0x3100, 64)
            assert int.from_bytes(actual[51:53], 'little') == size
            assert actual[49:51] == word(0x3400)
            assert m.read(0x3400, size) == (ROOT/'assets/system-rom/autoexec.emu-writable.bat').read_bytes()
            request(page, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=5))
            rows.append(dict(case='read-system-file-through-BIOS', caller=page, bytes=size))
        # Путь и данные в main специально отличаются от данных клиента банка 3.
        select(0); m.write(0x3200, b'H:\\WRONG\0'+bytes(119))
        select(3); m.write(0x3200, b'H:\\BNKTEST\0'+bytes(117))
        request(3, ks['MX_DOS_DIRECTORY'], regs(hl=0x3200, a=7))
        request(3, ks['MX_DOS_DIRECTORY'], regs(hl=0x3200, a=2))
        request(3, ks['MX_DOS_DIRECTORY'], regs(hl=0x3300, a=4))
        assert m.read(0x3300, 11) == b'H:\\BNKTEST\0'
        rows.append(dict(case='mkdir-chdir-caller-path'))
        select(1); m.write(ks['KD_PATH'], bytes([0xCC])*128)
        request(1, ks['MX_DOS_DIRECTORY'], regs(hl=0x3700, a=4))
        assert m.read(0x3700, 11) == b'H:\\BNKTEST\0'
        assert m.read(ks['KD_PATH'], 128) == bytes([0xCC])*128
        rows.append(dict(case='same-page-direct-NEAR-no-staging'))
        select(3)
        handle = bytearray(64); handle[:11] = b'BANKTESTBIN'
        handle[40] = 7; handle[41] = 1|2|4|8
        m.write(0x3100, handle)
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100))
        payload = bytes((i*37+i//256)&255 for i in range(8193))
        m.write(0x4000, payload)
        handle = bytearray(m.read(0x3100, 64)); handle[49:53] = word(0x4000)+word(len(payload))
        m.write(0x3100, handle)
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=2))
        handle = bytearray(m.read(0x3100, 64))
        assert int.from_bytes(handle[51:53], 'little') == len(payload)
        assert handle[49:51] == word(0x4000)
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=4))
        handle[53:58] = bytes(5); m.write(0x3100, handle)
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=3))
        handle = bytearray(m.read(0x3100, 64)); handle[49:53] = word(0x4000)+word(9000)
        m.write(0x3100, handle); m.write(0x4000, bytes([0xA5])*9000)
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=1))
        actual = m.read(0x3100, 64)
        assert int.from_bytes(actual[51:53], 'little') == len(payload)
        assert m.read(0x4000, len(payload)) == payload
        assert m.read(0x4000+len(payload), 9000-len(payload)) == bytes([0xA5])*(9000-len(payload))
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=5))
        rows.append(dict(case='create-write-flush-seek-read-close', bytes=len(payload)))
        print('BANK DOS FILE IO PASS', flush=True)
        for pointer in (0xF7C1, 0xFFF0):
            request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=pointer), expected=7)
            rows.append(dict(case='reject-handle-boundary', address=pointer))
        request(3, ks['MX_DOS_DIRECTORY'], regs(hl=0xF781, a=4), expected=7)
        select(3); m.write(0x3200, b'X'*128)
        request(3, ks['MX_DOS_DIRECTORY'], regs(hl=0x3200, a=2), expected=7)
        rows.append(dict(case='reject-path-boundary-and-missing-terminator'))
        # Закрытый дескриптор: ноль переданных байт, буфер не меняется.
        handle = bytearray(m.read(0x3100, 64)); handle[49:53] = word(0x3400)+word(128)
        m.write(0x3100, handle); m.write(0x3400, bytes([0xCC])*128)
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=1), expected=7)
        assert m.read(0x3100+51, 2) == b'\0\0'
        assert m.read(0x3400, 128) == bytes([0xCC])*128
        rows.append(dict(case='early-handle-error-zero-count'))
        # CLI и отрисовка DIR тоже исполняются в DOS с вложенными BIOS-вызовами.
        select(3); m.write(0x3200, b'dir\0'+bytes(124))
        request(3, ks['MX_DOS_CLI'], regs(hl=0x3200, a=8))
        request(3, ks['MX_DOS_CLI'], regs(hl=0x3200))
        select(0); m.screenshot(out/'bank-cli-dir.png')
        rows.append(dict(case='CLI-DIR-from-bank-nested-BIOS'))
        select(3); m.write(0x3200, b'cd /\0'+bytes(123))
        request(3, ks['MX_DOS_CLI'], regs(hl=0x3200))
        request(3, ks['MX_DOS_DIRECTORY'], regs(hl=0x3300, a=4))
        assert m.read(0x3300, 4) == b'H:\\\0'
        rows.append(dict(case='CLI-CD-from-bank'))
        # Полный файловый путь B: использует физическую страницу 1, не DOS 0.
        request(0, ks['MX_DOS_FILESELECTDRIVE'], regs(a=1))
        handle = bytearray(64); handle[:11] = b'BANKRAM BIN'; handle[40] = 1; handle[41] = 15
        select(0); m.write(0x3100, handle)
        request(0, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100))
        ram_payload = bytes((i*11+3)&255 for i in range(517))
        handle = bytearray(m.read(0x3100, 64)); handle[49:53] = word(0x3400)+word(517)
        m.write(0x3100, handle); m.write(0x3400, ram_payload)
        request(0, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=2))
        handle = bytearray(m.read(0x3100, 64)); handle[53:58] = bytes(5)
        m.write(0x3100, handle)
        request(0, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=3))
        handle = bytearray(m.read(0x3100, 64)); handle[51:53] = word(517)
        m.write(0x3100, handle); m.write(0x3400, bytes(517))
        request(0, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=1))
        assert m.read(0x3400, 517) == ram_payload
        request(0, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=5))
        rows.append(dict(case='RAMD-file-roundtrip-reserves-DOS-bank', bytes=517))
        def cli(command):
            data = command.encode('ascii')
            select(3); m.write(0x3200, data+bytes(128-len(data)))
            request(3, ks['MX_DOS_CLI'], regs(hl=0x3200))

        cli('copy H:/BNKTEST/BANKTEST.BIN B:/COPY.BIN')
        cli('ren B:/COPY.BIN B:/REN.BIN')
        cli('move B:/REN.BIN B:/FINAL.BIN')
        select(3)
        handle = bytearray(64); handle[:11] = b'FINAL   BIN'; handle[40] = 1; handle[41] = 1
        m.write(0x3100, handle)
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100))
        handle = bytearray(m.read(0x3100, 64)); handle[49:53] = word(0x4000)+word(len(payload))
        m.write(0x3100, handle); m.write(0x4000, bytes(len(payload)))
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=1))
        assert m.read(0x4000, len(payload)) == payload
        request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=5))
        cli('del B:/FINAL.BIN')
        cli('mkdir B:/EMPTY')
        cli('rmdir B:/EMPTY')
        rows.append(dict(case='CLI-COPY-REN-MOVE-DEL-MKDIR-RMDIR', copiedBytes=len(payload)))
        # Ошибки внешнего буфера не меняют дескриптор и соседнюю память.
        select(3)
        for address, count in ((0xF7FF, 2), (0xFFFF, 2), (0x3101, 20)):
            bad = bytearray(64); bad[:11] = b'BANKRAM BIN'; bad[40:43] = bytes([1, 1, 0x46])
            bad[49:53] = word(address)+word(count)
            m.write(0x3100, bad)
            request(3, ks['MX_DOS_FILEHANDLE'], regs(hl=0x3100, a=1), expected=7)
            assert m.read(0x3100, 64) == bad
            rows.append(dict(case='reject-client-buffer', address=address, count=count))
        packet = regs(hl=0x3200)
        request(3, 0x2FF, packet, expected=ks['DOS_ERR_UNSUPPORTED'])
        assert m.read(0x3000, 8) == packet
        rows.append(dict(case='unsupported-preserves-packet'))
        select(1); m.write(ks['kdBusy'], b'\1')
        request(3, ks['MX_DOS_GETERROR'], packet, expected=6)
        assert m.read(0x3000, 8) == packet
        select(1); assert m.read(ks['kdBusy'], 1) == b'\1'
        m.write(ks['kdBusy'], b'\0')
        rows.append(dict(case='nested-filesystem-request-rejected'))
        # Целый файл использует тот же FAT-поток: клиентский адрес может лежать
        # выше 4000h, а длина пересекает несколько внутренних пачек по 4 КБ.
        for page, length in ((0, 257), (3, 0), (3, 4097), (3, 20003)):
            select(page)
            data = bytes((i*29+i//257+length)&255 for i in range(length))
            buffer = 0x3400 if page == 0 else 0x4000
            path = f'B:/PATH{length}.BIN'.encode('ascii')
            m.write(0x3200, path+bytes(128-len(path)))
            obj = word(0x3200)+word(buffer)+word(length)+bytes(34)
            m.write(0x3100, obj); m.write(buffer, data+b'guard')
            request(page, ks['MX_DOS_FILESAVEPATH'], regs(hl=0x3100))
            assert m.read(0x3100, 6) == obj[:6]
            assert m.read(0x3106, 2) == word(length)
            m.write(buffer, bytes([0xA5])*(length+5))
            obj = word(0x3200)+word(buffer)+word(length+5)+bytes(34)
            m.write(0x3100, obj)
            request(page, ks['MX_DOS_FILELOADPATH'], regs(hl=0x3100))
            assert m.read(buffer, length) == data
            assert m.read(buffer+length, 5) == bytes([0xA5])*5
            assert m.read(0x3100, 6) == obj[:6]
            assert m.read(0x3106, 2) == word(length)
            if length:
                m.write(0x3104, word(length-1))
                request(page, ks['MX_DOS_FILELOADPATH'], regs(hl=0x3100), expected=7)
                assert m.read(0x3106, 2) == bytes(2)
            rows.append(dict(case='whole-file-path-roundtrip', caller=page, bytes=length))
        for buffer, length in ((0xF7FF, 2), (0xFFFF, 2), (0x3101, 40), (0x3201, 40)):
            select(3)
            m.write(0x3100, word(0x3200)+word(buffer)+word(length)+bytes(34))
            request(3, ks['MX_DOS_FILELOADPATH'], regs(hl=0x3100), expected=7)
            assert m.read(0x3106, 2) == bytes(2)
            rows.append(dict(case='reject-whole-file-buffer', address=buffer, count=length))
        print('BANK DOS WHOLE FILE IO PASS', flush=True)
        for page in (0, 3):
            select(page); m.write(0x3300, bytes([0xA5])*16)
            request(page, ks['MX_DOS_FILEGETDRIVES'], regs(hl=0x3300, bc=8))
            count = m.read(0x3005, 1)[0]
            assert m.read(0x3300, count) == bytes([0,1,7])
            assert m.read(0x3300+count, 16-count) == bytes([0xA5])*(16-count)
            rows.append(dict(case='enumerate-drives-in-client-page', caller=page))
        select(3)
        path = b'B:/PATH20003.BIN'
        m.write(0x3200, path+bytes(128-len(path))); m.write(0x3300, bytes([0xA5])*32)
        result = invoke(3, ks['BANK_DOS_GATE'], regs(hl=0x3200,de=0x3300), ks['MX_DOS_FILENAMEPREPARE'])
        assert not result[1], result
        assert m.read(0x3300, 32) == b'PATH2000BIN'+bytes([0xA5])*21
        assert m.read(0x3000, 4) == word(0x3200+len(path))+word(0x3300)
        result = invoke(3, ks['BANK_DOS_GATE'], regs(hl=0x3300), ks['MX_DOS_FILELOADINFO'])
        assert not result[1], result
        descriptor = m.read(0x3300,32)
        assert int.from_bytes(descriptor[28:32],'little') == 20003
        cursor = descriptor[26:28]+bytes(2)
        m.write(0x3100, cursor)
        offset = 0
        for length in (1,4096,73,20003-4170):
            m.write(0x4000,bytes([0xA5])*(length+3))
            request(3, ks['MX_DOS_FILESTREAM'], regs(hl=0x4000,de=0x3100,bc=length,a=2))
            expected = bytes((i*29+i//257+20003)&255 for i in range(offset,offset+length))
            assert m.read(0x4000,length) == expected
            assert m.read(0x4000+length,3) == bytes([0xA5])*3
            assert m.read(0x3000,6) == word(0x4000+length)+word(0x3100)+word(length)
            offset += length
        rows.append(dict(case='name-stat-stream-incremental-read', bytes=offset))
        for buffer,count,cur in ((0xF7FF,2,0x3100),(0x4000,5,0xF7FD),(0x3101,5,0x3100)):
            before = m.read(0x3100,4)
            request(3,ks['MX_DOS_FILESTREAM'],regs(hl=buffer,de=cur,bc=count,a=2),expected=7)
            assert m.read(0x3100,4) == before
            assert m.read(0x3004,2) == bytes(2)
        request(3,ks['MX_DOS_FILESTREAM'],regs(hl=0xFFFF,de=0xFFFF,bc=0,a=2))
        rows.append(dict(case='stream-boundaries-overlap-and-empty'))
        # Перезапись уже выделенной цепочки, затем чтение полным файловым ABI.
        m.write(0x3300,b'PATH4097BIN'+bytes(21))
        result = invoke(3,ks['BANK_DOS_GATE'],regs(hl=0x3300),ks['MX_DOS_FILELOADINFO'])
        assert not result[1], result
        m.write(0x3100,m.read(0x331A,2)+bytes(2))
        stream_data = bytes((i*13+7)&255 for i in range(4097))
        m.write(0x4000,stream_data)
        request(3,ks['MX_DOS_FILESTREAM'],regs(hl=0x4000,de=0x3100,bc=4097,a=1))
        request(3,ks['MX_DOS_FLUSHVOLUME'],regs())
        m.write(0x3100,word(0x3200)+word(0x4000)+word(4097)+bytes(34))
        m.write(0x3200,b'B:/PATH4097.BIN'+bytes(114)); m.write(0x4000,bytes(4097))
        request(3,ks['MX_DOS_FILELOADPATH'],regs(hl=0x3100))
        assert m.read(0x4000,4097) == stream_data
        # Вернуть исходные байты для последующих проверок загрузки в main.
        original = bytes((i*29+i//257+4097)&255 for i in range(4097))
        m.write(0x4000,original)
        request(3,ks['MX_DOS_FILESAVEPATH'],regs(hl=0x3100))
        rows.append(dict(case='stream-write-flush-read-by-path',bytes=4097))
        m.write(0x3300,b'PATH257 BIN'); m.write(0x3400,b'RENAMED BIN')
        result = invoke(3,ks['BANK_DOS_GATE'],regs(hl=0x3300,de=0x3400),ks['MX_DOS_FILERENAME'])
        assert not result[1],result
        result = invoke(3,ks['BANK_DOS_GATE'],regs(hl=0x3400,bc=0x2101),ks['MX_DOS_FILEGETSETATTR'])
        assert result == (0x21,False),result
        request(3,ks['MX_DOS_FILEDELETE'],regs(hl=0x3400),expected=6)
        result = invoke(3,ks['BANK_DOS_GATE'],regs(hl=0x3400,bc=0x2001),ks['MX_DOS_FILEGETSETATTR'])
        assert result == (0x20,False),result
        request(3,ks['MX_DOS_FILEDELETEONDRIVE'],regs(hl=0x3400))
        request(3,ks['MX_DOS_FILELOADINFO'],regs(hl=0x3400),expected=2)
        rows.append(dict(case='rename-attributes-protect-delete'))
        print('BANK DOS METADATA AND STREAM PASS', flush=True)
        # Список из своей страницы задаёт эталон для внешнего контракта.
        request(3, ks['MX_DOS_FILESELECTDRIVE'], regs(a=0))
        for fn, sub in ((ks['MX_DOS_FILELIST'], 0), (ks['MX_DOS_DIRECTORY'], 3)):
            request(1, fn, regs(hl=0x1000, de=128, bc=1, a=sub))
            reference = m.read(0x1000, 2049)
            end = next(i for i in range(0, 2049, 16) if reference[i] == 255)+1
            reference = reference[:end]
            total = m.read(0x3802, 2)
            for page in (0, 3):
                select(page); m.write(0x4000 if page else 0x3400, bytes([0xA5])*2049)
                buffer = 0x4000 if page else 0x3400
                request(page, fn, regs(hl=buffer, de=128, bc=1, a=sub))
                assert m.read(buffer, end) == reference
                assert m.read(buffer+end, 2049-end) == bytes([0xA5])*(2049-end)
                assert m.read(0x3002, 2) == total
                assert m.read(0x3000, 2) == word(buffer)
                assert m.read(0x3004, 2) == word(1)
                rows.append(dict(case='directory-list-preserves-count-skip-tail', caller=page, function=fn))
        request(0, ks['MX_DOS_DIRECTORY'], regs(hl=0x3200, a=4), inline=True)
        assert m.read(0x3200, 4) == b'A:\\\0'
        rows.append(dict(case='main-inline-MXCALL-routes-to-bank'))
        # На этом этапе больше не используем старую DOS в main. Её код/кэш
        # намеренно портятся; BIOS и новая DOS должны продолжить работу.
        select(0); m.write(0x4000, bytes([0xCC])*0x4C00)
        path = b'B:/PATH20003.BIN'
        m.write(0x3200, path+bytes(128-len(path)))
        # Малый буфер даёт штатную ошибку, не переход в затёртый код main.
        m.write(0x3100, word(0x3200)+word(0x4000)+word(8193)+bytes(34))
        request(0, ks['MX_DOS_FILELOADPATH'], regs(hl=0x3100), expected=7, inline=True)
        path = b'B:/PATH4097.BIN'
        m.write(0x3200, path+bytes(128-len(path)))
        request(0, ks['MX_DOS_FILELOADPATH'], regs(hl=0x3100), inline=True)
        assert m.read(0x4000, 4097) == bytes((i*29+i//257+4097)&255 for i in range(4097))
        rows.append(dict(case='main-DOS-code-erased-load-above-4000', bytes=4097))
        path = b'A:/AUTOEXEC.BAT'
        m.write(0x3200, path+bytes(128-len(path)))
        request(0, ks['MX_DOS_FILELOADPATH'], regs(hl=0x3100), inline=True)
        system_data = (ROOT/'assets/system-rom/autoexec.emu-writable.bat').read_bytes()
        assert m.read(0x4000, len(system_data)) == system_data
        rows.append(dict(case='ROM-bridge-in-BIOS-after-main-DOS-erased', bytes=len(system_data)))
        # Исторические JMP используют тот же маршрут после стирания main DOS.
        def legacy(name, packet, expected=0):
            result = invoke(0, 0, packet, legacy=bios[name])
            assert result == (expected, bool(expected)), (name, result, expected)
        legacy('bios_fileNamePrepare', regs(hl=0x3200, de=0x3300))
        assert m.read(0x3300, 11) == b'AUTOEXECBAT'
        legacy('bios_fileLoadInfo', regs(hl=0x3300))
        assert int.from_bytes(m.read(0x331C, 4), 'little') == len(system_data)
        cursor = m.read(0x331A, 2)+word(0)
        m.write(0x3500, cursor)
        legacy('bios_fileStream', regs(hl=0x4000, de=0x3500, bc=len(system_data), a=2))
        assert m.read(0x4000, len(system_data)) == system_data
        m.write(0x3300,b'REM banked fileExec'+bytes(110))
        legacy('bios_fileExec', regs(hl=0x3300))
        rows.append(dict(case='legacy-C800-metadata-stream-no-main-DOS'))
        # CP/M 14/25 тоже маршрутизируются в DOS, не читают старый v_drive.
        legacy('mx_cpmCall', regs(bc=14, de=1))
        result = invoke(0, 0, regs(bc=25), legacy=bios['mx_cpmCall'])
        assert result[0] == 1
        rows.append(dict(case='main-BDOS-drive-after-main-DOS-erased'))

        # Обратный вызов в страницу владельца, включая чужого инициатора.
        # Обработчик проверяет запрет повторного DOS и вызывает BIOS вложенно.
        callback = out/'observer.asm'
        callback.write_text(f''' INCLUDE "{(ROOT/'source/include/mxos.inc').as_posix()}"
 ORG 03800h
 ld (03600h),a
 ex de,hl
 ld (03601h),hl
 ld a,c
 ld (03603h),a
 ld a,({ts['bankPage']})
 ld (03604h),a
 ld hl,({ts['bankOwner']})
 ld (03605h),hl
 ld hl,03607h
 inc (hl)
 MXCALL MX_DOS_GETERROR
 ld (03608h),a
 ld c,'.'
 MXCALL MX_BIOS_PRINTCHAR
 ret
''', encoding='utf-8')
        subprocess.run([str(ROOT/'tasm/sjasmplus.exe'), '--i8080',
                        '--raw='+str(out/'observer.bin'), str(callback)], check=True, capture_output=True)
        for owner, caller in ((0,0), (3,3), (3,0)):
            select(owner); m.write(0x3800, (out/'observer.bin').read_bytes())
            m.write(0x3600, bytes(16))
            request(owner, ks['MX_DOS_CLI'], regs(hl=0x3800, bc=0x1234, de=0x5678, a=ks['CLI_OP_OBSERVER']))
            assert m.read(0x3000, 6) == regs(hl=0x3800, bc=0x1234, de=0x5678)[:6]
            select(caller)
            path = f'B:/OBS{owner}{caller}'.encode('ascii')
            m.write(0x3200, path+bytes(128-len(path)))
            request(caller, ks['MX_DOS_CLI'], regs(hl=0x3200, a=ks['CLI_OP_MKDIR']))
            select(owner); event = m.read(0x3600, 9)
            assert event == bytes([1,0,0,0,owner,7+caller,caller,1,6]), event.hex()
            request(owner, ks['MX_DOS_CLI'], regs(a=ks['CLI_OP_OBSERVER']))
            select(caller)
            request(caller, ks['MX_DOS_CLI'], regs(hl=0x3200, a=ks['CLI_OP_RMDIR']))
            select(owner); assert m.read(0x3607, 1) == b'\1'
            rows.append(dict(case='CLI-observer-callback-nested-BIOS-reject-DOS', owner=owner, caller=caller))
        request(3, ks['MX_DOS_CLI'], regs(hl=0xF800, a=ks['CLI_OP_OBSERVER']), expected=7)
        print('BANK DOS CALLBACK AND LEGACY PASS', flush=True)
        check_images()
        check_copies()
        # Отдельная пустая тестовая B: с 300 нулевыми файлами. Заполняем
        # метаданные стенда напрямую, чтобы проверить границу пачки списка
        # независимо от сотен одинаковых вызовов создания файла.
        request(0, ks['MX_DOS_FILESELECTDRIVE'], regs(a=1))
        request(0, ks['MX_DOS_FLUSHVOLUME'], regs())
        fixture = bytearray(ram_seed)
        fixture[17:19] = word(384)
        root = (int.from_bytes(fixture[14:16], 'little') +
                fixture[16]*int.from_bytes(fixture[22:24], 'little'))*256
        fixture[root:root+384*32] = bytes(384*32)
        entries = []
        for i in range(300):
            name = f'F{i:07}TXT'.encode('ascii')
            fixture[root+i*32:root+(i+1)*32] = name+b'\x20'+bytes(20)
            entries.append(name+b'\x20'+bytes(4))
        select(2); m.write(0, fixture)
        request(0, ks['MX_DOS_MOUNTVOLUME'], regs())
        for page, skip, capacity in ((0,0,300), (3,0,300), (3,0,256),
                                     (3,1,299), (3,255,45), (3,300,1), (3,0,0)):
            select(page); m.write(0x4000, bytes([0xA5])*(capacity*16+17))
            request(page, ks['MX_DOS_DIRECTORY'], regs(hl=0x4000, de=capacity, bc=skip, a=3))
            expected = b''.join(entries[skip:skip+capacity])+b'\xff'
            assert m.read(0x4000, len(expected)) == expected
            assert m.read(0x4000+len(expected), 16) == bytes([0xA5])*16
            assert m.read(0x3002, 2) == word(300)
            assert m.read(0x3004, 2) == word(skip)
            rows.append(dict(case='directory-list-crosses-4KB-batch', caller=page, skip=skip, capacity=capacity))
        for buffer, capacity in ((0xF7FF,1), (0x4000,4096), (0xFFFF,0)):
            request(3, ks['MX_DOS_FILELIST'], regs(hl=buffer, de=capacity), expected=7)
            rows.append(dict(case='reject-directory-list-range', address=buffer, capacity=capacity))
        print('BANK DOS MAIN PUBLIC ABI PASS', flush=True)
        finish_report()
    except Exception:
        if matrix.pid and h['mcp_owner']() == str(matrix.pid):
            state = matrix.m.call('cpu_state')
            state['transportPage'] = matrix.m.read(ts['bankPage'], 1).hex()
            state['stack'] = matrix.m.read(state['registers']['sp']['dec'], 48).hex()
            state['window'] = matrix.m.read(0xFFC0, 32).hex()
            state['bankState'] = matrix.m.read(ts['bankDepth'], 48).hex()
            (out/'failure.json').write_text(json.dumps(state, indent=2)+'\n', encoding='utf-8')
            matrix.m.screenshot(out/'failure.png')
        raise
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
