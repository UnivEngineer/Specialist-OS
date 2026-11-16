"""Full FILESTREAM requests: cursor words, partial faults and before/after timing."""
import struct,subprocess

def check_cursor(root,out,m,ks,ts,select,invoke,request,regs,rows,calls):
    word=lambda n:struct.pack('<H',n)
    # The common kernel fixture has already created this 20003-byte file on B:.
    select(3);m.write(0x3300,b'PATH2000BIN'+bytes(21))
    request(3,ks['MXOS_DOS_FILE_LOAD_INFO'],regs(hl=0x3300))
    first=int.from_bytes(m.read(0x331a,2),'little')
    select(2);boot=m.read(0,64)
    sector=int.from_bytes(boot[11:13],'little');cluster_bytes=sector*boot[13]
    fat=int.from_bytes(boot[14:16],'little')*sector
    chain=[first]
    for _ in range(100):
        following=int.from_bytes(m.read(fat+chain[-1]*2,2),'little')
        if following>=0xfff8:break
        assert following>=2 and following not in chain
        chain.append(following)
    payload=bytes((i*29+i//257+20003)&255 for i in range(20003))
    physical=ts['bankCopyPhysical']
    def clean(page):
        request(page,ks['MXOS_DOS_FILE_SELECT_DRIVE'],regs(a=1))
        request(page,ks['MXOS_DOS_FLUSH_VOLUME'],regs())
        request(page,ks['MXOS_DOS_MOUNT_VOLUME'],regs())
    def execute(page,operation,count,offset,fail_at=0,restore=False):
        cursor=0xe100 if page==1 else 0x3100
        buffer=0xc001 if page==1 else 0x4001
        clean(page);select(page)
        original=word(first)+word(offset)
        m.write(cursor-1,b'\xaa'+original+b'\xbb')
        source=payload[offset:offset+count]
        written=bytes(x^0x5a for x in source)
        m.write(buffer-1,b'\x96'+(written if operation==1 else bytes([0xa5])*count)+b'\x69')
        saved=None
        if fail_at:
            select(1);saved=m.read(physical,3)
            # Only data runs use klRamCopy. FAT/cache copies use other requests.
            asm=f""" ORG 0BA00h
 push af
 push de
 ld de,{ks['klRamCopy']}
 ld a,h
 cp d
 jp nz,forward
 ld a,l
 cp e
 jp nz,forward
 push hl
 ld hl,0BBF0h
 inc (hl)
 ld a,{fail_at}
 cp (hl)
 pop hl
 jp nz,forward
 pop de
 pop af
 ld a,1
 scf
 ret
forward:
 pop de
 pop af
 DB {','.join(str(b) for b in saved)}
 jp {physical+3}
 END
"""
            (out/'cursor-fault.asm').write_text(asm)
            subprocess.run([str(root/'tasm/sjasmplus.exe'),'--i8080',f'--raw={out/"cursor-fault.bin"}',str(out/'cursor-fault.asm')],check=True,capture_output=True)
            m.write(0xba00,(out/'cursor-fault.bin').read_bytes());m.write(0xbbf0,b'\0')
            m.write(physical,b'\xc3'+word(0xba00))
        try:
            result=invoke(page,ks['BANK_DOS_GATE'],regs(hl=buffer,de=cursor,bc=count,a=operation),ks['MXOS_DOS_FILESTREAM'])
            tick=calls[-1]['ticks']
        finally:
            if saved is not None:select(1);m.write(physical,saved)
        done=0 if fail_at==1 else 8192 if fail_at==2 else count
        expected=(ks['DOS_ERR_IO'],True) if fail_at else (0,False)
        assert result==expected,(page,operation,count,offset,fail_at,result)
        select(page)
        packet=0x7d00 if page==1 else 0x3000
        returned=m.read(packet,8)
        assert returned[:6]==word(buffer+done)+word(cursor)+word(done),(page,operation,fail_at,'registers',returned.hex())
        cluster_index,remainder=divmod(offset+done,cluster_bytes)
        assert m.read(cursor,4)==word(chain[cluster_index])+word(remainder),(page,operation,fail_at,'cursor')
        assert m.read(cursor-1,1)==b'\xaa' and m.read(cursor+4,1)==b'\xbb'
        assert m.read(buffer-1,1)==b'\x96' and m.read(buffer+count,1)==b'\x69'
        assert m.read(ks['BANK_PROFILE'],1)==b'\0'
        if operation==2:
            assert m.read(buffer,done)==source[:done]
            assert m.read(buffer+done,count-done)==bytes([0xa5])*(count-done)
        else:
            # Re-read the updated prefix, then restore it without reallocating.
            clean(page);select(page);m.write(cursor,original);m.write(buffer,bytes(count))
            request(page,ks['MXOS_DOS_FILESTREAM'],regs(hl=buffer,de=cursor,bc=count,a=2))
            assert m.read(buffer,count)==written[:done]+source[done:]
            m.write(cursor,original);m.write(buffer,source)
            request(page,ks['MXOS_DOS_FILESTREAM'],regs(hl=buffer,de=cursor,bc=count,a=1))
            request(page,ks['MXOS_DOS_FLUSH_VOLUME'],regs())
        rows.append(dict(case='filestream-full-request',caller=page,operation=operation,bytes=count,offset=offset,failedDataRun=fail_at,completed=done,ticks=tick,ms=tick/2000,cursorCluster=chain[cluster_index],cursorOffset=remainder,error=expected[0],commandOnReturn=True))
        print('FILESTREAM REQUEST',page,operation,count,offset,fail_at,done,tick,flush=True)
    for page in (0,1,3):
        for operation in (2,1):
            for count,offset in ((17,17),(256,1),(8192,0)):
                execute(page,operation,count,offset)
            # DOS-local arena has room for 8 KiB, use external callers for 16 KiB.
            if page!=1:
                for fail_at in (1,2):execute(page,operation,16384,0,fail_at)

    if 'kfCursorPutFirst' in ks:
        page=3;cursor=0x3100;buffer=0x4001
        clean(page);select(page)
        m.write(cursor,word(first)+word(0));m.write(buffer,bytes(256))
        m.write(0x3000,regs(hl=buffer,de=cursor,bc=256,a=2))
        m.write(0x2800,b'\xcd'+word(0xf800)+b'\xc3'+word(0x8d00))
        m.call('bp_clear')
        m.call('bp_set',addr=ks['kfCursorPutFirst'],type='exec')
        m.call('regs_set',pc=0x2800,sp=0x3f00,bc=ks['MXOS_DOS_FILESTREAM'],de=0x3000)
        m.until(ks['kfCursorPutFirst'],30)
        assert m.read(ts['bankPage'],1)==b'\1'
        assert m.read(ks['BANK_PROFILE'],1)==b'\0','first PUT16 did not select COMMAND'
        m.stop_at(ks['kfCursorPutSecond']);m.until(ks['kfCursorPutSecond'],30)
        assert m.read(ks['BANK_PROFILE'],1)==b'\0','second PUT16 left COMMAND'
        m.stop_at(0x8d00)
        m.call('bp_set',addr=ks['MEM_SET_PROFILE_ENTRY'],type='exec')
        for _ in range(30):
            state=m.call('cpu_state');pc=state['registers']['pc']['dec']
            assert pc!=ks['MEM_SET_PROFILE_ENTRY'],'profile copied again after second PUT16'
            if pc==0x8d00:break
            m.call('emu_run_for',ms=1000)
        else:raise AssertionError('cursor profile probe did not return')
        select(page)
        assert m.read(0x3004,2)==word(256) and m.read(buffer,256)==payload[:256]
        rows.append(dict(case='cursor-PUT16-COMMAND-profile',first=0,second=0,noSecondRestore=True))
        print('FILESTREAM COMMAND PUT PROBE PASS',flush=True)
