"""Build the standalone MX reset ROM and the reserved boot area of its diskette."""
from pathlib import Path
import runpy
import struct
import sys
import zlib

from build_cache import python_build

ROOT = Path(__file__).absolute().parents[2]
BIN = ROOT / 'bin'
OUT = BIN / 'mx'
ROTATE = BIN / 'fonts/Rotate.i80'
ROM_FONT = BIN / 'fonts/Graph8x4.i80'
ROM_BOOT_ORIGIN = 0x600
PACK = runpy.run_path(str(ROOT / 'scripts/build/pack-dos.py'))
CPU = runpy.run_path(str(ROOT / 'scripts/images/cpu.py'))
EXE = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))
PROFILE = ['MX_FLOPPY_BOOT=1']
APPS = ('NC', 'EDIT', 'CLI', 'FONT', 'RAMDISK')


def build_floppy_components():
    for name in APPS:
        source = ROOT / f'source/{name.lower()}/{name}.asm'
        target = OUT / f'{name}.i80'
        main = CPU['parse_cpu']((BIN / f'{name}.cpu').read_bytes())
        origin = main['load']
        defines = [*PROFILE, f'DRIVER_ORIGIN=0x{origin:04X}'] if name == 'RAMDISK' else PROFILE
        PACK['assemble'](source, target, defines)
        fields = CPU['fields_from_symbols'](target.with_suffix('.sym').read_text('utf-8'))
        target.with_suffix('.cpu').write_text(CPU['format_cpu'](origin, flags=main['flags'], fields=fields), 'ascii')
    overlays = runpy.run_path(str(ROOT / 'scripts/build/build-overlays.py'))
    for name in overlays['OVERLAYS']:
        overlays['build'](name, OUT, OUT / 'KERNEL.sym', PROFILE)


def build_mem():
    combined = OUT / 'LOCALBLOCK.bin'
    PACK['assemble'](ROOT / 'source/mem/LOCALBLOCK.asm', combined, PROFILE)
    symbols = PACK['symbols'](BIN / 'MEM.sym')
    image = bytearray((BIN / 'MEM.sys').read_bytes())
    start = symbols['MEM_LOCAL_OFFSET']
    end = symbols['MEM_TRANSPORT_OFFSET']
    local = combined.read_bytes()
    if len(local) != end - start:
        raise ValueError('MX local/client segments do not fit the MEM.SYS layout')
    image[start:end] = local
    (OUT / 'MEM.sys').write_bytes(image)


