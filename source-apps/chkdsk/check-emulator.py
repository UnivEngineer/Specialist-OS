"""CHKDSK и memmove на CPU Emu80; запись только в RAM отдельной тестовой установки."""
import argparse,hashlib,json,random,runpy,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
helpers=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))

def cs(data):
    b=c=0
    for i,x in enumerate(data):
        s=c+x;c=s&255
        if i+1<len(data):b=(b+x+(s>>8))&255
    return b*256+c

def image(length=600):
    raw=bytearray(b'\xff'*65536)
    struct.pack_into('<HBHBHHBH',raw,11,256,1,1,1,16,256,0xF8,2)
    raw[54:62]=b'FAT16   '
    raw[256:768]=bytes(512)
    struct.pack_into('<HH',raw,256,65535,65535)
    struct.pack_into('<H',raw,256+252*2,0xFFF7)
    payload=bytes((i*71+17)&255 for i in range(length))
    rks=struct.pack('<HH',0x1000,0x1000+len(payload)-1)+payload+struct.pack('<H',cs(payload))
    mx=bytearray(64);mx[:6]=b'\xcd\x20MXEX';mx[6]=1
    struct.pack_into('<HHHHHHH',mx,8,64,0x1000,0x1000,len(payload),len(payload),cs(payload),0)
    mx[22:30]=b'MXOS    ';mx[30]=1
    struct.pack_into('<H',mx,20,cs(mx))
    files=[(b'GOOD    RKS',rks),(b'APP     EXE',mx+payload),(b'PLAIN   COM',b'\xc9')]
    cluster=2
    for i,(name,data) in enumerate(files):
        off=3*256+i*32;raw[off:off+32]=bytes(32);raw[off:off+11]=name
        struct.pack_into('<H',raw,off+26,cluster);struct.pack_into('<I',raw,off+28,len(data)-1)
        for pos in range(0,len(data),256):
            last=pos+256>=len(data)
            struct.pack_into('<H',raw,256+2*cluster,cluster if last else cluster+1)
            sector=5+cluster-2;raw[sector*256:sector*256+min(256,len(data)-pos)]=data[pos:pos+256]
            cluster+=1
    return raw

