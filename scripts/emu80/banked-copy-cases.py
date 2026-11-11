"""COPY/MOVE с обратным подтверждением; часть собственного стенда kernel."""
import subprocess


def check_copies(root, out, m, ks, bios, select, invoke, request, regs, rows):
    word = lambda n: n.to_bytes(2,'little')
    size = ks['FILE_COPY_REQUEST']
    f = lambda name: ks['FILE_COPY_REQUEST.'+name]
    short = lambda name: name.partition('.')[0].encode().ljust(8)+name.partition('.')[2].encode().ljust(3)
    def path(page, name, drive="B"):
        select(page);data = (drive+':/'+name).encode()
        m.write(0x3200,data+bytes(128-len(data)))
    def save(page, name, data, drive="B"):
        path(page,name,drive)
        m.write(0x3100,word(0x3200)+word(0x5000)+word(len(data))+bytes(34))
        m.write(0x5000,data)
        request(page,ks['MXOS_DOS_FILE_SAVE_PATH'],regs(hl=0x3100))
    def read(page, name, drive="B"):
        path(page,name,drive)
        m.write(0x3100,word(0x3200)+word(0x5000)+word(8192)+bytes(34))
        request(page,ks['MXOS_DOS_FILE_LOAD_PATH'],regs(hl=0x3100))
        return m.read(0x5000,int.from_bytes(m.read(0x3106,2),'little'))
    def operation(page, src, dst, answer=1, hook=True, move=False, error=0, cancel=False, buffer=0x6000, destination=1):
        select(page)
        packet=bytearray(size)
        packet[f('buffer'):f('buffer')+2]=word(buffer)
        packet[f('bufferSize'):f('bufferSize')+2]=word(5000 if buffer else 0)
        packet[f('sourceDrive')]=1;packet[f('destinationDrive')]=destination
        packet[f('source'):f('source')+11]=short(src)
        packet[f('destination'):f('destination')+11]=short(dst)
        packet[f('onExists'):f('onExists')+2]=word(0x3800 if hook else 0)
        m.write(0x3300,packet+bytes([0xA5])*16)
        m.write(0x36EF,bytes([0,answer,int(cancel)]))
        if buffer:m.write(buffer,bytes([0xA5])*5000)
        if page == 0 and not move:
            result=invoke(page,ks['BANK_DOS_GATE'],regs(hl=0x3300),legacy=bios['bios_fileCopy'])
            assert result==(error,bool(error)),result
        else:
            request(page,ks['MXOS_DOS_CLI'],regs(hl=0x3300,a=7 if move else 1),expected=error)
        result=m.read(0x3300,size)
        assert result[:6]==packet[:6]
        assert result[f('onExists'):f('onExists')+2]==packet[f('onExists'):f('onExists')+2]
        assert m.read(0x3300+size,16)==bytes([0xA5])*16
        if buffer:assert m.read(buffer,5000)==bytes([0xA5])*5000
        count=m.read(0x36EF,1)[0]
        if count:
            seen=m.read(0x3600,size)
            assert seen[:6]==packet[:6]
            assert seen[f('onExists'):f('onExists')+2]==word(0x3800)
        return count

    source=out/'copy-callback.asm'
    source.write_text(f''' INCLUDE "{(root/'source/include/mxos.inc').as_posix()}"
 ORG 03800h
 ld de,03600h
 ld b,{size}
loop:
 ld a,(hl)
 ld (de),a
 inc hl
 inc de
 dec b
 jp nz,loop
 ld hl,036EFh
 inc (hl)
 ld c,'.'
 MXOS_CALL MXOS_BIOS_PRINT_CHAR
 ld a,(036F1h)
 or a
 jp z,answer
 scf
 ret
answer:
 ld a,(036F0h)
 or a
 ret
''',encoding='utf-8')
    subprocess.run([str(root/'tasm/sjasmplus.exe'),'--i8080','--raw='+str(out/'copy-callback.bin'),str(source)],check=True,capture_output=True)
    payload=bytes((i*13+i//251)&255 for i in range(6001))
    for page in (0,3):
        select(page);m.write(0x3800,(out/'copy-callback.bin').read_bytes())
        save(page,'SRC.BIN',payload);save(page,'DST.BIN',b'old')
        assert operation(page,'SRC.BIN','DST.BIN',hook=False,error=2)==0
        assert operation(page,'SRC.BIN','DST.BIN',answer=0)==1
        assert read(page,'DST.BIN')==b'old'
        assert operation(page,'SRC.BIN','DST.BIN',cancel=True,error=1)==1
        assert read(page,'DST.BIN')==b'old'
        assert operation(page,'SRC.BIN','DST.BIN',answer=1)==1
        assert read(page,'DST.BIN')==payload
        assert operation(page,'DST.BIN','MOVE'+str(page)+'.BIN',move=True,buffer=0)==0
        assert read(page,'MOVE'+str(page)+'.BIN')==payload
        rows.append(dict(case='COPY-No-Yes-Cancel-and-MOVE-no-buffer',caller=page,bytes=len(payload)))

    # При слиянии дерева Yes/No to all применяется ко всем дочерним файлам.
    for name in ('SRC_TREE','DST_TREE'):
        path(3,name);request(3,ks['MXOS_DOS_CLI'],regs(hl=0x3200,a=4))
    for leaf in ('A.BIN','B.BIN'):
        save(3,'SRC_TREE/'+leaf,b'new-'+leaf.encode())
        save(3,'DST_TREE/'+leaf,b'old-'+leaf.encode())
    assert operation(3,'SRC_TREE','DST_TREE',answer=3)==1
    for leaf in ('A.BIN','B.BIN'):assert read(3,'DST_TREE/'+leaf)==b'old-'+leaf.encode()
    assert operation(3,'SRC_TREE','DST_TREE',answer=2)==1
    for leaf in ('A.BIN','B.BIN'):assert read(3,'DST_TREE/'+leaf)==b'new-'+leaf.encode()
    rows.append(dict(case='tree-copy-NoAll-YesAll-single-callback'))
    save(3,'ACROSS.BIN',b'new-across')
    save(3,'BNKCOPY.BIN',b'old-across',drive='H')
    assert operation(3,'ACROSS.BIN','BNKCOPY.BIN',move=True,answer=0,destination=7)==1
    assert read(3,'ACROSS.BIN')==b'new-across'
    assert read(3,'BNKCOPY.BIN',drive='H')==b'old-across'
    assert operation(3,'ACROSS.BIN','BNKCOPY.BIN',move=True,answer=1,destination=7)==1
    assert read(3,'BNKCOPY.BIN',drive='H')==b'new-across'
    path(3,'ACROSS.BIN')
    m.write(0x3100,word(0x3200)+word(0x5000)+word(8192)+bytes(34))
    request(3,ks['MXOS_DOS_FILE_LOAD_PATH'],regs(hl=0x3100),expected=2)
    rows.append(dict(case='cross-drive-MOVE-No-keeps-source-Yes-deletes'))
    for buffer, capacity, hook in ((0xF7FF,256,0), (0x3301,512,0), (0x6000,255,0), (0x6000,512,0xF800)):
        select(3);packet=bytearray(size)
        packet[:4]=word(buffer)+word(capacity)
        packet[f('onExists'):f('onExists')+2]=word(hook)
        m.write(0x3300,packet)
        request(3,ks['MXOS_DOS_FILE_COPY'],regs(hl=0x3300),expected=1)
        assert m.read(0x3300,size)==packet
    rows.append(dict(case='COPY-invalid-buffer-overlap-capacity-callback'))
    print('BANK DOS COPY CALLBACK PASS',flush=True)