def build():
    OUT.mkdir(exist_ok=True)
    bios = PACK['symbols'](BIN / 'BIOS.sym')
    PACK['assemble'](ROOT / 'source/dos/DOS.asm', OUT / 'KERNEL.bin', PROFILE)
    kernel = (OUT / 'KERNEL.bin').read_bytes()
    build_floppy_components()
    build_mem()
    padded = kernel + bytes(-len(kernel) % 512)
    sectors = len(padded) // 512
    dos = PACK['symbols'](OUT / 'KERNEL.sym')
    start = dos['DOS_BANK_CODE_ADDR']
    # Sector padding may reach the DOS work area initialized by kernelCold.
    if not sectors or start + len(kernel) > dos['dosWorkStart'] or start + len(padded) > dos['dosZeroEnd']:
        raise ValueError('Invalid MX kernel load range')
    # Stage envelope is part of the disk format. Only the upper bound and
    # entry address are part of the ROM contract.
    stage_base = 0x3000
    stage_limit = 0x8e00
    stage_sectors = (stage_limit - stage_base) // 512
    # ROM вызывает fdTransfer уже при первом обращении к загрузочной дискете.
    service = (BIN / 'FLOPPYSVC.bin').read_bytes()
    size, entry, count = struct.unpack_from('<3H', service)
    if entry or len(service) != 6 + size + 2 * count:
        raise ValueError('Invalid boot floppy service descriptor')
    floppy = EXE['Image'](service[6:6+size], reloc_kind=EXE['WORD16'],
                          relocations=EXE['relocation_table'](service[6+size:]))
    (OUT / 'fd-resident.bin').write_bytes(EXE['relocated_body'](floppy, dos['DOS_FLOPPY_ADDR']))
    bios_bytes = (BIN / 'BIOS.sys').read_bytes()
    io_bytes = (BIN / 'IO.sys').read_bytes()
    mem_bytes = (OUT / 'MEM.sys').read_bytes()
    rom_font = ROM_FONT.read_bytes()
    rom_bundle_id = zlib.crc32(bios_bytes + rom_font + io_bytes + mem_bytes)
    boot_id = zlib.crc32(padded + ROTATE.read_bytes() + rom_bundle_id.to_bytes(4, 'little'))
    imports = {'MX_ROM_BUNDLE_ID': rom_bundle_id, 'MX_ROM_FONT_BYTES': len(rom_font),
               'MX_ROM_IO_BYTES': len(io_bytes), 'MX_ROM_MEM_BYTES': len(mem_bytes),
               'MX_BOOT_ID': boot_id, 'MX_BOOT_COOKIE': dos['mxBootCookie'],
               'MX_DISK_SECTORS': sectors, 'MX_KERNEL_LBA': 2 + stage_sectors,
               'MX_BOOT_FLOPPY_END': dos['bootFloppyEnd'], 'MX_FLOPPY_ADDR': dos['DOS_FLOPPY_ADDR'],
               'MX_BOOT_FLOPPY_RECORD': dos['bootFloppyRecord'],
               'MX_BOOT_FLOPPY_RECORD_BYTES': dos['bootFloppyRecordEnd'] - dos['bootFloppyRecord'],
               'MX_NC_PATH': bios['biosNcPath'], 'MX_NC_MAIN_PATH': bios['biosNcMainPath'],
               'MX_AUTOEXEC_PATH': bios['biosAutoexecPath'], 'MX_CLI_PATH': bios['biosCliPath'],
               'MX_ROM_BLOCKS': bios['biosBootRomBlocks'], 'MX_ROM_SLOT': bios['SYSTEM_FLASH_ADDR'],
               'MX_ROM_SLOT_END': bios['SYSTEM_FLASH_END'], 'MX_COPY_SECTOR': bios['bankCopySector'],
               'MX_ROM_TABLE': bios['v_drives'] + 2 * bios['SYSTEM_DRIVE'], 'MX_ROM_INFO': bios['v_diskInfo'],
               'MX_ROM_CAPS': bios['DISK_CAP_READ'] | (bios['DISK_TECH_MX_SYSTEM_ROM'] << 8),
               'MX_DOS_LOCAL_A': dos['dosSystemDriver'], 'MX_DOS_NEAR_COPY': dos['memcpy_bc_hl'],
               'MX_CALC_CS': bios['bios_calcCS']}
    for name in bios:
        if name.startswith(('IO_FDD_', 'IO_PROG_', 'DOS_ERR_')) or name in (
                'IO_PAGE_ROM', 'DISK_CAP_READ', 'DISK_TECH_MX_VV55_FLASH', 'DRIVER_KIND_FLOPPY'):
            imports[name] = bios[name]
    (OUT / 'imports.inc').write_text(''.join(f'{k} = {v}\n' for k, v in imports.items()))
    PACK['assemble'](ROOT / 'source/bios/MXROM.asm', OUT / 'MXROM.bin', [])
    PACK['assemble'](ROOT / 'source/bootstrap/MXFLASH.asm', OUT / 'flash-resident.bin', [])
    checksum = runpy.run_path(str(ROOT / 'scripts/images/executable.py'))['checksum']
    (OUT / 'checks.inc').write_text(''.join(
        f'    DW {checksum(padded[i:i+512])}\n' for i in range(0, len(padded), 512)))
    assemble = PACK['assemble']
    stage = OUT / 'stage.bin'
    assemble(ROOT / 'source/bootstrap/BOOTBANK.asm', stage, ['ORIGINAL_MX=1'])

    config = PACK['symbols'](stage.with_suffix('.sym'))
    segments = [(stage, config['BOOT_STAGE_BASE']), (BIN / 'BIOS.sys', 0x3000),
                (ROTATE, config['BOOT_FONT_ROM']), (BIN / 'IO.sys', config['BOOT_IO_ROM']),
                (OUT / 'MEM.sys', config['BOOT_MEM_ROM'])]
    payload = bytearray(stage_limit - stage_base)
    occupied = []
    for binary, destination in segments:
        body = binary.read_bytes()
        end = destination + len(body)
        if destination < stage_base or end > stage_limit or any(destination < b and a < end for a, b in occupied):
            raise ValueError(f'Overlapping MX staging area for {binary.name}')
        occupied.append((destination, end))
        payload[destination-stage_base:end-stage_base] = body
    def write_volume(name, data):
        header = b'MXB1' + bytes([stage_sectors]) + (sum(data) & 0xffff).to_bytes(2, 'little')
        (OUT / name).write_bytes(header.ljust(512, bytes(1)) + data + padded)

    write_volume('reserved.bin', payload)
    for start, end in occupied[1:]:
        payload[start-stage_base:end-stage_base] = bytes(end - start)
    write_volume('reserved-rom.bin', payload)
    loader = OUT / 'loader.bin'
    font = (ROOT / 'assets/system-rom/fonts/6x8/KOI8.pf').read_bytes()
    prompt = b'Please insert boot disk or ROM card, and press Reset.'
    width = len(prompt) * 6
    columns = (width + 7) // 8
    if columns > 48:
        raise ValueError('The MX boot message is wider than the screen')
    rows = []
    for y in range(8):
        bits = 0
        for char in prompt:
            bits = (bits << 6) | (font[char * 8 + y] & 63)
        rows.append((bits << (columns * 8 - width)).to_bytes(columns, 'big'))
    (OUT / 'prompt.bin').write_bytes(bytes(pixel for column in zip(*rows) for pixel in column))
    assemble(ROOT / 'source/bootstrap/MXSECTOR.asm', loader, [f'MX_ROM_BOOT_ORIGIN={ROM_BOOT_ORIGIN}'])
    assemble(ROOT / 'source/bootstrap/MXSECTOR.asm', OUT / 'MxRom2.bin', ['MX_BOOT_MINIMAL=1'])
    # The ROM reset jumps to the first FAT data cluster. Keep the fixed
    # recovery vectors used by stages built for both ROM layouts.
    boot_sector = bytearray((ROOT / 'assets/system-rom/boot-system.bin').read_bytes())
    boot_sector[0x3e:] = bytes(256 - 0x3e)
    boot_sector[:3] = b'\xc3' + ROM_BOOT_ORIGIN.to_bytes(2, 'little')
    (OUT / 'rom-sector.bin').write_bytes(boot_sector)
    boot_vectors = loader.read_bytes()[:9]
    if boot_vectors[::3] != b'\xc3\xc3\xc3':
        raise ValueError('Missing MX ROM entry vectors')
    recovery = bytearray((bytes(3) + boot_vectors[3:]).ljust(256, b'\xff'))
    recovery[0x20:0x24] = rom_bundle_id.to_bytes(4, 'little')
    (OUT / 'rom-recovery.bin').write_bytes(recovery)
    write_manuals()


