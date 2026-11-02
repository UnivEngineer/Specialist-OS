"""Файловое и прямое чтение/запись TMP заданного размера: только Emu80-flash-test."""
import json,runpy,struct,subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))
m=h['Bench']('http://127.0.0.1:19781/mcp');symbols=h['symbols']
runtime=Path('D:/Projects/RetroPC/Specialist/Emulator/Emu80-flash-test')
query='(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
exe=subprocess.run(['powershell.exe','-NoProfile','-Command',query],capture_output=True,text=True,check=True).stdout.strip()
assert Path(exe).resolve()==(runtime/'Emu80qt-mcp.exe').resolve()
s=symbols(ROOT/'source-apps/diskbench/build/DISKBEN.sym');nc=symbols(ROOT/'bin/NC.sym')
out=ROOT/'docs/diskbench/file-004';out.mkdir(parents=True,exist_ok=True)
m.call('debug_break')
assert m.read(0xC000,3)==bytes.fromhex('C309C0')
m.stop_at(0x8100)
m.subroutine(0xC842,af=0,de=1)
if m.subroutine(0xC863,bc=0xFF00)['registers']['bc']['dec']>>8!=ord('F'):
    m.install_flash('FlashSys')
results={}
scenarios=[('h-default','h','Flash','0','2',''),('h-one','h','FlashFst','0','1','1'),('h-nine','h','FlashFst','0','1','9'),('h-max','h','FlashFst','0','1','24'),('h-verify','h','FlashFst','1','1','16'),('a-max','a',None,'0','1','24'),('a-verify','a',None,'1','1','9'),('b-max','b',None,'0','2','24'),('h-zero','h','FlashFst','0','0',None)]
for name,drive,driver,choice,budget,size in scenarios:
    kib=int(size or '8');length=kib*1024;sectors=kib*4
    m.stop_at(0x8100)
    if driver:
        m.subroutine(0xC842,af=0,de=1)
        m.install_flash(driver)
    if drive=='b':
        # Независимая RAM-фикстура: не зависит от момента остановки холодного FORMAT.BAT.
        m.write(0xF100,(ROOT/'bin/FORMAT.com').read_bytes())
        m.write(0x8400,b'B R Y\r\0')
        m.write(0x8800,bytes.fromhex('CD00F1C30087'))
        m.stop_at(0x8700);m.call('regs_set',pc=0x8800,sp=0x8F00,de=0x8400);m.until(0x8700)
        m.stop_at(0x8100);assert not m.subroutine(0xC896)['flags']['C']
    image=None if drive=='b' else runtime/'specialist-os'/('system-flash.bin' if drive=='a' else 'flash-disk.bin')
    before=image.read_bytes() if image else None
    if before:
        word=lambda n:struct.unpack_from('<H',before,n)[0]
        fs=word(14)*256;fe=fs+word(22)*256;rs=fs+(fe-fs)*before[16]
        data=rs+((word(17)*32+255)//256)*256
    m.write(0,(ROOT/'bin/DISKBEN.com').read_bytes())
    for addr,value in [(0x1FFF,0xAA),(0x8100,0xBB),(0x8E00,0xCC)]:m.write(addr,bytes([value]))
    m.stop_at(s['menu_drive']);m.call('regs_set',pc=0);m.until(s['menu_drive'])
    m.stop_at(s['run_tests']);m.line(drive)
    if choice is not None:m.line(choice)
    m.line(budget)
    if size is not None:
        if name=='h-default':
            for invalid in ['0','25','0x8','1x','99999999']:
                m.line(invalid)
                assert m.call('cpu_state')['registers']['pc']['dec']!=s['run_tests']
        m.line(size)
    m.until(s['run_tests'])
    if int(budget):
        assert m.read(s['v_fileKiB'],1)==bytes([kib])
        assert m.read(s['v_fileSectors'],1)==bytes([sectors])
        assert m.read(s['v_bufferEndHi'],1)==bytes([0x20+sectors])
    original=m.read(0xC864,2)
    count=int(budget);buffers=1
    if count:
        m.stop_at(s['file_cleanup'])
        for buf in range(buffers):
            for attempt in range(1,count+1):
                m.until(s['file_cleanup'])
                assert m.read(s['v_pass'],1)==bytes([attempt]),name
                assert m.read(s['v_bufferMode'],1)==bytes([buf]),name
                assert m.read(s['v_fileOwned'],1)==b'\1'
                assert m.read(0xC864,2)==original
                payload=m.read(0x2000,length)
                assert payload==bytes(((i+0x2000)>>8)^((i+0x2000)&255)^attempt for i in range(length)),name
                if image:
                    now=image.read_bytes()
                    entries=[now[a:a+32] for a in range(rs,data,32) if now[a:a+11]==b'DISKBEN TMP']
                    assert len(entries)==1
                    d=entries[0];assert int.from_bytes(d[28:32],'little')==length-1
                    cluster=int.from_bytes(d[26:28],'little');chain=[]
                    for i in range(sectors):
                        chain.append(cluster)
                        nxt=int.from_bytes(now[fs+cluster*2:fs+cluster*2+2],'little')
                        if i==sectors-1:assert nxt==cluster
                        else:assert nxt!=cluster
                        cluster=nxt
                    on_disk=b''.join(now[data+(c-2)*256:data+(c-1)*256] for c in chain)
                    assert payload==on_disk,name
                if buf==buffers-1 and attempt==count:m.stop_at(s['file_results'])
                else:m.call('debug_step',mode='into')
        m.until(s['file_results'])
        assert m.read(s['v_fileOwned'],1)==b'\0'
        record={}
        for buf in range(buffers):
            raw=m.read(s['resultsLow' if buf==0 else 'resultsHigh'],40)
            record[str(buf)]={}
            for i,method in enumerate(['rawWrite','rawRead','fileWrite','fileRead']):
                ticks,flush,calls=struct.unpack_from('<IIH',raw,i*10)
                assert ticks>0 and ticks>=flush,(name,method,ticks,flush)
                expected=(count if method=='rawWrite' else 2*count if method=='fileWrite' else 0) if driver=='FlashFst' else 0
                assert calls==expected,(name,method,calls,expected)
                record[str(buf)][method]={'ticks':ticks,'flushTicks':flush,'flushCalls':calls}
        if drive=='a':
            high=m.read(s['resultsHigh'],40)
            assert struct.unpack_from('<IIH',high,0)==(0,0,0) and struct.unpack_from('<IIH',high,20)==(0,0,0)
            assert struct.unpack_from('<IIH',high,10)[0]>0 and struct.unpack_from('<IIH',high,30)[0]>0
        results[name]={'sizeKiB':kib,'clusters':sectors,'measurements':record}
    else:results[name]={'writesSkipped':True}
    m.stop_at(s['exit_wait']);m.until(s['exit_wait'])
    for addr,value in [(0x1FFF,0xAA),(0x8100,0xBB),(0x8E00,0xCC)]:assert m.read(addr,1)==bytes([value]),(name,addr)
    if image:
        after=image.read_bytes()
        assert before[fs:rs]==after[fs:rs],name
        assert b'DISKBEN TMP' not in after[rs:data],name
        for a in range(rs,data,32):
            if before[a]!=255:assert before[a:a+32]==after[a:a+32],(name,a)
        for c in range(2,(len(before)-data)//256+2):
            if fs+c*2+2>fe:break
            if before[fs+c*2:fs+c*2+2]!=b'\0\0':
                a=data+(c-2)*256;assert before[a:a+256]==after[a:a+256],(name,c)
    m.screenshot(out/(name+'.png'))
    print(name,results[name],flush=True)
    (out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
(out/'results.json').write_text(json.dumps(results,indent=2)+'\n')
