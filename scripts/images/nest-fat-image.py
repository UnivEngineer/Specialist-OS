"""Разложить файлы сборочного образа по путям манифеста, сохранив цепочки данных."""
import argparse
from pathlib import Path
import struct


def nest(image, paths):
    data=bytearray(image)
    u16=lambda offset:struct.unpack_from('<H',data,offset)[0]
    sector=u16(11);spc=data[13];reserved=u16(14);copies=data[16]
    fat_sectors=u16(22);root_entries=u16(17);total=u16(19)
    assert sector in (128,256,512) and spc and copies in (1,2)
    root=(reserved+copies*fat_sectors)*sector
    root_bytes=((root_entries*32+sector-1)//sector)*sector
    first_data=root+root_bytes;cluster_bytes=sector*spc
    clusters=(total*sector-first_data)//cluster_bytes
    fat=reserved*sector
    def get(c):return u16(fat+c*2)
    def put(c,value):
        for i in range(copies):struct.pack_into('<H',data,fat+i*fat_sectors*sector+c*2,value)
    def address(c):return first_data+(c-2)*cluster_bytes
    def short(name):
        if name in ('.','..') or not name or name.count('.')>1:raise ValueError('Invalid 8.3 component: '+name)
        base,_,ext=name.partition('.')
        if not base or len(base)>8 or len(ext)>3 or name.endswith('.') or any(ord(c)<33 or ord(c)>126 or c in '\\/:*?"<>|+,;=[]' for c in name):
            raise ValueError('Invalid 8.3 component: '+name)
        return (base.ljust(8)+ext.ljust(3)).encode('ascii')
    def descriptor(name,cluster,attribute=16):
        entry=bytearray(32);entry[:11]=name;entry[11]=attribute
        struct.pack_into('<H',entry,26,cluster)
        return entry
    entries=[];labels=[]
    for i in range(root_entries):
        entry=bytearray(data[root+i*32:root+(i+1)*32])
        if not entry[0]:break
        if entry[0]==0xE5:continue
        if entry[11]&8:labels.append(entry)
        else:
            if entry[11]&16:raise ValueError('Input must be the flat staging image')
            entries.append(entry)
    if len(entries)!=len(paths):raise ValueError('Manifest/staging file count mismatch')
    nodes={():dict(parent=None,items=[],names=set(),clusters=[])}
    for path,entry in zip(paths,entries):
        if len(path.encode('ascii'))+2>128:raise ValueError('Path exceeds MAX_PATH: '+path)
        parts=path.replace('\\','/').split('/')
        for part in parts:short(part)
        if short(parts[-1])!=entry[:11]:raise ValueError('Manifest/staging name mismatch: '+path)
        parent=()
        for part in parts[:-1]:
            key=parent+(part.upper(),)
            if key not in nodes:
                name=part.upper()
                if name in nodes[parent]['names']:raise ValueError('File/directory conflict: '+path)
                nodes[parent]['names'].add(name)
                nodes[key]=dict(parent=parent,items=[],names=set(),clusters=[])
                nodes[parent]['items'].append((short(part),key))
            parent=key
        name=parts[-1].upper()
        if name in nodes[parent]['names']:raise ValueError('Duplicate name: '+path)
        nodes[parent]['names'].add(name);nodes[parent]['items'].append((entry,None))
    if len(labels)+len(nodes[()]['items'])>root_entries:raise ValueError('Root directory full')
    candidate=2
    for key,node in nodes.items():
        if not key:continue
        count=((len(node['items'])+3)*32+cluster_bytes-1)//cluster_bytes
        for _ in range(count):
            while candidate<clusters+2 and get(candidate):candidate+=1
            if candidate>=clusters+2:raise ValueError('No free clusters for directories')
            node['clusters'].append(candidate);put(candidate,0xFFFF);candidate+=1
        for a,b in zip(node['clusters'],node['clusters'][1:]):put(a,b)
    def first(key):return nodes[key]['clusters'][0] if key else 0
    for key,node in nodes.items():
        content=bytearray()
        if key:
            content+=descriptor(b'.          ',first(key))
            content+=descriptor(b'..         ',first(node['parent']))
        else:
            for entry in labels:content+=entry
        for value,child in node['items']:
            content+=descriptor(value,first(child)) if child is not None else value
        if key:
            content+=bytes(len(node['clusters'])*cluster_bytes-len(content))
            for i,c in enumerate(node['clusters']):data[address(c):address(c)+cluster_bytes]=content[i*cluster_bytes:(i+1)*cluster_bytes]
        else:
            content+=bytes(root_bytes-len(content));data[root:root+root_bytes]=content
    return bytes(data),len(nodes)-1


if __name__=='__main__':
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument('image',type=Path);parser.add_argument('paths',nargs='+')
    args=parser.parse_args()
    data,count=nest(args.image.read_bytes(),args.paths)
    args.image.write_bytes(data)
    print('Nested FAT image:',count,'directories,',len(args.paths),'files')
