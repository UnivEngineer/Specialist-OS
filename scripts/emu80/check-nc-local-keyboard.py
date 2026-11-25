"""Drive the banked NC through real Emu80 keyboard input."""
import argparse
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--port', type=int, choices=range(19781, 19785), required=True)
    parser.add_argument('--font', help='Switch font in MAIN before starting NC_BANK, e.g. 4x8/Packed')
    parser.add_argument('--reselect', action='store_true', help='Start NC_BANK again from its own command prompt')
    parser.add_argument('--editor', action='store_true', help='Check banked F4 and TXT handoff and return')
    parser.add_argument('--editor-h', nargs='?', const='COPTER.RKS', choices=['COPTER.RKS','RAIN.RKS'],
                        help='Open a sample from H: using F4 on the second panel')
    parser.add_argument('--hex-batch', action='store_true',
                        help='Compare cold, prefetched and individually drawn HEX rows on H:')
    parser.add_argument('--hex-koi', action='store_true',
                        help='Run the HEX batch comparison in the 6x8 KOI layout')
    args = parser.parse_args()
    if args.hex_batch and not args.editor_h:
        parser.error('--hex-batch requires --editor-h')
    if args.hex_koi and not args.hex_batch:
        parser.error('--hex-koi requires --hex-batch')
    out = ROOT / ('.local/nc-editor-h' if args.editor_h else '.local/nc-local-keyboard')
    if args.editor_h:
        out /= Path(args.editor_h).stem + '-' + (args.font or 'Rotate').split('/')[-1]
        if args.hex_koi:
            out = out.with_name(out.name+'-KOI')
    out.mkdir(parents=True, exist_ok=True)
    helpers = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-disks.py'))
    profile = runpy.run_path(str(ROOT / 'scripts/emu80/benchmark-nc-profiles.py'))
    profile['symbols'].__globals__.update(SNAP=ROOT, OUT=out)
    helpers['Matrix'].__init__.__globals__.update(
        RUNTIME=args.runtime.resolve(), OUT=out,
        mcp_owner=lambda: helpers['shell'](
            f'$p=Get-NetTCPConnection -LocalPort {args.port} -State Listen -ErrorAction SilentlyContinue; '
            'if($p){$p[0].OwningProcess};exit 0'))
    matrix = helpers['Matrix']()
    m = matrix.m
    m.url = f'http://127.0.0.1:{args.port}/mcp'
    try:
        matrix.start('nc-local-keyboard')
        c = profile['Campaign'](m)

        def type_bank_command():
            # The matrix can sample the key and Shift on different scans.
            for attempt in range(3):
                try:
                    c.type('C:/DOS/NC.COM')
                    return
                except AssertionError:
                    if attempt == 2:
                        raise

        if args.font:
            c.command('FONT '+args.font)
            m.screenshot(out/'main-after-font.png')
        type_bank_command()
        c.tap('enter')
        m.call('emu_run_for', ms=6000)
        keys = c.read(1, c.k['residentKeys'], 16)
        slot = next(i for i in range(8) if keys[2*i:2*i+2] == b'NC')
        c.page = c.read(1, c.k['MODULE_TABLE']+slot*c.k['MODULE_RECORD_SIZE']+3, 1)[0]
        c.n = profile['symbols']('NC_CORE')
        c.wait(c.n['ncBankInputPoll'], c.page)
        m.call('bp_clear')
        assert c.read(c.page, c.n['ncInputState.timed'], 1) == bytes([1])
        if args.editor or args.editor_h:
            panel = int(bool(args.editor_h))
            side = 'B' if panel else 'A'
            filename = args.editor_h or 'README.TXT'
            stem, ext = filename.split('.')
            record_name = (stem.ljust(8) + ext.ljust(3)).encode('ascii')
            drive = 'H' if panel else 'A'
            path = (drive + ':\\' + filename).encode('ascii')
            payload = ROOT / 'assets' / ('games/rks' if panel else 'system-rom') / filename
            c.free()
            if c.panel()!=panel:
                c.tap('tab')
                c.free()
            c.tap('home' if panel else 'end')
            c.free()
            n = c.n
            def read_nc(name, length):
                return c.read(c.page, n[name], length)

            count = read_nc(f'state.panel{side}_filesCnt', 1)[0]
            slot = read_nc('vars.cacheSlot'+side, 1)[0]
            base = int.from_bytes(c.read(c.page, n['ncCatalogBases'] + 2*slot, 2), 'little')
            records = c.read(c.page, base, count*n['FILE_INFO'])
            order = read_nc('FILE_INDEX_'+side, count)
            target = next(i for i, record in enumerate(order)
                          if records[record*16:record*16+11].upper() == record_name)
            current = c.cursor()
            for _ in range(abs(current-target)):
                c.tap('up' if current > target else 'down')
            c.free()
            assert c.cursor() == target, (current, target, c.cursor())
            editor = profile['symbols']('EDITCORE')
            for key in (('f4',) if panel else ('f4','enter')):
                c.free()
                m.stop_at(editor['ebWaitReady'])
                c.tap(key)
                m.until(editor['ebWaitReady'], 60)
                bank = c.page_now()
                assert bank not in (0, 1, c.page)
                assert c.read(bank, editor['ebKeyState.timed'], 1) == bytes([1])
                assert c.read(bank, editor['ebQuiet'], 1) == bytes([1])
                assert c.read(bank, editor['FILE_NAME_HISTORY'], 64).split(bytes(1))[0].upper() == path
                start = int.from_bytes(c.read(bank, editor['v_textBeginLo'], 2), 'little')
                end = int.from_bytes(c.read(bank, editor['v_textEnd'], 2), 'little')
                # Immediately after opening, the whole file precedes the gap.
                assert c.read(bank, start, end-start) == payload.read_bytes()
                def editor_key(key, shift=False, ctrl=False):
                    c.free()
                    if shift:
                        m.call('kbd_send',keys=['shift'],action='press')
                        m.call('emu_run_for',ms=500)
                    if ctrl:
                        m.call('kbd_send',keys=['rpt'],action='press')
                        m.call('emu_run_for',ms=200)
                    m.stop_at(editor['ebWaitReady'])
                    c.tap(key)
                    m.until(editor['ebWaitReady'],60,page=bank)
                    if shift:m.call('kbd_send',keys=['shift'],action='release')
                    if ctrl:m.call('kbd_send',keys=['rpt'],action='release')

                if panel:
                    assert c.read(bank,editor['hexActive'],1)==b'\x01'
                    assert c.read(bank,editor['hexViewType'],1)==b'\x02'
                    m.screenshot(out/'f4-auto-exec.png')
                    editor_key('f9')
                    assert c.read(bank,editor['hexActive'],1)==b'\x00'
                    editor_key('f3')
                    assert c.read(bank,editor['uiModalCurrent'],2)==editor['editorFindDescriptor'].to_bytes(2,'little')
                    editor_key('esc')

                for digit in (b'7', b'8'):
                    editor_key('f2', shift=True)
                    assert c.read(bank,editor['editorEncodingDigit'],1)==digit
                    assert c.read(bank,start,end-start)==payload.read_bytes()

                for active in (1,0):
                    editor_key('f9', shift=bool(active))
                    assert c.read(bank,editor['hexActive'],1)==bytes([active])
                    if active:
                        if panel:
                            assert c.read(1,c.k['v_drive'],1)==bytes([7])
                        if not c.read(bank,editor['hexTextCol'],1)[0]:
                            m.screenshot(out/(key+'-hex-CS.png'))
                        line = int.from_bytes(c.read(bank,editor['hexCachedLine'],2),'little')
                        address = int.from_bytes(c.read(bank,editor['hexLineBytes'],2),'little')
                        count = c.read(bank,editor['hexLineCount'],1)[0]
                        assert count and c.read(bank,address,count)==payload.read_bytes()[line:line+count]
                        if args.hex_batch:
                            if args.hex_koi:
                                assert c.read(bank,editor['v_charStep'],1)==b'\x03'
                                editor_key('f6')
                                assert c.read(bank,editor['hexTextCol'],1)==b'\x2e'
                            handle = int.from_bytes(c.read(bank,editor['ebGfxHandle'],2),'little')
                            gfx = profile['graphics_symbols'](lambda at,size:c.read(1,at,size),c.k,handle)
                            def page(arrow, clear=False, individual=False):
                                entry = 'hexPageDown' if arrow == 'down' else 'hexPageUp'
                                c.free()
                                m.call('kbd_send', keys=['rpt'], action='press')
                                m.call('emu_run_for', ms=250)
                                m.stop_at(editor[entry])
                                c.tap(arrow)
                                first = m.until(editor[entry],60,page=bank)['cpu_ticks']
                                count_at = gfx['ngBatches']
                                initial_batches = int.from_bytes(c.read(0,count_at,2),'little')
                                code = editor['hexPacketEnable']
                                saved = c.read(bank,code,1)
                                if clear:
                                    m.write(editor['hexCacheTags'],b'\xff'*editor['HEX_CACHE_ROWS'])
                                if individual:
                                    m.write(code,b'\xc9')
                                try:
                                    m.stop_at(editor['ebWaitReady'])
                                    last = m.until(editor['ebWaitReady'],60,page=bank)['cpu_ticks']
                                finally:
                                    if individual:
                                        assert c.page_now()==bank
                                        m.write(code,saved)
                                    m.call('kbd_send', keys=['rpt'], action='release')
                                batches = (int.from_bytes(c.read(0,count_at,2),'little')-initial_batches)&65535
                                return c.read(0,0x9000,0x3000), last-first, batches

                            slots = c.read(bank,editor['hexCacheSlots'],1)[0]
                            assert slots in (64,128)
                            def cached(row):
                                addr = editor['hexCacheTags']+(row&(slots-1))
                                return c.read(bank,addr,1)[0]==row//64

                            top = int.from_bytes(c.read(bank,editor['hexTop'],2),'little')
                            visible = c.read(bank,editor['hexBar.visible'],1)[0]
                            before = c.read(0,0x9000,0x3000)
                            last_prefetched = top+visible+(visible-1)*(slots//64)-1
                            assert not cached(last_prefetched)
                            c.free()
                            m.call('emu_run_for',ms=1400)
                            assert cached(last_prefetched)
                            print('HEX CACHE SLOTS', slots,
                                  'registered',c.read(bank,editor['ebCacheRows'],1)[0],flush=True)
                            warm, warm_ticks, warm_batches = page('down')
                            reverse, reverse_ticks, reverse_batches = page('up')
                            assert reverse==before, 'cached PgUp bitmap differs'
                            cold, cold_ticks, cold_batches = page('down',clear=True)
                            assert warm==cold, 'prefetched and cold PgDn bitmaps differ'
                            page('up')
                            separate, separate_ticks, separate_batches = page('down',individual=True)
                            assert separate==cold, 'batched and individual PgDn bitmaps differ'
                            print('HEX PAGE PROFILE',args.font or 'Rotate',
                                  dict(prefetched=(warm_ticks,warm_batches),back=(reverse_ticks,reverse_batches),
                                       cold=(cold_ticks,cold_batches),individual=(separate_ticks,separate_batches)),flush=True)
                            page('up')
                            # Force a checksum failure after reading the text DLL.
                            expected_at = editor['textOverlayFile']+6
                            expected = c.read(bank,expected_at,1)
                            m.write(expected_at,bytes([expected[0]^1]))
                            try:
                                editor_key('f9')
                                assert c.read(bank,editor['hexActive'],1)==b'\x01'
                                assert c.read(bank,editor['ebSnapshotActive'],1)==b'\x01'
                            finally:
                                m.write(expected_at,expected)
                            editor_key('enter')
                            assert c.read(0,0x9000,0x3000)==before, 'overlay rollback bitmap differs'
                            print('HEX OVERLAY ROLLBACK PASS',flush=True)
                            if args.hex_koi:
                                m.screenshot(out/'f4-hex-koi-layout.png')
                                for _ in range(3):
                                    editor_key('f6')
                                assert c.read(bank,editor['hexTextCol'],1)==b'\x00'
                        for arrow, entry in [('down','hexPageDown'), ('up','hexPageUp')]:
                            c.free()
                            m.call('kbd_send', keys=['rpt'], action='press')
                            m.call('emu_run_for', ms=250)
                            m.stop_at(editor[entry])
                            c.tap(arrow)
                            begin=m.until(editor[entry],60,page=bank)['cpu_ticks']
                            m.stop_at(editor['ebWaitReady'])
                            end_ticks=m.until(editor['ebWaitReady'],60,page=bank)['cpu_ticks']
                            m.call('kbd_send', keys=['rpt'], action='release')
                            print(path, args.font or 'Rotate', entry, end_ticks-begin, 'T', flush=True)
                        narrow = not c.read(bank,editor['hexTextCol'],1)[0]
                        modes = [(255,'koi7'),(1,'koi8'),(0,'all')] if narrow else [(0,'all'),(255,'koi7'),(1,'koi8')]
                        for mode,label in modes:
                            editor_key('f6')
                            assert c.read(bank,editor['hexTextCol'],1)[0]
                            assert c.read(bank,editor['hexFiltered'],1)==bytes([mode])
                            m.screenshot(out/(key+'-hex-'+(args.font or 'Rotate').split('/')[-1]+'-'+label+'.png'))
                        if narrow:
                            editor_key('f6')
                            assert not c.read(bank,editor['hexTextCol'],1)[0]
                        if panel:
                            editor_key('f7')
                            assert c.read(bank,editor['hexViewType'],1)==b'\x02'
                            assert c.read(bank,editor['hexBase'],2)==b'\x04\x00'
                            assert c.read(bank,editor['hexContainerWarn'],1)==b'?'
                            editor_key('home', ctrl=True)
                            editor_key('down')
                            assert c.read(bank,editor['hexCaret'],2)==b'\x04\x00'
                            before=c.read(0,0x9000,0x3000)
                            editor_key('f5')
                            editor_key('esc')
                            assert c.read(0,0x9000,0x3000)==before
                            editor_key('f8')
                            assert c.read(bank,editor['hexContainerWarn'],1)==b' '
                            assert c.read(bank,start,end-start)==payload.read_bytes()
                            m.screenshot(out/(key+'-hex-exec.png'))
                            editor_key('lf', ctrl=True)
                            assert c.read(bank,editor['hexCaret'],2)==c.read(bank,editor['hexSize'],2)
                            m.screenshot(out/(key+'-hex-rks-tail.png'))
                            editor_key('f7')
                            assert c.read(bank,editor['hexViewType'],1)==b'\x00'
                if panel:
                    editor_key('f9', shift=True)
                    assert c.read(bank, editor['hexActive'], 1) == b'\x01'
                c.free()
                c.tap('f10')
                c.wait(n['ncBankInputPoll'], c.page)
                assert c.read(c.page, n['ncInputState.timed'], 1) == bytes([1])
                assert c.cursor() == target
                print(f'{key}: {path.decode()} -> EDIT -> same NC panel PASS', flush=True)
            return
        if args.reselect:
            type_bank_command()
            resume = c.n['ncBankResumeCommandReady']
            m.call('bp_set', addr=resume, type='exec')
            launcher = profile['symbols']('NC_DOS')
            m.call('bp_set', addr=launcher['failed'], type='exec')
            c.tap('enter')
            for _ in range(32):
                state = c.state()
                pc = state['registers']['pc']['dec']
                if pc == launcher['failed'] and c.page_now() == 1:
                    error = state['registers']['af']['dec'] >> 8
                    raise AssertionError(f'NC_BANK launcher error {error:02X}')
                if pc == resume and c.page_now() == c.page:
                    break
                if pc in (launcher['failed'], resume):
                    m.call('debug_step', mode='into')
                m.call('emu_run_for', ms=500)
            else:
                raise AssertionError('NC_BANK did not resume its existing resident')
            c.wait(c.n['ncBankInputPoll'], c.page)
            m.call('bp_clear')
            print('NC_BANK reselect PASS', flush=True)

        def read(name, size=1):
            return c.read(c.page, c.n[name], size)

        def press(key, duration=100):
            m.call('kbd_send', keys=[key], action='tap', duration_ms=duration)
            m.call('emu_run_for', ms=220)

        def shift_key(key, action):
            m.call('kbd_send', keys=[key], action=action)
            m.call('emu_run_for', ms=260)

        # Shift alone changes the toolbar while the CPU waits in EXEC.
        assert read('ncShiftState') == b'\x02'
        shift_key('shift', 'press')
        assert read('ncShiftState') == b'\0'
        m.screenshot(out/'shift-toolbar.png')
        shift_key('shift', 'release')
        for _ in range(5):
            if read('ncShiftState') == b'\x02':
                break
            m.call('emu_run_for', ms=200)
        assert read('ncShiftState') == b'\x02'

        shift_key('shift', 'press')
        try:
            m.call('bp_set', addr=c.n['ncSortKeyLoop'], type='exec')
            c.tap('f1')
            c.wait(c.n['ncSortKeyLoop'], c.page)
        finally:
            m.call('bp_clear')
            shift_key('shift', 'release')
        press('esc')

        # The key byte of F1 is zero. Enter and Escape still work in modals.
        press('f1')
        assert read('ncDialogActive') != b'\0'
        m.screenshot(out/'drive.png')
        press('esc')
        assert read('ncDialogActive') == b'\0'

        if c.panel() == 0:
            press('tab')
        press('home')
        before = c.cursor()
        press('down')
        assert c.cursor() == before+1
        m.call('kbd_send', keys=['down'], action='press')
        try:
            m.call('emu_run_for', ms=1000)
        finally:
            m.call('kbd_send', keys=['down'], action='release')
        assert c.cursor() > before+2
        m.call('emu_run_for', ms=120)
        before = c.cursor()
        press('up')
        assert c.cursor() == before-1

        # Ctrl+Enter inserts the selected name in the command prompt.
        shift_key('rpt', 'press')
        press('enter')
        shift_key('rpt', 'release')
        end = int.from_bytes(read('vars.cmdLineEnd', 2), 'little')
        inserted = read('vars.cmdLine', end-c.n['vars.cmdLine'])
        path = read('vars.pathArg', c.n['MAX_PATH']).split(b'\0', 1)[0]
        assert path[1:3] == b':\\' and inserted == path+b' ', inserted
        m.screenshot(out/'ctrl-enter.png')
        press('esc')
        c.type('C:/DOS/CLI')
        cli = profile['symbols']('CLI')
        m.call('bp_clear')
        m.call('bp_set', addr=cli['cliPrompt'], type='exec')
        c.tap('enter')
        c.wait(cli['cliPrompt'], 0)
        c.free()
        for key in 'exit':
            press(key)
        m.call('bp_set', addr=c.n['ncBankInputPoll'], type='exec')
        c.tap('enter')
        c.wait(c.n['ncBankInputPoll'], c.page)
        print('NC local keyboard PASS: Shift, F1/Escape, arrows/repeat, Ctrl+Enter, command line, CLI return')
        bank, core = c.page, c.n
        main_nc = profile['symbols']('NC')
        c.type('C:/DOS/NC_MN.COM')
        m.call('bp_set', addr=main_nc['mainLoop'], type='exec')
        c.tap('enter')
        c.wait(main_nc['mainLoop'], 0)
        c.page, c.n = 0, main_nc
        c.free()
        assert c.read(0, 0, 12) == (ROOT/'bin/NC.i80').read_bytes()[:12]
        assert read('aCommanderVer', 14) == b'Commander 3.0\0'
        press('f3')
        m.screenshot(out/'main-f3.png')
        print('BANK -> NC_MN.COM F3 PASS', flush=True)
        type_bank_command()
        m.call('bp_set', addr=core['ncBankInputPoll'], type='exec')
        c.tap('enter')
        c.wait(core['ncBankInputPoll'], bank)
        c.page, c.n = bank, core
        for command in ('nc', 'nc.com'):
            c.free()
            # MAIN NC can report a truncated H: catalog on startup.
            if c.page == 0 and read('ncDialogActive') != bytes([0]):
                press('esc')
                assert read('ncDialogActive') == bytes([0])
            shift_key('shift', 'press')
            try:
                m.call('bp_set', addr=cli['cliPrompt'], type='exec')
                c.tap('f10')
                c.wait(cli['cliPrompt'], 0)
            finally:
                m.call('kbd_send', keys=['shift'], action='release')
            c.free()
            for key in command:
                press('period' if key == '.' else key)
            assert c.read(0, cli['CMD_LINE'], len(command)).lower() == command.encode()
            m.call('bp_set', addr=core['ncBankInputPoll'], type='exec')
            c.tap('enter')
            c.wait(core['ncBankInputPoll'], bank)
            c.page, c.n = bank, core
            print(f'Shift+F10 -> CLI {command} -> BANK NC.COM PASS', flush=True)

        # Reset abandons the input scope without running its normal cleanup.
        # The NC bank survives, but DOS must grant it a fresh timer lease.
        c.free()
        owner = c.read(1, c.k['timerOwner'], 1)
        assert owner != b'\x00'
        m.call('emu_reset')
        c.wait(core['ncBankInputPoll'], bank)
        assert c.read(0, c.b['biosBootWarm'], 1) == b'\x01'
        assert c.read(bank, core['ncInputState.timed'], 1) == b'\x01'
        assert c.read(1, c.k['timerOwner'], 1) == owner
        print('Warm Reset -> resident NC reacquires the keyboard timer PASS', flush=True)
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
