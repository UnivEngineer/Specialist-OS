"""8080 regressions: F3 state, hidden root entry, status names and late FAT scan."""
from pathlib import Path
import re
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/nc-state-status-cpu'
W = lambda x: int(x).to_bytes(2, 'little')


def sy(path):
    return {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text(encoding='utf-8-sig'),re.M)}


def main():
    OUT.mkdir(exist_ok=True)
    n,b = sy(ROOT/'bin/NC.sym'),sy(ROOT/'bin/BIOS.sym')
    trace_tools=runpy.run_path(str(ROOT/'scripts/tests/benchmark-nc-color-scroll.py'))
    asm = f'''
    ORG 8200h
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
    ld de,3
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
visibleCount: DB 0
requestedOp: DB 0
contextDrive: DB 7
contextDir: DW 0
cursorCalls: DB 0
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
        for at,path in [(0,'NC.i80'),(0xc000,'BIOS.sys'),(0xf000,'IO.sys'),(0xd800,f'fonts/{font}.i80')]:
            data=(ROOT/'bin'/path).read_bytes();m[at:at+len(data)]=data
        m[b['bios_vars']:b['bios_vars']+b['BIOS_VARIABLES']]=m[b['initVars']:b['initVarsEnd']]
        m[b['bios_upper.escMode']]=255
        m=call(m,'FONT_ENGINE_INIT');m=call(m,'ncLayoutInit')
        data=(OUT/'fixture.bin').read_bytes();m[0x8200:0x8200+len(data)]=data
        patch(m,'mx_callRegs','gateway');patch(m,'bios_diskDriver','driver')
        patch(m,'bios_getMemTop','getMemTop');patch(m,'getSelectedFile','getSelected')
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
        prompt[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=W(50)
        newline=call(prompt,'bios_printChar',bc=10)
        banner=call(prompt,'ncStartupCopyright')
        assert banner[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]==newline[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2],(font,'extra newline before copyright')
        koi7=call(m,'bios_setFontEncoding',a=1)
        assert koi7[n['bios_upper.koi7']]!=0
        restored=call(koi7,'Start',stop=n['ncLayoutInit'])
        baseline=call(m,'Start',stop=n['ncLayoutInit'])
        assert restored[n['bios_upper.koi7']]==0
        # Both paths print the banner, including renderer SMC, but only one
        # starts in KOI7. BIOS cursor/scratch/stack lie above the font window.
        assert restored[0xd800:0xec80]==baseline[0xd800:0xec80],(font,'startup did not restore physical KOI-8 font layout')
        for panel in (0,1):
            q=bytearray(m);q[n['state.activePanel']]=panel
            fast=call(q,'printInfoContents',stop=s['slowQuery'])
            final=call(q,'printInfoContents')
            changed=[i for i,(x,y) in enumerate(zip(fast[0x9000:0xc000],final[0x9000:0xc000])) if x!=y]
            assert changed and all(1+24*panel<=i//256<7+24*panel and 137<=i%256<=144 for i in changed), (font,panel,'late update rewrote other text')
            assert final[s['didFree']]==1
            label=bytearray(q)
            label[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=bytes([50,32+96*panel])
            label=call(label,'bios_printStringReal',hl=n['aMainRAM'])
            for x in range(8+24*panel,23+24*panel):
                assert fast[0x9000+x*256+43:0x9000+x*256+51]==label[0x9000+x*256+43:0x9000+x*256+51], (font,'memory label')
            # Normal metadata has four rows, centered at baselines 182..212.
            q[n['pipDetailX']]=fast[n['pipDetailX']]
            q[n['pipDetailChars']]=fast[n['pipDetailChars']]
            q[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=bytes([202,4+96*panel])
            expected=call(q,'pip_printDetail',hl=n['aDiskTechnology'],de=n['aTypeFlash'])
            expected[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=bytes([212,4+96*panel])
            expected=call(expected,'ncPrintPanelText',hl=0x8648)
            for x in range(1+24*panel,23+24*panel):
                assert fast[0x9000+x*256+195:0x9000+x*256+213]==expected[0x9000+x*256+195:0x9000+x*256+213], (font,'Type/description order')
            # Independently place each label/value: common left value edge,
            # right-aligned labels, no colon, and no label padding over values.
            char_width={'Cpm4':2,'Compact':3,'Fast8':4}[font]
            value_x={'Cpm4':20,'Compact':28,'Fast8':32}[font]
            assert fast[n['pipDetailX']]==value_x
            for y,label,value in [(182,b'Label ',b'SYSTEM'),(192,b'Driver ',b'AT512KS'),(202,b'Type ',b'Flash')]:
                expected=bytearray(m)
                expected[0x8500:0x8500+len(label)+1]=label+b'\0'
                expected[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=bytes([y,value_x-len(label)*char_width+96*panel])
                expected=call(expected,'bios_printStringReal',hl=0x8500)
                expected[0x8500:0x8500+len(value)+1]=value+b'\0'
                expected[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=bytes([y,value_x+96*panel])
                expected=call(expected,'bios_printStringReal',hl=0x8500)
                for x in range(1+24*panel,23+24*panel):
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
            for name,text,isdir in [(b'..      ',b'Go to parent dir',True),(b'\\       ',b'Go to drive root',True),(b'FILE    ',b'FILE.TXT',False)]:
                q[0x8000:0x8010]=name+(b'   ' if isdir else b'TXT')+bytes([16 if isdir else 0,0,0])+W(42)
                result=call(q,'printInfoLine')
                expected=call(q,'printInfoEmpty')
                expected[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=W((n['P_FILE_NAME_X']+panel*96)*256+n['P_FILE_NAME_Y'])
                expected[0x8700:0x8700+len(text)+1]=text+b'\0'
                if isdir:
                    expected=call(expected,'bios_printStringReal',hl=0x8700)
                else:
                    expected=call(expected,'ncPrintField',hl=0x8700,bc=m[n['ncStatusChars']]<<8)
                    expected[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=bytes([n['P_FILE_DATA_Y'],m[n['ncStatusSizeX']]+panel*96])
                    expected=call(expected,'ncPrintFileSize',de=42,bc=32<<8)
                assert result[0x9000:0xc000]==expected[0x9000:0xc000],(font,panel,name,'status text')
                q=result
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
        q[n['bios_getch']:n['bios_getch']+3]=b'\xc3'+W(0x8500)
        reader=bytes.fromhex('3a 00 88 3c 32 00 88 fe 02 ca')+W(trace_tools['TRACE_STOP'])+bytes.fromhex('3e 61 b7 c9')
        q[0x8500:0x8500+len(reader)]=reader
        final,_,trace,initial=trace_tools['execute'](q,n['mainLoop_cursorReady'],OUT,trace=True)
        seen.clear()
        trace_tools['replay_trace'](trace,initial,attrs,final,attrs,write_observer=watch_prefix)
        assert not seen
        # Ctrl+Enter appends the selected absolute path by repainting text only.
        q[n['vars.pathValidA']]=1
        q[n['vars.pathA']:n['vars.pathA']+4]=b'A:\\\0'
        q[0x8000:0x8010]=b'FILE    TXT'+bytes(3)+W(42)
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
            assert fast[n['pipRows']]==4 and fast[n['bios_vars.cursorY']]==212,'fixed fields must never wrap'
            expected=bytearray(m);prefix=b'ABCDEFGHIJKLMNO\0'
            expected[0x8500:0x8500+len(prefix)]=prefix
            expected[n['bios_vars.cursorY']:n['bios_vars.cursorY']+2]=bytes([192,32])
            expected=call(expected,'bios_printStringReal',hl=0x8500)
            for col in range(8,23):
                assert fast[0x9000+col*256+185:0x9000+col*256+193]==expected[0x9000+col*256+185:0x9000+col*256+193],'driver value must stop at 15 characters in 8px'
            # Description alone can wrap; cap its lines before the frame.
            desc=b'DESCRIPTION '*12+b'\0'
            at=0x8640+driver.index(b'\0')+1
            q[at:at+len(desc)]=desc
            fast=call(q,'printInfoContents',stop=s['slowQuery'])
            assert fast[n['pipRows']]==6 and fast[n['bios_vars.cursorY']]==222,'description must stay centered inside the frame'
    m=seed('Compact')
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
            patch(q,'bios_fileGetSetDrive','getDrive')
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
        final=call(q,'lf_ReadCatalog')
        assert final[s['listSkip']:s['listSkip']+2]==W(bool(directory))
        assert final[s['visibleCount']]==3-bool(directory)
    print('F3 state restores either panel; subdirectory listing skips only FAT dot PASS')


if __name__=='__main__':
    main()
