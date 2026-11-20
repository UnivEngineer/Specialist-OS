"""Exercise IO.SYS keyboard entries installed by bootstrap and by MODULE_LOAD."""
import argparse
import re
import runpy
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
word = lambda n: struct.pack('<H', n)
parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument('--runtime', type=Path, required=True)
parser.add_argument('--port', type=int, choices=range(19781, 19785), required=True)
args = parser.parse_args()
matrix = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
context = matrix['Matrix'].__init__.__globals__
output = ROOT/'.local/local-keyboard-test'
output.mkdir(parents=True, exist_ok=True)
context.update(OUT=output, RUNTIME=args.runtime.resolve())
context['mcp_owner'] = lambda: matrix['shell'](
    f'$p=Get-NetTCPConnection -State Listen -LocalPort {args.port} -ErrorAction SilentlyContinue;'
    'if($p){$p[0].OwningProcess};exit 0')
test = matrix['Matrix']()
m = test.m
m.url = f'http://127.0.0.1:{args.port}/mcp'
symbols = matrix['symbols']
k = {name:int(value,16) for name,value in re.findall(r'^([\w.]+): EQU 0x([\da-fA-F]+)', (ROOT/'bin/banked/KERNEL.sym').read_text(), re.M)}
t = symbols(ROOT/'bin/banked/TRANSPORT.sym')
driver = (ROOT/'bin/banked/KEYBANK.bin').read_bytes()
bios = symbols(ROOT/'bin/BIOS.sym')
key = symbols(ROOT/'bin/banked/KEYBANK.sym')
api = runpy.run_path(str(ROOT/'scripts/emu80/file-api.py'))

def page(n):
    m.write(0xfffd if n else 0xfffc, bytes([n-1 if n else 0]))

