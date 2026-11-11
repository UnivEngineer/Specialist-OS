"""Установка штатного MEM.SYS в память автономных CPU-тестов."""
from pathlib import Path
import struct

def install(memory, symbols):
    bios=(Path(__file__).resolve().parents[2]/'bin/BIOS.sys').read_bytes()
    memory[0xC000:0xC000+len(bios)]=bios
    io=(Path(__file__).resolve().parents[2]/'bin/IO.sys').read_bytes()
    memory[0xF000:0xF000+len(io)]=io
    data=(Path(__file__).resolve().parents[2]/'bin/MEM.sys').read_bytes()
    assert data[:6]==b'MXMM\x01\x04'
    for i in range(data[5]):
        address,size,offset=struct.unpack_from('<HHH',data,6+6*i)
        memory[address:address+size]=data[offset:offset+size]
    for name,target in [('MEM_NATIVE_CALL','sysCall'),('MEM_LOCAL_A','bios_blockIO'),('MEM_LOCAL_CHECKED','localBlockChecked'),('MEM_NEAR_COPY','bios_memcpy_bc_hl')]:
        address=symbols[name]
        memory[address:address+2]=symbols[target].to_bytes(2,'little')
