"""Сверка банкового BIOS ABI с прямыми вызовами основного BIOS в Emu80."""
from pathlib import Path
import argparse
import hashlib
import json
import re
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(), re.M)}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, default=ROOT/'.local/monitor2-001')
    parser.add_argument('--report', type=Path, default=ROOT/'docs/banked/bios-001')
    args = parser.parse_args()
    out = args.report.resolve(); out.mkdir(parents=True, exist_ok=True)
    subprocess.run(['python', str(ROOT/'scripts/build/build-banked-transport.py')], check=True)
    base = ROOT/'bin/banked'
    ts = symbols(base/'TRANSPORT.sym'); bs = symbols(base/'MAINBIOS.sym')
    cs = symbols(base/'CLIENT.sym')
    live = symbols(ROOT/'bin/BIOS.sym')
    bios = (ROOT/'bin/IO.sys').read_bytes()
    bridge = (base/'MAINBIOS.bin').read_bytes()
    assert bios[bs['BANK_BIOS_GATE']-live['BIOS_IO_ADDR']:][:len(bridge)] == bridge, 'Rebuild BIOS and system image first'
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    g = h['Matrix'].__init__.__globals__; g['OUT'] = out; g['RUNTIME'] = args.runtime.resolve()
    matrix = h['Matrix'](); rows = []
    word = lambda n: n.to_bytes(2, 'little')
    regs = lambda hl=0, de=0, bc=0, af=2: word(hl)+word(de)+word(bc)+word(af)
    try:
        matrix.start('banked-bios')
        m = matrix.m; m.call('debug_break'); m.call('bp_clear')
        runtime = (base/'TRANSPORT.bin').read_bytes(); window = (base/'WINDOW.bin').read_bytes()

        def select(page):
            m.write(0xFFFD if page else 0xFFFC, bytes([page-1 if page else 0]))

        for page in (0, 1, 3):
            select(page); m.write(ts['BANK_LANDING'], runtime)
            m.write(ts['bankPage'], bytes([page])); m.write(ts['bankOwner'], bytes([7+page, page]))
            top, low = (bs['BANK_BIOS_STACK_TOP'], bs['BANK_BIOS_STACK_LOW']) if page == 0 else (cs['DOS_BANK_STACK_TOP'], cs['DOS_BANK_STACK_BOTTOM'])
            m.write(ts['bankServiceSP'], word(top)); m.write(ts['bankStackFloor'], word(low+64))
            m.write(low-16, b'guard-bios-stack')
            m.write(cs['mx_call'], (base/'CLIENT.bin').read_bytes())
            m.write(cs['LOCAL_BLOCK_ENTRY'], (base/'LOCALBLOCK.bin').read_bytes())
            if page == 0:
                m.write(live['MEM_NATIVE_CALL'], word(live['sysCall']))
                m.write(live['MEM_LOCAL_A'], word(live['bios_blockIO']))
        select(0)
        assert m.read(bs['BANK_BIOS_GATE'], len(bridge)) == bridge, 'Gateway must be loaded by bootstrap'
        m.write(0xFFC0, window)
        bios_state = m.read(live['bios_vars'], 33)

        def reset_screen():
            select(0); m.write(0x9000, bytes(0x3000)); m.write(live['bios_vars'], bios_state)
            m.write(live['bios_vars.cursorY'], b'\0\0')
            m.write(live['bios_vars.escMode'], b'\0')
            m.write(live['bios_vars.inverse'], b'\0\0')
            m.write(live['bios_vars.koi7'], b'\0')

        def run(page, operation, packet, data=b'', address=0x3200, inline=False, packet_at=0x3000, key=None, busy=0):
            select(page)
            m.write(packet_at, packet)
            if data: m.write(address, data)
            if inline:
                # Установка регистров CPU с последующим CALL/DW, без номера в B.
                stub = b'\xcd'+word(cs['mx_callRegs'])+word(operation)+b'\xc3'+word(0x7000)
                cpu = dict(hl=int.from_bytes(packet[:2], 'little'), de=int.from_bytes(packet[2:4], 'little'),
                           bc=int.from_bytes(packet[4:6], 'little'), af=int.from_bytes(packet[6:8], 'little'))
            else:
                stub = b'\xcd'+word(cs['mx_call'])+b'\xc3'+word(0x7000)
                cpu = dict(bc=operation, de=packet_at)
            m.write(0x2000, stub); m.stop_at(0x7000)
            m.call('regs_set', pc=0x2000, sp=0x7000, **cpu)
            ticks = m.call('cpu_state')['cpu_ticks']
            if key:
                m.call('emu_run_for', ms=50)
                m.call('kbd_send', keys=[key], action='tap', duration_ms=100)
            state = m.until(0x7000, 10)
            elapsed = state['cpu_ticks']-ticks
            result = b''.join(word(state['registers'][k]['dec']) for k in ('hl', 'de', 'bc', 'af'))
            assert state['registers']['sp']['dec'] == 0x7000
            assert m.read(ts['bankPage'], 1) == bytes([page])
            assert m.read(ts['bankOwner'], 2) == bytes([7+page, page])
            assert m.read(ts['bankDepth'], 1) == b'\0'
            if page: assert m.read(0xFFC0, 12) == window[:12]
            saved = m.read(packet_at, 8)
            select(0)
            screen = m.read(0x9000, 0x3000)
            cursor = m.read(live['bios_vars.cursorY'], 2)
            assert m.read(bs['bbBusy'], 1) == bytes([busy])
            for p in (0, 1, 3):
                select(p)
                low = bs['BANK_BIOS_STACK_LOW'] if p == 0 else cs['DOS_BANK_STACK_BOTTOM']
                assert m.read(low-16, 16) == b'guard-bios-stack'
                assert m.read(ts['bankDepth'], 1) == b'\0'
            select(page)
            return dict(registers=result.hex(), saved=saved.hex(), cursor=cursor.hex(), ticks=elapsed), screen

        # Скалярные вызовы должны возвращать в точности тот же AF/BC/DE/HL.
        cases = [('char', 2, regs(0x1234, 0x5678, 0xAB41, 0x55D7)),
                 ('set-cursor', 10, regs(0x2820, 0x4321, 0x5678, 0xAA03)),
                 ('decimal', 21, regs(0x1234, 12345, 0x2000, 2)),
                 ('uppercase', 23, regs(0x1234, 0x5678, 0x9ABC, ord('q') << 8)),
                 ('subtract', 26, regs(10, 23, 0x1234)),
                 ('divide-two', 27, regs(0xFEDC, 0x1234, 0x5678)),
                 ('multiply', 28, regs(123, 71, 0x1234)),
                 ('divide', 29, regs(12345, 71, 0x1234))]
        for name, fn, packet in cases:
            reset_screen(); direct, screen = run(0, 0x100+fn, packet)
            for page in (1, 3):
                for inline in (False, True):
                    reset_screen(); banked, actual = run(page, 0x100+fn, packet, inline=inline)
                    assert banked['registers'] == direct['registers'], (name, page, inline, direct, banked)
                    assert actual == screen and banked['cursor'] == direct['cursor'], name
                    if not inline: assert banked['saved'] == banked['registers']
                    rows.append(dict(case=name, page=page, inline=inline, directTicks=direct['ticks'], bankTicks=banked['ticks']))
        for text in (b'BIOS from DOS bank\r\n\0', b'x'*255+b'\0', b'y'*256+b'\0', b'z'*521+b'\0'):
            packet = regs(0x3200, 0x4567, 0x89AB, 0x5503)
            reset_screen(); direct, screen = run(0, 0x107, packet, text)
            # Основная память намеренно содержит другую строку по тому же адресу.
            select(0); m.write(0x3200, b'WRONG MAIN POINTER\0')
            reset_screen(); banked, actual = run(1, 0x107, packet, text, inline=True)
            assert actual == screen and banked['registers'] == direct['registers'], (len(text), direct, banked)
            rows.append(dict(case='string', bytes=len(text)-1, directTicks=direct['ticks'], bankTicks=banked['ticks']))
        # Конец буфера прямо перед хвостом. Никаких чтений через F800h.
        reset_screen(); result, screen = run(3, 0x107, regs(0xF7FC), b'end\0', 0xF7FC)
        assert bytes.fromhex(result['registers'])[:2] == word(0xF7FF)
        rows.append(dict(case='string-at-boundary'))
        # Геометрия содержит нули; линия отличается от окна длиной записи.
        geometry = bytes([2, 40, 0x92, 54, 18, 0x81, 65, 0x94, 5, 0xFF, 0xFF, 3, 50, 0x95, 40, 0x80, 0])
        toolbar = bytes([2])+b'F1\0'+bytes([0x84])+b'Left\0'+bytes([2])+b'F2\0'+bytes([0x84])+b'Right\0\0'
        for name, fn, data in [('geometry', 34, geometry), ('toolbar', 36, toolbar)]:
            packet = regs(0x3200, bc=0xE0B0)
            reset_screen(); direct, screen = run(0, 0x100+fn, packet, data)
            select(0); m.write(0x3200, bytes(256))
            reset_screen(); banked, actual = run(3, 0x100+fn, packet, data, inline=True)
            assert actual == screen, name
            rows.append(dict(case=name, directTicks=direct['ticks'], bankTicks=banked['ticks']))
        pattern = bytes((i*7+i//256)&255 for i in range(0x3000))
        for name, fn, packet in [('inverse', 35, regs(0x950F, 0xABCD, 0x0410)),
                                 ('copy-rectangle', 37, regs(0x950F, 0x0410, 0x900A))]:
            reset_screen(); select(0); m.write(0x9000, pattern)
            direct, screen = run(0, 0x100+fn, packet)
            reset_screen(); select(0); m.write(0x9000, pattern)
            banked, actual = run(1, 0x100+fn, packet, inline=True)
            assert actual == screen and banked['registers'] == direct['registers'], name
            rows.append(dict(case=name, directTicks=direct['ticks'], bankTicks=banked['ticks']))
        # Необслуживаемые указатели не передаются в основной BIOS вслепую.
        for fn in (4, 13, 15, 16, 18, 24, 25, 38, 39, 42, 43, 44, 255):
            packet = regs(0x3200, 0x3250, 0x3300)
            result, _ = run(1, 0x100+fn, packet)
            af = int.from_bytes(bytes.fromhex(result['registers'])[6:8], 'little')
            assert af >> 8 == live['DOS_ERR_UNSUPPORTED'] and af & 1
            assert result['saved'] == packet.hex()
            rows.append(dict(case='unsupported', function=fn))
        for fn, packet, data, addr in [(7, regs(0xF800), b'', 0x3200),
                                       (34, regs(0x3200), bytes([2])*256, 0x3200),
                                       (36, regs(0x3200), b'\x84'+b'x'*255, 0x3200)]:
            reset_screen(); result, screen = run(1, 0x100+fn, packet, data, addr)
            af = int.from_bytes(bytes.fromhex(result['registers'])[6:8], 'little')
            assert af >> 8 == live['DOS_ERR_ARGUMENT'] and af & 1, result
            assert result['saved'] == packet.hex() and not any(screen)
            rows.append(dict(case='bad-pointer-or-list', function=fn))
        select(0); m.write(bs['bbBusy'], b'\x01')
        packet = regs(bc=ord('Q'))
        result, _ = run(1, 0x102, packet, busy=1)
        af = int.from_bytes(bytes.fromhex(result['registers'])[6:8], 'little')
        assert af >> 8 == live['DOS_ERR_ACCESS'] and af & 1
        assert result['saved'] == packet.hex()
        select(0); m.write(bs['bbBusy'], b'\0')
        rows.append(dict(case='reentry-retains-outer-busy'))
        # BIOS ожидает клавишу, пока активен вложенный служебный стек main.
        result, _ = run(3, 0x100, regs(), key='enter')
        af = int.from_bytes(bytes.fromhex(result['registers'])[6:8], 'little')
        assert af >> 8 == 13, result
        assert result['saved'] == result['registers']
        rows.append(dict(case='getch-from-bank', key=af >> 8))
        # Реальная последовательность main -> банк DOS -> BIOS -> DOS -> main.
        # Внутри DOS используется F806h и пакет на его служебном стеке F7xxh.
        select(1)
        nested_text = b'Nested DOS -> BIOS\r\n\0'
        m.write(0x3200, nested_text)
        m.write(0x2100, b'\x21'+word(0x3200)+b'\xcd'+word(cs['mx_callRegs'])+word(0x107)+b'\xc9')
        select(0)
        m.write(0x2000, b'\x3e\x01\x21'+word(0x2100)+b'\xcd'+word(ts['bankCall'])+b'\xc3'+word(0x7000))
        m.stop_at(0x7000); m.call('regs_set', pc=0x2000, sp=0x7000)
        state = m.until(0x7000, 10)
        assert state['registers']['sp']['dec'] == 0x7000
        assert state['registers']['hl']['dec'] == 0x3200+len(nested_text)-1
        assert m.read(ts['bankPage'], 1) == b'\0'
        for page in (0, 1, 3):
            select(page)
            assert m.read(ts['bankDepth'], 1) == b'\0'
            assert m.read(ts['bankOwner'], 2) == bytes([7+page, page])
        rows.append(dict(case='main-DOS-BIOS-DOS-main'))
        reset_screen(); run(1, 0x107, regs(0x3200), b'BIOS 6.0 bank gateway\r\nText and graphics from bank 0\0')
        select(0); m.screenshot(out/'bank-bios.png')
        report = dict(cases=rows, mainBridgeBytes=(base/'MAINBIOS.bin').stat().st_size,
                      bankClientBytes=(base/'CLIENT.bin').stat().st_size, emulator=True, cpuHz=2000000,
                      sha256={name: hashlib.sha256((base/(name+'.bin')).read_bytes()).hexdigest()
                              for name in ('TRANSPORT', 'WINDOW', 'CLIENT', 'MAINBIOS')},
                      biosSha256=hashlib.sha256((ROOT/'bin/BIOS.sys').read_bytes()).hexdigest())
        (out/'result.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
        print(f'Bank BIOS: {len(rows)} cases passed')
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
