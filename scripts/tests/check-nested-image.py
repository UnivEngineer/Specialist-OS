"""Независимый обход FAT16: вложения, . / .., размеры и владельцы цепочек."""
from pathlib import Path
import hashlib,json,struct

ROOT=Path(__file__).resolve().parents[2]


def read_tree(image):
    u16=lambda offset:struct.unpack_from('<H',image,offset)[0]
    sector=u16(11);spc=image[13];fat=u16(14)*sector;fat_size=u16(22)*sector
    root=(u16(14)+image[16]*u16(22))*sector;entries=u16(17)
    data=root+((entries*32+sector-1)//sector)*sector;size=sector*spc
    total=u16(19)*sector;clusters=(total-data)//size;owners={};files={};directories={}
    def chain(first,owner):
        parts=[];current=first
        while current<0xFFF8:
            assert 2<=current<clusters+2,(owner,current)
            assert current not in owners,(owner,current,owners.get(current))
            owners[current]=owner
            parts.append(image[data+(current-2)*size:data+(current-1)*size])
            current=u16(fat+current*2)
        return b''.join(parts)
    def visit(raw,path,cluster,parent):
        names=set()
        if path:
            assert raw[:11]==b'.          ' and raw[32:43]==b'..         ',path
            assert struct.unpack_from('<H',raw,26)[0]==cluster,path
            assert struct.unpack_from('<H',raw,58)[0]==parent,path
        for at in range(0,len(raw),32):
            e=raw[at:at+32]
            if not e[0]:break
            if e[0]==0xE5 or e[11]&8:continue
            base=e[:8].rstrip(b' ').decode('ascii');ext=e[8:11].rstrip(b' ').decode('ascii')
            name=base+('.'+ext if ext else '')
            if name in ('.','..'):continue
            assert name.upper() not in names,(path,name)
            names.add(name.upper());child=path+'/'+name if path else name
            first=struct.unpack_from('<H',e,26)[0];length=struct.unpack_from('<I',e,28)[0]
            assert not any(e[20:22]),child
            if e[11]&16:
                assert not length,child
                directories[child]=first;visit(chain(first,child),child,first,cluster)
            else:
                payload=chain(first,child) if first else b''
                assert len(payload)>=length and (not length)==(not first),(child,length)
                assert len(payload)-length<size if length else not payload,child
                files[child]=payload[:length]
    visit(image[root:root+entries*32],'',0,0)
    for c in range(2,clusters+2):
        value=u16(fat+c*2)
        assert value==0 or value==0xFFF7 or c in owners,('orphan',c,value)
    for copy in range(1,image[16]):assert image[fat:fat+fat_size]==image[fat+copy*fat_size:fat+(copy+1)*fat_size]
    return files,directories


if __name__=='__main__':
    image=(ROOT/'bin/nested-Games2M.bin').read_bytes();files,dirs=read_tree(image)
    manifest=ROOT/'scripts/images/manifests/H-Directories.txt'
    expected=[line.split()[0].replace('\\','/') for line in manifest.read_text(encoding='utf-8-sig').splitlines() if line.strip() and not line.lstrip().startswith(('#','@'))]
    assert set(files)==set(expected),(set(files)-set(expected),set(expected)-set(files))
    assert set(dirs)=={'Almaz','Almaz/MXOS','Almaz/STD','Almaz/RAMFOS'}
    report=dict(files={p:dict(bytes=len(b),sha256=hashlib.sha256(b).hexdigest()) for p,b in files.items()},directories=dirs)
    out=ROOT/'docs/fat/directories-001';out.mkdir(parents=True,exist_ok=True)
    (out/'image.json').write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
    print('NESTED IMAGE PASS',len(files),'files;',len(dirs),'directories')
