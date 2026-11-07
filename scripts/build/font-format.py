"""Заголовок MXFN v1: диапазоны по 32 символа и отдельные глифы ё/Ё."""
import struct
MAGIC=b'MXFN';HEADER=32

def parse(data):
    if len(data)<HEADER or data[:4]!=MAGIC or data[4]!=1:raise ValueError('Invalid MXFN v1 header')
    width,height,advance_x,advance_y,packing=data[5:10]
    size=struct.unpack_from('<H',data,10)[0]
    if size!=len(data):raise ValueError('MXFN size mismatch')
    if (width,height,advance_x,advance_y,packing)!=(6,8,6,10,0):raise ValueError('BIOS currently supports only unpacked 6x8 glyphs, advance 6x10; packed 4x8 needs its own renderer')
    ranges=struct.unpack_from('<8H',data,12);yo=struct.unpack_from('<2H',data,28)
    for off in ranges:
        if off and (off<HEADER or off+32*height>size):raise ValueError('MXFN range outside file')
    for off in yo:
        if off and (off<HEADER or off+height>size):raise ValueError('MXFN yo glyph outside file')
    if not ranges[1]:raise ValueError('Space range is required')
    return dict(size=size,ranges=ranges,yo=yo,packing=packing)

def wrap(payload):
    if len(payload) in (1280,1296):
        ranges=(0,32,288,544,0,0,800,1056)
        yo=(1312,1320) if len(payload)==1296 else (0,0)
    elif len(payload)==2048:
        ranges=tuple(32+i*256 for i in range(8));yo=(32+0xa3*8,32+0xb3*8)
    elif len(payload)==1024:
        ranges=(32,288,544,800,0,0,0,800);yo=(0,0)
    else:raise ValueError('Unsupported legacy font size')
    return struct.pack('<4s6BH8H2H',MAGIC,1,6,8,6,10,0,HEADER+len(payload),*ranges,*yo)+payload
