"""Initialize a DOS bank for CPU fixtures that enter below kernelCold."""
from pathlib import Path
import struct


def install_dos_mem(memory, bundle):
    # The banked DOS calls the resident MEM move routines directly. Install the
    # matching segments before tests enter DOS below the real bootstrap.
    image = (bundle / 'MEM.sys').read_bytes()
    assert image[:6] == b'MXMM\x01\x04'
    for index in range(image[5]):
        address, size, offset = struct.unpack_from('<HHH', image, 6 + 6 * index)
        memory[address:address + size] = image[offset:offset + size]


def initialize_dos_state(memory, symbols, bundle=Path(__file__).resolve().parents[2] / 'bin'):
    if 'bankMoveFullEntry' in symbols:
        install_dos_mem(memory, bundle)
    start = symbols['dosWorkStart']
    defaults_end = symbols['dosDefaultsEnd']
    source = symbols['dosColdDefaults']
    end = symbols['dosZeroEnd']
    memory[start:defaults_end] = memory[source:source + defaults_end - start]
    memory[defaults_end:end] = bytes(end - defaults_end)
    if 'dosResidentWorkEnd' in symbols:
        start = symbols['DOS_RESIDENT_WORK']
        end = symbols['dosResidentWorkEnd']
        memory[start:end] = bytes(end - start)
