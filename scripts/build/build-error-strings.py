"""Generate the MXOS indexed DOS error strings database."""
from pathlib import Path
import struct
import sys

MESSAGES = (
    "Unknown error",
    "Disk input/output error",
    "File or path not found",
    "Destination already exists",
    "Disk is full",
    "Directory is full",
    "Access denied",
    "Invalid argument",
    "File system is corrupted",
    "Disk driver is not installed",
    "Operation is not supported",
    "Invalid executable file",
    "Operation cancelled",
    "Disk is write-protected",
)

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
    data = encode(MESSAGES)
    if not out.exists() or out.read_bytes() != data:
        out.write_bytes(data)
