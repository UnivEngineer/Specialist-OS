"""Сборка фиксированных и перемещаемых установщиков для арены DOS."""
from pathlib import Path
import runpy
import sys
import tempfile

ROOT = Path(__file__).absolute().parents[2]
BIN = ROOT / 'bin'
CPU = runpy.run_path(str(ROOT / 'scripts/images/cpu.py'))
EXE = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))
PACK = runpy.run_path(str(ROOT / 'scripts/build/pack-dos.py'))


def fixed_origin(source, target, kernel):
    origin = kernel['DOS_BANK_DRIVER_ADDR']
    for _ in range(8):
        meta = PACK['symbols'](target.with_suffix('.sym'))
        bss = meta.get('IMAGE_BSS_SIZE', 0)
        if bss == 0xffff:
            raise ValueError('DOS initializer requires a known workspace size')
        end = meta.get('IMAGE_MEM_END', origin + target.stat().st_size + bss)
        new = origin + kernel['DOS_BANK_DRIVER_END'] - end
        if new < kernel['DOS_BANK_DRIVER_ADDR']:
            raise ValueError('Installer exceeds the shared DOS arena')
        if new == origin:
            if meta.get('IMAGE_MEM_BEGIN', origin) < kernel['DOS_BANK_DRIVER_ADDR']:
                raise ValueError('Installer workspace starts below the shared arena')
            return origin
        origin = new
        PACK['assemble'](source, target, [f'DRIVER_ORIGIN=0x{origin:04X}'])
    raise ValueError('Installer alignment did not stabilize')


def verify_driver(source, target, image, kernel):
    if image.stack > kernel['DOS_BANK_STACK_TOP'] - kernel['DOS_BANK_STACK_BOTTOM'] - 64:
        raise ValueError('Installer stack requirement exceeds the DOS stack')
    alignment = 1 << image.align_log2
    low = (kernel['DOS_ARENA_START'] + alignment - 1) & -alignment
    high = (kernel['DOS_BANK_DRIVER_END'] - image.memory()[2] - image.min_heap) & -alignment
    if low > high:
        raise ValueError('No aligned position for installer in the DOS arena')
    for base in sorted({low, high}):
        reference = target.with_name('reference.i80')
        PACK['assemble'](source, reference, [f'DRIVER_ORIGIN=0x{base:04X}'])
        size = PACK['symbols'](reference.with_suffix('.sym'))['IMAGE_BODY_SIZE']
        if reference.read_bytes()[:size] != EXE['relocated_body'](image, base):
            raise ValueError(f'{source.name}: relocation differs from assembly at {base:04X}h')


def build(source):
    source = Path(source).absolute()
    kernel = PACK['symbols'](BIN / 'banked/KERNEL.sym')
    (ROOT / '.local').mkdir(exist_ok=True)
    with tempfile.TemporaryDirectory(prefix='driver-build-', dir=ROOT / '.local') as folder:
        if source.stem.upper() == 'FLOPPY':
            # У службы свой ORG и таблица. Установщик включает готовые байты.
            service = Path(folder) / 'FLOPPYSVC.bin'
            PACK['assemble'](source.with_name('service.asm'), service, [])
            service.replace(BIN / service.name)
        target = Path(folder) / source.with_suffix('.i80').name
        PACK['assemble'](source, target, [])
        meta = PACK['symbols'](target.with_suffix('.sym'))
        relocatable = 'IMAGE_RELOC_KIND' in meta
        origin = meta.get('IMAGE_LOAD_OFFSET', 0) if relocatable else fixed_origin(source, target, kernel)
        entry = meta.get('IMAGE_ENTRY_OFFSET', origin)
        symbols = target.with_suffix('.sym').read_text('utf-8')
        descriptor = target.with_suffix('.cpu')
        descriptor.write_text(CPU['format_cpu'](origin, entry, flags=1, fields=CPU['fields_from_symbols'](symbols)), 'ascii')
        image = EXE['decode'](descriptor.read_bytes(), 'cpu', path=descriptor)
        if relocatable:
            verify_driver(source, target, image, kernel)
        for ext in ('.i80', '.sym', '.lst', '.cpu'):
            target.with_suffix(ext).replace(BIN / source.with_suffix(ext).name)
    print(f'{source.name}: load {origin:04X}h, {len(image.data)} bytes, {len(image.relocations)} relocations')


if __name__ == '__main__':
    build(sys.argv[1])
