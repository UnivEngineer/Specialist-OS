"""Параметры файлового ABI: отказ до дисковых вызовов и записи в буфер."""
import json
from pathlib import Path
import re
import subprocess
from dos_state import initialize_dos_state
from relocated_component import load_at

ROOT=Path(__file__).resolve().parents[2]
OUT=ROOT/'docs/dos/file-ops-004/cpu'
TICKS=Path('D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe')


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    syms={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(ROOT/'bin/banked/KERNEL.sym').read_text(),re.M)}
    dos=(ROOT/'bin/DOS.sys').read_bytes()
    fileops,_=load_at(ROOT/'source/fileops/FILEOPS.asm','FILEOPS','OVERLAY_ORIGIN',syms['DOS_ARENA_START'])
    def word(v):return v.to_bytes(2,'little')
    def memory():
        m=bytearray(65536);m[0x100:0x100+len(dos)]=dos
        initialize_dos_state(m,syms)
        addr=syms['DOS_ARENA_START'];m[addr:addr+len(fileops)]=fileops
        struct_base = syms['fileopsOverlay']
        m[struct_base:struct_base+2] = word(addr)
        jump = struct_base + syms['OV_JUMP']
        m[jump:jump+3] = bytes([0xc3]) + word(addr)
        m[syms['fileopsEnsure']:syms['fileopsEnsure']+2]=bytes.fromhex('AF C9')
        # Монтирование и stat заменены счётчиком и ошибкой отсутствия файла.
        stub=bytes.fromhex('F5 3A 00 85 3C 32 00 85 F1 32')+word(syms['v_drive'])+bytes.fromhex('B7 C9')
        m[0x8400:0x8400+len(stub)]=stub
        m[syms['fileSelectDrive']:syms['fileSelectDrive']+3]=b'\xC3'+word(0x8400)
        m[syms['fileLoadInfo']:syms['fileLoadInfo']+2]=bytes.fromhex('37 C9')
        m[syms['v_diskInfo']+syms['DISK_INFO.isValid']]=syms['DISK_VALID']
        return m
    def run(m,label,hl,bc=0,a=0):
        code=b'\x31'+word(syms['DOS_BANK_STACK_TOP'])+b'\x21'+word(hl)+b'\x01'+word(bc)+bytes((0x3E,a,0xCD))+word(syms[label])+bytes.fromhex('F5 C1 60 69 22 10 85 C3 00 83')
        m[0x8200:0x8200+len(code)]=code;(OUT/'in.bin').write_bytes(m)
        r=subprocess.run([str(TICKS),'-m8080','-pc','8200','-end','8300','-counter','2000000','-output',str(OUT/'out.bin'),str(OUT/'in.bin')],capture_output=True,text=True,check=True,timeout=15)
        b=(OUT/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x8300,r.stdout
        return b[:65536]
    rows=[]
    top=syms['DOS_BANK_STACK_BOTTOM']
    request_size=syms['FILE_COPY_REQUEST']
    for name,request,buffer,size,calls in [
        ('zero-size',0xD000,0xD200,0,0),('short',0xD000,0xD200,255,0),
        ('overflow',0xD000,0xFFF0,512,0),('stack-overlap',0xD000,top-255,256,0),
        ('context-at-start',0xD000,0xD000,512,0),('context-inside',0xD000,0xCF00,512,0),
        ('context-at-end',0xD000,0xD020,512,0),('bad-context',top-request_size+1,0xD200,256,0),
        ('valid-before',0xD000,0xCF00,256,2),('valid-after',0xD000,0xD000+request_size,256,2),
        ('valid-last-ram',0xD000,top-256,256,2),('unaligned-capacity',0xD000,0xD201,257,2)]:
        m=memory();data=bytearray(syms['FILE_COPY_REQUEST']);data[:4]=word(buffer)+word(size);data[4:6]=bytes((1,7));m[request:request+len(data)]=data
        before=bytes(m[request:request+len(data)])
        # За пределами явного буфера также лежат код, стек и регистры 8080.
        guard_start=buffer if buffer+size<=top else 0xD400
        guard=bytes(m[guard_start:guard_start+size])
        b=run(m,'dos_fileCopy',request)
        assert b[0x8510]&1 and b[0x8511]==1,(name,b[0x8510:0x8512])
        assert b[0x8500]==calls+1,(name,b[0x8500])  # FILEOPS восстанавливает диск вызывающего.
        assert b[request:request+len(data)]==before and b[guard_start:guard_start+size]==guard,name
        rows.append(dict(case=name,disk_selections=calls,caller_data_unchanged=True))
    b=run(memory(),'dos_fileGetDrives',0xD000,7)
    assert b[0x8510]&1 and b[0x8500]==0
    b=run(memory(),'dos_fileChangeDrive',0,0,a=8)
    assert b[0x8510]&1 and b[0x8500]==0
    for public,target in [('dos_fileRename','fileRename'),('dos_fileDelete','fileDelete')]:
        offset=syms[public]-0x100;assert dos[offset:offset+3]==b'\xC3'+word(syms[target])
    (OUT/'result.json').write_text(json.dumps(rows,indent=2)+'\n')
    print('FILE OPERATIONS CPU PASS',len(rows)+4)


if __name__=='__main__':main()