def check_keys(bank, timed=False):
    page(bank)
    assert m.read(k['IO_BANK_KEYS'], len(driver)) == driver
    state, stub, end, stack = 0x78, 0x90, 0x98, 0xef
    original = m.read(state, 0x70)
    hook = m.read(k['MEM_QUEUE_HOOK'], 2)
    gate = m.read(t['bankQueueBefore'], 1)
    window = m.read(0xffc0,32)
    def call(name, waiting=False):
        m.write(stub, b'\xcd'+word(k[name])+b'\xc3'+word(end))
        m.stop_at(end)
        m.call('regs_set', pc=stub, sp=stack, hl=state, bc=0x2345, de=0x6789)
        if waiting:
            m.call('emu_run_for',ms=10)
            pc=m.call('cpu_state')['registers']['pc']['dec']
            assert k['IO_BANK_KEYS'] <= pc < k['MEM_IMAGE_LOCAL_START'],hex(pc)
            press('enter')
        regs = m.until(end, 5, page=bank)['registers']
        assert [regs[r]['dec'] for r in ('sp','hl','bc','de')] == [stack,state,0x2345,0x6789]
        return regs['af']['dec']
    def press(key): m.call('kbd_send', keys=[key], action='press')
    def release(key): m.call('kbd_send', keys=[key], action='release')
    def poll(): return call('io_bankKeyPoll')
    def arm_cursor():
        m.write(stub,b'\xcd'+word(k['io_bankGetChCursor'])+b'\xc3'+word(end))
        m.call('regs_set',pc=stub,sp=stack,hl=state,bc=0x2345,de=0x6789)
    def idle(): assert poll() & 0x41 == 0x40
    m.write(state, bytes([255])+bytes(k['IO_KEY_STATE']-1))
    m.write(state+k['IO_KEY_STATE.timed'],bytes([timed]))
    # A bound queue callback would change the guard byte at 008Ah.
    m.write(0x8a,bytes.fromhex('00 21 8a 00 34 C9'))
    m.write(k['MEM_QUEUE_HOOK'],word(0x8b))
    m.write(t['bankQueueBefore'],b'\xc3')
    try:
        # Enter shares the Ctrl row but has its own column. Reading
        # modifiers with it held must neither report spurious Ctrl nor consume Enter.
        initial_state = m.read(state,k['IO_KEY_STATE'])
        def modifiers(expected):
            for entry,mask in [('io_bankKeyShift',1),('io_bankKeyModifiers',3)]:
                af = call(entry)
                value = expected & mask
                assert af >> 8 == value and af & 0x41 == (0 if value else 0x40), (entry,expected,hex(af))
        for ordinary in (False,True):
            if ordinary: press('enter')
            modifiers(0)
            press('rpt'); modifiers(2)
            press('shift'); modifiers(3)
            release('rpt'); modifiers(1)
            release('shift'); modifiers(0)
            assert m.read(state,k['IO_KEY_STATE'])==initial_state and m.read(0xffc0,32)==window
            if ordinary:
                assert poll() & 0xff41 == 0x0d00
                release('enter'); idle()
        window = m.read(0xffc0,32)
        print('PASS local Shift/Ctrl mask with and without Enter; bank',bank,flush=True)
        idle()
        assert call('io_bankKeyScan') >> 8 == 255
        assert m.read(0xffc0,32)==window
        event = call('io_bankGetCh', waiting=True)
        pressed_at = m.call('cpu_state')['cpu_ticks']
        assert event >> 8 == 13 and not event & 0x41,hex(event)
        if timed:
            def countdown():
                m.write(k['IO_TIMER_MODE'],bytes([0x80]))
                return int.from_bytes(m.read(k['IO_TIMER_CH2'],1)+m.read(k['IO_TIMER_CH2'],1),'little')
            before=countdown()
            m.write(end,b'\xc3'+word(end))
            m.call('bp_clear')
            m.call('emu_run_for',ms=12)
            after=countdown()
            assert 0 < before <= k['KEY_CLICK_TICKS'] and after > 0xff00,(before,after)
            print('PASS nonblocking one-shot across terminal count while application runs',flush=True)
        held_window=m.read(0xffc0,32)
        idle()
        assert m.read(0xffc0,32)==held_window
        if timed:
            def wait_repeat():
                start=m.call('cpu_state')['cpu_ticks']
                assert call('io_bankGetCh') >> 8 == 13
                return (m.call('cpu_state')['cpu_ticks']-start)/2000
            wait_repeat()
            first = (m.call('cpu_state')['cpu_ticks']-pressed_at)/2000
            following = wait_repeat()
            assert 300 < first < 440 and 32 < following < 100, (first,following)
            print('PASS PIT first/repeat %.1f/%.1f ms' % (first, following),flush=True)
        else:
            m.write(state+k['IO_KEY_STATE.repeatCount'],bytes([0]))
            assert poll() >> 8 == 13
        release('enter'); idle()
        press('rpt')
        idle()
        ctrl_window = m.read(0xffc0,32)
        idle()
        assert m.read(0xffc0,32)==ctrl_window
        press('enter')
        assert poll() & 0xff41 == 0x0d01
        release('enter'); release('rpt'); idle()
        press('shift'); press('1')
        assert poll() >> 8 == ord('!')
        release('1'); release('shift')
        press('enter')       # A new switch replaces the held one between polls.
        assert poll() >> 8 == 13
        release('enter'); idle()
        press('a')
        raw = call('io_bankKeyScan') >> 8
        assert poll() >> 8 == raw ^ 0x20
        release('a'); idle()
        press('caps'); idle(); idle()
        assert m.read(state+2,1)==b'\x01'
        release('caps'); idle()
        press('a')
        assert poll() >> 8 == (raw ^ 0x20) + 0x80
        release('a'); idle()
        m.write(state+3,b'\xff'); press('caps'); idle()
        assert m.read(state+2,1)==b'\x81'
        release('caps'); idle()
        press('f1')
        event=poll()
        assert event >> 8 == 0 and not event & 0x40,hex(event)
        release('f1'); idle()
        # Observe the real XOR in video RAM after each return from MAIN.
        page(0)
        cfg = next(int(line.split()[-1],16) for line in (ROOT/'bin/BIOS.sym').read_text().splitlines() if line.startswith('bios_upper.cursorCfg:'))
        saved_cfg = m.read(cfg,1)
        m.write(cfg,bytes([0x21]))
        original_pixels = m.read(0x9000,0x3000)
        page(bank)
        arm_cursor()
        edges = []
        for visible in (1,0,1):
            m.stop_at(bios['bankCursorSet'])
            edges.append(m.until(bios['bankCursorSet'],5,page=0)['cpu_ticks'])
            m.stop_at(key['keyCursorWait'])
            m.until(key['keyCursorWait'],5,page=bank)
            page(0)
            assert m.read(bios['bankCaretVisible'],1)==bytes([visible])
            assert (m.read(0x9000,0x3000)!=original_pixels)==bool(visible)
            page(bank)
        press('enter')
        m.stop_at(end)
        regs = m.until(end,5,page=bank)['registers']
        assert regs['af']['dec'] & 0xff41 == 0x0d00
        assert [regs[r]['dec'] for r in ('sp','hl','bc','de')] == [stack,state,0x2345,0x6789]
        release('enter'); idle()
        page(0)
        assert m.read(bios['bankCaretVisible'],1)==b'\0'
        assert m.read(0x9000,0x3000)==original_pixels
        for disabled in (0x20,0xa1):
            m.write(cfg,bytes([disabled]))
            page(bank)
            arm_cursor()
            m.stop_at(key['keyCursorOff'])
            m.until(key['keyCursorOff'],5,page=bank)
            m.stop_at(bios['bankCursorSet'])
            assert not m.call('emu_run_for',ms=600).get('stoppedByBreakpoint')
            press('enter')
            m.stop_at(end)
            assert m.until(end,5,page=bank)['registers']['af']['dec'] >> 8 == 13
            release('enter'); idle()
            page(0)
            assert m.read(0x9000,0x3000)==original_pixels
        m.write(cfg,saved_cfg)
        page(bank)
        if timed:
            assert all(450 < (b-a)/2000 < 535 for a,b in zip(edges,edges[1:])),edges
        print('PASS BIOS caret show/hide/key, interval %.1f ms' % ((edges[1]-edges[0])/2000),flush=True)
        assert m.read(0x8a,1)==b'\0'
        print('PASS local keyboard, blocked wait, modifiers, repeat, F1, locks, queue and window; bank',bank,flush=True)
    finally:
        m.write(t['bankQueueBefore'],gate)
        m.write(k['MEM_QUEUE_HOOK'],hook)
        m.write(state,original)

