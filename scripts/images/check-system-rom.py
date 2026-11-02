"""Побайтовая проверка файлов системного ROM по манифесту, включая STD-хвост MX2."""
import argparse
import json
import runpy
import struct
from pathlib import Path

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
    entries={image[i:i+11].upper():image[i:i+32] for i in
             range(root*256,root*256+word(17)*32,32) if image[i] not in (0,0xE5,0xFF)}
    label_lines = [line.strip()[7:].strip() for line in
                   (ROOT/'scripts/images/manifests/SystemRom.txt').read_text('utf-8-sig').splitlines()
                   if line.strip().startswith('@label ')]
    assert len(label_lines) == 1
    assert image[0x2B:0x36] == label_lines[0].ljust(11).encode('ascii')
    checked=[]
    for line in (ROOT/'scripts/images/manifests/SystemRom.txt').read_text('utf-8-sig').splitlines():
        if not line.strip() or line.lstrip().startswith(('#','@')):continue
        name,source,*_=line.split()
        if source=='{AUTOEXEC}':source=f'system-rom/autoexec.{args.autoexec}.bat'
        path=ROOT/'assets'/source
        expected=path.read_bytes()
        assert path.suffix.lower()!='.rks','Системный манифест ожидается без снятия RKS-контейнера'
        base,ext=name.rsplit('.',1)
        key=(base.ljust(8)+ext.ljust(3)).upper().encode('ascii')
        _,actual=get_file(image,entries[key][:11])
        assert actual==expected,f'{name}: содержимое образа отличается от {path}'
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
