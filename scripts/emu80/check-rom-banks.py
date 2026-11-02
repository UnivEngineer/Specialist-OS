"""Чтение системного ROM по банкам и загрузка файлов выше 64 КБ; отдельный Emu80."""
import argparse
import json
import re
import runpy
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(Path(__file__).with_name('check-copy.py')))['FileMachine']
symbols = runpy.run_path(str(Path(__file__).with_name('check-flash-allocation.py')))['symbols']
BOUNDARIES = [0, 1, 127, 128, 255, 256, 511, 512, 1023, 1024, 1535, 1536, 2047]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', type=Path, required=True)
    p.add_argument('--phase', choices=['pattern', 'read', 'relocate', 'boot', 'rom64'], required=True)
    p.add_argument('--size', type=int, choices=[128, 256, 512], default=512)
    args = p.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужна отдельная установка Emu80-flash-test')
    path = args.runtime / 'specialist-os/system-flash.bin'
    reportdir = ROOT / 'docs/rom-banks'
    reportdir.mkdir(parents=True, exist_ok=True)
    if args.phase == 'pattern':
        source = ROOT / ('images/SystemRom_emu80_writable.bin' if args.size == 512 else f'images/SystemRom_{args.size}K.bin')
        b = bytearray(source.read_bytes())
        for sector in BOUNDARIES:
            if sector >= 127 and sector < args.size * 4:
                b[sector*256:(sector+1)*256] = bytes((i*29 + sector*7 + (sector >> 8)*43) & 255 for i in range(256))
        path.write_bytes(b)
        print(f'Уникальные данные на границах банков, том {args.size} КБ')
        return
    if args.phase == 'relocate':
        b = bytearray((ROOT / 'images/SystemRom_emu80_writable.bin').read_bytes())
        word = lambda n: struct.unpack_from('<H', b, n)[0]
        fat = word(14)*256
        root = word(14)+b[16]*word(22)
        datastart = root+(word(17)*32+255)//256
        moves = {b'FONT    FNT': 0x200, b'FORMAT  BAT': 0x210, b'FORMAT  COM': 0x220,
                 b'FLASHSYSCOM': 0x280, b'AUTOEXECBAT': 0x300}
        for name, sector in moves.items():
            entry = next(i for i in range(root*256, datastart*256, 32) if b[i:i+11] == name)
            c = word(entry+26)
            chain = []
            while c not in chain:
                chain.append(c)
                c = word(fat+2*c)
            data = b''.join(bytes(b[(datastart+c-2)*256:(datastart+c-1)*256]) for c in chain)
            for c in chain:
                struct.pack_into('<H', b, fat+2*c, 0)
            new = sector-datastart+2
            for n in range(len(chain)):
                assert word(fat+2*(new+n)) == 0
                struct.pack_into('<H', b, fat+2*(new+n), new+n+1 if n+1 < len(chain) else new+n)
            b[sector*256:sector*256+len(data)] = data
            struct.pack_into('<H', b, entry+26, new)
        path.write_bytes(b)
        (reportdir / 'relocated.json').write_text(json.dumps({k.decode():v*256 for k,v in moves.items()}, indent=2)+'\n')
        print('FONT, FORMAT.BAT/COM, FLASHSYS.COM и AUTOEXEC целиком перенесены выше 64 КБ')
        return
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    nc = (ROOT / 'bin/NC.com').read_bytes()
    nccode = nc[:symbols(ROOT / 'bin/NC.lst')['state']-0xE800]
    for _ in range(180):
        m.call('emu_run_for', ms=500)
        if m.read(0xE800, len(nccode)) == nccode:
            break
    else:
        raise RuntimeError('NC не загружен')
    m.call('bp_set', addr=0x8100, type='exec')
    syms = symbols(ROOT / 'bin/DOS.lst')
    def select(drive):
        assert not m.subroutine(0xC842, af=drive<<8, de=1)['flags']['C']
    def native():
        select(1)
        m.subroutine(0xC860, af=0, hl=syms['diskDriver'])
        select(0)
    if args.phase == 'boot':
        assert m.read(0xE000, 2048) == (ROOT / 'assets/system-rom/FONT.FNT').read_bytes()
        assert m.read(0xDC00,8)==m.resident_bytes('FlashSys')[:8]
        assert m.read(0xDE00,8)==m.resident_bytes('Flash')[:8]
        # Запускаем высоко размещённый AUTOEXEC непосредственно встроенным драйвером.
        select(1)
        m.write(0xDE00, (ROOT / 'bin/Rom.com').read_bytes())
        m.subroutine(0xDE00)
        native()
        m.write(0x7000, b'A:AUTOEXEC.BAT\0')
        assert not m.subroutine(0xC85D, hl=0x7000)['flags']['C']
        assert m.read(0xDE00,8)==m.resident_bytes('Flash')[:8]
        report = dict(boot=True, font=True, highFlashSysLoaded=True, nativeHighAutoexec=True)
        (reportdir / 'boot.json').write_text(json.dumps(report, indent=2)+'\n')
        print(report)
        return
    native()
    if args.phase == 'rom64':
        b = (args.runtime / 'specialist-os/system.bin').read_bytes()
        logical = b[32768:]+b[4:32768]+bytes([255])*4
        sectors = [0,1,127,128,254,255]
        total = 256
    else:
        logical = path.read_bytes()
        total = args.size*4
        sectors = [s for s in BOUNDARIES if s < total]
    def reads(address, checked):
        for sector in sectors:
            m.write(0x4000, b'\xBE'+bytes(256)+b'\xEF')
            m.write(0x123, b'\x6A')
            state = m.subroutine(address, af=0x55D7, bc=0x025A, de=sector, hl=0x4001)
            if checked:
                assert not state['flags']['C'], sector
            else:
                assert state['flags']['C'], 'Legacy сохраняет входной CF'
            for reg,value in [('bc',0x025A),('de',sector),('hl',0x4001),('sp',0x8F00)]:
                assert state['registers'][reg]['dec'] == value
            length = 252 if args.phase == 'rom64' and sector == 255 else 256
            assert m.read(0x4001,length) == logical[sector*256:sector*256+length], hex(sector)
            assert m.read(0x4000,1) == b'\xBE' and m.read(0x4101,1) == b'\xEF'
            assert m.read(0x123,1) == b'\x6A', 'Основное ОЗУ не восстановлено'
    reads(0xC863, True)
    # Необёрнутый вариант проверяем тем же аппаратным маппером, только чтение.
    legacy = (ROOT / 'bin/DOS-rom-legacy.sys').read_bytes()
    legsyms = symbols(ROOT / 'bin/DOS-rom-legacy.lst')
    m.write(0xC000, legacy)
    m.write(legsyms['v_drive'], b'\0')
    m.write(legsyms['v_diskInfo']+10, struct.pack('<H',total))
    reads(legsyms['diskDriver'], False)
    report = dict(layout=args.phase, sizeKiB=total//4, physicalImageBytes=len(logical), sectors=sectors,
                  checked=True, legacy=True, registersPreserved=True, ramRestored=True)
    (reportdir / f'{args.phase}-{total//4}.json').write_text(json.dumps(report, indent=2)+'\n')
    print(report)


if __name__ == '__main__':
    main()
