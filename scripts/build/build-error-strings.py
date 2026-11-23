"""Generate the MXOS indexed DOS error strings database."""
from pathlib import Path
import re
import struct
import sys

MXOS_INCLUDE = Path(__file__).resolve().parents[2] / 'source/include/mxos.inc'
MESSAGES = {
    'DOS_ERR_IO':              'Disk input/output error',
    'DOS_ERR_NOT_FOUND':       'File or path not found',
    'DOS_ERR_EXISTS':          'Destination already exists',
    'DOS_ERR_FULL':            'Disk is full',
    'DOS_ERR_DIR_FULL':        'Directory is full',
    'DOS_ERR_ACCESS':          'Access denied',
    'DOS_ERR_ARGUMENT':        'Invalid argument',
    'DOS_ERR_CORRUPT':         'File system is corrupted',
    'DOS_ERR_NO_DRIVER':       'Disk driver is not installed',
    'DOS_ERR_UNSUPPORTED':     'Operation is not supported',
    'DOS_ERR_BAD_IMAGE':       'Invalid executable file',
    'DOS_ERR_CANCELLED':       'Operation cancelled',
    'DOS_ERR_WRITE_PROTECTED': 'Disk is write-protected',
}


def ordered_messages(path):
    codes = {
        name: int(value)
        for name, value in re.findall(
            r'^[ \t]*(DOS_ERR_\w+)[ \t]*=[ \t]*(\d+)[ \t]*(?=;|\r?$)',
            path.read_text(encoding='utf-8'), re.MULTILINE,
        )
    }
    if codes.keys() != MESSAGES.keys():
        missing = sorted(codes.keys() - MESSAGES.keys())
        stale = sorted(MESSAGES.keys() - codes.keys())
        raise ValueError(f'DOS error texts out of sync: missing={missing}, stale={stale}')
    if sorted(codes.values()) != list(range(1, codes['DOS_ERR_WRITE_PROTECTED'] + 1)):
        raise ValueError('DOS error codes must be contiguous, ending at DOS_ERR_WRITE_PROTECTED')
    names = sorted(codes, key=codes.__getitem__)
    return ['Unknown error', *(MESSAGES[name] for name in names)]


def encode(messages):
    records = bytearray()
    strings = bytearray()
    start = 3 * len(messages)
    for value in messages:
        data = value.encode('ascii')
        if not 1 <= len(data) <= 31 or any(c < 32 or c > 126 for c in data):
            raise ValueError(value)
        records.extend(struct.pack('<HB', start + len(strings), len(data)))
        strings.extend(data)
    return records + strings

if __name__ == '__main__':
    out = Path(sys.argv[1])
    out.parent.mkdir(parents=True, exist_ok=True)
    data = encode(ordered_messages(MXOS_INCLUDE))
    if not out.exists() or out.read_bytes() != data:
        out.write_bytes(data)
