"""Параметры файлового ABI: отказ до дисковых вызовов и записи в буфер."""
import json
from pathlib import Path
import re
import subprocess

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/dos/file-ops-004/cpu'
TICKS=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    syms={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/DOS.sym').read_text(),re.M)}
    dos=(ROOT/'bin/DOS.sys').read_bytes()
    def word(v):return v.to_bytes(2,'little')
    def memory():
        m=bytearray(65536);m[0xC000:0xC000+len(dos)]=dos
        # Монтирование и stat заменены счётчиком и ошибкой отсутствия файла.
        stub=bytes.fromhex('F5 3A 00 85 3C 32 00 85 F1 32')+word(syms['v_drive'])+bytes.fromhex('B7 C9')
        m[0x8400:0x8400+len(stub)]=stub
        m[syms['fileSelectDrive']:syms['fileSelectDrive']+3]=b'\xC3'+word(0x8400)
        m[syms['fileLoadInfo']:syms['fileLoadInfo']+2]=bytes.fromhex('37 C9')
        m[syms['v_diskInfo']+syms['DISK_INFO.isValid']]=syms['DISK_VALID']
        return m
    def run(m,label,hl,bc=0,a=0):
        code=b'\x31'+word(syms['STACK_ADDR'])+b'\x21'+word(hl)+b'\x01'+word(bc)+bytes((0x3E,a,0xCD))+word(syms[label])+bytes.fromhex('F5 C1 60 69 22 10 85 C3 00 83')
        m[0x8200:0x8200+len(code)]=code;(OUT/'in.bin').write_bytes(m)
        r=subprocess.run([str(TICKS),'-m8080','-pc','8200','-end','8300','-counter','2000000','-output',str(OUT/'out.bin'),str(OUT/'in.bin')],capture_output=True,text=True,check=True,timeout=15)
        b=(OUT/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x8300,r.stdout
        return b[:65536]
    rows=[]
    for name,request,buffer,size,calls in [
        ('zero-size',0x7000,0x6000,0,0),('short',0x7000,0x6000,255,0),
        ('overflow',0x7000,0xFFF0,512,0),('bios-overlap',0x7000,0x8F00,256,0),
        ('context-at-start',0x7000,0x7000,512,0),('context-inside',0x7000,0x6F00,512,0),
        ('context-at-end',0x7000,0x7020,512,0),('bad-context',0x8FC0,0x6000,256,0),
        ('valid-before',0x7000,0x6F00,256,2),('valid-after',0x7000,0x704C,256,2),
        ('valid-last-ram',0x7000,0x8EDF,256,2),('unaligned-capacity',0x7000,0x6001,257,2)]:
        m=memory();data=bytearray(syms['FILE_COPY_REQUEST']);data[:4]=word(buffer)+word(size);data[4:6]=bytes((1,7));m[request:request+len(data)]=data
        m[0x6000:0x6100]=bytes([0xA5])*256 if not (0x6000<=request<0x6100) else m[0x6000:0x6100]
        before=bytes(m[request:request+len(data)]);guard=bytes(m[0x6000:0x6100])
        b=run(m,'bios_fileCopy',request)
        assert b[0x8510]&1 and b[0x8511]==1,(name,b[0x8510:0x8512])
        assert b[0x8500]==calls,(name,b[0x8500])
        assert b[request:request+len(data)]==before and b[0x6000:0x6100]==guard,name
        rows.append(dict(case=name,disk_selections=calls,caller_data_unchanged=True))
    b=run(memory(),'bios_fileGetDrives',0x6000,7)
    assert b[0x8510]&1 and b[0x8500]==0
    b=run(memory(),'bios_fileSelectDrive',0,0,a=8)
    assert b[0x8510]&1 and b[0x8500]==0
    for public,target in [('bios_fileRenameOnDrive','fileRename'),('bios_fileDeleteOnDrive','fileDelete')]:
        offset=syms[public]-0xC000;assert dos[offset:offset+3]==b'\xC3'+word(syms[target])
    (OUT/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
    print('FILE OPERATIONS CPU PASS',len(rows)+4)


if __name__=='__main__':main()
