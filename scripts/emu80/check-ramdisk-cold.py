"""Bootstrap: случайное ОЗУ B:, сохранение при Reset и Enter после ошибки AUTOEXEC."""
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / 'bin/tests/ramdisk-cold'
OUT.mkdir(parents=True, exist_ok=True)
h = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=OUT)
x = h['Matrix']()
m = x.m
k = h['symbols'](ROOT / 'bin/banked/KERNEL.sym')
b = h['symbols'](ROOT / 'bin/BIOS.sym')
n = h['symbols'](ROOT / 'bin/NC.sym')


def page(bank):
    m.write(0xfffd if bank else 0xfffc, bytes([bank-1 if bank else 0]))


def reset_to(address):
    page(0)
    m.stop_at(address)
    m.call('emu_reset')
    m.until(address, 60, page=0)


def pool():
    page(1)
    result = tuple(m.read(k[name], 1)[0] for name in
                   ('memoryRamPages', 'memoryRamOwner', 'memoryRamFresh'))
    page(0)
    return result


try:
    x.start('RAM disk cold/warm boot')
    m.call('debug_break')
    m.call('bp_clear')
    assert pool() == (4, 1, 0), pool()
    # Уничтожаем первый сектор и код хвоста только в собственном стенде.
    # Готовность DOS остаётся 1: именно проверка хвоста должна запретить warm.
    page(2)
    random_boot = bytearray((i*37+19) & 255 for i in range(256))
    random_boot[3:5] = b'MX'  # Не доверять даже случайно похожему заголовку.
    m.write(0, random_boot)
    m.write(0xff04, bytes([m.read(0xff04, 1)[0] ^ 0xff]))
    reset_to(n['mainLoop'])
    assert pool() == (4, 1, 0), pool()
    page(2)
    boot = m.read(0, 256)
    assert boot[11:13] == b'\0\x01' and boot[19:21] == b'\0\x04'
    assert boot[54:62] == b'FAT16   '
    marker = bytes((i*13+71) & 255 for i in range(256))
    m.write(0x6000, marker)
    reset_to(n['mainLoop'])
    assert pool() == (4, 1, 0), pool()
    page(2)
    assert m.read(0, 256) == boot
    assert m.read(0x6000, 256) == marker
    page(0)
    print('PASS: random RAM + lost MEM tail formats B:; warm Reset preserves B:', flush=True)
    # Отказ RAMLIMIT: закреплённый пул не совпадает с -P 4 в AUTOEXEC.
    page(1)
    m.write(k['memoryRamPages'], b'\x03')
    reset_to(b['biosBootWaitEnter'])
    m.screenshot(OUT / 'autoexec-error.png')
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.stop_at(b['biosBootWaitEnter'])
    m.tap('x')
    m.until(b['biosBootWaitEnter'], 10, page=0)
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    m.stop_at(n['mainLoop'])
    m.tap('enter')
    m.until(n['mainLoop'], 60, page=0)
    print('PASS: AUTOEXEC line 3 failure waits for Enter; another key does not dismiss it', flush=True)
finally:
    x.stop()
