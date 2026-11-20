"""Независимый эталон COM/EXE/DLL/FNT/RKS для Python-проверок. Сборщик использует executable.ps1."""
from dataclasses import dataclass, replace
from pathlib import Path
import struct
import runpy
parse_cpu=runpy.run_path(str(Path(__file__).with_name("cpu.py")))["parse_cpu"]

MAGIC = b'\xcd\x20MX\x02\x00'
HEADER_SIZE = 32
POLICY_EXCLUSIVE, POLICY_PRESERVE, POLICY_MANAGED = range(3)


def checksum(data):
    if not data:
        return 0
    total = sum(data[:-1]) * 0x101
    return (total & 0xFF00) | (((total & 255) + data[-1]) & 255)


@dataclass
class Image:
    data: bytes
    load: int = 0
    entry: int = 0
    flags: int = 0
    bss: int | None = None
    memory_begin: int | None = None
    memory_end: int | None = None
    min_heap: int = 0
    stack: int = 0
    policy: int | None = None
    api: int | None = None
    mode: int = 0
    options: int = 0

    def memory(self):
        bss=self.bss if self.bss is not None else (0 if self.flags else 0xffff)
        policy=self.policy if self.policy is not None else (POLICY_MANAGED if self.flags else POLICY_EXCLUSIVE)
        api=self.api if self.api is not None else (2 if self.flags else 0)
        begin=self.load if self.memory_begin is None else self.memory_begin
        end=self.load+len(self.data)+(0 if bss==0xffff else bss) if self.memory_end is None else self.memory_end
        if not (0<=bss<=65535 and 0<=begin<=self.load and self.load+len(self.data)<=end<=65535):
            raise ValueError('Invalid image memory interval')
        if bss!=0xffff and self.load+len(self.data)+bss>end:
            raise ValueError('BSS exceeds image memory interval')
        if policy not in (0,1,2) or bss==0xffff and policy!=POLICY_EXCLUSIVE:
            raise ValueError('Invalid image memory policy')
        if not (0<=self.min_heap<=65535 and 0<=self.stack<=65535 and api in (0,2) and self.mode in range(5) and self.options in (0,1,3)):
            raise ValueError('Invalid image requirements')
        return bss,begin,end,policy,api

    def validate(self):
        if not self.data or not 0 <= self.load < 65536 or self.load + len(self.data) > 65536:
            raise ValueError('Invalid image address or size')
        if not self.load <= self.entry < self.load + len(self.data):
            raise ValueError('Entry is outside the image')
        if self.flags not in (0, 1, 2, 4):
            raise ValueError("Unknown COM/EXE flags")
        return self


def decode(data, kind, load=None, entry=None, path=None):
    kind = kind.lower().lstrip('.')
    if kind in ('com', 'exe', 'dll', 'fnt'):
        if len(data)<16 or data[:4]!=MAGIC[:4] or data[4] not in (1,2):
            raise ValueError('Expected MXOS2 container header')
        version=data[4];header=16 if version==1 else HEADER_SIZE
        if len(data)<header: raise ValueError('Short image header')
        start,run,size,cs=struct.unpack_from('<4H',data,6)
        hs=struct.unpack_from('<H',data,header-2)[0]
        if len(data)!=header+size or checksum(data[:header-2])!=hs or checksum(data[header:])!=cs:
            raise ValueError('Invalid image size or checksum')
        image=Image(data[header:],start,run,data[5])
        if version==2:
            bss,begin,end,heap=struct.unpack_from('<4H',data,14)
            stack=struct.unpack_from('<H',data,24)[0]
            if data[27]!=32 or data[29]: raise ValueError('Invalid image header size/reserve')
            image=Image(data[header:],start,run,data[5],bss,begin,end,heap,stack,data[22],data[23],data[26],data[28])
            image.memory()
        return image.validate()
    if kind == 'rks':
        if len(data) < 6:
            raise ValueError('Short RKS')
        start, end = struct.unpack_from('<HH', data)
        if end < start or len(data) != end - start + 7 or checksum(data[4:-2]) != int.from_bytes(data[-2:], 'little'):
            raise ValueError('Invalid RKS size or checksum')
        return Image(data[4:-2], start, start if entry is None else entry).validate()
    if kind == 'cpu' and path is not None and Path(path).with_suffix('.i80').exists():
        descriptor=parse_cpu(data)
        fields=descriptor['fields'];number=lambda key,default=None:int(fields[key],16) if key in fields else default
        choice=lambda key,names,default=None:names.get(fields[key].upper(),None) if key in fields and fields[key].upper() in names else number(key,default)
        image=Image(Path(path).with_suffix('.i80').read_bytes(),descriptor['load'],descriptor['entry'],descriptor['flags'],
                    number('BSS'),number('MEM_BEGIN'),number('MEM_END'),number('MIN_HEAP',0),number('STACK',0),
                    choice('POLICY',{'EXCLUSIVE':0,'PRESERVE':1,'MANAGED':2}),number('API'),
                    choice('MODE',{'AUTO':0,'MX':1,'STD':2,'MON':3,'RF':4},0),number('CACHEABLE',0))
        return image.validate()
    if kind == 'cpu':
        return Image(data, 0, 0).validate()
    # Старый EXE/BIN принимается только как явно заданный RAW-вход сборщика.
    start = 0 if load is None else load
    return Image(data, start, start if entry is None else entry).validate()


