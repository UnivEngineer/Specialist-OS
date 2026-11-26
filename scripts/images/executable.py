"""Независимый эталон COM/EXE/DLL/FNT/RKS для Python-проверок. Сборщик использует executable.ps1."""
from dataclasses import dataclass, replace
from pathlib import Path
import struct
import runpy
parse_cpu=runpy.run_path(str(Path(__file__).with_name("cpu.py")))["parse_cpu"]

MAGIC = b'\xcd\x20MX\x02\x00'
HEADER_SIZE = 32
RELOC_HEADER_SIZE = 40
WORD16, HIGH8 = range(2)
POLICY_EXCLUSIVE, POLICY_PRESERVE, POLICY_MANAGED, POLICY_TRANSIENT = range(4)


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
    reloc_kind: int | None = None
    align_log2: int = 0
    relocations: tuple[int, ...] = ()

    def memory(self):
        native = self.flags or self.reloc_kind is not None
        bss=self.bss if self.bss is not None else (0 if native else 0xffff)
        policy=self.policy if self.policy is not None else (POLICY_MANAGED if native else POLICY_EXCLUSIVE)
        api=self.api if self.api is not None else (2 if native else 0)
        begin=(0 if self.reloc_kind is not None else self.load) if self.memory_begin is None else self.memory_begin
        end=self.load+len(self.data)+(0 if bss==0xffff else bss) if self.memory_end is None else self.memory_end
        if not (0<=bss<=65535 and 0<=begin<=self.load and self.load+len(self.data)<=end<=65535):
            raise ValueError('Invalid image memory interval')
        if bss!=0xffff and self.load+len(self.data)+bss>end:
            raise ValueError('BSS exceeds image memory interval')
        if policy not in (0,1,2,3) or bss==0xffff and policy!=POLICY_EXCLUSIVE:
            raise ValueError('Invalid image memory policy')
        if not (0<=self.min_heap<=65535 and 0<=self.stack<=65535 and api in (0,2) and self.mode in range(5) and self.options in (0,1,3)):
            raise ValueError('Invalid image requirements')
        return bss,begin,end,policy,api

    def validate(self):
        if not self.data or not 0 <= self.load < 65536 or self.load + len(self.data) > 65536:
            raise ValueError('Invalid image address or size')
        if not self.load <= self.entry < self.load + len(self.data):
            raise ValueError('Entry is outside the image')
        if self.policy == POLICY_TRANSIENT and (self.flags != 4 or self.reloc_kind is not None or self.min_heap or self.options):
            raise ValueError('TRANSIENT requires a fixed MAIN image without minimum heap or cache flags')
        if self.flags not in (0, 1, 2, 4):
            raise ValueError("Unknown COM/EXE flags")
        if self.reloc_kind is not None:
            if self.options == 3 and self.flags not in (0, 4):
                raise ValueError('REUSABLE requires MAIN-compatible placement')
            if self.reloc_kind not in (WORD16, HIGH8) or not 0 <= self.align_log2 <= 15:
                raise ValueError('Invalid relocation kind or alignment')
            if self.reloc_kind == HIGH8 and self.align_log2 < 8:
                raise ValueError('HIGH8 requires a page-aligned base')
            bss, begin, end, _, _ = self.memory()
            if begin != 0 or bss == 0xffff or end + self.min_heap > 65535:
                raise ValueError('Invalid relocatable memory interval')
            if RELOC_HEADER_SIZE + len(self.data) + 2 * len(self.relocations) > 65535:
                raise ValueError('Relocatable file exceeds 65535 bytes')
            width = 2 if self.reloc_kind == WORD16 else 1
            next_offset = 0
            for offset in self.relocations:
                if not next_offset <= offset <= len(self.data) - width:
                    raise ValueError('Unordered, overlapping or out-of-bounds relocation')
                next_offset = offset + width
        elif self.relocations or self.align_log2:
            raise ValueError('Missing relocation kind')
        return self


def check_reloc_target(kind, flags):
    if kind != 'dll' and not (kind in ('com', 'exe') and flags == 1):
        raise ValueError('Relocatable image requires DLL or DRIVER COM/EXE')


def relocation_table(data):
    if len(data) % 2:
        raise ValueError('Truncated relocation table')
    return tuple(value for value, in struct.iter_unpack('<H', data))


def relocated_body(image, base):
    """Эталон содержимого тела после загрузки по указанной базе интервала."""
    image.validate()
    if image.reloc_kind is None or not 0 <= base <= 0xffff or base % (1 << image.align_log2):
        raise ValueError('Invalid relocation base')
    if base + image.memory()[2] + image.min_heap > 0xffff:
        raise ValueError('Relocated interval exceeds address space')
    data = bytearray(image.data)
    for offset in image.relocations:
        if image.reloc_kind == WORD16:
            value = struct.unpack_from('<H', data, offset)[0]
            struct.pack_into('<H', data, offset, (value + base) & 0xffff)
        else:
            data[offset] = (data[offset] + (base >> 8)) & 255
    return bytes(data)


