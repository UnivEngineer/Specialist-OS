"""Свободные интервалы DOS по фактическим адресам зарегистрированных образов."""
import struct


def free_intervals(s, dos_read):
    occupied = []
    floppy_end, = struct.unpack('<H', dos_read(s['diFloppyEnd'], 2))
    if floppy_end:
        floppy_start, = struct.unpack('<H', dos_read(s['diFloppyStart'], 2))
        occupied.append((floppy_start, floppy_end))
    if dos_read(s['diActive'], 1)[0]:
        start, = struct.unpack('<H', dos_read(s['diStart'], 2))
        end, = struct.unpack('<H', dos_read(s['diEnd'], 2))
        occupied.append((start, end))
    for name in ('fileopsOverlay', 'enumOverlay'):
        overlay = dos_read(s[name], s['OV_END'] + 2)
        if overlay[s['OV_READY']]:
            occupied.append((struct.unpack_from('<H', overlay)[0],
                             struct.unpack_from('<H', overlay, s['OV_END'])[0]))
    stride = s['MODULE_RECORD_SIZE']
    modules = dos_read(s['MODULE_TABLE'], s['MODULE_MAX'] * stride)
    for offset in range(0, len(modules), stride):
        if any(modules[offset:offset+2]) and modules[offset+3] == s['DOS_IO_PAGE']:
            occupied.append(struct.unpack_from('<HH', modules, offset+10))
    bottom = s['DOS_ARENA_START']
    free = []
    for start, end in sorted(occupied):
        if bottom < start:
            free.append((bottom, start))
        bottom = max(bottom, end)
    if bottom < s['DOS_BANK_MODULE_END']:
        free.append((bottom, s['DOS_BANK_MODULE_END']))
    return free