def main():
    p=argparse.ArgumentParser(description=__doc__);p.add_argument('--runtime',type=Path,required=True)
    p.add_argument('--report',type=Path,default=ROOT/'docs/chkdsk/validation.json');a=p.parse_args()
    assert a.runtime.name=='Emu80-flash-test'
    q='(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    exe=subprocess.run(['powershell.exe','-NoProfile','-Command',q],capture_output=True,text=True,check=True).stdout.strip()
    assert Path(exe).resolve()==(a.runtime/'Emu80qt-mcp.exe').resolve()
    m=helpers['Bench']('http://127.0.0.1:19781/mcp')
    read_part=m.read
    m.read=lambda addr,length:b''.join(read_part(addr+i,min(4096,length-i)) for i in range(0,length,4096))
    m.call('debug_break');m.write(0xFFFC,b'\0');m.stop_at(0x8100)
    assert m.read(0xC000,32)==(ROOT/'bin/DOS.sys').read_bytes()[:32]
    ds=helpers['symbols'](ROOT/'bin/DOS.sym');s=helpers['symbols'](ROOT/'bin/CHKDSK.sym')
    report={'status':'emulator-only','moves':[],'disks':[]}
    rand=random.Random(8080)
    # Независимое ожидаемое состояние: снимок источника до любого изменения.
    cases=[(0x2000,0x5000,n) for n in [0,1,2,3,15,16,255,256,257,4096]]
    cases += [(0x20FD,0x20FD+d,n) for d in [-257,-2,-1,0,1,2,257] for n in [1,2,3,255,257]]
    cases += [(rand.randrange(0x2100,0x3100),rand.randrange(0x2100,0x3100),rand.randrange(512)) for _ in range(20)]
    initial=bytes(rand.randrange(256) for _ in range(0x5000))
    for src,dst,n in cases:
        m.write(0x1000,initial);expected=bytearray(initial);expected[dst-0x1000:dst-0x1000+n]=initial[src-0x1000:src-0x1000+n]
        state=m.subroutine(0xC42D,hl=src,de=src+n,bc=dst)
        assert m.read(0x1000,len(initial))==expected,(src,dst,n)
        assert state['registers']['hl']['dec']==src+n and state['registers']['bc']['dec']==dst+n
        assert state['registers']['de']['dec']==src+n and state['registers']['sp']['dec']==0x8F00
    # Внутренний memmove сохраняет регистры, DEBUG T использует тот же движок.
    m.write(0x2000,bytes(range(256)));state=m.subroutine(ds['memmove_bc_hl'],hl=0x2000,de=0x2011,bc=0x2001)
    assert all(state['registers'][k]['dec']==v for k,v in {'hl':0x2000,'de':0x2011,'bc':0x2001}.items())
    m.write(0xF100,(ROOT/'bin/Debug.i80').read_bytes());debug=helpers['symbols'](ROOT/'bin/Debug.sym')
    m.write(0x2000,bytes(range(256)));m.subroutine(debug['Dir_T'],hl=0x2000,de=0x2010,bc=0x2001)
    assert m.read(0x2001,17)==bytes(range(17))
    report['move_cases']=len(cases)+2
    print('memmove:',report['move_cases'],'cases OK',flush=True)
    tmp=ROOT/'.codex-tmp/chkdsk';tmp.mkdir(parents=True,exist_ok=True)
    old=subprocess.run(['git','show','5e83e32:source/dos/memcpy_bc_hl.inc'],cwd=ROOT,capture_output=True,text=True,check=True).stdout
    cmp=subprocess.run(['git','show','5e83e32:source/dos/cmp_hl_de.inc'],cwd=ROOT,capture_output=True,text=True,check=True).stdout
    (tmp/'old.asm').write_text('    ORG 07000h\n'+old+cmp)
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(tmp/'old.bin'),str(tmp/'old.asm')],check=True,capture_output=True)
    m.write(0x7000,(tmp/'old.bin').read_bytes())
    for n in [16,256,4096]:
        times={}
        for label,addr in [('historical',0x7000),('stack',0xC42D)]:
            before=m.call('cpu_state')['cpu_ticks']
            state=m.subroutine(addr,hl=0x2000,de=0x2000+n,bc=0x5000)
            times[label]=state['cpu_ticks']-before-27
        report['moves'].append({'bytes':n,**times})
        print('copy cycles',n,times,flush=True)
    m.subroutine(0xC842,af=0x0100,de=1)
    m.write(0xFFFD,b'\0');backup=m.read(0,0xFF00);m.write(0xFFFC,b'\0')
    base=image()
    def mutation(offset,data):
        x=bytearray(base);x[offset:offset+len(data)]=data;return x
    scenarios=[('good',base,0),('bad-rks',mutation(5*256+8,b'\x99'),2),
      ('bad-mx-header',mutation(8*256+20,b'\0\0'),2),
      ('cycle',mutation(256+2*3,struct.pack('<H',2)),1),
      ('cross-link',mutation(3*256+32+26,struct.pack('<H',2)),1),
      ('short-chain',mutation(256+4,struct.pack('<H',2)),1),
      ('long-chain',mutation(256+8,struct.pack('<H',5)),1),
      ('bad-cluster',mutation(256+4,struct.pack('<H',0xFFF7)),1),
      ('free-in-chain',mutation(256+4,b'\0\0'),1),
      ('out-of-range',mutation(3*256+26,struct.pack('<H',253)),1),
      ('lost-cluster',mutation(256+40,struct.pack('<H',20)),1),
      ('duplicate',mutation(3*256+32,b'GOOD    RKS'),1),
      ('unsupported-bpb',mutation(13,b'\x02'),3),
      ('reserved-fat',mutation(256,b'\0\0'),1),
      ('directory-not-supported',mutation(3*256+11,b'\x10'),1),
      ('lfn-not-supported',mutation(3*256+11,b'\x0f'),1),
      ('high-size',mutation(3*256+30,b'\1\0'),1),
      ('missing-first-cluster',mutation(3*256+26,b'\0\0'),1)]
    scenarios += [('checksum-length-'+str(n),image(n),0) for n in [1,255,256,257,511,512,513,16000]]
    scenarios += [('lowercase-rks-extension',mutation(3*256+8,b'rks'),0),
                  ('mixedcase-rks-extension',mutation(3*256+8,b'RkS'),0)]
    try:
        for name,data,want in scenarios:
            m.write(0xFFFD,b'\0');m.write(0,data[:0xFF00]);m.write(0xFFFC,b'\0')
            m.write(0,(ROOT/'bin/CHKDSK.i80').read_bytes());m.stop_at(s['waitKey']);m.call('regs_set',pc=s['scan'],sp=0x8F00)
            m.until(s['waitKey'],60)
            got=m.read(s['result'],1)[0];errors=int.from_bytes(m.read(s['errors'],2),'little')
            checked=int.from_bytes(m.read(s['checked'],2),'little');skipped=int.from_bytes(m.read(s['skipped'],2),'little')
            assert got==want,(name,got,want,errors,checked,skipped)
            if want==0:assert (errors,checked,skipped)==(0,2,1),(errors,checked,skipped)
            m.write(0xFFFD,b'\0');assert m.read(0,0xFF00)==data[:0xFF00];m.write(0xFFFC,b'\0')
            report['disks'].append({'name':name,'result':got,'errors':errors,'checked':checked,'skipped':skipped,'disk_unchanged':True})
            print(name,got,errors,checked,skipped,flush=True)
    finally:
        m.write(0xFFFD,b'\0');m.write(0,backup);m.write(0xFFFC,b'\0')
    # Отказ физического чтения: выполнение прерывается с отдельным статусом.
    dispatch=m.read(0xC863,3)
    try:
        m.write(0xC863,b'\x37\xc9\0');m.write(0,(ROOT/'bin/CHKDSK.i80').read_bytes())
        m.stop_at(s['waitKey']);m.call('regs_set',pc=s['scan'],sp=0x8F00);m.until(s['waitKey'],30)
        assert m.read(s['result'],1)==b'\4'
        report['io_error_status']=4
    finally:m.write(0xC863,dispatch)
    report['sha256']={n:hashlib.sha256((ROOT/'bin'/n).read_bytes()).hexdigest() for n in ['DOS.sys','Debug.com','CHKDSK.com']}
    a.report.parent.mkdir(parents=True,exist_ok=True);a.report.write_text(json.dumps(report,indent=2)+'\n')

if __name__=='__main__':main()
