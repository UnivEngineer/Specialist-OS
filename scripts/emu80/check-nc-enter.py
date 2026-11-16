"""NC Enter: unsupported files preserve panels; associations remain launchable."""
import json
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'docs/nc/enter-2026-11-13'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    symbols = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-sort.py'))['symbols']
    runtime = ROOT/'.local/nc-sort-benchmark-runtime'
    h['Matrix'].__init__.__globals__.update(RUNTIME=runtime,OUT=OUT)
    machine = h['Matrix']()
    n = symbols(ROOT/'bin/NC.sym')
    results = []
    try:
        machine.start('nc-enter',disk=ROOT/'images/FlashDisk_emu80_seed.bin')
        m = machine.m
        m.call('debug_break')
        if m.read(n['state.activePanel'],1)!=b'\0':
            m.call('bp_clear');m.call('emu_run_for',ms=500);m.tap('tab')

        def select(name):
            m.call('bp_clear');m.call('emu_run_for',ms=500)
            m.call('debug_break')
            assert m.read(n['state.activePanel'],1)==b'\0'
            slot = m.read(n['vars.cacheSlotA'],1)[0]
            count = m.read(n['state.panelA_filesCnt'],1)[0]
            data = m.read(n[f'FILE_CACHE_{"ABC"[slot]}'],count*16)
            indices = m.read(n['FILE_INDEX_A'],count)
            at = next(i for i,k in enumerate(indices) if data[k*16:k*16+11].upper()==name.upper())
            current = m.read(n['state.panelA_curFile'],1)[0]
            m.call('emu_run_for',ms=100)
            for _ in range(abs(at-current)):
                m.tap('down' if at>current else 'up')
            m.call('emu_run_for',ms=250)
            m.call('debug_break')
            assert m.read(n['state.panelA_curFile'],1)[0]==at

        def enter(stop, forbidden=()):
            m.call('bp_clear');m.call('emu_run_for',ms=500)
            m.call('bp_set',addr=stop,type='exec')
            for address in forbidden:
                m.call('bp_set',addr=address,type='exec')
            m.tap('enter')
            for _ in range(30):
                state = m.call('cpu_state');pc=state['registers']['pc']['dec']
                if pc in forbidden:
                    m.screenshot(OUT/'unexpected.png')
                    raise AssertionError(('unexpected path',state,m.read(n['vars.cmdLine'],128)))
                if pc==stop:
                    return
                m.call('emu_run_for',ms=500)
            raise RuntimeError(m.call('cpu_state'))

        def bitmap():
            return b''.join(m.read(0x9000+x*256,228) for x in range(48))

        def ignored(name):
            select(name)
            for attempt in range(3):
                before = bitmap()
                command_end = m.read(n['vars.cmdLineEnd'],2)
                enter(n['mainLoop_cursorReady'],(n['EnterCmdLine'],n['EnterExec'],n['drawCursor']))
                assert bitmap()==before,('panels changed',name,attempt)
                assert m.read(n['vars.cmdLineEnd'],2)==command_end
                assert m.read(n['vars.cmdLine'],1)==b'\0'
            m.screenshot(OUT/(name.decode().strip().replace(' ','_')+'.png'))
            results.append(dict(name=name.decode(),ignored=True,repeats=3,panelsUnchanged=True))

        ignored(b'FONT    FNT')
        ignored(b'CLI     EXT')
        select(b'BENCH      ');enter(n['mainLoop'])
        select(b'DISKBEN    ');enter(n['mainLoop'])
        ignored(b'DISKBANKDLL')
        select(b'\\          ');enter(n['mainLoop'])
        select(b'README  TXT')
        enter(n['EnterExec'],(n['ncCommandError'],))
        command = m.read(n['vars.cmdLine'],128).split(b'\0')[0]
        assert command.upper()==b'A:\\README.TXT',command
        results.append(dict(name='README.TXT',registered=True,launchCommand=command.decode()))

        # Separate ABI check for native extensions and case folding. All UI
        # scenarios above use the normal NC keyboard/selection path.
        for ext,allowed in [('COM',True),('com',True),('BAT',True),('bat',True),
                            ('EXE',True),('exe',True),('TXT',True),('RKS',True),
                            ('DLL',False),('dll',False),('FNT',False),('ZZZ',False)]:
            m.write(0x7500,b'TEST    '+ext.encode()+bytes(5))
            code = (b'\x21\x00\x75\xcd'+n['ncEnterFileAction'].to_bytes(2,'little')+
                    b'\xf5\xe1\x22\x00\x76\xc3\xff\x81')
            m.write(0x8100,code);m.stop_at(0x81ff)
            m.call('regs_set',pc=0x8100,sp=0x8b00)
            m.until(0x81ff,60)
            carry = bool(m.read(0x7600,1)[0]&1)
            assert carry != allowed,(ext,carry)
        (OUT/'result.json').write_text(json.dumps(dict(ui=results,extensionCases=12),indent=2)+'\n')
        print('NC ENTER PASS: FNT/EXT/DLL x3, exact panel bitmaps, TXT association and 12 extension cases.')
    finally:
        machine.stop()


if __name__=='__main__':
    main()