try:
    test.start('local keyboard',system=ROOT/'images/SystemRom_emu80_writable.bin')
    m.call('debug_break'); m.call('bp_clear')
    check_keys(1)
    page(0)
    fmt=runpy.run_path(str(ROOT/'scripts/images/executable.py'))
    image=fmt['encode'](fmt['Image'](b'\xcd'+word(k['mx_callRegs'])+word(k['MXOS_DOS_TIMER'])+b'\xc9',0x6000,0x6000,k['IMAGE_FLAG_BANK']),'dll')
    call=lambda op,hl=0,a=0: api['main_call'](m,k,op,hl,a)
    m.write(0x3000,b'B:/KEYTEST.DLL\0'+bytes(114)); m.write(0x4800,image)
    m.write(0x3200,word(0x3000)+word(0x4800)+word(len(image))+bytes(34))
    api['path_io'](m,call,k,0x3200,True)
    call(k['MXOS_DOS_MODULE_LOAD'],0x3000,k['MODULE_TARGET_BANK'])
    handle=m.read(0x2f80,2)
    page(1)
    records=m.read(k['MODULE_TABLE'],k['MODULE_MAX']*k['MODULE_RECORD_SIZE'])
    bank=next(records[i+3] for i in range(0,len(records),k['MODULE_RECORD_SIZE']) if records[i:i+2]==handle)
    assert bank>=2
    page(0)
    call(k['MXOS_DOS_TIMER'], a=k['TIMER_CLOCK'])
    assert call(k['MXOS_DOS_TIMER'], a=k['TIMER_QUERY'])['registers']['af']['dec'] >> 8 == 1
    def module_timer(op):
        m.write(0x3100,struct.pack('<4H',0,0,0,(op<<8)|2))
        m.write(0x2f80,struct.pack('<4H',int.from_bytes(handle,'little'),0x3100,0,2))
        m.stop_at(0x8b00)
        m.call('regs_set',pc=0x2800,sp=0x8fdf,bc=k['MXOS_DOS_MODULE_CALL'],de=0x2f80)
        regs=m.until(0x8b00,10,page=0)['registers']
        assert not regs['af']['dec'] & 1
        return int.from_bytes(m.read(0x3106,2),'little')
    assert module_timer(k['TIMER_QUERY']) >> 8 == 2
    for op in ('TIMER_RELEASE','TIMER_CLOCK','TIMER_RAW'):
        assert module_timer(k[op]) & 1, op
    call(k['MXOS_DOS_TIMER'], a=k['TIMER_RELEASE'])
    assert call(k['MXOS_DOS_TIMER'], a=k['TIMER_QUERY'])['registers']['af']['dec'] >> 8 == 0
    assert not module_timer(k['TIMER_CLOCK']) & 1
    check_keys(bank, timed=True)
    page(0)
    assert not module_timer(k['TIMER_RELEASE']) & 1
    assert not module_timer(k['TIMER_RAW']) & 1
    assert module_timer(k['TIMER_RAW']) & 1
    call(k['MXOS_DOS_MODULE_UNLOAD'],int.from_bytes(handle,'little'))
    assert call(k['MXOS_DOS_TIMER'],a=k['TIMER_QUERY'])['registers']['af']['dec'] >> 8 == 0
    print('PASS exclusive lease, foreign release denied, unload releases owner',flush=True)
    # A legacy game owns its own carrier and clock. Force a fresh C803h
    # press, observe the software beep, then compare the live counters.
    call(k['MXOS_DOS_TIMER'], a=k['TIMER_RAW'])
    for channel, divisor in enumerate((1001,4096,0)):
        m.write(k['IO_TIMER_MODE'],bytes([0x34+64*channel]))
        for part in word(divisor):
            m.write(k['IO_TIMER_CH0']+channel,bytes([part]))
    def latched(channel):
        m.write(k['IO_TIMER_MODE'],bytes([channel*64]))
        port=k['IO_TIMER_CH0']+channel
        return int.from_bytes(m.read(port,1)+m.read(port,1),'little')
    m.write(0x2800,b'\xcd'+word(k['bios_getch'])+b'\xc3'+word(0x8b00))
    m.stop_at(bios['loc_C23C'])
    m.call('regs_set',pc=0x2800,sp=0x8fdf)
    start=m.until(bios['loc_C23C'],10,page=0)['cpu_ticks']
    carrier, count=latched(0), latched(2)
    m.stop_at(bios['beep'])
    m.call('kbd_send',keys=['enter'],action='press')
    m.until(bios['beep'],5,page=0)
    m.stop_at(0x8b00)
    finish=m.until(0x8b00,5,page=0)
    m.call('kbd_send',keys=['enter'],action='release')
    elapsed=finish['cpu_ticks']-start
    assert finish['registers']['af']['dec'] >> 8 == 13
    assert abs(((count-latched(2))&0xffff)-elapsed/4096)<2
    phase=(carrier-latched(0)-elapsed)%1001
    assert min(phase,1001-phase)<=2,phase
    call(k['MXOS_DOS_TIMER'],a=k['TIMER_RELEASE'])
    print('PASS C803h software click preserves game carrier and clock',flush=True)
    page(1); m.write(k['IO_BANK_KEYS'],bytes(3))
    page(0)
    boot = symbols(ROOT/'bin/BIOS.sym')['biosTryShell']
    m.stop_at(boot); m.call('emu_reset'); m.until(boot,120,page=0)
    page(1); assert m.read(k['IO_BANK_KEYS'],len(driver)) == driver
    print('PASS Reset reinstalls keyboard segment',flush=True)
finally:
    test.stop()
