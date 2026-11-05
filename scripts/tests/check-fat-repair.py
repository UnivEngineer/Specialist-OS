"""FAT MXOS-256 Рё РѕС€РёР±РєРё DOS РЅР° CPU 8080 СЃ СѓРїСЂР°РІР»СЏРµРјС‹Рј РґРёСЃРєРѕРІС‹Рј РґСЂР°Р№РІРµСЂРѕРј."""
import argparse
import json
from pathlib import Path
import re
import struct
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'docs/fat/repair-001/cpu'
TICKS = Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--variant',choices=['DOS','DOS4000'],default='DOS')
    parser.add_argument('--stream-only', action='store_true')
    parser.add_argument('--logical-only', action='store_true')
    parser.add_argument('--mount-only', action='store_true')
    parser.add_argument('--directory-only', action='store_true')
    parser.add_argument('--subdirectories-only', action='store_true')
    parser.add_argument('--timings', action='store_true')
    parser.add_argument('--binary-dir', type=Path, default=ROOT/'bin')
    args = parser.parse_args()
    global OUT
    if args.variant != 'DOS': OUT = OUT.with_name('cpu-'+args.variant)
    OUT.mkdir(parents=True, exist_ok=True)
    sy = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
          (args.binary_dir/f'{args.variant}.sym').read_text('utf-8'), re.M)}
    word = lambda n: struct.pack('<H',n)
    copy_target='driverCopy' if args.timings else f"0{sy['moveBytesForward']:04X}h"
    count_io='''    ld a,b
    cp 1
    jp z,countWrite
    ld hl,(08618h)
    inc hl
    ld (08618h),hl
    jp countDone
countWrite:
    ld hl,(0861Ah)
    inc hl
    ld (0861Ah),hl
countDone:
''' if args.timings else ''
    stub = OUT/'driver.asm'
    stub.write_text(f''' ORG 08400h
driver:
    ld a,b
    cp 3
    jp z,info
    push hl
    push de
    push bc
{count_io}    ld a,(08600h)
    cp b
    jp nz,transfer
    ld a,(08601h)
    cp e
    jp z,failed
transfer:
    ld hl,(08602h)
    call 0{sy['mul_hl_de']:04X}h
    ld (08606h),hl
    pop bc
    pop de
    pop hl
    push hl
    push de
    push bc
    push hl
    ld hl,(08606h)
    ex de,hl
    pop hl
    ld a,b
    cp 1
    jp z,copy
    ex de,hl
copy:
    push hl
    ld hl,(08602h)
    ld b,h
    ld c,l
    pop hl
    call {copy_target}
    pop bc
    pop de
    pop hl
    or a
    ret
failed:
    pop bc
    pop de
    pop hl
    scf
    ret
info:
    ld hl,(08604h)
    ex de,hl
    ld bc,0
    ld hl,0{sy['v_diskInfo']:04X}h
    or a
    ret
driverCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,driverCopy
    ret
 END
''', encoding='utf-8')
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080',
                    '--raw='+str(stub.with_suffix('.bin')),str(stub)],
                   capture_output=True, check=True)
    driver = stub.with_suffix('.bin').read_bytes()
    def run(m, label, hl=0, de=0, bc=0, a=0):
        io_before=struct.unpack_from('<HH',m,0x8618)
        code = b'\x31'+word(sy['STACK_ADDR'])+b'\x21'+word(hl)+b'\x11'+word(de)
        code += b'\x01'+word(bc)+bytes((0x3E,a,0xCD))+word(sy[label])
        code += b'\x22'+word(0x8610)+b'\xEB\x22'+word(0x8612)
        code += b'\xF5\xE1\x22'+word(0x8614)+b'\xC3'+word(0x8300)
        m[0x8200:0x8200+len(code)] = code
        (OUT/'in.bin').write_bytes(m)
        p = subprocess.run([str(TICKS),'-m8080','-pc','8200','-end','8300',
                            '-counter','20000000','-output',str(OUT/'out.bin'),
                            str(OUT/'in.bin')], capture_output=True, check=True)
        result = (OUT/'out.bin').read_bytes()
        assert int.from_bytes(result[65542:65544],'little') == 0x8300, p.stdout
        m[:] = result[:65536]
        return dict(cycles=int(p.stdout.strip()), reads=(int.from_bytes(m[0x8618:0x861A],'little')-io_before[0])&65535, writes=(int.from_bytes(m[0x861A:0x861C],'little')-io_before[1])&65535, bc=int.from_bytes(result[65538:65540],'little'), hl=int.from_bytes(m[0x8610:0x8612],'little'),
                    de=int.from_bytes(m[0x8612:0x8614],'little'),
                    a=m[0x8615], carry=bool(m[0x8614]&1), error=m[sy['v_dosError']] if 'v_dosError' in sy else None)
    def fresh(entries=13, sector=256, physical=256, spc=1, fats=1):
        m = bytearray(65536)
        if args.variant == 'DOS4000':
            m[:] = (ROOT/'bin/DOS4000.memory.bin').read_bytes()
            assert sy['dosCoreEnd'] <= 0x6000
        else:
            raw = (args.binary_dir/'DOS.sys').read_bytes()
            m[0xC000:0xC000+len(raw)] = raw
        # Стенд не загружает DOSMOD: верхнее ОЗУ свободно для тестовых буферов.
        m[sy['v_memTop']:sy['v_memTop']+2]=word(0x7FFF if args.variant=='DOS4000' else sy['LEGACY_APP_MEMORY_END']-1)
        m[0x8400:0x8400+len(driver)] = driver
        m[sy['j_diskDriver']+1:sy['j_diskDriver']+3] = word(0x8400)
        m[0x8602:0x8606]=word(physical)+word((5000 if sector==512 else 64)*sector//physical)
        if 'v_blockShifts' in sy:m[sy['v_blockShifts']+m[sy['v_drive']]]=physical.bit_length()-1
        m[3:11] = b'MXOS128 ' if sector==128 else b'MXOS256 '
        m[11:13] = word(sector); m[13] = spc; m[16] = fats
        m[14:16] = word(1); m[17:19] = word(entries)
        m[19:21] = word(5000 if sector==512 else 64); m[21] = 0xF8; m[22:24] = word(20 if sector==512 else 1)
        m[43:54] = b'TEST       '
        m[54:62] = b'FAT16   '
        m[sector:sector+4] = bytes.fromhex('F8 FF FF FF')
        if sector==512:m[510:512]=word(0xAA55)
        if 'v_blockShifts' not in sy:m[sector*2:sector*2+entries*32]=bytes([255])*(entries*32)
        run(m,'resetSectorCache')
        assert not run(m,'fatReadBootSector')['carry']
        return m
    def entry(name, cluster=0, size=0, attr=0):
        assert len(name)==11, name
        b = bytearray(32); b[:11] = name; b[11] = attr
        struct.pack_into('<HI', b, 26, cluster, size)
        return b
    if args.subdirectories_only:
        results=[]
        def fixture(sector=256,physical=256,spc=1):
            m=fresh(sector=sector,physical=physical,spc=spc)
            if args.variant=='DOS4000':
                # Стенд использует свободное окно 6000h–7FFFh над нижним ядром.
                m[sy['v_memTop']:sy['v_memTop']+2]=word(0x7FFF)
            get=lambda n:int.from_bytes(m[sy['v_diskInfo.'+n]:sy['v_diskInfo.'+n]+2],'little')
            root=get('dirStartSector')*sector;data=get('dataStartSector')*sector;cluster_bytes=sector*spc
            m[root:root+32]=entry(b'ALMAZ      ',2,attr=0x10)
            for cluster,next_cluster in [(2,5),(3,0xFFFF),(4,0xFFFF),(5,0xFFFF),(6,0xFFFF)]:
                m[sector+cluster*2:sector+cluster*2+2]=word(next_cluster)
            def directory(cluster,parent):
                start=data+(cluster-2)*cluster_bytes
                m[start:start+64]=entry(b'.          ',cluster,attr=0x10)+entry(b'..         ',parent,attr=0x10)
                return start
            start=directory(2,0)
            for i in range(2,cluster_bytes//32):m[start+i*32:start+(i+1)*32]=entry(f'IGNORE{i:02d}TXT'.encode(),attr=8)
            start=data+3*cluster_bytes
            m[start:start+64]=entry(b'MXOS       ',3,attr=0x10)+entry(b'STD        ',4,attr=0x10)
            start=directory(3,2);m[start+64:start+96]=entry(b'FILE    BIN',6,5)
            directory(4,2);m[data+4*cluster_bytes:data+4*cluster_bytes+5]=b'HELLO'
            return m
        m=fixture()
        def current():
            result=run(m,'bios_directory',a=0)
            assert not result['carry'],result
            return result['de']
        def cd(path,cluster=None,error=0):
            raw=path if isinstance(path,bytes) else path.encode()
            m[0x7000:0x7080]=raw+bytes(128-len(raw))
            m[0x6FFF]=0xA5;m[0x7080]=0x5A
            saved=bytes(m[0x6FFF:0x7081])
            before=current()
            result=run(m,'bios_directory',a=2,hl=0x7000)
            assert bytes(m[0x6FFF:0x7081])==saved,path
            assert result['carry']==bool(error),(path,result)
            if error:
                assert result['error']==error,(path,result)
                assert current()==before,path
            else:assert current()==cluster,(path,current(),cluster)
            results.append(dict(path=raw.decode('ascii'),cluster=cluster,error=error))
        cd('/ALMAZ/MXOS',3)
        m[0x7100:0x710B]=b'FILE    BIN'
        result=run(m,'bios_fileLoad2',hl=0x7100,de=0x6000)
        assert not result['carry'] and m[0x6000:0x6005]==b'HELLO',result
        m[0x7100:0x7120]=entry(b'NEW     TXT',size=3);m[0x6000:0x6003]=b'NEW'
        assert not run(m,'bios_fileCreate',hl=0x7100,de=0x6000)['carry']
        assert m[1280+96:1280+107]==b'NEW     TXT'
        assert not run(m,'bios_fileDelete',hl=0x7100)['carry']
        assert m[1280+96]==0xE5
        results.append(dict(fileReadCreateDelete=True))
        cd('../STD',4);cd('./../MXOS/',3);cd('\\ALMAZ\\STD',4)
        cd('../missing',error=sy['DOS_ERR_NOT_FOUND'])
        cd('/ALMAZ/MXOS/FILE.BIN',error=sy['DOS_ERR_ACCESS'])
        cd('/ALMAZ/TOOLONG99',error=sy['DOS_ERR_ARGUMENT'])
        cd('/ALMAZ/*',error=sy['DOS_ERR_ARGUMENT'])
        cd('/ALMAZ/A.B.C',error=sy['DOS_ERR_ARGUMENT'])
        cd(bytes([65])*128,error=sy['DOS_ERR_ARGUMENT'])
        cd('/',0);cd('..',0);cd('/ALMAZ/MXOS',3)
        result=run(m,'bios_directory',a=1,de=2)
        assert not result['carry'] and current()==2,result
        result=run(m,'bios_directory',a=3,hl=0x6000,de=20)
        assert not result['carry'] and result['de']==4,result
        assert m[0x6000:0x600B]==b'.          ' and m[0x6020:0x602B]==b'MXOS       '
        results.append(dict(directoryListIncludesDotsAndFolders=True))
        m[256+5*2:258+5*2]=word(2)
        for i in range(8):m[1792+i*32:1792+(i+1)*32]=entry(b'IGNORE  TXT',attr=8)
        run(m,'resetSectorCache')
        cd('MISSING',error=sy['DOS_ERR_CORRUPT'])
        for sector,physical,spc in [(128,128,2),(256,256,4),(512,256,1)]:
            m=fixture(sector,physical,spc);cd('/ALMAZ/MXOS',3)
            m[0x7100:0x710B]=b'FILE    BIN'
            result=run(m,'bios_fileLoad2',hl=0x7100,de=0x6000)
            assert not result['carry'] and m[0x6000:0x6005]==b'HELLO',(sector,spc,result)
            results.append(dict(sector=sector,physical=physical,clusterSectors=spc))
        result=run(m,'bios_directory',a=2,hl=0xF000)
        assert result['carry'] and result['error']==sy['DOS_ERR_ARGUMENT']
        results.append(dict(invalidPathBufferRejected=True))
        m=fixture();cd('/ALMAZ/MXOS',3)
        m[544:576]=entry(b'ROOT    TXT')
        m[0x7100:0x710C]=b'B:\\ROOT.TXT\0'
        run(m,'resetSectorCache')
        run(m,'bios_fileNamePrepare',hl=0x7100,de=0x7200)
        assert m[0x7200:0x720B]==b'ROOT    TXT'
        assert not run(m,'bios_fileLoadInfo',hl=0x7200)['carry']
        assert current()==3
        results.append(dict(explicitRootKeepsSavedCurrentDirectory=True))
        m=fixture()
        m[1792:1803]=b'COLLECT1MX2'
        run(m,'resetSectorCache')
        cd('B:/ALMAZ/COLLECT1.MX2',3)
        cd('../COLLECTIONS',error=sy['DOS_ERR_ARGUMENT'])
        m[0x7100:0x7180]=b'B:/ALMAZ/COLLECT1.MX2/FILE.BIN /Q\0'.ljust(128,b'\0')
        r=run(m,'bios_fileNamePrepare',hl=0x7100,de=0x7200)
        assert not r['carry'] and m[0x7200:0x720B]==b'FILE    BIN',r
        assert not run(m,'bios_fileLoad2',hl=0x7200,de=0x6000)['carry']
        assert m[0x6000:0x6005]==b'HELLO'
        results.append(dict(fullFilePathAnd83Directory=True))
        (OUT/'subdirectories.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
        print('FAT SUBDIRECTORIES CPU PASS',len(results))
        return
    if args.directory_only:
        results=[]
        for sector,entries,slot in [(256,13,0),(256,13,7),(256,13,12),(128,13,3),(512,33,15),(512,33,32)]:
            m=fresh(entries=entries,sector=sector,physical=min(sector,256))
            root=int.from_bytes(m[sy['v_diskInfo.dirStartSector']:sy['v_diskInfo.dirStartSector']+2],'little')*sector
            end=root+((entries*32+sector-1)//sector)*sector
            for i in range(entries):m[root+i*32:root+(i+1)*32]=entry(f'F{i:07d}TXT'.encode())
            m[root+slot*32]=0
            after=bytes(m[root:end])
            m[0x7000:0x7020]=entry(b'NEW     TXT')
            result=run(m,'bios_fileCreate',hl=0x7000,de=0x6000)
            assert not result['carry'],(slot,result)
            assert m[root+slot*32:root+slot*32+11]==b'NEW     TXT'
            if slot<entries-1:assert m[root+(slot+1)*32]==0,slot
            else:assert m[root+entries*32:end]==after[entries*32:]
            result=run(m,'bios_fileList',hl=0x6000,de=40)
            assert not result['carry'],result
            assert m[0x6000+(slot+1)*16]==0xFF,(slot,m[0x6000:0x6100].hex())
            results.append(f'sector {sector}: new end marker at slot {slot}, root entries {entries}')
        m=fresh();m[512:576]=entry(b'DELETED TXT')+entry(b'KEEP    TXT');m[512]=0xE5
        m[0x7000:0x7020]=entry(b'NEW     TXT')
        assert not run(m,'bios_fileCreate',hl=0x7000,de=0x6000)['carry']
        assert m[544:555]==b'KEEP    TXT'
        results.append('reusing E5 preserves subsequent live entries')
        m=fresh()
        for i in range(7):m[512+i*32:544+i*32]=entry(f'F{i:07d}TXT'.encode())
        m[768:800]=entry(b'GARBAGE TXT');m[0x7000:0x7020]=entry(b'NEW     TXT')
        m[0x8600:0x8602]=bytes((2,3))
        result=run(m,'bios_fileCreate',hl=0x7000,de=0x6000)
        assert result['carry'] and result['error']==sy['DOS_ERR_IO'],result
        assert m[512+7*32]==0
        results.append('next-sector read failure does not publish a new entry')
        for operation in ('bios_fileLoadInfo','bios_fileDelete','bios_fileCreate'):
            m=fresh();m[512:544]=entry(b'INVALID TXT');m[532:534]=word(1)
            m[0x7000:0x7020]=entry(b'INVALID TXT')
            disk=bytes(m[:8192])
            result=run(m,operation,hl=0x7000,de=0x6000)
            assert result['carry'] and result['error']==sy['DOS_ERR_CORRUPT'],(operation,result)
            assert bytes(m[:8192])==disk,operation
            results.append(f'{operation} rejects nonzero FAT16 FstClusHI without modifying disk')
        (OUT/'directory.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
        print('FAT DIRECTORY CPU PASS',len(results))
        return
    if args.mount_only:
        results=[]
        def mount_case(name, m, error=0):
            disk=bytes(m[:8192])
            result=run(m,'fatReadBootSector')
            assert result['carry']==bool(error),(name,result)
            assert m[sy['v_diskInfo.isValid']]==(sy['DISK_INVALID'] if error else sy['DISK_VALID']),(name,result)
            if error:assert result['error']==error,(name,result)
            assert bytes(m[:8192])==disk,name
            results.append(dict(name=name,error=error))
        for physical,sector in [(128,128),(128,256),(256,256),(128,512),(256,512),(512,512)]:
            for fats in (1,2):
                m=fresh(sector=sector,physical=physical,fats=fats)
                mount_case(f'geometry {physical}/{sector}, FATs={fats}',m)
        for flags in (0x3FFF,0x7FFF,0xBFFF,0xFFFF):
            m=fresh(sector=512);m[514:516]=word(flags)
            mount_case(f'FAT16 clean/error flags {flags:04X}',m)
        for media in (0xF0,*range(0xF8,0x100)):
            m=fresh();m[21]=m[256]=media
            mount_case(f'media {media:02X}',m)
        for media in (0,0xEF,0xF1,0xF7):
            m=fresh();m[21]=m[256]=media
            mount_case(f'invalid media {media:02X}',m,sy['DOS_ERR_UNSUPPORTED'])
        for offset,value in [(256,0xF0),(257,0),(258,0xFE),(259,0xFC),(259,0xFF&~0x20)]:
            m=fresh();m[offset]=value
            mount_case(f'corrupt FAT header {offset}/{value:02X}',m,sy['DOS_ERR_CORRUPT'])
        for count in (4084,4085):
            m=fresh(sector=512);m[19:21]=word(count+22)
            mount_case(f'cluster count {count}',m,sy['DOS_ERR_UNSUPPORTED'] if count<4085 else 0)
        for offset,data in [(510,word(0)),(22,word(1)),(14,word(0xFFFF)),(16,bytes([0])),(19,word(5001))]:
            m=fresh(sector=512);m[offset:offset+len(data)]=data
            mount_case(f'invalid BPB {offset}/{data.hex()}',m,sy['DOS_ERR_UNSUPPORTED'])
        m=fresh();m[19:21]=word(0);m[32:36]=struct.pack('<I',64)
        mount_case('TotSec32 within 16-bit LBA',m)
        m=fresh();m[19:21]=word(0);m[32:36]=struct.pack('<I',0x10000)
        mount_case('TotSec32 exceeds supported LBA',m,sy['DOS_ERR_UNSUPPORTED'])
        m=fresh();m[14:16]=word(3);m[3*256:3*256+4]=bytes.fromhex('F8 FF FF FF')
        mount_case('reserved area contains three sectors',m)
        m=fresh();m[0x8600:0x8602]=bytes((2,1))
        mount_case('first FAT read failure',m,sy['DOS_ERR_IO'])
        (OUT/'mount.json').write_text(json.dumps(results,indent=2)+'\n',encoding='utf-8')
        print('FAT MOUNT CPU PASS',len(results))
        return
    if args.timings:
        times={}
        m=fresh(entries=64)
        for i in range(64):m[512+i*32:544+i*32]=entry(f'F{i:07d}BIN'.encode())
        times['directory_64']=run(m,'bios_fileList',hl=0x6000,de=64)['cycles']
        times['directory_64_cached']=run(m,'bios_fileList',hl=0x6000,de=64)['cycles']
        def source(length,index=0):
            m=fresh(entries=64)
            for i in range(index):m[512+i*32:544+i*32]=entry(f'F{i:07d}BIN'.encode())
            m[512+index*32:544+index*32]=entry(b'SOURCE  BIN',2,length)
            for i in range((length+255)//256):m[260+i*2:262+i*2]=word(3+i if i+1<(length+255)//256 else 0xFFFF)
            m[2560:2560+length]=bytes((i*37+5)&255 for i in range(length))
            m[0x8100:0x810B]=b'SOURCE  BIN'
            return m
        m=source(8192);r=run(m,'bios_fileLoad2',hl=0x8100,de=0x6000);assert not r['carry'];times['read_8192']=r['cycles']
        m=fresh(entries=64);m[0x8100:0x8120]=entry(b'NEW     BIN',size=8192)
        r=run(m,'bios_fileCreate',hl=0x8100,de=0x6000);assert not r['carry'],r;times['create_8192']=r['cycles']
        phases={}
        def phase(name,m,label,**regs):
            r=run(m,label,**regs);assert not r['carry'],(name,r)
            phases[name]={k:r[k] for k in ['cycles','reads','writes']}
            return r
        m=source(8192,40);phase('read_8192_at_40',m,'bios_fileLoad2',hl=0x8100,de=0x6000)
        m=source(8192,40);phase('find_at_40',m,'fileFind',hl=0x8100)
        m=source(8192,40);m[512+40*32:544+40*32]=bytes([255] if 'v_blockShifts' not in sy else [0])*32
        m[260:324]=bytes(64)
        m[0x8100:0x8120]=entry(b'NEW     BIN',size=8192)
        phase('create_8192_at_40',bytearray(m),'bios_fileCreate',hl=0x8100,de=0x6000)
        phase('allocate_8192_at_40',m,'bios_fileCreatePrepare',hl=0x8100,de=32)
        cluster=int.from_bytes(m[0x811A:0x811C],'little')
        m[0x6000:0x8000]=bytes((i*13+7)&255 for i in range(8192))
        if 'fatStream' in sy:
            m[sy['v_streamOffset']:sy['v_streamOffset']+2]=word(0)
            phase('write_allocated_8192',m,'fatStream',hl=0x6000,de=cluster,bc=8192,a=1)
        else:phase('write_allocated_8192',m,'fatFileTransferClusters',hl=0x6000,de=cluster,bc=0x0120)
        phase('final_flush',m,'bios_flushVolume')
        assert m[2560:10752]==m[0x6000:0x8000]
        times['phases']=phases
        if 'FILE_COPY_REQUEST' not in sy:
            print(json.dumps(times));return
        m=source(4096);req=bytearray(sy['FILE_COPY_REQUEST']);req[:4]=word(0x6001)+word(2065);req[4:6]=bytes((1,1));req[6:17]=b'SOURCE  BIN';req[38:49]=b'COPY    BIN';m[0x7600:0x7600+len(req)]=req
        r=run(m,'bios_fileCopy',hl=0x7600);assert not r['carry'];times['copy_4096']=r['cycles']
        print(json.dumps(times));return
    if args.logical_only:
        cases=0
        for physical,sector in [(128,128),(128,256),(128,512),(256,256),(256,512),(512,512)]:
            for spc in ([1] if sector==512 else [1,2,4]):
                for fats in (1,2):
                    m=fresh(sector=sector,physical=physical,spc=spc,fats=fats)
                    get=lambda name:int.from_bytes(m[sy['v_diskInfo.'+name]:sy['v_diskInfo.'+name]+2],'little')
                    data=get('dataStartSector'); root=get('dirStartSector'); fatsectors=get('fatSectors')
                    clusterbytes=sector*spc
                    payload=bytes((i*23+7)&255 for i in range(clusterbytes*2))
                    m[data*sector:data*sector+len(payload)]=payload
                    m[sector+4:sector+8]=word(3)+word(0xFFF8)
                    if fats==2:m[(1+fatsectors)*sector:(2+fatsectors)*sector]=m[sector:2*sector]
                    offset=clusterbytes-7
                    m[0x7000:0x7004]=word(2)+word(offset)
                    m[0x6000:0x6014]=bytes([0xA5])*20
                    r=run(m,'bios_fileStream',hl=0x6001,de=0x7000,bc=18,a=2)
                    assert not r['carry'] and r['bc']==18,(physical,sector,spc,fats,r)
                    assert m[0x6001:0x6013]==payload[offset:offset+18]
                    assert m[0x6000]==m[0x6013]==0xA5
                    m[0x7000:0x7004]=word(2)+word(offset)
                    m[0x6001:0x6013]=b'ABCDEFGHIJKLMNOPQR'
                    assert not run(m,'bios_fileStream',hl=0x6001,de=0x7000,bc=18,a=1)['carry']
                    assert not run(m,'bios_flushVolume')['carry']
                    assert m[data*sector+offset:data*sector+offset+18]==b'ABCDEFGHIJKLMNOPQR'
                    assert not run(m,'fatWriteCluster',hl=4,de=0xFFF7)['carry']
                    assert not run(m,'bios_flushVolume')['carry']
                    for copy in range(fats):assert m[(1+copy*fatsectors)*sector+8:(1+copy*fatsectors)*sector+10]==word(0xFFF7),(physical,sector,spc,fats,copy,m[(1+copy*fatsectors)*sector+8:(1+copy*fatsectors)*sector+10].hex())
                    run(m,'resetSectorCache')
                    for i in range(13):m[root*sector+i*32:root*sector+(i+1)*32]=entry(b'IGNORE  TXT',attr=8)
                    idx=12;m[root*sector+idx*32:root*sector+(idx+1)*32]=entry(b'LAST    TXT')
                    m[0x7000:0x700B]=b'LAST    TXT'
                    assert not run(m,'bios_fileLoadInfo',hl=0x7000)['carry']
                    assert run(m,'bios_fileGetSetAttr',hl=0x7000,bc=0x2001)['a']==0x20
                    assert m[root*sector+idx*32+11]==0x20
                    run(m,'resetSectorCache')
                    length=clusterbytes+5
                    m[root*sector:root*sector+64]=entry(b'SOURCE  BIN',2,length)+bytes(32)
                    expected=bytes(m[data*sector:data*sector+length])
                    request=bytearray(sy['FILE_COPY_REQUEST'])
                    request[:4]=word(0x6001)+word(513);request[4:6]=bytes((1,1))
                    request[6:17]=b'SOURCE  BIN';request[38:49]=b'TARGET  BIN'
                    m[0x7600:0x7600+len(request)]=request
                    r=run(m,'bios_fileCopy',hl=0x7600)
                    assert not r['carry'],(physical,sector,spc,fats,r)
                    current=int.from_bytes(m[root*sector+58:root*sector+60],'little');actual=bytearray()
                    for _ in range(10):
                        if current>=0xFFF8:break
                        start=(data+(current-2)*spc)*sector
                        actual+=m[start:start+clusterbytes]
                        current=int.from_bytes(m[sector+current*2:sector+current*2+2],'little')
                    assert actual[:length]==expected,(physical,sector,spc,fats,'copy')
                    cases+=1
        for physical,sector in [(128,256),(128,512),(256,512)]:
            m=fresh(sector=sector,physical=physical)
            m[0x8600:0x8602]=bytes((2,4*sector//physical+1))
            assert run(m,'bios_sectorIO',hl=0x6000,de=4,bc=0x0200)['carry']
            m[0x8600]=0
            assert not run(m,'bios_sectorIO',hl=0x6000,de=4,bc=0x0200)['carry']
            assert run(m,'bios_sectorIO',hl=0x6000,de=5000 if sector==512 else 64,bc=0x0200)['carry']
        for marker in [0,1,*range(0xFFF0,0x10000)]:
            m=fresh();m[512:544]=entry(b'MARKER  BIN',2,1)
            m[260:262]=word(marker);m[0x7000:0x700B]=b'MARKER  BIN'
            r=run(m,'bios_fileLoad2',hl=0x7000,de=0x6000)
            assert r['carry']==(marker<0xFFF8),(marker,r)
        print('LOGICAL SECTORS PASS' ,cases,'geometry combinations')
        return
    if args.stream_only:
        def prepared(offset=250):
            m = fresh()
            m[260:264] = word(3)+word(0xFFFF)
            m[1024:1536] = bytes(n & 255 for n in range(512))
            m[0x7000:0x7004] = word(2)+word(offset)
            return m
        m = prepared()
        r = run(m,'bios_fileStream',a=2,hl=0x6000,de=0x7000,bc=20)
        assert not r['carry'] and r['bc']==20 and r['hl']==0x6014 and r['de']==0x7000, r
        assert m[0x6000:0x6014] == bytes(range(250,256))+bytes(range(14))
        assert m[0x7000:0x7004] == word(3)+word(14)
        r = run(m,'bios_fileStream',a=2,hl=0x6014,de=0x7000,bc=4)
        assert not r['carry'] and r['bc']==4 and m[0x6014:0x6018]==bytes(range(14,18)), r
        m = prepared()
        m[0x6000:0x6014] = b'ABCDEFGHIJKLMNOPQRST'
        r = run(m,'bios_fileStream',a=1,hl=0x6000,de=0x7000,bc=20)
        assert not r['carry'] and r['bc']==20, r
        assert not run(m,'fatVolumeFlush')['carry']
        assert m[1274:1294] == b'ABCDEFGHIJKLMNOPQRST'
        m = prepared()
        m[0x8600:0x8602] = bytes((2,5))
        r = run(m,'bios_fileStream',a=2,hl=0x6000,de=0x7000,bc=20)
        assert r['carry'] and r['bc']==6 and r['hl']==0x6006, r
        assert m[0x7000:0x7004] == word(3)+word(0)
        m[0x8600] = 0
        r = run(m,'bios_fileStream',a=2,hl=0x6006,de=0x7000,bc=14)
        assert not r['carry'] and r['bc']==14 and r['a']==0, r
        m = prepared()
        m[0x8600:0x8602] = bytes((2,1))
        r = run(m,'bios_fileStream',a=2,hl=0x6000,de=0x7000,bc=20)
        assert r['carry'] and r['bc']==6 and m[0x7000:0x7004]==word(2)+word(256), r
        m[0x8600] = 0
        r = run(m,'bios_fileStream',a=2,hl=0x6006,de=0x7000,bc=14)
        assert not r['carry'] and r['bc']==14 and m[0x7000:0x7004]==word(3)+word(14), r
        for mode, offset, count in [(0,0,1),(3,0,1),(2,257,1),(2,0,0)]:
            m = prepared(offset)
            before = bytes(m[0x7000:0x7004])
            r = run(m,'bios_fileStream',a=mode,hl=0x6000,de=0x7000,bc=count)
            assert r['carry'] == (count!=0) and r['bc']==0 and r['hl']==0x6000 and r['de']==0x7000, r
            assert m[0x7000:0x7004] == before
        # Полные секторы: отказ второй прямой записи и продолжение.
        m=prepared(0);payload=bytes((i*17+3)&255 for i in range(512))
        m[0x6000:0x6200]=payload;m[0x8600:0x8602]=bytes((1,5))
        r=run(m,'bios_fileStream',a=1,hl=0x6000,de=0x7000,bc=512)
        assert r['carry'] and r['bc']==256 and r['hl']==0x6100,r
        assert m[0x7000:0x7004]==word(3)+word(0) and m[1024:1280]==payload[:256]
        m[0x8600]=0
        assert not run(m,'bios_fileStream',a=1,hl=0x6100,de=0x7000,bc=256)['carry']
        assert m[1024:1536]==payload
        # Полная запись при попадании в кэш: чтение должно видеть новые данные.
        m=prepared(0);m[0x6000:0x6100]=bytes([0x5A])*256
        assert not run(m,'getSectorAddressInCache',de=4,a=0)['carry']
        assert not run(m,'bios_fileStream',a=1,hl=0x6000,de=0x7000,bc=256)['carry']
        m[0x7000:0x7004]=word(2)+word(0)
        assert not run(m,'bios_fileStream',a=2,hl=0x6100,de=0x7000,bc=256)['carry']
        assert m[0x6100:0x6200]==bytes([0x5A])*256
        assert not run(m,'bios_flushVolume')['carry'] and m[1024:1280]==bytes([0x5A])*256
        print('FileStream ABI: read, continuation, write/flush, partial error/retry, invalid arguments, zero length passed')
        return
    m=fresh()
    r=run(m,'bios_reservedC88D',hl=0x6000,de=0x7000,bc=123)
    assert r['carry'] and r['error']==sy['DOS_ERR_UNSUPPORTED'] and r['hl']==0x6000 and r['de']==0x7000 and r['bc']==123,r
    rows = []
    def record(name): rows.append(name)
    m = fresh()
    assert int.from_bytes(m[sy['v_diskInfo.dirSectors']:sy['v_diskInfo.dirSectors']+2],'little') == 2
    assert run(m,'bios_getDriveFreeSpace')['de'] == 60
    record('rounded root; free space excludes reserved entries and FAT padding')
    # Р’СЃРµ С‚СЂРё РІРёРґР° СЃР»СѓР¶РµР±РЅС‹С… Р·Р°РїРёСЃРµР№ Рё СѓРґР°Р»С‘РЅРЅР°СЏ Р·Р°РїРёСЃСЊ РїСЂРѕРїСѓСЃРєР°СЋС‚СЃСЏ.
    m[512:544] = entry(b'LABEL      ',attr=8)
    m[544:576] = entry(b'LFN        ',attr=15)
    m[576:608] = entry(b'DIR        ',attr=16)
    m[608:640] = entry(b'\xE5ELETED TXT')
    m[640:672] = entry(b'VISIBLE TXT')
    m[704:736] = entry(b'AFTERENDTXT')
    result = run(m,'bios_fileList',hl=0x6000,de=20)
    assert not result['carry'] and result['de'] == 1 and m[0x6000:0x600B] == b'VISIBLE TXT'
    assert m[0x6010] == 0xFF
    record('00 end, E5 deletion, LFN/label/directory filtering')
    m[0x7000:0x700B] = b'VISIBLE TXT'
    assert not run(m,'bios_fileLoadInfo',hl=0x7000)['carry']
    assert run(m,'bios_fileGetSetAttr',hl=0x7000,bc=0x2101)['a'] == 0x21
    assert run(m,'bios_fileGetSetAttr',hl=0x7000)['a'] == 0x21
    record('attribute setter uses B and returns the committed attribute')
    assert run(m,'bios_fileDelete',hl=0x7000)['carry']
    assert run(m,'bios_getError')['a'] == sy['DOS_ERR_ACCESS']
    record('read-only deletion reports access denied')
    m = fresh(); m[512:544] = entry(b'\x05NAME   TXT')
    m[0x7000:0x700B] = b'\xE5NAME   TXT'
    assert not run(m,'bios_fileLoadInfo',hl=0x7000)['carry']
    assert m[sy['v_tmpFileDescr']] == 0xE5
    assert run(m,'bios_fileList',hl=0x6000,de=20)['de'] == 1 and m[0x6000] == 0xE5
    m = fresh(); m[0x7000:0x7020] = entry(b'\xE5NAME   TXT')
    assert not run(m,'bios_fileCreate',hl=0x7000,de=0x7100)['carry']
    assert m[512] == 5
    record('05 escapes the first KOI-8 E5 byte on disk; stat/list restore the original byte')
    m = fresh(entries=13)
    for i in range(13): m[512+i*32:544+i*32] = entry(f'F{i:07}TXT'.encode())
    m[0x7000:0x7020] = entry(b'NEW     TXT')
    result = run(m,'bios_fileCreate',hl=0x7000,de=0x7100)
    assert result['carry'] and result['error'] == sy['DOS_ERR_DIR_FULL']
    assert m[512+13*32:544+13*32] == bytes(32)
    record('full non-sector-aligned root does not spill into its padding')
    m = fresh(); m[0x7000:0x7020] = entry(b'NEW     BIN',size=3*256)
    for c in range(4,62): m[256+c*2:258+c*2] = word(0xFFFF)
    result = run(m,'bios_fileCreatePrepare',hl=0x7000,de=3)
    assert result['carry'] and result['error'] == sy['DOS_ERR_FULL'], result
    assert m[512] == 0xE5 and m[260:264] == bytes(4)
    record('out-of-space allocation removes the partial file and releases its chain')
    for last in [0xFFF8,0xFFFF]:
        m = fresh(); m[512:544] = entry(b'CHAIN   BIN',2,512)
        m[260:264] = word(3)+word(last); m[0x7000:0x700B] = b'CHAIN   BIN'
        result=run(m,'bios_fileDelete',hl=0x7000)
        assert not result['carry'], (last,result,m[260:264].hex())
        assert m[512] == 0xE5 and m[260:264] == bytes(4)
    record('delete accepts complete FAT16 EOC range and writes E5')
    for bad in [0,1,0xFFF0,0xFFF7,0xFFFE]:
        m = fresh(); m[260:262] = word(bad)
        # EOC Р·Р°РєРѕРЅРµРЅ С‚РѕР»СЊРєРѕ РїРѕСЃР»Рµ РїРѕСЃР»РµРґРЅРµРіРѕ Р·Р°РїСЂРѕС€РµРЅРЅРѕРіРѕ Р±Р»РѕРєР°.
        m[0x7000:0x7004]=word(2)+word(0)
        result = run(m,'bios_fileStream',hl=0x6000,de=0x7000,bc=512,a=2)
        assert result['carry'] and result['error'] == sy['DOS_ERR_CORRUPT'], (bad,result)
    record('free, reserved, bad and premature EOC links fail before the next data access')
    m = fresh(); m[512:544] = entry(b'CYCLE   BIN',2,512)
    m[260:264] = word(3)+word(2); m[0x7000:0x700B] = b'CYCLE   BIN'
    assert run(m,'bios_fileDelete',hl=0x7000)['carry']
    record('cyclic deletion terminates with corruption')
    m = fresh(); m[0x8600:0x8602] = bytes((2,2))
    result = run(m,'bios_fileList',hl=0x6000,de=20)
    assert result['carry'] and result['error'] == sy['DOS_ERR_IO'] and m[0x6000] == 255
    record('directory read errors propagate with a terminated output')
    m = fresh(); run(m,'getSectorAddressInCache',de=2,a=1)
    m[0x8600:0x8602] = bytes((1,2))
    result = run(m,'bios_flushVolume')
    assert result['carry'] and result['error'] == sy['DOS_ERR_IO']
    cache = sy['cacheData']
    assert any(m[cache+i*8+6] for i in range(sy['FAT_CACHE_CAPACITY']))
    m[0x8600] = 0
    assert not run(m,'bios_flushVolume')['carry']
    assert not any(m[cache+i*8+6] for i in range(sy['FAT_CACHE_CAPACITY']))
    record('failed write retains dirty cache; retry flush succeeds')
    m = fresh(); run(m,'getSectorAddressInCache',de=2,a=1)
    m[0x8600:0x8602] = bytes((1,2)); original = m[sy['v_drive']]
    result = run(m,'bios_fileSelectDrive',a=7)
    assert result['carry'] and m[sy['v_drive']] == original
    record('failed old-volume flush prevents drive switch and cache discard')
    for offset,value in [(12,0),(13,0),(13,3),(16,3),(22,0),(19,3)]:
        m = fresh(); m[offset] = value
        result = run(m,'fatReadBootSector')
        assert result['carry'] and m[sy['v_diskInfo.isValid']] != sy['DISK_VALID'], (offset,result)
    record('unsupported or invalid BPB is rejected before a valid mount')
    if args.variant == 'DOS4000':
        for valid,error in [(0,sy['DOS_ERR_NO_DRIVER']),(1,sy['DOS_ERR_UNSUPPORTED'])]:
            m=fresh();m[sy['v_diskInfo.isValid']]=valid
            result=run(m,'bios_getDriveFreeSpace')
            assert result['carry'] and result['error']==error
        record('free-space query reports an unmounted volume instead of false success')
        m=fresh()
        assert run(m,'bios_fileSelectDrive',a=8)['error']==sy['DOS_ERR_ARGUMENT']
        assert not run(m,'bios_fileSelectDrive',a=1)['carry']
        assert run(m,'bios_getError')['a']==0
        record('successful selection of the same drive clears the previous error')
        m=fresh();m[512:544]=entry(b'BADIMG  EXE',2,17)
        m[260:262]=word(0xFFFF);m[0x7000:0x700B]=b'BADIMG  EXE'
        result=run(m,'bios_imageOpen',hl=0x7000,a=16)
        assert result['carry'] and result['error']==sy['DOS_ERR_BAD_IMAGE'],result
        record('invalid executable image reports its own error code')
        for length in [0,1,255,256,257,513]:
            m=fresh(); count=(length+255)//256
            m[512:544]=entry(b'EXACT   BIN',2 if length else 0,length)
            payload=bytes((i*29+3)&255 for i in range(length))
            for i in range(count):
                c=2+i
                m[256+c*2:258+c*2]=word(c+1 if i+1<count else 0xFFF8)
                chunk=payload[i*256:(i+1)*256]
                m[(4+i)*256:(5+i)*256]=chunk+bytes([0xCC])*(256-len(chunk))
            m[0x7000:0x700B]=b'EXACT   BIN'
            m[0x6000:0x6002+length]=bytes([0xA5])*(length+2)
            result=run(m,'bios_fileLoad2',hl=0x7000,de=0x6001)
            assert not result['carry'], (length,result)
            assert m[0x6001:0x6001+length]==payload
            assert m[0x6000]==m[0x6001+length]==0xA5
        record('exact load-at copies no sector padding for 0/1/255/256/257/513 bytes')
        for following in [0,0xFFF7,2,3]:
            m=fresh(); m[512:544]=entry(b'BADTAIL BIN',2,1)
            m[260:262]=word(following);m[0x7000:0x700B]=b'BADTAIL BIN'
            result=run(m,'bios_fileLoad2',hl=0x7000,de=0x6001)
            assert result['carry'] and result['error']==sy['DOS_ERR_CORRUPT'], (following,result)
        record('whole-file load rejects free/bad/cyclic/overlong tails')
        m=fresh();m[512:544]=entry(b'BIG     BIN',2,65536)
        m[0x7000:0x700B]=b'BIG     BIN'
        result=run(m,'bios_fileLoad2',hl=0x7000,de=0x6001)
        assert result['carry'] and result['error']==sy['DOS_ERR_ARGUMENT']
        record('load-at rejects a length above its address-space capacity')
        m=fresh();descriptor=entry(b'METADATATXT')
        descriptor[12]=255;descriptor[20:22]=word(0x1234)
        m[0x7000:0x7020]=descriptor
        assert not run(m,'bios_fileCreatePrepare',hl=0x7000,de=0)['carry']
        assert m[512+12]==0x18 and m[512+20:512+22]==bytes(2)
        record('create normalizes NT reserved bits and zeroes FstClusHI')
        m=fresh();m[0x7000:0x7020]=entry(b'WRONG   BIN',size=257)
        result=run(m,'bios_fileCreatePrepare',hl=0x7000,de=1)
        assert not result['carry'] and m[512:523]==b'WRONG   BIN'
        record('stream allocation derives cluster count from the 32-bit file length')
        for start,size in [(0x4000,256),(0x3FFF,2),(sy['dosCoreEnd']-1,2)]:
            m=fresh();before=bytearray(m[sy['dosCoreStart']:sy['dosCoreEnd']])
            result=run(m,'fileOpRange',hl=start,bc=size)
            assert result['carry'] and result['error']==sy['DOS_ERR_ARGUMENT']
            before[sy['v_dosError']-sy['dosCoreStart']]=sy['DOS_ERR_ARGUMENT']
            assert m[sy['dosCoreStart']:sy['dosCoreEnd']]==before
        record('caller buffers cannot overlap the DOS4000 implementation')
        for fail in [False,True]:
            m=fresh();payload=bytes((i*17+5)&255 for i in range(513))
            m[512:544]=entry(b'STREAM  BIN',2,len(payload))
            m[260:266]=word(3)+word(4)+word(0xFFFF)
            m[1024:1024+len(payload)]=payload
            req=bytearray(sy['FILE_COPY_REQUEST'])
            req[:4]=word(0x6001)+word(513);req[4:6]=bytes((1,1))
            req[6:17]=b'STREAM  BIN';req[38:49]=b'COPY    BIN'
            m[0x7600:0x7600+len(req)]=req
            m[0x6000:0x6203]=bytes([0xA5])*515
            if fail:m[0x8600:0x8602]=bytes((2,4))
            result=run(m,'bios_fileCopy',hl=0x7600)
            if fail:
                assert result['carry'] and result['error']==sy['DOS_ERR_IO'],result
                assert m[544]==0xE5 and m[266:272]==bytes(6)
            else:
                assert not result['carry'],result
                assert m[544:555]==b'COPY    BIN'
                current=int.from_bytes(m[570:572],'little');actual=bytearray()
                while current<0xFFF8:
                    actual+=m[(current+2)*256:(current+3)*256]
                    current=int.from_bytes(m[256+current*2:258+current*2],'little')
                assert actual[:513]==payload
            assert m[0x6000]==m[0x6202]==0xA5
        record('DOS4000 streaming copy preserves guards and cleans up a failed physical read')
        # РњРѕРЅРёС‚РѕСЂ-СЃРѕРІРјРµСЃС‚РёРјС‹Рµ СЃРµСЂРІРёСЃС‹ СЂР°Р±РѕС‚Р°СЋС‚ РїРѕСЃР»Рµ СѓРґР°Р»РµРЅРёСЏ РІСЃРµРіРѕ РЅРёР¶РЅРµРіРѕ DOS.
        m=fresh();m[sy['dosCoreStart']:sy['dosCoreEnd']]=bytes([0xA5])*(sy['dosCoreEnd']-sy['dosCoreStart'])
        assert run(m,'bios_getMemTop')['hl']==0x3FFF
        payload=bytes(range(256));m[0x2000:0x2100]=payload
        run(m,'bios_memcpy_bc_hl',hl=0x2000,de=0x2100,bc=0x3000)
        assert m[0x3000:0x3100]==payload
        m[0x9000:0xC000]=bytes(0x3000)
        run(m,'bios_printChar',bc=ord('A'))
        assert any(m[0x9000:0xC000])
        m[sy['j_diskDriver']+1:sy['j_diskDriver']+3]=word(sy['diskDriver'])
        result=run(m,'bios_diskDriver',bc=0x0300)
        assert not result['carry'] and result['hl']==sy['v_diskInfo'] and result['de']>0
        record('BIOS memTop/memcpy/printChar/disk-info work with the DOS4000 region erased')
    (OUT/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
    print('FAT REPAIR CPU PASS',len(rows))


if __name__ == '__main__': main()
