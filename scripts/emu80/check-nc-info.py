"""Owned visible/muted Emu80: F3 text/colors and narrow drive selection."""
from pathlib import Path
import argparse
import runpy
import re
import sys
import time

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT / '.local/nc-info-ui'


def main():
    parser=argparse.ArgumentParser(add_help=False)
    parser.add_argument('--port',type=int,choices=range(19781,19785),default=19781)
    parser.add_argument('--runtime',type=Path,default=ROOT/'.local/nc-sort-benchmark-runtime')
    parser.add_argument('--startup-font',choices=('Cpm4','Compact','Fast8'))
    args,_=parser.parse_known_args()
    runtime=args.runtime.resolve()
    OUT.mkdir(parents=True, exist_ok=True)
    helpers = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
    def owner():
        return helpers['shell'](f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
    helpers['mcp_owner']=owner
    helpers['Matrix'].__init__.__globals__.update(RUNTIME=runtime, OUT=OUT,mcp_owner=owner)
    machine = helpers['Matrix']()
    machine.m.url=f'http://127.0.0.1:{args.port}/mcp'
    n = {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
         (ROOT / 'bin/NC.sym').read_text(encoding='utf-8'), re.M)}
    expected_nc = (ROOT / 'bin/NC.i80').read_bytes()[:32]
    try:
        if '--no-build' in sys.argv:
            if helpers['mcp_owner']():
                raise RuntimeError('MCP port belongs to another emulator')
            helpers['command'](['powershell.exe', '-NoProfile', '-File',
                ROOT / 'scripts/emu80/run.ps1', '-EmulatorDirectory', runtime,
                '-Flash', '-Mcp', '-Mute'] + ([] if '--keep-system' in sys.argv else ['-UpdateSystemFlash']))
            pid = helpers['shell']("$p=Get-Process Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object { $_.Path -eq " +
                helpers['ps'](runtime / 'Emu80qt-mcp.exe') + " }; $p.Id")
            machine.pid = int(pid)
            for _ in range(30):
                if helpers['mcp_owner']() == str(machine.pid):
                    break
                time.sleep(.2)
            else:
                raise RuntimeError('Owned no-build emulator did not acquire MCP port')
            machine.m.call('debug_break')
            machine.m.stop_at(n['mainLoop'])
            if machine.m.read(0, 32) == expected_nc:
                machine.m.tap('esc')
            for _ in range(120):
                machine.m.until(n['mainLoop'], 120, page=0)
                if machine.m.read(0, 32) == expected_nc:
                    break
                machine.m.call('debug_step', mode='into')
            else:
                raise AssertionError('No-build launch did not load current NC')
            machine.m.call('bp_clear')
            machine.m.call('emu_run_for', ms=500)
            print('NO-BUILD BOOT: current NC loaded from updated system flash', flush=True)
        else:
            custom_system=None
            if '--navigation-settings' in sys.argv:
                # A copied test image changes only NC.CFG, without rebuilding NC.
                data=bytearray((ROOT/'images/SystemRom_emu80_writable.bin').read_bytes())
                word=lambda at:int.from_bytes(data[at:at+2],'little')
                sector=word(11);reserved=word(14);fat=data[16]*word(22)
                root=(reserved+fat)*sector;root_bytes=word(17)*32
                entry=next(at for at in range(root,root+root_bytes,32) if data[at:at+11]==b'NC      CFG')
                config=(b'FileColorUser Yellow txt\r\nFileColorUser LightRed cfg\r\n'
                        b'FileColorUser LightGreen exe com rks mon fnt\r\n'
                        b'FileColorSys DarkGray\r\nFileColorDef LightCyan\r\nDefaultSortMode 1\r\n')
                original_size=int.from_bytes(data[entry+28:entry+32],'little')
                assert len(config)<original_size
                config+=b'#'+b' '*(original_size-len(config)-1)
                cluster=word(entry+26);left=config
                data_start=root+(root_bytes+sector-1)//sector*sector
                while left:
                    assert 2<=cluster<0xfff0
                    at=data_start+(cluster-2)*data[13]*sector;chunk=data[13]*sector
                    data[at:at+min(chunk,len(left))]=left[:chunk];left=left[chunk:]
                    cluster=word(reserved*sector+cluster*2)
                data[entry+28:entry+32]=len(config).to_bytes(4,'little')
                custom_system=OUT/'settings-system.bin';custom_system.write_bytes(data)
            machine.start('nc-info', system=custom_system, disk=ROOT / 'images/FlashDisk_emu80_seed.bin')
        m = machine.m
        if '--boot-only' in sys.argv:
            assert m.read(n['bios_upper.koi7'],1)==b'\x00'
            m.screenshot(OUT/'current-boot.png')
            print('Current NC boot and KOI-8 PASS',flush=True)
            return

        def wait():
            m.call('emu_run_for', ms=2000)

        def wait_nc():
            m.stop_at(n['ncReadKey'])
            if m.read(0,32)==expected_nc:
                m.tap('esc')
            for _ in range(120):
                m.until(n['ncReadKey'],120,page=0)
                if (m.read(0,32)==expected_nc and
                    int.from_bytes(m.read(n['vars.cmdLineEnd'],2),'little')==n['vars.cmdLine'] and
                    m.read(n['ncFontWidth'],1)==m.read(n['FONT_ENGINE_HEADER']+5,1)):
                    break
                m.call('debug_step',mode='into')
            else:
                raise AssertionError('NC return timeout')
            m.call('bp_clear')
            m.call('emu_run_for',ms=500)

        def type_char(char):
            keys = {' ': 'space', ':': 'colon', '/': 'slash', '.': 'period', '-': 'minus'}
            if char == '\\':
                # Specialist's unshifted backslash key produces '|'.
                m.call('kbd_send',keys=['shift'],action='press')
                m.tap('backslash')
                m.call('kbd_send',keys=['shift'],action='release')
                m.call('emu_run_for',ms=120)
            else:
                m.tap(keys.get(char,char.lower()))

        def command(value):
            m.tap('esc')
            m.call('emu_run_for',ms=250)
            for char in value:
                type_char(char)
            actual=m.read(n['vars.cmdLine'],127).split(b'\0')[0]
            assert actual.upper()==value.upper().encode(),(value,actual)
            m.stop_at(n['ncReadKey'])
            m.tap('enter')
            # COPY can take longer than a fixed two-second delay. Wait for the
            # actual NC key loop before typing the next command.
            for _ in range(120):
                m.until(n['ncReadKey'],120,page=0)
                if (m.read(0,32)==expected_nc and
                    int.from_bytes(m.read(n['vars.cmdLineEnd'],2),'little')==n['vars.cmdLine'] and
                    m.read(n['ncFontWidth'],1)==m.read(n['FONT_ENGINE_HEADER']+5,1)):
                    break
                m.call('debug_step',mode='into')
            else:raise AssertionError(('NC command return timeout',value))
            m.call('bp_clear');m.call('emu_run_for',ms=500)

        if '--startup-prepared' in sys.argv:
            cli={key:int(value,16) for key,value in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',
                (ROOT/'bin/CLI.sym').read_text('utf-8'),re.M)}
            def reach(label):
                for _ in range(240):
                    cpu=m.call('cpu_state');pc=cpu['registers']['pc']['dec']
                    if pc==n[label] and m.read(0,32)==expected_nc:return
                    if cpu.get('breaked'):m.call('debug_step',mode='into')
                    m.call('emu_run_for',ms=500)
                raise AssertionError(('startup timeout',label,m.call('cpu_state')))
            for font,info in [('Cpm4',0),('Compact',1),('Fast8',0)]:
                if args.startup_font and font!=args.startup_font:continue
                command('FONT '+font)
                active=m.read(n['state.activePanel'],1)[0]
                other=active^1
                if m.read(n['state.panelA_info']+other,1)[0]!=info:m.tap('f3');wait()
                m.stop_at(cli['cliPrompt']);m.tap('f10');m.until(cli['cliPrompt'],120,page=0)
                m.call('bp_clear');m.call('emu_run_for',ms=500)
                m.tap('esc')
                value='A:/NC.COM'
                for char in value:type_char(char)
                assert m.read(n['CMD_LINE'],len(value)).upper()==value.encode()
                # Breakpoints are installed only after ordinary command editing.
                m.stop_at(n['ncStartupBannerReady']);m.tap('enter');reach('ncStartupBannerReady')
                banner=m.read(0x9000,12288)
                m.screenshot(OUT/f'{font}-startup-copyright.png')
                watched={'lf_ReadCatalog','buildFileIndex','formatCalled'}
                m.call('bp_clear')
                for label in watched:m.call('bp_set',addr=n[label],type='exec')
                reads=[];sorts=[]
                for _ in range(480):
                    m.call('emu_run_for',ms=500)
                    cpu=m.call('cpu_state');pc=cpu['registers']['pc']['dec']
                    if m.read(0,32)==expected_nc:
                        panel=m.read(n['state.activePanel'],1)[0]
                        if pc==n['lf_ReadCatalog']:reads.append(panel)
                        if pc==n['buildFileIndex']:sorts.append(panel)
                        if pc==n['formatCalled']:break
                    if cpu.get('breaked'):m.call('debug_step',mode='into')
                else:raise AssertionError(('preparation timeout',font))
                assert reads==[0,1] and sorts==[0,1],(font,reads,sorts)
                assert m.read(0x9000,12288)==banner,'catalog preparation painted the interface'
                assert m.read(n['vars.pathValidA'],2)==b'\1\1'
                assert m.read(n['ncStartupPrepared'],1)==b'\1'
                assert m.read(n['state.activePanel'],1)[0]==active
                m.screenshot(OUT/f'{font}-startup-prepared.png')
                m.call('bp_clear')
                forbidden={'lf_ReadCatalog','buildFileIndex','panelPathMiss'}
                for label in forbidden|{'ncReadKey'}:m.call('bp_set',addr=n[label],type='exec')
                for _ in range(480):
                    m.call('emu_run_for',ms=500)
                    cpu=m.call('cpu_state');pc=cpu['registers']['pc']['dec']
                    if m.read(0,32)==expected_nc:
                        assert pc not in [n[label] for label in forbidden],(font,'slow preparation during first paint',pc)
                        if pc==n['ncReadKey']:break
                    if cpu.get('breaked'):m.call('debug_step',mode='into')
                else:raise AssertionError(('first paint timeout',font))
                assert m.read(n['ncStartupPrepared'],1)==b'\0'
                assert m.read(n['state.panelA_info']+other,1)[0]==info
                m.call('bp_clear');m.call('emu_run_for',ms=500)
                m.screenshot(OUT/f'{font}-startup-ready.png')
                print(font,'banner preserved through both reads/sorts; first paint has no catalog/path reads or sorting; F3',info,'PASS',flush=True)
            return

        if '--text-state' in sys.argv:
            cli={key:int(value,16) for key,value in re.findall(r'^([\w.]+): EQU 0x([0-9a-fA-F]+)',
                (ROOT/'bin/CLI.sym').read_text('utf-8'),re.M)}
            def records(panel):
                count=m.read(n['state.panelA_filesCnt']+panel,1)[0]
                slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
                base=int.from_bytes(m.read(n['ncCatalogBases']+slot*2,2),'little')
                return [m.read(base+index*16,16) for index in m.read(n['FILE_INDEX_A']+panel*256,count)]
            def leaf(record):
                return record[:8].rstrip()+((b'.'+record[8:11].rstrip()) if record[8:11].strip() else b'')
            def selected(panel):
                cursor=m.read(n['state.panelA_curFile']+panel,1)[0]
                return leaf(records(panel)[cursor]).upper()
            def focus(name):
                panel=m.read(n['state.activePanel'],1)[0]
                index=next(i for i,row in enumerate(records(panel)) if leaf(row).upper()==name.upper())
                m.tap('home')
                for _ in range(index):m.tap('down')
                wait();assert selected(panel)==name.upper()
            def assert_restore(active,infos,names):
                assert m.read(n['state.activePanel'],1)[0]==active
                assert m.read(n['state.panelA_info'],2)==infos
                assert m.read(n['state.panelA_sort'],2)==b'\1\2'
                for panel,want in enumerate((b'A:\\FONTS\\6X8',b'H:\\')):
                    if infos[panel]:
                        m.tap('f3');wait() # Reveal the inactive F3 panel's list.
                    path=n['vars.pathA'] if panel==0 else n['vars.pathB']
                    assert m.read(path,128).split(b'\0')[0].upper()==want
                    assert selected(panel)==names[panel],(panel,selected(panel),names[panel])
                    cur=m.read(n['state.panelA_curFile']+panel,1)[0]
                    first=m.read(n['state.panelA_firstFile']+panel,1)[0]
                    assert first<=cur<first+m.read(n['ncVisibleCount'],1)[0]
                    if infos[panel]:m.tap('f3');wait()
            command('FONT Compact')
            if m.read(n['state.activePanel'],1)[0]:m.tap('tab');wait()
            if m.read(n['state.panelB_info'],1)[0]:m.tap('f3');wait()
            command('CD A:/FONTS/6x8');focus(b'ROTATE.FNT')
            m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=600)
            m.tap('f1');m.call('kbd_send',keys=['shift'],action='release');wait()
            m.tap('up');m.tap('enter');wait()
            assert m.read(n['state.panelA_sort'],1)==b'\1'
            m.tap('tab');wait();command('CD H:/')
            visible=m.read(n['ncVisibleCount'],1)[0]
            assert len(records(1))>visible+3
            for _ in range(visible+3):m.tap('down')
            wait()
            assert m.read(n['state.panelB_firstFile'],1)[0]>0
            names=[selected(0),selected(1)]
            m.tap('f3');wait() # Left information, active right files.
            for font in ('Cpm4','Fast8','Compact'):
                command('FONT '+font)
                assert_restore(1,b'\1\0',names)
                print(font,'text state restores paths, independent sorts, names, visible scroll and left F3 PASS',flush=True)
            # Swap the active/info panels, preserving their selected file names.
            m.tap('f3');wait();m.tap('tab');wait();m.tap('f3');wait()
            old_cursor=m.read(n['state.panelA_curFile'],1)[0]
            existing={leaf(row).upper() for row in records(0)}
            added_name=next(f'{index:03}.TXT' for index in range(1000) if f'{index:03}.TXT'.encode() not in existing)
            m.stop_at(n['ncStateFinish']);m.tap('f10');m.until(n['ncStateFinish'],120,page=0)
            size=int.from_bytes(m.read(n['ncStateIO']+n['FILE_PATH_IO.count'],2),'little')
            saved=m.read(n['NC_SORT_KEYS'],size)
            assert b'\0' not in saved and saved.count(b'\r\n')==11,saved
            assert b'FILECOLOR' not in saved
            for line in (b'ACTIVEPANEL 0',b'LEFTPATH A:\\FONTS\\6x8',b'LEFTSORT 1',
                         b'RIGHTPATH H:\\',b'RIGHTSORT 2',b'RIGHTINFO 1'):
                assert line.upper() in saved.upper(),(line,saved)
            (OUT/'saved-NC-STATE.CFG').write_bytes(saved)
            m.call('bp_clear');m.stop_at(cli['cliPrompt']);m.until(cli['cliPrompt'],120,page=0)
            m.call('bp_clear');m.call('emu_run_for',ms=500)
            def cli_command(value):
                m.tap('esc')
                for char in value:type_char(char)
                actual=m.read(cli['CMD_LINE'],127).split(b'\0')[0]
                assert actual.upper()==value.upper().encode(),(value,actual)
                m.stop_at(cli['cliPrompt']);m.tap('enter');m.until(cli['cliPrompt'],120,page=0)
                m.call('bp_clear');m.call('emu_run_for',ms=500)
            cli_command('TYPE B:/NC-STATE.CFG')
            m.screenshot(OUT/'text-state-in-cli.png')
            # Insert an earlier item while NC is absent: saved ordinals are stale.
            cli_command('COPY A:/README.TXT A:/FONTS/6x8/'+added_name)
            m.tap('esc')
            for char in 'A:/NC.COM':type_char(char)
            m.tap('enter');wait_nc()
            assert_restore(0,b'\0\1',names)
            assert m.read(n['state.panelA_curFile'],1)[0]==old_cursor+1
            m.screenshot(OUT/'text-state-restored-after-cli.png')
            print('CLI shows real NC-STATE.CFG; added file shifts ordinal, saved name remains selected; right F3 restored PASS',flush=True)
            return

        if '--navigation-settings' in sys.argv:
            assert m.read(n['ncDefaultSort'],1)==b'\1', {
                'sort':m.read(n['ncDefaultSort'],1).hex(),
                'rules':m.read(n['vars.fileColorRules'],20).hex(),
                'io':m.read(n['ncStateIO'],40).hex(),
                'scratch':m.read(n['NC_SORT_KEYS'],90).hex()}
            assert m.read(n['state.panelA_sort'],2)==b'\1\1'
            assert m.read(n['ncFileDefaultColor'],1)==b'\xb1'
            rules=m.read(n['vars.fileColorRules'],129)
            assert rules[:4]==b'TXT\xe1','changed disk configuration was not loaded'
            print('Runtime NC.CFG from changed test image: TXT Yellow, default LightCyan, sort name; same NC binary PASS',flush=True)

            def rows():
                panel=m.read(n['state.activePanel'],1)[0]
                count=m.read(n['state.panelA_filesCnt']+panel,1)[0]
                slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
                base=int.from_bytes(m.read(n['ncCatalogBases']+slot*2,2),'little')
                index=m.read(n['FILE_INDEX_A']+panel*256,count)
                return [m.read(base+item*16,16) for item in index]
            def leaf(record):
                return record[:8].rstrip()+((b'.'+record[8:11].rstrip()) if record[8:11].strip() else b'')
            def selected():
                panel=m.read(n['state.activePanel'],1)[0]
                cur=m.read(n['state.panelA_curFile']+panel,1)[0]
                first=m.read(n['state.panelA_firstFile']+panel,1)[0]
                visible=m.read(n['ncVisibleCount'],1)[0]
                assert first<=cur<first+visible,(first,cur,visible)
                return leaf(rows()[cur])
            def focus(name):
                target=next(i for i,row in enumerate(rows()) if leaf(row).upper()==name.upper())
                m.tap('home')
                for _ in range(target):m.tap('down')
                wait();assert selected().upper()==name.upper()

            for font in ('Cpm4','Compact','Fast8'):
                command('FONT '+font)
                for _ in range(2):
                    panel=m.read(n['state.activePanel'],1)[0]
                    if m.read(n['state.panelA_info']+(panel^1),1)[0]:m.tap('f3');wait()
                    command('CD A:/FONTS/6x8')
                    m.tap('home');m.tap('enter');wait()
                    assert selected().upper()==b'6X8',(font,panel,selected())
                    command('CD ..');assert selected().upper()==b'FONTS'
                    m.tap('enter');wait();assert selected().upper()!=b'FONTS'
                    command('CD 8x8');command('CD ..');assert selected().upper()==b'8X8'
                    m.tap('tab');wait()
                m.screenshot(OUT/f'{font}-parent-selection.png')
                print(font,'parent leaf selection through Enter and CD, both panels PASS',flush=True)

            command('FONT Compact')
            for value in ('MD B:/PAR','MD B:/PAR/ZZZ.EXT','CD B:/PAR/ZZZ.EXT',
                          'MD B:/PAR/AAA','CD ..'):command(value)
            assert selected().upper()==b'ZZZ.EXT'
            m.tap('enter');wait();m.tap('home');m.tap('enter');wait()
            assert selected().upper()==b'ZZZ.EXT'
            print('Parent catalog changed while inside child: refreshed sorted index selects ZZZ.EXT PASS',flush=True)

            command('COPY A:/README.TXT B:/CASE.TXT');command('CD B:/')
            def rename_case(name):
                focus(name);before=selected()
                m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=600)
                m.tap('f6');m.call('kbd_send',keys=['shift'],action='release');wait()
                initial=m.read(n['vars.input'],128).split(b'\0')[0]
                assert initial==before,(initial,before)
                for _ in initial:m.tap('backspace')
                type_char(chr(initial[0]))
                first=m.read(n['vars.input'],1)
                if first==initial[:1]:
                    m.tap('backspace');m.call('kbd_send',keys=['shift'],action='press')
                    m.tap(chr(initial[0]).lower());m.call('kbd_send',keys=['shift'],action='release')
                    m.call('emu_run_for',ms=120)
                for char in initial[1:].decode():type_char(char)
                wanted=m.read(n['vars.input'],128).split(b'\0')[0]
                assert wanted.upper()==initial.upper() and wanted!=initial,(initial,wanted)
                m.screenshot(OUT/('rename-'+name.decode()+'-input.png'))
                m.tap('enter');wait()
                assert m.read(n['ncDialogActive'],1)==b'\0','rename returned an error dialog'
                actual=next(leaf(row) for row in rows() if leaf(row).upper()==name.upper())
                assert actual==wanted,(wanted,actual)
                print('Shift-F6 case-only rename',initial.decode(),'->',actual.decode(),'PASS',flush=True)
            rename_case(b'CASE.TXT');rename_case(b'PAR')
            m.screenshot(OUT/'rename-case-success.png')
            return

        if '--bitmap-changes' in sys.argv:
            command('FONT Compact');command('A:');command('CD /')
            panel=m.read(n['state.activePanel'],1)[0];other=panel^1
            if m.read(n['state.panelA_info']+other,1)[0]:m.tap('f3');wait()
            def panel_bitmap(which):
                return b''.join(m.read(0x9000+x*256,230) for x in range(which*24,(which+1)*24))
            def observe_finish(key,expected):
                hits=[];m.call('bp_clear')
                for entry in ('fullRedraw','loadAndPrintPanel','ncReadKey'):m.call('bp_set',addr=n[entry],type='exec')
                m.tap(key)
                for _ in range(200):
                    cpu=m.call('cpu_state');pc=cpu['registers']['pc']['dec']
                    if m.read(machine.pageAddress,1)==b'\0':
                        assert pc!=n['fullRedraw'],'dialog close cleared both panels'
                        if pc==n['loadAndPrintPanel']:hits.append(cpu['registers']['bc']['dec']&255)
                        if pc==n['ncReadKey']:break
                    m.call('emu_run_for',ms=500)
                else:raise AssertionError('finish timeout')
                m.call('bp_clear');m.call('emu_run_for',ms=500)
                assert hits==expected,(hits,expected)
                assert m.read(n['ncDialogActive'],1)==b'\0'
            before=panel_bitmap(other)
            m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=600)
            m.tap('f1' if panel==0 else 'f2');wait()
            m.call('kbd_send',keys=['shift'],action='release')
            m.tap('up');observe_finish('enter',[panel])
            assert panel_bitmap(other)==before
            print('Sort accept restores snapshot before radix and repaints only its cached list PASS',flush=True)
            command('H:');command('CD /')
            m.tap('tab');wait();command('B:');command('CD /');m.tap('tab');wait()
            panel=m.read(n['state.activePanel'],1)[0];other=panel^1
            slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
            base=int.from_bytes(m.read(n['ncCatalogBases']+slot*2,2),'little')
            count=m.read(n['state.panelA_filesCnt']+panel,1)[0]
            indices=m.read(n['FILE_INDEX_A']+panel*256,count)
            rows=[m.read(base+i*16,16) for i in indices]
            target=max((i for i,row in enumerate(rows) if not row[11]&16),key=lambda i:int.from_bytes(rows[i][14:16],'little'))
            # Select through normal panel keys; no PC/command-buffer injection.
            m.tap('home')
            for _ in range(target//19):m.tap('right')
            for _ in range(target%19):m.tap('down')
            wait();before=panel_bitmap(panel)
            # Only the owned test's lazy-display threshold is shortened. This
            # makes the progress transition deterministic even on fast disks.
            at=m.read(n['ncProgressUpdate'],n['ncProgressFallback']-n['ncProgressUpdate']).index(b'\x11'+(1954).to_bytes(2,'little'))+n['ncProgressUpdate']
            m.write(at+1,b'\0\0')
            m.tap('f5');wait();assert m.read(n['ncDialogActive'],1)==b'\1'
            m.stop_at(n['ncProgressValue']);m.tap('enter');m.until(n['ncProgressValue'],120,page=0)
            assert m.read(n['ncDialogActive'],1)==b'\1'
            m.screenshot(OUT/'bitmap-progress.png')
            # Resume and watch actual panel refresh after completion.
            m.call('bp_clear')
            hits=[]
            for entry in ('fullRedraw','loadAndPrintPanel','ncReadKey'):m.call('bp_set',addr=n[entry],type='exec')
            m.call('emu_run_for',ms=500)
            for _ in range(200):
                cpu=m.call('cpu_state');pc=cpu['registers']['pc']['dec']
                if m.read(machine.pageAddress,1)==b'\0':
                    assert pc!=n['fullRedraw']
                    if pc==n['loadAndPrintPanel']:hits.append(cpu['registers']['bc']['dec']&255)
                    if pc==n['ncReadKey']:break
                m.call('emu_run_for',ms=500)
            else:raise AssertionError('progress completion timeout')
            m.call('bp_clear');m.call('emu_run_for',ms=500)
            assert hits==[other],hits
            assert m.read(n['ncDialogActive'],1)==b'\0'
            assert panel_bitmap(panel)==before,'source panel changed during copy'
            m.screenshot(OUT/'bitmap-progress-complete.png')
            print('Input -> progress -> close: source pixels restored; only destination list refreshed PASS',flush=True)
            return

        if '--bitmap-dialogs' in sys.argv:
            def panels(panel=None):
                columns=range(48) if panel is None else range(panel*24,(panel+1)*24)
                return b''.join(m.read(0x9000+x*256,230) for x in columns)
            def close_without_redraw(before,label):
                m.call('bp_clear')
                watched=('fullRedraw','loadAndPrintPanel','lf_ReadCatalog','printInfoContents')
                for entry in watched+('ncReadKey',):m.call('bp_set',addr=n[entry],type='exec')
                m.tap('esc')
                for _ in range(120):
                    pc=m.call('cpu_state')['registers']['pc']['dec']
                    if m.read(machine.pageAddress,1)==b'\0':
                        assert pc not in [n[x] for x in watched],(label,'unnecessary redraw',hex(pc))
                        if pc==n['ncReadKey']:break
                    m.call('emu_run_for',ms=500)
                else:raise AssertionError((label,'close timeout'))
                m.call('bp_clear');m.call('emu_run_for',ms=500)
                assert m.read(n['ncDialogActive'],1)==b'\0'
                assert panels()==before,(label,'panel pixels did not restore')
            command('COPY A:/README.TXT B:/NCLOOK.TXT')
            for font in ('Cpm4','Compact','Fast8'):
                command('FONT '+font);command('A:');command('CD /');m.tap('home');wait()
                panel=m.read(n['state.activePanel'],1)[0]
                other=panel^1
                for info in (False,True):
                    if bool(m.read(n['state.panelA_info']+other,1)[0])!=info:m.tap('f3');wait()
                    for key,shift in [('f1',False),('f2',False),('f1',True),('f7',False),
                                      ('f8',False),('f5',False),('f6',False),('f6',True)]:
                        before=panels()
                        if shift:
                            m.call('kbd_send',keys=['shift'],action='press');m.call('emu_run_for',ms=600)
                        m.tap(key);wait()
                        if shift:m.call('kbd_send',keys=['shift'],action='release')
                        assert m.read(n['ncDialogActive'],1)==b'\1',(font,key,shift,'snapshot missing')
                        m.screenshot(OUT/f'{font}-bitmap-{key}-{int(shift)}-info-{int(info)}.png')
                        close_without_redraw(before,(font,key,shift,info))
                    print(font,'F3' if info else 'files','8 dialog cancels: exact pixels, no panel redraw or list reads PASS',flush=True)
                if m.read(n['state.panelA_info']+other,1)[0]:m.tap('f3');wait()
                for _ in range(9):m.tap('down')
                for value,target in [('COPY A:/README.TXT B:/NCLOOK.TXT','ncOverwriteKey'),
                                     ('DEL A:/NCSAVEZZ.TMP','copyFileAnyKey')]:
                    m.tap('esc');m.call('emu_run_for',ms=250)
                    for char in value:type_char(char)
                    before=panels();m.stop_at(n[target]);m.tap('enter');m.until(n[target],120,page=0)
                    m.call('bp_clear');m.call('emu_run_for',ms=500)
                    assert m.read(n['ncDialogActive'],1)==b'\1'
                    m.screenshot(OUT/f'{font}-bitmap-{target}.png')
                    close_without_redraw(before,(font,target))
                print(font,'overwrite and error: retained cursor restored, no redraw PASS',flush=True)
            return

        if '--benchmark-color-scroll' in sys.argv:
            command('FONT Rotate');command('H:');command('CD /')
            runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-color-scroll.py'))['run_pit'](m)
            return

        if '--file-colors' in sys.argv or '--launch-colors' in sys.argv:
            def check_colors():
                panel=m.read(n['state.activePanel'],1)[0]
                first=m.read(n['state.panelA_firstFile']+panel,1)[0]
                count=m.read(n['state.panelA_filesCnt']+panel,1)[0]
                slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
                base=int.from_bytes(m.read(n['ncCatalogBases']+slot*2,2),'little')
                visible=min(m.read(n['ncVisibleCount'],1)[0],count-first)
                indices=m.read(n['FILE_INDEX_A']+panel*256+first,visible)
                wanted=[]
                for index in indices:
                    record=m.read(base+index*16,16);ext=record[8:11].decode().strip().upper()
                    color=m.read(n['ncFileDefaultColor'],1)[0]
                    if record[n['FILE_INFO.attrib']]&16:color=n['COLOR_DIRECTORY']
                    elif ext in configured_colors:color=configured_colors[ext]
                    elif record[n['FILE_INFO.attrib']]&6:color=m.read(n['ncFileSystemColor'],1)[0]
                    wanted.append(color)
                actual=m.read(n['vars.rowColors']+panel*3*n['P_FILE_LIST_H'],visible)
                assert actual==bytes(wanted),(first,actual.hex(),bytes(wanted).hex())
            configured_colors={}
            rules=m.read(n['vars.fileColorRules'],129)
            for i in range(0,128,4):
                if not rules[i]:break
                configured_colors.setdefault(rules[i:i+3].decode('koi8-r').strip(),rules[i+3])
            for font in (('Compact',) if '--launch-colors' in sys.argv else ('Cpm4','Compact','Fast8')):
                command('FONT '+font);command('A:');command('CD /')
                check_colors();m.screenshot(OUT/f'{font}-file-colors.png')
                command('H:');command('CD /');m.tap('home');wait();check_colors()
                columns=m.read(n['ncColumns'],1)[0]
                for _ in range(columns):m.tap('right')
                wait();check_colors()
                for _ in range(3):m.tap('down')
                wait();check_colors();m.screenshot(OUT/f'{font}-colored-scroll.png')
                for _ in range(columns):m.tap('left')
                wait();check_colors()
                print(font,'normal colors, forward/backward bitmap scroll, palette table and cursor PASS',flush=True)
                m.tap('tab');wait()
            if '--launch-colors' in sys.argv:
                command('CD A:/FONTS/6x8');check_colors()
                m.screenshot(OUT/'launch-FNT-system-modules.png')
                print('Launchable FNT and MON retain executable colors despite System/Read-only attributes PASS',flush=True)
                return
            command('COPY A:/README.TXT B:/COLOR.ZIP')
            command('B:');command('CD /');check_colors()
            m.screenshot(OUT/'Fast8-archive-color.png')
            print('Archive palette and ordinary DOS COPY through the NC editor PASS',flush=True)
            return

        if '--delete-dialog' in sys.argv:
            for font in ('Cpm4','Compact','Fast8'):
                command('FONT '+font);command('A:');command('CD /')
                m.tap('home');wait()
                for want_directory in (True,False):
                    panel=m.read(n['state.activePanel'],1)[0]
                    slot=m.read(n['vars.cacheSlotA']+panel,1)[0]
                    base=int.from_bytes(m.read(n['ncCatalogBases']+2*slot,2),'little')
                    for _ in range(255):
                        cursor=m.read(n['state.panelA_curFile']+panel,1)[0]
                        index=m.read(n['FILE_INDEX_A']+panel*256+cursor,1)[0]
                        record=m.read(base+index*16,16)
                        if bool(record[n['FILE_INFO.attrib']]&16)==want_directory:break
                        m.tap('down')
                    else:raise AssertionError('No suitable delete-dialog fixture')
                    leaf=record[:8].rstrip()+((b'.'+record[8:11].rstrip()) if record[8:11].strip() else b'')
                    m.stop_at(n['loc_D54C']);m.tap('f8');m.until(n['loc_D54C'],120,page=0)
                    m.call('bp_clear');m.call('emu_run_for',ms=500)
                    actual=m.read(n['vars.input'],32).split(b'\0')[0]
                    assert actual==leaf and all(c>=32 for c in actual),(font,leaf,actual)
                    m.screenshot(OUT/f'{font}-delete-{"dir" if want_directory else "file"}.png')
                    m.tap('esc');wait()
                print(font,'delete dialog: plain directory/file name, no CR; Escape leaves files intact PASS',flush=True)
            return

        if '--same-context' in sys.argv:
            command('FONT Compact')
            def quiet_context(value):
                m.tap('esc');m.call('emu_run_for',ms=250)
                for char in value:type_char(char)
                actual=m.read(n['vars.cmdLine'],127).split(b'\0')[0]
                assert actual.upper()==value.upper().encode(),(value,actual)
                before=b''.join(m.read(0x9000+x*256,230) for x in range(48))
                state=m.read(n['state'],16)
                m.call('bp_clear')
                watched=('drawCursor','loadAndPrintPanel','fullRedraw','lf_ReadCatalog','printInfoContents')
                for entry in watched+('ncReadKey',):m.call('bp_set',addr=n[entry],type='exec')
                m.tap('enter')
                for _ in range(120):
                    pc=m.call('cpu_state')['registers']['pc']['dec']
                    if m.read(machine.pageAddress,1)==b'\0':
                        assert pc not in [n[entry] for entry in watched],(value,'unnecessary redraw/list read',hex(pc))
                        if pc==n['ncReadKey']:break
                    m.call('emu_run_for',ms=500)
                else:raise AssertionError((value,'command timeout'))
                m.call('bp_clear');m.call('emu_run_for',ms=500)
                assert m.read(n['state'],16)==state,(value,'state/selection reset')
                after=b''.join(m.read(0x9000+x*256,230) for x in range(48))
                assert after==before,(value,'panel bitmap changed')
            for _ in range(2):
                panel=m.read(n['state.activePanel'],1)[0]
                if m.read(n['state.panelA_info']+(panel^1),1)[0]:m.tap('f3');wait()
                command('H:');command('CD /')
                for _ in range(m.read(n['ncVisibleCount'],1)[0]+3):m.tap('down')
                assert m.read(n['state.panelA_firstFile']+panel,1)[0]>0
                for value in ('H:\\','H:/','H:','CD .'):quiet_context(value)
                m.tap('home');wait();m.tap('enter');wait()
                def directory():
                    lo=m.read(n['state.panelA_dirLow']+panel,1)[0]
                    hi=m.read(n['state.panelA_dirHigh']+panel,1)[0]
                    return lo+hi*256
                folder=directory();assert folder!=0,'H: first entry must open a directory'
                m.tap('down');wait();quiet_context('H:')
                command('A:');assert m.read(n['state.panelA_drive']+panel,1)==b'\0'
                command('H:');assert directory()==folder,'saved H: directory lost'
                quiet_context('H:')
                command('H:\\');assert directory()==0,'real root transition did not occur'
                assert m.read(n['state.panelA_curFile']+panel,1)==b'\0'
                quiet_context('H:\\')
                m.tap('f3');wait()
                quiet_context('H:');quiet_context('H:\\')
                m.screenshot(OUT/f'same-context-panel-{panel}.png')
                m.tap('f3');wait();m.tap('tab');wait()
                print('Panel',panel,'same root/saved directory: no redraw, no list read, selection/scroll preserved; real transitions PASS',flush=True)
            return

        fonts=('Cpm4','Compact','Fast8')
        if '--fonts' in sys.argv:
            fonts=tuple(sys.argv[sys.argv.index('--fonts')+1].split(','))
            assert all(font in ('Cpm4','Compact','Fast8') for font in fonts)
        overwrite_fixture=False
        for font in (() if '--state-only' in sys.argv else fonts):
            command('FONT ' + font)
            if '--overwrite-ui' in sys.argv:
                copy='COPY A:/README.TXT B:/NCLOOK.TXT'
                if not overwrite_fixture:
                    command(copy) # README has no read-only attribute.
                    overwrite_fixture=True
                m.tap('esc');m.call('emu_run_for',ms=250)
                for char in copy:m.tap({' ':'space',':':'colon','/':'slash','.':'period'}.get(char,char.lower()))
                actual=m.read(n['vars.cmdLine'],127).split(b'\0')[0]
                assert actual.upper()==copy.upper().encode(),(font,copy,actual,m.read(n['vars.cmdLineEnd'],2).hex())
                m.stop_at(n['ncOverwriteKey']);m.tap('enter')
                m.until(n['ncOverwriteKey'],120,page=0)
                m.call('bp_clear');m.call('emu_run_for',ms=500)
                for choice in (2,3,0,1):
                    assert m.read(n['ncOverwriteChoice'],1)==bytes([choice]),(font,'overwrite navigation')
                    m.screenshot(OUT/(font+'-overwrite-'+str(choice)+'.png'))
                    if choice!=1:m.tap('right');wait()
                m.tap('esc');wait()
                print(font,'overwrite text and pixel buttons, all four choices, Escape PASS',flush=True)
                if '--info-navigation' not in sys.argv:continue
            if '--info-navigation' in sys.argv:
                command('A:')
                command('CD /')
                for key in ('f1','f2'):
                    m.tap(key);wait()
                    m.screenshot(OUT/(font+'-'+key+'-pixel-buttons.png'))
                    m.tap('esc');wait()
                panel=m.read(n['state.activePanel'],1)[0]
                info_panel=panel^1
                if not m.read(n['state.panelA_info']+info_panel,1)[0]:
                    m.tap('f3');wait()
                m.screenshot(OUT/(font+'-aligned-info.png'))
                def info_bitmap():
                    return b''.join(m.read(0x9000+(24*info_panel+x)*256,230) for x in range(24))
                def unchanged(value=None):
                    if value:
                        m.tap('esc');m.call('emu_run_for',ms=250)
                        for char in value:m.tap({' ':'space','/':'slash'}.get(char,char.lower()))
                        assert m.read(n['vars.cmdLine'],len(value)).upper()==value.upper().encode()
                    before=info_bitmap()
                    m.call('bp_clear')
                    watched=('printInfoPanel','printInfoContents','pip_freeSpaceLast')
                    for entry in watched+('ncReadKey',):
                        m.call('bp_set',addr=n[entry],type='exec')
                    m.tap('enter')
                    for _ in range(120):
                        pc=m.call('cpu_state')['registers']['pc']['dec']
                        page=m.read(machine.pageAddress,1)[0]
                        if page==0:
                            assert pc not in [n[entry] for entry in watched],(font,'unnecessary info redraw')
                            if pc==n['ncReadKey']:break
                        m.call('emu_run_for',ms=500)
                    else:raise AssertionError((font,'navigation timeout'))
                    m.call('bp_clear');m.call('emu_run_for',ms=500)
                    assert info_bitmap()==before,(font,'same-drive navigation changed info pixels')
                unchanged('CD FONTS')
                unchanged('CD 6X8')
                m.tap('home');wait()
                unchanged() # .. -> FONTS through ordinary Enter.
                path=n['vars.pathA'] if panel==0 else n['vars.pathB']
                assert m.read(path,128).split(b'\0')[0].upper()==b'A:\\FONTS'
                command('H:')
                assert m.read(n['ncInfoDrive'],1)==b'\x07','drive switch did not refresh information'
                m.screenshot(OUT/(font+'-info-drive-H.png'))
                m.tap('f3');wait()
                m.tap('home');wait()
                m.screenshot(OUT/(font+'-pixel-file-cursor.png'))
                print(font,'pixel drive/file cursor, aligned F3, same-drive CD/Enter without redraw, drive refresh PASS',flush=True)
                continue
            if '--scroll-only' in sys.argv:
                command('H:')
                panel=m.read(n['state.activePanel'],1)[0]
                visible=m.read(n['ncVisibleCount'],1)[0]
                assert m.read(n['state.panelA_filesCnt']+panel,1)[0]>visible+3
                # Keyboard Home: establish the same origin without patching state.
                m.tap('home')
                wait()
                for _ in range(visible+3):m.tap('down')
                wait()
                assert m.read(n['state.panelA_firstFile']+panel,1)==b'\x04'
                assert m.read(n['uiTextRect.width'],1)==b'\x00'
                assert m.read(n['uiTextRect.height'],1)[0]>0
                m.screenshot(OUT/(font+'-specialized-forward.png'))
                for _ in range(visible+3):m.tap('up')
                wait()
                assert m.read(n['state.panelA_firstFile']+panel,1)==b'\x00'
                assert m.read(n['state.panelA_curFile']+panel,1)==b'\x00'
                m.screenshot(OUT/(font+'-specialized-backward.png'))
                print(font,'specialized bitmap scroll: real keyboard forward/backward PASS',flush=True)
                continue
            command('A:')
            for key in (() if '--info-only' in sys.argv else ('f1', 'f2')):
                m.tap(key)
                wait()
                m.screenshot(OUT / (font + '-' + key + '.png'))
                m.tap('esc')
                wait()
            m.tap('f3')
            wait()
            m.screenshot(OUT / (font + '-info-A.png'))
            m.tap('f3')
            wait()
            command('B:')
            m.tap('f3')
            wait()
            m.screenshot(OUT / (font + '-info-B.png'))
            m.tap('f3')
            wait()
            if font != 'Compact' and '--info-only' not in sys.argv:
                command('H:')
                visible = m.read(n['ncVisibleCount'], 1)[0]
                assert m.read(n['state.panelA_filesCnt'], 1)[0] > visible + 2
                for _ in range(visible + 3):
                    m.tap('down')
                wait()
                assert m.read(n['vars.scrollRows'], 1) == b'\x01'
                assert m.read(n['bitmapDirection'], 1) == b'\x01'
                assert m.read(n['bitmapColumnsLeft'], 1) == b'\x00'
                m.screenshot(OUT / (font + '-scroll-forward.png'))
                for _ in range(visible + 3):
                    m.tap('up')
                wait()
                assert m.read(n['bitmapDirection'], 1) == b'\x02'
                assert m.read(n['state.panelA_firstFile'], 1) == b'\x00'
                m.screenshot(OUT / (font + '-scroll-backward.png'))
            print(font, 'F1/F2 and F3 A:/B: keyboard scenarios PASS', flush=True)
        if '--text-benchmark' in sys.argv:
            command('FONT Compact');command('H:');command('CD /')
            bench=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-text-alignment.py'))
            bench['run'](m,n,ROOT/'docs/nc/text-align-2026-11-15')
            print('H: aligned/centered text benchmark PASS',flush=True)
            return
        if '--state-status' in sys.argv or '--state-only' in sys.argv:
            command('FONT Compact')
            command('A:')
            m.tap('f3');wait()
            assert m.read(n['state.panelB_info'],1)==b'\x01'
            command('FONT Compact')
            assert m.read(n['state.activePanel'],1)==b'\x00'
            assert m.read(n['state.panelB_info'],1)==b'\x01','right F3 lost on external program return'
            m.screenshot(OUT/'right-F3-restored.png')
            m.tap('f10');wait()
            command('EXIT');wait_nc()
            assert m.read(n['state.panelB_info'],1)==b'\x01','right F3 lost on CLI return'
            m.tap('f3');wait();m.tap('tab');wait();m.tap('f3');wait()
            assert m.read(n['state.panelA_info'],1)==b'\x01'
            command('FONT Compact')
            assert m.read(n['state.activePanel'],1)==b'\x01'
            assert m.read(n['state.panelA_info'],1)==b'\x01','left F3 lost on external program return'
            m.screenshot(OUT/'left-F3-restored.png')
            m.tap('f3');wait();m.tap('tab');wait()
            command('A:')
            command('CD FONTS/6X8')
            count=m.read(n['state.panelA_filesCnt'],1)[0]
            assert count>1
            slot=m.read(n['vars.cacheSlotA'],1)[0]
            base=int.from_bytes(m.read(n['ncCatalogBases']+2*slot,2),'little')
            records=[m.read(base+i*16,11) for i in range(count)]
            assert records[0].startswith(b'.. ') and all(not x.startswith(b'\\') for x in records)
            m.screenshot(OUT/'parent-dir-status.png')
            m.tap('down');wait()
            chosen=m.read(n['FILE_INDEX_A']+1,1)[0]
            rec=m.read(base+chosen*16,11)
            leaf=rec[:8].rstrip()+((b'.'+rec[8:11].rstrip()) if rec[8:11].strip() else b'')
            assert m.read(n['vars.pathArg'],128).split(b'\0')[0].upper()==leaf.upper(),'status contains a path instead of the leaf'
            m.screenshot(OUT/'file-leaf-status.png')
            m.tap('home');m.tap('enter');wait()
            assert m.read(n['vars.pathA'],128).split(b'\0')[0].upper()==b'A:\\FONTS'
            print('F3 persistence: FONT return on both panels, CLI return, dot filtering, parent navigation and leaf status PASS',flush=True)
    finally:
        try:
            machine.m.screenshot(OUT / 'last.png')
        except Exception:
            pass
        machine.stop()


if __name__ == '__main__':
    main()
