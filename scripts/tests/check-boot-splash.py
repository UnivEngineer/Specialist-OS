"""8080: early screen clear, complete BOOT relocation and historical bitmap."""
from pathlib import Path
import re
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
TICKS = Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')


def symbols(name):
    return {k: int(v, 16) for k, v in re.findall(
        r'^([\w.]+): EQU 0x([\da-fA-F]+)',
        (ROOT / 'bin/bootstrap' / (name + '.sym')).read_text(), re.M)}


sector = symbols('sector')
boot = symbols('BOOT')
image = (ROOT / 'bin/bootstrap/BOOT.bin').read_bytes()
memory = bytearray([0xA5]) * 65536
memory[:256] = (ROOT / 'bin/bootstrap/sector.bin').read_bytes()
memory[0x2000:0x2000 + len(image)] = image


def run(source, start, end):
    with tempfile.TemporaryDirectory(prefix='mxos-splash-') as folder:
        folder = Path(folder)
        (folder / 'in').write_bytes(source)
        result = subprocess.run([
            str(TICKS), '-m8080', '-pc', f'{start:x}', '-end', f'{end:x}',
            '-counter', '2000000', '-output', str(folder / 'out'),
            str(folder / 'in')], capture_output=True, text=True, check=True)
        output = (folder / 'out').read_bytes()
        assert int.from_bytes(output[65542:65544], 'little') == end
        return bytearray(output[:65536]), int(result.stdout.strip())


cleared, ticks = run(memory, 0, sector['screenCleared'])
assert cleared[0x9000:0xC000] == bytes(0x3000)
assert cleared[:0x9000] == memory[:0x9000]
assert cleared[0xC000:0xFFF8] == memory[0xC000:0xFFF8]
assert cleared[0xFFF8] == 0xF0
assert ticks < 100000
print(f'Screen clear: {ticks} cycles, {ticks / 2000:.3f} ms at 2 MHz')

relocated, copy_ticks = run(cleared, sector['screenCleared'], boot['stage'])
assert relocated[0x8000:0x8000 + len(image)] == image
assert relocated[0x9000:0xC000] == bytes(0x3000)
shown, draw_ticks = run(relocated, boot['stage'], boot['logoShown'])
expected = bytearray(0x3000)
bitmap = (ROOT / 'source/bootstrap/splash.bin').read_bytes()
for column in range(46):
    offset = 0x100 + column * 256 + 108
    expected[offset:offset + 39] = bitmap[column * 39:(column + 1) * 39]
assert shown[0x9000:0xC000] == expected
assert shown[0xC000:0xFFF8] == memory[0xC000:0xFFF8], 'BIOS copied before logo'
assert shown[0xFFF8] == 0xF0, 'Default color not restored'
assert 0x8000 + len(image) <= boot['BOOT_STAGE_LIMIT']
print(f'BOOT SPLASH PASS: {len(image)} bytes relocated, exact bitmap before BIOS')
print(f'Copy BOOT: {copy_ticks / 2000:.3f} ms; '
      f'draw: {draw_ticks / 2000:.3f} ms; '
      f'total: {(copy_ticks + draw_ticks) / 2000:.3f} ms at 2 MHz')