def write_manuals():
    # Split on existing section headings, retaining every original byte.
    manuals = [
        ('games/families/elite/manuals/Elite.txt', 'ELITE',
         ('Орбитальная станция.', 'Боевые действия.', 'Снаряжение и вооружение.')),
        ('system-rom/README.TXT', 'GUIDE',
         ('4. Диски, каталоги и команды', '6. Настройка: AUTOEXEC и драйверы')),
    ]
    for source, stem, headings in manuals:
        lines = (ROOT / 'assets' / source).read_bytes().splitlines(keepends=True)
        start = 0
        for index, heading in enumerate((*headings, None), 1):
            end = len(lines) if heading is None else next(
                n for n in range(start, len(lines)) if lines[n].strip() == heading.encode('koi8-r'))
            part = b''.join(lines[start:end])
            if not 0 < len(part) <= 8192:
                raise ValueError(f'{stem} part {index} exceeds the MAIN reading budget')
            (OUT / f'{stem}{index}.TXT').write_bytes(part)
            start = end


if __name__ == '__main__':
    sys.exit(python_build(__file__, 'Original MX bootstrap', build,
        asm=['source/bootstrap/BOOTBANK.asm', 'source/bootstrap/MXSECTOR.asm', 'source/bootstrap/MXFLASH.asm', 'source/dos/DOS.asm',
             'source/bios/MXROM.asm', 'source/mem/LOCALBLOCK.asm', 'source/enumerate/ENUM.asm', 'source/fileops/FILEOPS.asm',
             *(f'source/{name.lower()}/{name}.asm' for name in APPS)],
        inputs=['bin/banked/KERNEL.sym', 'bin/FLOPPYSVC.bin', 'bin/BIOS.sym', 'bin/BIOS.sys', 'bin/DOS.sys', 'bin/IO.sys', 'bin/MEM.sys', ROTATE, ROM_FONT,
                'bin/MEM.sym', *(BIN / f'{name}.cpu' for name in APPS), 'scripts/build/build-overlays.py',
                'scripts/build/pack-dos.py', 'scripts/images/executable.py', 'scripts/images/cpu.py',
                'assets/system-rom/boot-system.bin', 'assets/system-rom/fonts/6x8/KOI8.pf', 'assets/system-rom/README.TXT',
                'assets/games/families/elite/manuals/Elite.txt'],
        outputs=[OUT / name for name in ('reserved.bin', 'reserved-rom.bin', 'rom-sector.bin', 'rom-recovery.bin', 'loader.bin', 'MxRom2.bin', 'GUIDE1.TXT', 'GUIDE2.TXT',
                                         'GUIDE3.TXT', 'ELITE1.TXT', 'ELITE2.TXT', 'ELITE3.TXT', 'ELITE4.TXT')],
        scan_outputs=[OUT], exclude=[OUT], input_content=True))
