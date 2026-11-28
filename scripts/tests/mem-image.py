"""Установка штатного MEM.SYS в память автономных CPU-тестов."""
from pathlib import Path
import struct

def read_bios_overlays(bin_dir):
    """Декодировать пакет накладок BIOS, общий для BOOT и восстановления."""
    data = (bin_dir / 'BIOS.main-overlay.bin').read_bytes()
    offset = 0
    while data[offset]:
        size, address = struct.unpack_from('<BH', data, offset)
        offset += 3
        end = offset + size
        assert end < len(data)
        yield address, data[offset:end]
        offset = end
    assert offset == len(data) - 1


def install(memory, symbols, bin_dir=None, *, main=False):
    if bin_dir is None:
        bin_dir = Path(__file__).resolve().parents[2] / 'bin'
    bios=(bin_dir/'BIOS.sys').read_bytes()
    memory[0xC000:0xC000+len(bios)]=bios
    io=(bin_dir/'IO.main.bin').read_bytes()
    memory[0xF000:0xF000+len(io)]=io
    data=(bin_dir/'MEM.sys').read_bytes()
    assert data[:6]==b'MXMM\x01\x04'
    for i in range(data[5]):
        address,size,offset=struct.unpack_from('<HHH',data,6+6*i)
        if main and address == symbols['MEM_BANK_CLIENT_START']:
            skipped = symbols['mx_callPacket'] - address
            address += skipped
            size -= skipped
            offset += skipped
        if main and address == symbols['MEM_IMAGE_LOCAL_START']:
            size = symbols['BANK_MAIN_LOCAL_BYTES']
        memory[address:address+size]=data[offset:offset+size]
    if main and (bin_dir / 'BIOS.main-overlay.bin').exists():
        for address, body in read_bios_overlays(bin_dir):
            memory[address:address+len(body)] = body
    for name,target in [('MEM_NATIVE_CALL','sysCall'),('MEM_LOCAL_SYSTEM','blockIO'),('MEM_LOCAL_CHECKED','localBlockChecked'),('MEM_NEAR_COPY','mon2_memcpy_bc_hl')]:
        address=symbols[name]
        memory[address:address+2]=symbols[target].to_bytes(2,'little')
    if 'bankMxRegsRouteCall' in symbols:
        address = symbols['bankMxRegsRouteCall'] + 1
        memory[address:address+2] = symbols['sysRegsEntry'].to_bytes(2, 'little')
