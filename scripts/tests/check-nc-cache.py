"""CPU 8080: отдельные кэши каталогов NC, окна, инвалидация и границы."""
import argparse
import json
from pathlib import Path
import re
import subprocess

ROOT = Path(__file__).resolve().parents[2]


def main():
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--ticks',type=Path,default=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe'))
    p.add_argument('--report-directory',type=Path,default=ROOT/'docs/nc/cache-005/cpu')
    args=p.parse_args();out=args.report_directory.resolve();out.mkdir(parents=True,exist_ok=True)
    syms=lambda name:{k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin'/name).read_text(),re.M)}
    ns,ds=syms('NC.sym'),syms('DOS.sym');dos=(ROOT/'bin/DOS.sys').read_bytes();nc=(ROOT/'bin/NC.i80').read_bytes()
    def memory():
        m=bytearray(65536);m[0xC000:0xC000+len(dos)]=dos;m[:len(nc)]=nc
        m[ns['FILE_CACHE_A']:ns['NC_END']]=b'\xA5'*(ns['NC_END']-ns['FILE_CACHE_A'])
        m[ns['vars.cacheValidA']:ns['vars.cacheValidA']+2]=bytes(2)
        m[ds['bios_fileGetSetDrive']]=0xC9
        return m
    def fixture(m,count,tag):
        n=min(count,ns['FILE_CACHE_CAPACITY'])
        data=b''.join((f'{tag}{i:07}TXT'.encode()+bytes(5)) for i in range(n))
        m[0x5000:0x5000+len(data)]=data
        code=bytes.fromhex('C5 E5 3A 00 86 3C 32 00 86 44 4D 21 00 50 11')+(0x5000+len(data)).to_bytes(2,'little')
        code+=b'\xCD'+ds['bios_memcpy_bc_hl'].to_bytes(2,'little')+bytes.fromhex('3E FF 02 E1 C1 11')+count.to_bytes(2,'little')+b'\xC9'
        m[0x8400:0x8400+len(code)]=code;m[ds['bios_fileList']:ds['bios_fileList']+3]=bytes.fromhex('C3 00 84')
        return data
    def call(m,label,a=0):
        code=bytes.fromhex('31 00 8F 3E')+bytes([a,0xCD])+ns[label].to_bytes(2,'little')+bytes.fromhex('C3 00 83');m[0x8200:0x8200+len(code)]=code
        inp,target=out/'in.bin',out/'out.bin';inp.write_bytes(m)
        r=subprocess.run([str(args.ticks),'-m8080','-pc','8200','-end','8300','-counter','2000000','-output',str(target),str(inp)],check=True,capture_output=True,text=True,timeout=15)
        b=target.read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x8300,r.stdout;assert int.from_bytes(b[65544:65546],'little')==0x8F00
        return bytearray(b[:65536]),int(r.stdout.strip())
    def view(m,panel,first,selected):
        m[ns['state']+1]=panel;m[ns['state']+8+panel]=first;m[ns['state']+10+panel]=selected
    def ptr(m):return int.from_bytes(m[ns['vars.fileListPtr']:ns['vars.fileListPtr']+2],'little')
    rows=[];m=memory();source_a=fixture(m,140,'A');view(m,0,0,0);m,cold=call(m,'loadFiles',0)
    assert m[0x8600]==1 and ptr(m)==ns['FILE_CACHE_A'] and m[ns['state']+6]==140
    view(m,0,1,36);m,hit=call(m,'loadFiles',0);assert m[0x8600]==1 and ptr(m)==ns['FILE_CACHE_A']+16
    source_b=fixture(m,192,'B');view(m,1,19,54);m,_=call(m,'loadFiles',7)
    assert m[0x8600]==2 and ptr(m)==ns['FILE_CACHE_B']+19*16
    assert m[ns['FILE_CACHE_A']:ns['FILE_CACHE_A']+len(source_a)]==source_a
    view(m,0,1,36);m,_=call(m,'loadFiles',0);assert m[0x8600]==2 and ptr(m)==ns['FILE_CACHE_A']+16
    changed=fixture(m,36,'C');m,_=call(m,'invalidateFileCache');view(m,0,1,36);m,_=call(m,'loadFiles',0)
    assert m[0x8600]==3 and m[ns['state']+10]==35
    assert m[ns['FILE_CACHE_A']:ns['FILE_CACHE_A']+len(changed)]==changed
    m,_=call(m,'loadFiles',1);assert m[0x8600]==4 and m[ns['vars.cacheDriveA']]==1
    rows.append(dict(case='panel isolation, hit, refresh, disk change',cold_ticks=cold,hit_ticks=hit,list_calls=4))
    for count in [0,1,36,140,255,256]:
        m=memory();fixture(m,count,'F');view(m,0,255,255);code_before=m[:ns['state']];m,_=call(m,'loadFiles',7)
        total=min(count,255);last=max(0,total-1);assert m[ns['state']+6]==total
        assert m[ns['state']+8]==m[ns['state']+10]==last
        assert ptr(m)==ns['FILE_CACHE_A']+last*16 and m[ns['FILE_CACHE_A']+total*16]==255
        assert m[ns['FILE_CACHE_B']]==0xA5 and m[:ns['state']]==code_before
        m,_=call(m,'loadFiles',7);assert m[0x8600]==1
        rows.append(dict(count=count,visible_count=total,clamped=last,guard_preserved=True))
    report=dict(cases=rows,copy_buffer=ns['COPY_BUFFER'],copy_sectors=ns['COPY_BUFFER_SECTORS'])
    (out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report))


if __name__=='__main__':main()
