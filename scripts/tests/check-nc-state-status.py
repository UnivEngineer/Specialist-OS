"""8080 regressions: F3 state, hidden root entry, status line and late FAT scan."""
from pathlib import Path
import re
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/nc-state-status-cpu'
W = lambda x: int(x).to_bytes(2, 'little')


def file_info(name, extension, size=0, attrib=0):
    return name.ljust(8) + extension + bytes([attrib]) + size.to_bytes(4, 'little')


def sy(path):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(encoding='utf-8-sig'),re.M)}


def main():
    OUT.mkdir(exist_ok=True)
    n,b = sy(ROOT/'bin/NC.sym'),sy(ROOT/'bin/BIOS.sym')
    trace_tools=runpy.run_path(str(ROOT/'scripts/tests/benchmark-nc-color-scroll.py'))
    executable=runpy.run_path(str(ROOT/'scripts/images/executable.py'))
    asm = f'''
    ORG 8200h
readCatalog:
    xor a
    push af                 ; lf_ReadCatalog снимает сохранённый статус диска.
    jp {n['lf_ReadCatalog']}
gateway:
    ld (requestedOp),a
    ex (sp),hl
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ex (sp),hl
    ld a,e
    cp {n['MXOS_DOS_GET_DRIVE_FREE_SPACE']&255}
    jp z,slowQuery
    cp {n['MXOS_DOS_MEMORY']&255}
    jp z,memoryQuery
    cp {n['MXOS_DOS_DIRECTORY']&255}
    jp z,directoryQuery
    cp {n['MXOS_DOS_FILE_GET_SET_DRIVE']&255}
    jp z,getDrive
    cp {n['MXOS_DOS_FILE_SELECT_DRIVE']&255}
    jp z,restoreDrive
    cp {n['MXOS_DOS_FILEHANDLE']&255}
    jp z,statusHandle
    xor a
    ret
slowQuery:
    ld a,1
    ld (didFree),a
    ld de,70
    xor a
    ret
memoryQuery:
    ld b,4
    xor a
    ret
directoryQuery:
    ld a,(requestedOp)
    cp {n['DIR_OP_GET']}
    jp z,contextQuery
    ld a,b
    ld (listSkip+1),a
    ld a,c
    ld (listSkip),a
    ld hl,(listTotal)
    ex de,hl
    xor a
    ret
contextQuery:
    ld hl,(contextDir)
    ex de,hl
    xor a
    ret
getDrive:
    ld a,(contextDrive)
    ret
restoreDrive:
    ld a,(requestedOp)
    ld (contextDrive),a
    xor a
    ret
statusHandle:
    ld a,(requestedOp)
    cp {n['FILE_OP_OPEN_AT']}
    jp z,statusOpen
    cp {n['FILE_OP_READ']}
    jp z,statusRead
    cp {n['FILE_OP_CLOSE']}
    scf
    ret nz
    ld hl,statusClosed
    inc (hl)
    xor a
    ret
statusOpen:
    ld a,({n['flHandle']+n['FH_DRIVE']})
    ld (contextDrive),a
    ld (statusOpenedDrive),a
    ld hl,({n['flHandle']+n['FH_SEEK']})
    ld (statusOpenedParent),hl
    ld hl,12345
    ld ({n['flHandle']+n['FILE_DESCRIPTOR.size']}),hl
    xor a
    ret
statusRead:
    ld hl,({n['flHandle']+n['FH_COUNT']})
    ld (statusRequested),hl
    ld hl,({n['flHandle']+n['FH_BUFFER']})
    ex de,hl
    ld hl,8740h
    ld b,{n['IMAGE_HEADER_SIZE']}
statusReadByte:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,statusReadByte
    xor a
    ret
cursorDrawn:
    ld hl,cursorCalls
    inc (hl)
    ret
driver:
    ld a,b
    cp 3
    jp nz,driverName
    ld hl,8600h
    ld de,2048
    xor a
    ret
driverName:
    ld hl,8640h
    ld b,{n['DISK_TECH_FLASH']}
    xor a
    ret
getMemTop:
    ld hl,8BFFh
    ret
getSelected:
    ld hl,8000h
    ret
stopCount:
    ld (visibleCount),a
    ret
filesDrawn:
    ld hl,didFiles
    inc (hl)
    ret
panelDrawn:
    ld hl,panelCalls
    inc (hl)
    ld a,c
    ld (lastPanel),a
    ret
didFree: DB 0
didFiles: DB 0
panelCalls: DB 0
lastPanel: DB 0
listSkip: DW 0
listTotal: DW 3
visibleCount: DB 0
requestedOp: DB 0
contextDrive: DB 7
contextDir: DW 0
cursorCalls: DB 0
statusOpenedDrive: DB 0
statusOpenedParent: DW 0
statusRequested: DW 0
statusClosed: DB 0
    ASSERT $ < 8500h
'''
    (OUT/'fixture.asm').write_text(asm,encoding='utf-8')
    subprocess.run([str(ROOT/'tasm/sjasmplus.exe'),'--i8080','-Wno-rdlow',
                    '--raw='+str(OUT/'fixture.bin'),'--sym='+str(OUT/'fixture.sym'),
                    str(OUT/'fixture.asm')],check=True,capture_output=True)
    s=sy(OUT/'fixture.sym')

    def call(m,entry,a=0,bc=0,de=0,hl=0,stop=0x81ff):
        m=bytearray(m)
        code=b'\x31'+W(0x8b00)+b'\x3e'+bytes([a])+b'\x01'+W(bc)+b'\x11'+W(de)+b'\x21'+W(hl)+b'\xcd'+W(n.get(entry,b.get(entry,s.get(entry))))+b'\xc3'+W(0x81ff)
        m[0x8100:0x8100+len(code)]=code
        (OUT/'input.bin').write_bytes(m)
        subprocess.run(['D:/Projects/RetroPC/Specialist/MySoft/z88dk/bin/z88dk-ticks.exe',
                        '-m8080','-pc','8100','-end',f'{stop:x}','-counter','30000000',
                        '-output',str(OUT/'output.bin'),str(OUT/'input.bin')],check=True,capture_output=True,timeout=20)
        data=(OUT/'output.bin').read_bytes()
        assert data[65542:65544]==W(stop),(entry,data[65542:65544].hex())
        return bytearray(data[:65536])

    def patch(m,entry,target):
        m[n[entry]:n[entry]+3]=b'\xc3'+W(s[target])

    def seed(font):
        m=bytearray(65536)
        for at,path in [(0,'NC.i80'),(0xc000,'BIOS.sys'),(0xf000,'IO.main.bin'),(0xd800,f'fonts/{font}.i80')]:
            data=(ROOT/'bin'/path).read_bytes();m[at:at+len(data)]=data
        m[b['mon2_vars']:b['mon2_vars']+b['MON2_VARIABLES']]=(ROOT/'bin/BIOS.defaults.bin').read_bytes()[:b['MON2_VARIABLES']]
        m[b['bios_upper.escMode']]=255
        m=call(m,'FONT_ENGINE_INIT');m=call(m,'ncLayoutInit')
        data=(OUT/'fixture.bin').read_bytes();m[0x8200:0x8200+len(data)]=data
        patch(m,'mx_callRegs','gateway');patch(m,'sysDiskDriver','driver')
        patch(m,'mx_getMemTop','getMemTop');patch(m,'getSelectedFile','getSelected')
        m[0x8600+n['DISK_INFO.isValid']]=n['DISK_VALID']
        m[0x8600+n['DISK_INFO.blockSize']:0x8600+n['DISK_INFO.blockSize']+2]=W(256)
        m[0x8600+n['DISK_INFO.clusterBytes']:0x8600+n['DISK_INFO.clusterBytes']+2]=W(512)
        m[0x8600+n['DISK_INFO.volumeLabel']:0x8600+n['DISK_INFO.volumeLabel']+12]=b'SYSTEM     \0'
        driver=b'AT512KS\0' + b'512K ONBOARD FLASH\0'
        m[0x8640:0x8640+len(driver)]=driver
        return m

    for font in ('Cpm4','Compact','Fast8'):
        m=seed(font)
        prompt=bytearray(m)
        prompt[n['mon2_vars.cursorY']:n['mon2_vars.cursorY']+2]=W(50)
        newline=call(prompt,'mon2_printCharOld',bc=10)
        banner=call(prompt,'ncStartupCopyright')
        assert banner[n['mon2_vars.cursorY']:n['mon2_vars.cursorY']+2]==newline[n['mon2_vars.cursorY']:n['mon2_vars.cursorY']+2],(font,'extra newline before copyright')
        koi7=call(m,'FONT_ENGINE_ENCODING',a=1)
        assert koi7[n['bios_upper.koi7']]!=0
        restored=call(koi7,'Start',stop=n['ncLayoutInit'])
        baseline=call(m,'Start',stop=n['ncLayoutInit'])
        assert restored[n['bios_upper.koi7']]==0
        # Both paths print the banner, including renderer SMC, but only one
        # starts in KOI7. BIOS cursor/scratch/stack lie above the font window.
        assert restored[0xd800:0xec80]==baseline[0xd800:0xec80],(font,'startup did not restore physical KOI-8 font layout')
        for panel in (0,1):
            value_x=28 if font=='Fast8' else 32
            label_end=26 if font=='Fast8' else 28
            numeric_end=6 if font=='Fast8' else 7
            q=bytearray(m);q[n['state.activePanel']]=panel
            fast=call(q,'printInfoContents',stop=s['slowQuery'])
            final=call(q,'printInfoContents')
            changed=[i for i,(x,y) in enumerate(zip(fast[0x9000:0xc000],final[0x9000:0xc000])) if x!=y]
            assert changed and all(1+24*panel<=i//256<numeric_end+24*panel and 123<=i%256<=130 for i in changed), (font,panel,'late update rewrote other text')
            assert final[s['didFree']]==1
            total=call(q,'pip_printNumberOnly',a=120,de=512)
            for col in range(1+24*panel,numeric_end+24*panel):
                start=0x9000+col*256+113
                assert total[start:start+8]==fast[start:start+8],(font,panel,'disk capacity')
            label=bytearray(q)
            label[n['mon2_vars.cursorY']:n['mon2_vars.cursorY']+2]=bytes([46,value_x+96*panel])
            label=call(label,'FONT_ENGINE_RAWZ',hl=n['aMainRAM'])
            for x in range(value_x//4+24*panel,23+24*panel):
                assert fast[0x9000+x*256+39:0x9000+x*256+47]==label[0x9000+x*256+39:0x9000+x*256+47], (font,'memory label')
            # Independently place each label/value: common left value edge,
            # right-aligned labels, no colon, and no label padding over values.
            char_width={'Cpm4':2,'Compact':3,'Fast8':4}[font]
            assert fast[n['pipDetailX']]==value_x
            rows = [(160,b'Label',b'SYSTEM'),(170,b'Driver',b'AT512KS'),(180,b'Type',b'Flash'),
                    (190,b'Info',b'512K ONBOARD FLASH')]
            for y,label,value in rows:
                value = value[:(92-value_x)//char_width]
                expected=bytearray(m)
                expected[0x8500:0x8500+len(label)+1]=label+b'\0'
                expected[n['mon2_vars.cursorY']:n['mon2_vars.cursorY']+2]=bytes([y,label_end-len(label)*char_width+96*panel])
                expected=call(expected,'FONT_ENGINE_RAWZ',hl=0x8500)
                expected[0x8500:0x8500+len(value)+1]=value+b'\0'
                expected[n['mon2_vars.cursorY']:n['mon2_vars.cursorY']+2]=bytes([y,value_x+96*panel])
                expected=call(expected,'FONT_ENGINE_RAWZ',hl=0x8500)
                for x in range(24*panel,23+24*panel):
                    assert fast[0x9000+x*256+y-7:0x9000+x*256+y+1]==expected[0x9000+x*256+y-7:0x9000+x*256+y+1],(font,panel,y,'metadata column alignment')
            # Same-drive Enter/CD redraws only the file panel. A drive change
            # must still refresh the opposite information panel.
            for displayed_drive,calls in [(0,1),(7,2)]:
                nav=bytearray(m);nav[n['state.activePanel']]=panel
                nav[n['state.panelA_drive']+panel]=0
                nav[n['state.panelA_info']+(panel^1)]=1
                nav[n['ncInfoDrive']]=displayed_drive
                nav[n['mainPrepareReload']]=0xc9
                patch(nav,'loadAndPrintPanel','panelDrawn')
                nav=call(nav,'mainReloadActive',stop=n['mainPanelsReady'])
                assert nav[s['panelCalls']]==calls,(font,panel,'same-drive info refresh')
            # Deferred mode must render immediate data without calling FAT.
            q[n['ncInfoFreeDeferred']]=1
            deferred=call(q,'printInfoContents')
            assert deferred[s['didFree']]==0
            # Full redraw must finish both file-list phases before the request.
            deferred=bytearray(m);deferred[n['state.activePanel']]=panel
            deferred[n['state.panelA_info']+(panel^1)]=1
            for entry in ('fullPanelLoad','updateCommandPath','printPanelPath','printInfoLine'):
                deferred[n[entry]]=0xc9
            patch(deferred,'fullPanelFiles','filesDrawn')
            before_scan=call(deferred,'fullRedraw',stop=s['slowQuery'])
            assert before_scan[s['didFiles']]==2 and before_scan[n['ncInfoFreeDeferred']]==0
            # Status navigation clears old size; ordinary files show only a leaf.
            q=bytearray(m);q[n['state.activePanel']]=panel;q[n['state.panelA_filesCnt']+panel]=1
            for name,text,suffix,isdir in [
                    (b'..      ',b'Go to parent dir',b'',True),
                    (bytes([92])+b'       ',b'Go to drive root',b'',True),
                    (b'GAMES   ',b'GAMES',b'<DIR>',True),
                    (b'FILE    ',b'FILE.TXT',b'42',False)]:
                q[0x8000:0x8010]=file_info(name,b'   ' if isdir else b'TXT',42,16 if isdir else 0)
                result=call(q,'printInfoLine')
                width=m[n['ncStatusChars']]
                assert width=={'Cpm4':43,'Compact':29,'Fast8':21}[font],(font,'right margin')
                line=text.ljust(width-len(suffix))+suffix
                assert len(line)==width
                expected=bytearray(q)
                expected[n['mon2_vars.cursorY']:n['mon2_vars.cursorY']+2]=W((n['P_FILE_NAME_X']+panel*96)*256+n['P_FILE_NAME_Y'])
                expected[0x8700:0x8700+width]=line
                expected=call(expected,'FONT_ENGINE_LIM',hl=0x8700,bc=width)
                assert result[0x9000:0xc000]==expected[0x9000:0xc000],(font,panel,name,'status text')
                q=result
            for size, suffix in [(0,b'0'),(65535,b'65535'),(65536,b'64K'),(443802,b'433K'),
                                 (10000*1024-1,b'9999K'),(10000*1024,b'9M'),(0xffffffff,b'4095M')]:
                q[0x8000:0x8010]=file_info(b'FILE',b'TXT',size)
                result=call(q,'printInfoLine')
                start=n['ncTextBuffer']
                assert result[start:start+width]==b'FILE.TXT'.ljust(width-len(suffix))+suffix,(font,panel,size)
                if font=='Fast8':
                    detailed=call(q,'printFileNamOnPanel',hl=0x8000)
                    assert detailed[start:start+18]==b'FILE     TXT'+suffix.rjust(5)+b'\0',(panel,size)
            for stem,flags,mode,extension,full,short,bad in [
                    (b'PLATFORM',4,0,b'COM',b'MXOS',b'MX',False),
                    (b'P',0,1,b'EXE',b'MXOS',b'MX',False),
                    (b'STD',0,2,b'exe',b'STD',b'ST',False),
                    (b'MONITOR',0,3,b'EXE',b'MON2',b'M2',False),
                    (b'LEGACY',0,4,b'EXE',b'RAMFOS',b'RF',False),
                    (b'AUTO',0,0,b'EXE',b'',b'',False),
                    (b'BAD',0,4,b'EXE',b'',b'',True)]:
                q=bytearray(m);q[n['state.activePanel']]=panel;q[n['state.panelA_filesCnt']+panel]=1
                q[n['state.panelA_drive']+panel]=panel+2
                q[n['state.panelA_dirLow']+panel]=0x34
                q[n['state.panelA_dirHigh']+panel]=0x12
                q[0x8000:0x8010]=file_info(stem,extension,12345)
                image=executable['Image'](bytes(12345-n['IMAGE_HEADER_SIZE']),flags=flags,mode=mode)
                header=bytearray(executable['encode'](image,'exe')[:n['IMAGE_HEADER_SIZE']])
                if bad:
                    header[-1]^=1
                q[0x8740:0x8740+len(header)]=header
                result=call(q,'printInfoLine')
                width=m[n['ncStatusChars']]
                line=result[n['ncTextBuffer']:n['ncTextBuffer']+width]
                leaf=stem+b'.'+extension
                assert line==leaf.ljust(width-5)+b'12345',(font,panel,line)
                assert result[s['statusClosed']]==0 and result[n['ncStatusPending']]==1
                quiet=n['ncStatusQuiet']
                result[quiet:quiet+2]=W(1)
                # Ожидание клавиши в диалоге не должно обслуживать статус.
                result=call(result,'ncStatusIdle')
                assert result[quiet:quiet+2]==W(1) and result[s['statusClosed']]==0
                result[n['ncStatusInput']]=1
                result[n['IO_KEYB_B']]=0xfb
                result=call(result,'ncStatusIdle')
                assert result[quiet:quiet+2]==W(n['NC_STATUS_QUIET_POLLS'])
                assert result[s['statusClosed']]==0
                result[n['IO_KEYB_B']]=0xff
                result[quiet:quiet+2]=W(2)
                waiting=call(result,'ncStatusIdle')
                assert waiting[quiet:quiet+2]==W(1) and waiting[s['statusClosed']]==0
                position=n['mon2_vars.cursorY']
                inverse=n['mon2_vars.inverse']
                waiting[position:position+2]=W(0x16f2)
                waiting[inverse:inverse+2]=W(1)
                waiting[n['ncCurrentColor']]=0x71
                result=call(waiting,'ncStatusIdle')
                assert result[position:position+2]==W(0x16f2)
                assert result[inverse:inverse+2]==W(1) and result[n['ncCurrentColor']]==0x71
                platform=short if font=='Fast8' else full
                expected=leaf
                if platform:
                    expected=expected.ljust(13 if font=='Fast8' else 14)+platform
                expected=expected.ljust(width-5)+b'12345'
                reference=bytearray(q)
                reference[position:position+2]=W((n['P_FILE_NAME_X']+panel*96)*256+n['P_FILE_NAME_Y'])
                reference[0x8700:0x8700+width]=expected
                reference=call(reference,'FONT_ENGINE_LIM',hl=0x8700,bc=width)
                assert result[0x9000:0xc000]==reference[0x9000:0xc000],(font,panel,expected)
                assert result[s['statusOpenedDrive']]==panel+2
                assert result[s['statusOpenedParent']:s['statusOpenedParent']+2]==W(0x1234)
                assert result[s['statusRequested']:s['statusRequested']+2]==W(n['IMAGE_HEADER_SIZE'])
                assert result[s['statusClosed']]==1 and result[s['contextDrive']]==7
                assert result[n['ncStatusPending']]==0
                assert call(result,'ncStatusIdle')[s['statusClosed']]==1
            # Смена выбора на обычный файл отменяет запланированное чтение.
            q=call(q,'printInfoLine')
            q[0x8000:0x8010]=file_info(b'FILE',b'TXT',42)
            q=call(q,'printInfoLine')
            q[n['ncStatusInput']]=1
            q[n['IO_KEYB_B']]=0xff
            assert q[n['ncStatusPending']]==0
            assert call(q,'ncStatusIdle')[s['statusClosed']]==0
        print(font,'fast information, numeric-only late update, Type/description and status PASS')
        # Repainting an unchanged prompt must never erase its pixels first.
        q=bytearray(m)
        q[n['vars.path']:n['vars.path']+4]=b'A:\\\0'
        q[n['vars.cmdLineEnd']:n['vars.cmdLineEnd']+2]=W(n['vars.cmdLine'])
        q=call(q,'redrawCommandLine')
        prefix_bytes=q[n['vars.cmdTextPos']+1]//4
        seen=[]
        def watch_prefix(addr,value,old):
            if 0x9000<=addr<0x9000+prefix_bytes*256 and 236<=addr%256<=243:
                assert value==old,(font,'prompt temporarily erased')
                seen.append(addr)
        attrs=bytearray([0x70])*12288
        final,_,trace,initial=trace_tools['execute'](q,n['clearCmdLine'],OUT,trace=True)
        trace_tools['replay_trace'](trace,initial,attrs,final,attrs,write_observer=watch_prefix)
        assert seen,'prompt repaint was not exercised'
        # First typed character and entry into Lineedit never write the prefix.
        q[n['ncInputHookBegin']]=q[n['ncInputHookEnd']]=0xc9
        q[n['mon2_getchOld']:n['mon2_getchOld']+3]=b'\xc3'+W(0x8500)
        reader=bytes.fromhex('3a 00 88 3c 32 00 88 fe 02 ca')+W(trace_tools['TRACE_STOP'])+bytes.fromhex('3e 61 b7 c9')
        q[0x8500:0x8500+len(reader)]=reader
        final,_,trace,initial=trace_tools['execute'](q,n['mainLoop_cursorReady'],OUT,trace=True)
        seen.clear()
        trace_tools['replay_trace'](trace,initial,attrs,final,attrs,write_observer=watch_prefix)
        assert not seen
        # Ctrl+Enter appends the selected absolute path by repainting text only.
        q[n['vars.pathValidA']]=1
        q[n['vars.pathA']:n['vars.pathA']+4]=b'A:\\\0'
        q[0x8000:0x8010]=file_info(b'FILE',b'TXT',42)
        trampoline=b'\x21'+W(0x8000)+b'\xcd'+W(n['selFileToCmdLine'])+b'\xc9'
        q[0x8500:0x8500+len(trampoline)]=trampoline
        final,_,trace,initial=trace_tools['execute'](q,0x8500,OUT,trace=True)
        seen.clear()
        trace_tools['replay_trace'](trace,initial,attrs,final,attrs,write_observer=watch_prefix)
        assert not seen and final[n['vars.cmdLine']:n['vars.cmdLine']+12].split(b'\0')[0]==b'A:\\FILE.TXT'
        print(font,'KOI7 -> KOI8 before layout; prompt stable on first key and Ctrl+Enter PASS')
        if font=='Fast8':
            q=bytearray(m)
            label=b'ABCDEFGHIJK\0'
            at=0x8600+n['DISK_INFO.volumeLabel'];q[at:at+12]=label
            driver=b'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789\0'+b'LONG FLASH DESCRIPTION\0'
            q[0x8640:0x8640+len(driver)]=driver
            fast=call(q,'printInfoContents',stop=s['slowQuery'])
            assert fast[n['pipRows']]==4 and fast[n['mon2_vars.cursorY']]==219,'fixed fields must never wrap'
            for y, prefix in [(170,b'ABCDEFGHIJKLMNOP\0'),(190,b'LONG FLASH DESCR\0')]:
                expected=bytearray(m)
                expected[0x8500:0x8500+len(prefix)]=prefix
                expected[n['mon2_vars.cursorY']:n['mon2_vars.cursorY']+2]=bytes([y,28])
                expected=call(expected,'FONT_ENGINE_RAWZ',hl=0x8500)
                for col in range(7,24):
                    start=0x9000+col*256+y-7
                    assert fast[start:start+8]==expected[start:start+8],(y,'value must stop at 16 characters in 8px')
    m=seed('Compact')
    for panel in (0,1):
        for truncated in (0,1):
            q=bytearray(m)
            q[n['state.activePanel']]=panel
            q[n['vars.panelFrameA']+panel]=truncated
            final=call(q,'printInfoEmpty')
            expected=0xc1 if truncated else 0xf1
            assert final[n['ncCurrentColor']]==expected,(panel,truncated)
    # Compare the actual returned context, including both cluster bytes, before
    # updating panel state. CLI's "context command" flag alone is insufficient.
    for panel in (0,1):
        for old_drive,new_drive,old_dir,new_dir,changed in [
            (7,7,0,0,False),(7,7,0x1201,0x1201,False),
            (7,7,0x1201,0,True),(7,7,0x1201,0x1301,True),(7,0,0,0,True)]:
            q=bytearray(m);q[n['state.activePanel']]=panel
            q[n['state.panelA_drive']+panel]=old_drive
            q[n['state.panelA_dirLow']+panel]=old_dir&255
            q[n['state.panelA_dirHigh']+panel]=old_dir>>8
            q[s['contextDrive']]=new_drive;q[s['contextDir']:s['contextDir']+2]=W(new_dir)
            q[n['ncCommandContext']]=4
            result=call(q,'ncCommandUpdateContext')
            assert result[n['ncCommandContext']]==4*changed,(panel,old_drive,new_drive,old_dir,new_dir)
            assert result[n['state.panelA_drive']+panel]==new_drive
            assert result[n['state.panelA_dirLow']+panel]==new_dir&255
            assert result[n['state.panelA_dirHigh']+panel]==new_dir>>8
        for changed in (False,True):
            q=bytearray(m);q[n['ncCommandContext']]=4*changed
            q[n['state.activePanel']]=panel
            q[n['ncCommandScreen']]=0;q[n['ncCommandCursorKept']]=1
            patch(q,'drawCursor','cursorDrawn')
            stop=n['directoryChanged'] if changed else n['ncRefreshDirty']
            result=call(q,'ncCommandContextReady',stop=stop)
            assert result[s['cursorCalls']]==changed
            assert result[n['ncCommandCursorKept']]==1-changed
    print('Same drive/directory commands preserve the panel and highlight; real context changes reload PASS')
    for panel in (0,1):
        for was_info in (0,1):
            q=bytearray(m)
            q[n['state.activePanel']]=panel
            q[n['state.panelA_info']+(panel^1)]=was_info
            q[n['ncShiftState']]=2
            patch(q,'drawCursor','cursorDrawn')
            patch(q,'loadAndPrintPanel','panelDrawn')
            result=call(q,'mainLoop_dispatchKey',a=2,stop=n['mainLoop_cursorReady'])
            assert result[s['cursorCalls']]==0,(panel,was_info,'selection repainted')
            assert result[s['panelCalls']]==1 and result[s['lastPanel']]==(panel^1)
            assert result[n['state.activePanel']]==panel
            assert result[n['state.panelA_info']+(panel^1)]==1-was_info
    print('F3 toggles only the opposite panel, preserving the file selection PASS')
    for active,flags in [(0,(0,1)),(1,(1,0)),(0,(1,1)),(3,(1,1))]:
        q=bytearray(m)
        q[n['state.activePanel']]=active&1
        q[n['state.panelA_info']:n['state.panelA_info']+2]=bytes(flags)
        final=call(q,'ncStateValidate')
        want=list(flags);want[active&1]=0
        assert final[n['state.panelA_info']:n['state.panelA_info']+2]==bytes(want)
    for directory in (0,1,256):
        q=bytearray(m);q[n['vars.cacheSlotA']]=0
        q[n['state.panelA_dirLow']:n['state.panelA_dirLow']+1]=bytes([directory&255])
        q[n['state.panelA_dirHigh']]=directory>>8
        patch(q,'lf_StoreCount','stopCount')
        final=call(q,'readCatalog')
        assert final[s['listSkip']:s['listSkip']+2]==W(bool(directory))
        assert final[s['visibleCount']]==3-bool(directory)
    print('F3 state restores either panel; subdirectory listing skips only FAT dot PASS')
    for skip, capacity, total, old, overflow in [
        (0, 64, 64, 0, 0), (0, 64, 65, 0, 1),
        (1, 64, 65, 2, 0), (1, 64, 66, 2, 2),
        (0, 255, 256, 0, 1), (1, 255, 256, 0, 0),
    ]:
        q=bytearray(m)
        q[n['vars.cacheSlotA']]=0
        q[n['state.panelA_dirLow']]=skip
        q[n['ncCatalogCapacity']]=capacity
        q[n['ncCatalogs']+n['CATALOG_OVERFLOW']]=old
        q[s['listTotal']:s['listTotal']+2]=W(total)
        patch(q,'lf_StoreCount','stopCount')
        final=call(q,'readCatalog')
        assert final[s['visibleCount']]==min(total-skip,capacity)
        assert final[n['ncCatalogs']+n['CATALOG_OVERFLOW']]==overflow
        if overflow:
            at=n['ncCatalogTotals']
            assert final[at:at+2]==W(total-skip)
    for shown,total in [(105,271),(255,65535)]:
        q=bytearray(m)
        at=n['ncCatalogTotals']+2
        q[at:at+2]=W(total)
        q[n['ncCatalogCapacity']]=shown
        final=call(q,'ncCatalogFormatWarning',a=1)
        line=f'Displayed {shown} records of {total}'.encode()
        at=n['vars.displayPath']
        assert len(line)<=30 and final[at:at+len(line)+1]==line+b'\0'
    q=bytearray(m)
    slot=n['ncCatalogs']+8
    q[slot:slot+8]=bytes([0,7,0x34,0x12,64,255,0,2])
    q[n['vars.cacheSlotB']]=1
    q[n['state.panelA_dirLow']]=0x34
    q[n['state.panelA_dirHigh']]=0x12
    final=call(q,'ncCatalogAcquire',bc=7)
    assert final[n['vars.cacheSlotA']]==1
    assert final[slot]==0 and final[slot+n['CATALOG_OVERFLOW']]==2
    print('Catalog overflow PASS: exact capacity, 16-bit totals, dot skip, acknowledgment reset')


if __name__=='__main__':
    main()
