"""Новый BIOS 6x8: загрузка NC и прямой ABI-тест обоих путей в Emu80."""
from pathlib import Path
import argparse
import json
import re
import runpy
import subprocess

R = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--report', type=Path, default=R/'docs/bench/bios-font/emulator')
p.add_argument('--runtime', type=Path, default=R/'.local/bios-font-20261109')
a = p.parse_args()
O = a.report.resolve()
O.mkdir(parents=True, exist_ok=True)
h = runpy.run_path(str(R/'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=O, RUNTIME=a.runtime.resolve())
x = h['Matrix']()
m = x.m
S = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', (R/'bin/BIOS.sym').read_text(), re.M)}
timer_asm=O/'local-timer.asm'
timer_asm.write_text('    INCLUDE "'+(R/'source/include/mxos.inc').as_posix()+'"\n    ORG 08400h\n    INCLUDE "'+(R/'source/include/timing-pit.inc').as_posix()+'"\n',encoding='utf8')
subprocess.run([str(R/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(O/'local-timer.bin'),'--sym='+str(O/'local-timer.sym'),str(timer_asm)],check=True,capture_output=True)
timer_sy={k:int(v,16) for k,v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)',(O/'local-timer.sym').read_text(),re.M)}
W = lambda n: n.to_bytes(2, 'little')
mx = lambda name: b'\xcd'+W(S['mx_callRegs'])+W(S['MXOS_BIOS_'+name])

def api(name, af=0, hl=0):
    m.stop_at(0x8E10)
    m.write(0x8E00, mx(name)+b'\xc3'+W(0x8E10))
    m.call('regs_set', pc=0x8E00, sp=0x8FDF, af=af, hl=hl, bc=0, de=0)
    return m.until(0x8E10, 120, page=0)

try:
    x.start('bios-font', system=R/'images/SystemRom_emu80_writable.bin')
    m.screenshot(O/'nc.png')
    # F7 открывает штатный диалог NC с логическим inverse=1; отмена без записи.
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.tap('f7')
    m.screenshot(O/'dialog.png')
    assert m.read(S['bios_vars.inverse'], 1)[0] in (1, 255)
    m.tap('esc')
    m.call('emu_run_for', ms=500)
    assert m.read(S['bios_vars.inverse'], 1) == b'\0'
    # Это отдельная проверка ABI после нормальной загрузки, не ввод команды NC.
    # Дальнейший тест владеет main и завершается закрытием своего экземпляра.
    m.call('debug_break')
    source = (R/'assets/system-rom/README.TXT').read_bytes().replace(b'\r\n', b' ').replace(b'\r', b' ').replace(b'\n', b' ')
    source = bytes(c if c >= 32 else 32 for c in source)[:1152].ljust(1152, b' ')
    images = []
    times = {}
    for mode, label, scalar in [(2, 'rotate', False), (1, 'prerender', False), (2, 'char-rotate', True), (1, 'char-prerender', True)]:
        state = api('FONT_RENDER', af=mode<<8, hl=0x6000)
        assert not state['flags']['C'], state
        assert state['registers']['af']['dec']>>8 == (mode == 1), state
        m.write(0x3000, b''.join(source[row*48:row*48+48]+b'\0' for row in range(24)))
        code = b'\x31'+W(0x8FDF)+b'\xcd'+W(S['bios_clearScreenOld'])
        m.write(0x8400,(O/'local-timer.bin').read_bytes())
        code += b'\xcd'+W(timer_sy['pitTimerInit'])+b'\xcd'+W(timer_sy['pitTimerRead'])+b'\x22'+W(0x4000)
        for row in range(24):
            code += b'\x21'+W(7+row*8)+mx('SET_CURSOR_POS')
            code += b'\x21'+W(0x3000+row*49)
            if scalar:
                # Штатный посимвольный ABI сохраняет указатель и счётчик.
                code += b'\x06\x30'
                loop = 0x1000+len(code)
                code += b'\x4E\xCD'+W(S['bios_printChar'])+b'\x23\x05\xC2'+W(loop)
            else:
                code += mx('PRINT_STRING_MAIN')
        code += b'\xcd'+W(timer_sy['pitTimerRead'])+b'\x22'+W(0x4002)+b'\xc3'+W(0x2000)
        m.write(0x1000, code)
        m.stop_at(0x2000)
        m.call('regs_set', pc=0x1000, sp=0x8FDF)
        m.until(0x2000, 120, page=0)
        counters = m.read(0x4000, 4)
        elapsed = (int.from_bytes(counters[:2], 'little')-int.from_bytes(counters[2:], 'little')) & 65535
        times[label] = round(elapsed*.128, 3)
        assert elapsed > 0
        images.append(m.read(0x9000, 12288))
        m.write(0x4500, f'BIOS 6x8 {label}: {times[label]:.3f} ms\0'.encode())
        api('SET_CURSOR_POS', hl=207)
        api('PRINT_STRING_MAIN', hl=0x4500)
        m.screenshot(O/(label+'.png'))
    assert all(image == images[0] for image in images), 'scalar/batch/rotate/prerender bitmap mismatch'
    assert not api('FONT_RENDER', af=0x200)['flags']['C']
    (O/'results.json').write_text(json.dumps(dict(ncBoot='PASS', inverseDialog='PASS', publicAPI='PASS', bitmap='PASS', bytes=10240, ms=times, timer='8253, Emu80 2 MHz', preparationExcluded=True), indent=2)+'\n')
    print('BIOS FONT EMULATOR PASS', times, flush=True)
finally:
    x.stop()
