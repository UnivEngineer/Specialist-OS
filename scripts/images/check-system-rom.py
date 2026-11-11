"""Побайтовая проверка файлов системного ROM по манифесту, включая STD-хвост MX2."""
import argparse
import json
import runpy
import struct
from pathlib import Path
from executable import convert

ROOT=Path(__file__).resolve().parents[2]
get_file=runpy.run_path(str(ROOT/'scripts/emu80/check-copy.py'))['get_file']


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--image',required=True,type=Path)
    p.add_argument('--autoexec',default='emu')
    p.add_argument('--mx2',action='store_true')
    p.add_argument('--report',type=Path)
    args=p.parse_args()
    image=args.image.read_bytes()
    if args.mx2:
        assert len(image)==65536
        image=image[32768:]+image[4:32768]+bytes([255])*4
    word=lambda n:struct.unpack_from('<H',image,n)[0]
    root=word(14)+image[16]*word(22)
    label_lines = [line.strip()[7:].strip() for line in
                   (ROOT/'scripts/images/manifests/A-System.txt').read_text('utf-8-sig').splitlines()
                   if line.strip().startswith('@label ')]
    assert len(label_lines) == 1
    assert image[0x2B:0x36] == label_lines[0].ljust(11).encode('ascii')
    checked=[]
    # Обходим компоненты пути, включая цепочки подкаталогов.
    sector=word(11)
    data_start=(root+(word(17)*32+sector-1)//sector)*sector
    def chain(cluster):
        content=bytearray();visited=set()
        while cluster < 0xFFF8:
            assert cluster >= 2 and cluster not in visited, 'Повреждённая цепочка'
            visited.add(cluster)
            offset=data_start+(cluster-2)*image[13]*sector
            content+=image[offset:offset+image[13]*sector]
            cluster=word(word(14)*sector+cluster*2)
        return bytes(content)
    def lookup(path):
        directory=image[root*sector:root*sector+word(17)*32]
        parts=path.replace('\\','/').split('/')
        for index,part in enumerate(parts):
            base,_,ext=part.partition('.')
            key=(base.ljust(8)+ext.ljust(3)).upper().encode('ascii')
            entry=next(directory[i:i+32] for i in range(0,len(directory),32)
                       if directory[i:i+11].upper()==key)
            payload=chain(struct.unpack_from('<H',entry,26)[0]) if entry[26:28]!=bytes(2) else b''
            if index+1<len(parts):
                assert entry[11]&16, path
                directory=payload
            else:
                return entry,payload[:struct.unpack_from('<I',entry,28)[0]]
    for line in (ROOT/'scripts/images/manifests/A-System.txt').read_text('utf-8-sig').splitlines():
        if not line.strip() or line.lstrip().startswith(('#','@')):continue
        name,source,*options=line.split()
        if source=='{AUTOEXEC}':source=f'system-rom/autoexec.{args.autoexec}.bat'
        path=ROOT/'assets'/source
        ext=name.rsplit('.',1)[-1].lower()
        expected=(convert(path,ext,int(options[0],0) if options else 0)[0]
                  if ext in ('com','exe','dll','rks') else path.read_bytes())
        entry,actual=lookup(name)
        assert entry[18:20]==bytes(2), name
        assert actual==expected,f'{name}: содержимое образа отличается от {path}'
        if ext.lower() == 'com':
            assert actual[5] in (1,2,4), f'{name}: system COM requires an explicit execution flag'
        if name.replace('\\','/').split('/')[-1].upper() == 'RAMDISK.COM':
            # Адрес берётся из CPU/манифеста и уже проверен actual==expected.
            assert actual[5] == 1, name
        checked.append(name)
    if args.mx2:
        data=root+(word(17)*32+255)//256
        last_cluster=255-data+2
        assert word(word(14)*256+last_cluster*2)==0xFFF7
    report=dict(image=str(args.image),files=checked,allPayloadsMatch=True,
                mx2TailSectorReserved=args.mx2)
    if args.report:
        args.report.parent.mkdir(parents=True,exist_ok=True)
        args.report.write_text(json.dumps(report,indent=2)+'\n')
    print(f'{args.image}: {len(checked)} files match manifest')


if __name__=='__main__':main()
