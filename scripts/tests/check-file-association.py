"""CPU 8080: системные ассоциации, временный буфер и восстановление диска."""
from pathlib import Path
import json,re,subprocess


def main():
    R=Path(__file__).resolve().parents[2];O=R/'docs/nc/low-004/association';O.mkdir(parents=True,exist_ok=True)
    s={k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',(R/'bin/DOS.sym').read_text(),re.M)};dos=(R/'bin/DOS.sys').read_bytes();fixture=b'TXT A:E.COM\rRKS A:RUN.COM\rEXE A:RUN.COM\r';rows=[]
    cases=[('TXT',fixture,None,b'A:E.COM'),('txt',fixture,None,b'A:E.COM'),('RKS',fixture,None,b'A:RUN.COM'),('BIN',fixture,None,None),('TXT',b'TXTA:BAD.COM\rTXT A:E.COM\r',None,b'A:E.COM'),('TXT',b'TXT \rTXT A:E.COM\r',None,b'A:E.COM'),('TXT',b'TXT A:E.COM',None,b'A:E.COM'),('TXT',b'TXT',None,None),('TXT',b'',None,None),('TXT',fixture,1024,None),('TXT',fixture,65536,None),('TXT',fixture+bytes([255])*(1023-len(fixture)),None,b'A:E.COM'),('TXT',fixture,'stat-error',None)]
    for ext,data,size,want in cases:
     m=bytearray(65536);m[0xC000:0xC000+len(dos)]=dos;m[0x3000:0x3401]=b'\xA5'*1025;m[0x4000:0x4400]=data[:1024]+bytes([255])*(1024-len(data[:1024]));m[0x7000:0x7003]=ext.encode();m[s['v_drive']]=7
     n=len(data) if size is None or isinstance(size,str) else size
     m[s['v_tmpFileDescr']+28:s['v_tmpFileDescr']+32]=n.to_bytes(4,'little');m[s['v_tmpFileDescr']+26:s['v_tmpFileDescr']+28]=b'\2\0'
     name=bytes.fromhex('AF 32')+s['v_drive'].to_bytes(2,'little')+b'\xC9';m[0x8400:0x8400+len(name)]=name
     m[0x8420:0x8422]=bytes((0x37 if size=='stat-error' else 0xAF,0xC9))
     count=(len(data)+255)//256
     transfer=bytes.fromhex('79 32 00 85 44 4D 21 00 40 11')+(0x4000+count*256).to_bytes(2,'little')+b'\xCD'+s['bios_memcpy_bc_hl'].to_bytes(2,'little')+bytes.fromhex('B7 C9');m[0x8440:0x8440+len(transfer)]=transfer
     sel=b'\x32'+s['v_drive'].to_bytes(2,'little')+bytes.fromhex('37 C9');m[0x8480:0x8480+len(sel)]=sel
     for label,target in [('fileNamePrepare',0x8400),('fileLoadInfo',0x8420),('fileTransferClusters',0x8440),('fileSelectDrive',0x8480)]:m[s[label]:s[label]+3]=b'\xC3'+target.to_bytes(2,'little')
     code=bytes.fromhex('31 00 8F 21 00 70 11 00 30 01 00 04 CD')+s['bios_fileAssociation'].to_bytes(2,'little')+bytes.fromhex('22 00 87 F5 E1 22 02 87 C3 00 83');m[0x8200:0x8200+len(code)]=code;(O/'in.bin').write_bytes(m)
     r=subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe','-m8080','-pc','8200','-end','8300','-counter','2000000','-output',str(O/'out.bin'),str(O/'in.bin')],capture_output=True,text=True,check=True,timeout=15)
     b=(O/'out.bin').read_bytes();assert int.from_bytes(b[65542:65544],'little')==0x8300,r.stdout;assert int.from_bytes(b[65544:65546],'little')==0x8F00
     assert bool(b[0x8702]&1)==(want is None),(ext,n,want,b[0x8702]);assert b[s['v_drive']]==7;assert b[0x3400]==0xA5
     if want:
      at=int.from_bytes(b[0x8700:0x8702],'little');actual=b[at:at+100].split(b'\r')[0].split(b'\xff')[0];assert actual==want,(actual,want)
     if n<1024 and size!='stat-error':assert b[0x8500]==count
     else:assert b[0x8500]==0
     rows.append(dict(extension=ext,size=n,stat_error=size=='stat-error',result=want.decode() if want else None,disk_restored=True,guard_preserved=True))
    (O/'result.json').write_text(json.dumps(rows,indent=2)+'\n');print('ASSOCIATION ABI PASS',len(rows))


if __name__ == "__main__":
    main()