def encode(image, kind):
    image.validate()
    kind = kind.lower().lstrip('.')
    if kind in ('com','exe','dll','fnt'):
        if kind in ('dll','fnt'):
            image=replace(image,bss=0 if image.bss is None else image.bss,policy=POLICY_MANAGED if image.policy is None else image.policy,api=2 if image.api is None else image.api)
        if len(image.data)>65535: raise ValueError('Image payload exceeds 65535 bytes')
        header=bytearray(HEADER_SIZE)
        header[:5]=MAGIC[:5];header[5]=image.flags
        struct.pack_into('<4H',header,6,image.load,image.entry,len(image.data),checksum(image.data))
        bss,begin,end,policy,api=image.memory()
        struct.pack_into('<4H',header,14,bss,begin,end,image.min_heap)
        header[22:24]=bytes([policy,api]);struct.pack_into('<H',header,24,image.stack)
        header[26:30]=bytes([image.mode,32,image.options,0])
        struct.pack_into('<H',header,len(header)-2,checksum(header[:-2]))
        return bytes(header)+image.data
    if kind == 'rks':
        if image.flags:
            raise ValueError('RKS cannot represent executable flags')
        if image.entry != image.load:
            raise ValueError('RKS cannot represent a separate entry address')
        return struct.pack('<HH', image.load, image.load + len(image.data) - 1) + image.data + struct.pack('<H', checksum(image.data))
    return image.data


def convert(path, target, load=None):
    path = Path(path)
    kind = path.suffix.lower().lstrip('.')
    # До миграции старые EXE без сигнатуры — допустимый вход, но не выход.
    data = path.read_bytes()
    if kind in ('exe', 'com', 'dll', 'fnt') and data[2:4] != b'MX':
        kind = 'raw'
    image = decode(data, kind, load=load, path=path)
    return encode(image, target), image


def convert_batch(requests):
    """Один процесс на весь манифест; порядок результатов совпадает с запросами."""
    import base64
    result = []
    for request in requests:
        source = request['source']
        try:
            target = request['format'].lower()
            if target not in ('com', 'exe', 'dll', 'fnt', 'rks'):
                raise ValueError('Expected COM, EXE, DLL, FNT or RKS target')
            packed, _ = convert(source, target, request.get('load'))
        except (OSError, ValueError) as error:
            raise ValueError(f'{source}: {error}') from error
        result.append(base64.b64encode(packed).decode('ascii'))
    return result


if __name__ == '__main__':
    import argparse
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('source', type=Path, nargs='?')
    parser.add_argument('output', type=Path, nargs='?')
    parser.add_argument('--format', type=str.lower, choices=('com','exe','dll','fnt','rks'))
    parser.add_argument('--batch', type=Path, help='UTF-8 JSON: список source/format/load')
    parser.add_argument('--load', type=lambda s: int(s, 0))
    parser.add_argument('--stdout', action='store_true')
    args = parser.parse_args()
    if args.batch:
        import json
        if args.source or args.output or args.format or args.load is not None or args.stdout:
            parser.error('--batch cannot be combined with single-file options')
        print(json.dumps(convert_batch(json.loads(args.batch.read_text('utf-8-sig')))))
        raise SystemExit(0)
    if args.source is None or args.format is None:
        parser.error('source and --format are required')
    result = convert(args.source, args.format, args.load)[0]
    if args.stdout:
        import base64
        print(base64.b64encode(result).decode('ascii'))
    elif args.output:
        args.output.write_bytes(result)
    else:
        parser.error('output or --stdout is required')