def decode(data, kind, load=None, entry=None, path=None):
    kind = kind.lower().lstrip('.')
    if kind in ('com', 'exe', 'dll', 'fnt'):
        if len(data)<16 or data[:4]!=MAGIC[:4] or data[4] not in (1,2,3):
            raise ValueError('Expected MXOS2 container header')
        version=data[4];header={1:16, 2:HEADER_SIZE, 3:RELOC_HEADER_SIZE}[version]
        if len(data)<header: raise ValueError('Short image header')
        start,run,size,cs=struct.unpack_from('<4H',data,6)
        sum_at = 14 if version == 1 else 30
        hs=struct.unpack_from('<H',data,sum_at)[0]
        body = data[header:header+size]
        table = data[header+size:]
        header_sum = checksum(data[:30] + bytes(2) + data[32:header]) if version == 3 else checksum(data[:sum_at])
        if len(body)!=size or header_sum!=hs or checksum(body)!=cs:
            raise ValueError('Invalid image size or checksum')
        if version != 3 and table:
            raise ValueError('Unexpected bytes after fixed image')
        image=Image(body,start,run,data[5])
        if version>=2:
            bss,begin,end,heap=struct.unpack_from('<4H',data,14)
            stack=struct.unpack_from('<H',data,24)[0]
            if data[27]!=header or data[29]: raise ValueError('Invalid image header size/reserve')
            image=Image(body,start,run,data[5],bss,begin,end,heap,stack,data[22],data[23],data[26],data[28])
            if image.policy == POLICY_TRANSIENT and kind not in ('com', 'exe'):
                raise ValueError('TRANSIENT requires COM or EXE')
            image.memory()
        if version == 3:
            check_reloc_target(kind, image.flags)
            mode, alignment, count, table_sum, reserved = struct.unpack_from('<BBHHH', data, 32)
            if reserved or len(table) != 2 * count or checksum(table) != table_sum:
                raise ValueError('Invalid relocation table size, checksum or reserve')
            image = replace(image, reloc_kind=mode, align_log2=alignment, relocations=relocation_table(table))
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
                    choice('POLICY',{'EXCLUSIVE':0,'PRESERVE':1,'MANAGED':2,'TRANSIENT':3}),number('API'),
                    choice('MODE',{'AUTO':0,'MX':1,'STD':2,'MON':3,'RF':4},0),number('CACHEABLE',0))
        if 'RELOC_KIND' in fields:
            size = number('BODY_SIZE')
            alignment = number('ALIGN_LOG2')
            if size is None or alignment is None or not 0 < size <= len(image.data):
                raise ValueError('Relocatable CPU requires BODY_SIZE and ALIGN_LOG2')
            image = replace(image, data=image.data[:size], reloc_kind=number('RELOC_KIND'),
                            align_log2=alignment, relocations=relocation_table(image.data[size:]))
        elif 'BODY_SIZE' in fields or 'ALIGN_LOG2' in fields:
            raise ValueError('Missing CPU RELOC_KIND')
        return image.validate()
    if kind == 'cpu':
        return Image(data, 0, 0).validate()
    # Старый EXE/BIN принимается только как явно заданный RAW-вход сборщика.
    start = 0 if load is None else load
    return Image(data, start, start if entry is None else entry).validate()


def encode(image, kind):
    image.validate()
    if image.policy == POLICY_TRANSIENT and kind not in ('com', 'exe'):
        raise ValueError('TRANSIENT requires COM or EXE')
    kind = kind.lower().lstrip('.')
    if image.reloc_kind is not None:
        check_reloc_target(kind, image.flags)
    if kind in ('com','exe','dll','fnt'):
        if kind in ('dll','fnt'):
            image=replace(image,bss=0 if image.bss is None else image.bss,policy=POLICY_MANAGED if image.policy is None else image.policy,api=2 if image.api is None else image.api)
        if len(image.data)>65535: raise ValueError('Image payload exceeds 65535 bytes')
        header=bytearray(RELOC_HEADER_SIZE if image.reloc_kind is not None else HEADER_SIZE)
        header[:5]=MAGIC[:5];header[5]=image.flags
        struct.pack_into('<4H',header,6,image.load,image.entry,len(image.data),checksum(image.data))
        bss,begin,end,policy,api=image.memory()
        struct.pack_into('<4H',header,14,bss,begin,end,image.min_heap)
        header[22:24]=bytes([policy,api]);struct.pack_into('<H',header,24,image.stack)
        header[26:30]=bytes([image.mode,len(header),image.options,0])
        table = b''
        if image.reloc_kind is not None:
            header[4] = 3
            table = b''.join(offset.to_bytes(2, 'little') for offset in image.relocations)
            struct.pack_into('<BBHHH', header, 32, image.reloc_kind, image.align_log2,
                             len(image.relocations), checksum(table), 0)
            header_sum = checksum(header)
        else:
            header_sum = checksum(header[:30])
        struct.pack_into('<H',header,30,header_sum)
        return bytes(header)+image.data+table
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
