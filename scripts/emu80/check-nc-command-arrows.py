"""NC prompt: Up/Down move the panel and Ctrl+Enter appends multiple paths."""
import json
from pathlib import Path
import runpy

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'docs/nc/command-arrows-2026-11-14'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    symbols = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-nc-sort.py'))['symbols']
    h['Matrix'].__init__.__globals__.update(RUNTIME=ROOT/'.local/nc-sort-benchmark-runtime',OUT=OUT)
    machine = h['Matrix']()
    n = symbols(ROOT/'bin/NC.sym')
    try:
        machine.start('nc-command-arrows',disk=ROOT/'images/H-Flash-2M-Games.bin')
        m = machine.m
        m.call('bp_clear');m.call('emu_run_for',ms=500)

        def byte(name):
            return m.read(n[name],1)[0]

        def command():
            end = int.from_bytes(m.read(n['vars.cmdLineEnd'],2),'little')
            return m.read(n['vars.cmdLine'],end-n['vars.cmdLine'])

        def ctrl_enter():
            m.call('kbd_send',keys=['rpt'],action='press')
            try:
                m.tap('enter')
            finally:
                m.call('kbd_send',keys=['rpt'],action='release')
            m.call('emu_run_for',ms=250)

        if byte('state.activePanel')==0:
            m.tap('tab')
        m.tap('esc');m.tap('home')
        assert byte('state.panelB_curFile')==0
        steps = n['FILE_LIST_SIZE']+2
        assert byte('state.panelB_filesCnt')>steps
        for key in ('e','c','h','o','space'):
            m.tap(key)
        original = command()
        assert original.upper()==b'ECHO '
        # Leave the text cursor inside the command while crossing a scroll edge.
        m.tap('left');m.tap('left')
        position = byte('lineEditPosition')
        assert position==3
        for _ in range(steps):
            m.tap('down')
        assert byte('state.panelB_curFile')==steps
        assert byte('state.panelB_firstFile')>0
        assert command()==original
        assert byte('lineEditPosition')==position
        m.screenshot(OUT/'scrolled-command.png')
        for _ in range(steps):
            m.tap('up')
        assert byte('state.panelB_curFile')==0
        assert byte('state.panelB_firstFile')==0
        assert command()==original
        assert byte('lineEditPosition')==position
        m.tap('end')
        ctrl_enter()
        first = command()
        assert first.upper().startswith(b'ECHO H:\\') and len(first)>len(original),first
        m.tap('space');m.tap('down');ctrl_enter()
        combined = command()
        assert combined.startswith(first+b' '),combined
        second = combined[len(first)+1:]
        assert second.upper().startswith(b'H:\\') and second!=first[len(original):],combined
        m.screenshot(OUT/'two-selected-paths.png')
        # Empty prompt keeps normal panel navigation too.
        m.tap('esc');m.tap('down');m.tap('up')
        assert command()==b''
        assert byte('state.panelB_curFile')==1
        (OUT/'result.json').write_text(json.dumps(dict(scrollSteps=steps,
            commandPreserved=True,textCursorPreserved=True,
            ctrlEnterCommand=combined.decode(),emptyPrompt=True),indent=2)+'\n',encoding='utf-8')
        print('NC COMMAND ARROWS PASS: scrolling both ways, text/cursor preserved, two Ctrl+Enter paths.')
    finally:
        machine.stop()


if __name__=='__main__':
    main()
