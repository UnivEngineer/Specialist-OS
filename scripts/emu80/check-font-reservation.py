"""Проверка резерва 6x8/Fast при запуске FONT из NC_MN, CLI и банкового NC."""
import argparse
import json
import os
from pathlib import Path
import re
import runpy
import struct

ROOT = Path(__file__).resolve().parents[2]


def check(m, out):
    def symbols(path):
        return {key: int(value, 16) for key, value in re.findall(
            r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)', path.read_text('utf-8-sig'), re.M)}
    kernel = symbols(ROOT/'bin/banked/KERNEL.sym')
    bios = symbols(ROOT/'bin/BIOS.sym')
    cli = symbols(ROOT/'bin/CLI.sym')
    core = symbols(ROOT/'bin/NC_CORE.sym')
    main = symbols(ROOT/'bin/NC.sym')
    font = symbols(ROOT/'bin/FONT.sym')
    fast = (ROOT/'bin/fonts/Fast12K.pre').read_bytes()
    images = {name: (ROOT/'bin'/f'{name}.i80').read_bytes()[:12]
              for name in ('NC', 'CLI', 'FONT')}
    rows = []

    def bank():
        return m.read(kernel['bankPage'], 1)[0]

    def mapped(page):
        m.write(0xfffd if page else 0xfffc, bytes([page-1 if page else 0]))

    def memory():
        previous = bank()
        mapped(1)
        try:
            slot = struct.unpack('<HH', m.read(kernel['mrSlots']+4*kernel['MEM_RESIDENT_FONT'], 4))
            modules = m.read(kernel['MODULE_TABLE'], kernel['MODULE_MAX']*kernel['MODULE_RECORD_SIZE'])
            allocations = m.read(kernel['MEM_ALLOC_TABLE'], kernel['MEM_ALLOC_RECORDS']*6)
        finally:
            mapped(previous)
        return dict(slot=slot, modules=modules.hex(), allocations=allocations.hex())

    def wait(points, key=None):
        m.call('bp_clear')
        for pc in points:
            m.call('bp_set', addr=pc, type='exec')
        if key:
            m.tap(key)
        for _ in range(320):
            state = m.call('cpu_state')
            pc = state['registers']['pc']['dec']
            if pc in points:
                page = bank()
                name = points[pc]
                if (name == 'BANK' and page > 1) or (name in images and page == 0 and m.read(0, 12) == images[name]):
                    return pc, state
                m.call('debug_step', mode='into')
            else:
                m.call('emu_run_for', ms=1000)
        raise RuntimeError((points, state, memory()))

    def shell(name, key=None):
        target = {'BANK': core['ncBankInputPoll'], 'NC': main['mainLoop'], 'CLI': cli['cliPrompt']}[name]
        wait({target: name}, key)

    def type_line(line):
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        m.tap('esc')
        m.call('emu_run_for', ms=250)
        for char in line.lower():
            if char == '_':
                m.call('kbd_send', keys=['shift'], action='press')
                try:
                    m.tap('0')
                finally:
                    m.call('kbd_send', keys=['shift'], action='release')
            else:
                m.tap({' ': 'space', '/': 'slash', ':': 'colon', '.': 'period'}.get(char, char))
        actual = m.read(cli['CMD_LINE'], len(line))
        assert actual.upper() == line.upper().encode(), (line, actual)
        m.screenshot(out/f'{len(rows)}-command.png')

    def verify(label, use_fast):
        state = memory()
        assert state['slot'] == ((0x6400, len(fast)) if use_fast else (0, 0)), (label, state)
        previous = bank()
        mapped(0)
        try:
            if use_fast:
                assert m.read(0x6400, len(fast)) == fast, label
            state['mode'] = m.read(bios['FONT_ENGINE_HEADER']+7, 1)[0]
            state['memTop'] = int.from_bytes(m.read(bios['v_memTop'], 2), 'little')
        finally:
            mapped(previous)
        assert state['mode'] == (2 if use_fast else 0), (label, state)
        if use_fast:
            assert state['memTop'] < 0x6400, (label, state)
        m.screenshot(out/f'{len(rows)}-result.png')
        rows.append(dict(case=label, **state))
        (out/'result.json').write_text(json.dumps(rows, indent=2)+'\n', 'utf-8')
        print('PASS', label, state['slot'], flush=True)

    def finish_install(label, use_fast, target):
        pc, state = wait({font['committed']: 'FONT', font['error']: 'FONT'}, 'enter')
        assert pc == font['committed'], (label, state['registers']['af']['hex'], memory())
        shell(target)
        verify(label, use_fast)

    def install(label, use_fast, target):
        type_line('FONT 6x8/'+('Fast' if use_fast else 'Rotate'))
        finish_install(label, use_fast, target)

    def panel_fast():
        type_line('CD C:/FONTS/6x8')
        m.tap('enter')
        m.call('emu_run_for', ms=1500)
        shell('BANK')
        panel = m.read(core['state.activePanel'], 1)[0]
        path = core['vars.pathB' if panel else 'vars.pathA']
        assert m.read(path, len(b'C:\\FONTS\\6X8')).upper() == b'C:\\FONTS\\6X8'
        count = m.read(core['state.panelA_filesCnt']+panel, 1)[0]
        slot = m.read(core['vars.cacheSlotA']+panel, 1)[0]
        base = int.from_bytes(m.read(core['ncCatalogBases']+2*slot, 2), 'little')
        order = m.read(core['FILE_INDEX_A']+256*panel, count)
        names = [m.read(base+16*i, 11).upper() for i in order]
        index = names.index(b'FAST    FNT')
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        m.tap('home')
        for _ in range(index):
            m.tap('down')
        shell('BANK')
        assert m.read(core['state.panelA_curFile']+panel, 1)[0] == index
        m.screenshot(out/'bank-panel-fast-selected.png')
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        finish_install('BANK-panel-Enter-Fast', True, 'BANK')

    install('MAIN-Fast-CLI', True, 'CLI')
    type_line('C:/DOS/NC.COM')
    shell('BANK', 'enter')
    verify('CLI-Fast-return-to-banked-NC', True)
    install('BANK-Rotate', False, 'BANK')
    install('BANK-Fast', True, 'BANK')
    install('BANK-Fast-to-Rotate', False, 'BANK')
    panel_fast()
    install('BANK-panel-Fast-to-Rotate', False, 'BANK')
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    shell('CLI', 'f10')
    install('CLI-Fast', True, 'CLI')
    install('CLI-Rotate', False, 'CLI')
    type_line('C:/DOS/NC_MN.COM')
    shell('NC', 'enter')
    verify('Rotate-return-to-MAIN', False)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--port', type=int, choices=range(19781, 19797), default=19781)
    parser.add_argument('--report', type=Path, default=ROOT/'.local/font-reservation')
    args = parser.parse_args()
    out = args.report.resolve()
    out.mkdir(parents=True, exist_ok=True)
    bench = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    runtime = Path(os.environ['LOCALAPPDATA'])/f'Specialist-OS/emu80/{args.port}'
    def owner():
        return bench['shell'](f'$p=Get-NetTCPConnection -LocalPort {args.port} -State Listen '
                              f'-ErrorAction SilentlyContinue; if($p){{$p[0].OwningProcess}};exit 0')
    bench['Matrix'].__init__.__globals__.update(OUT=out, RUNTIME=runtime, mcp_owner=owner)
    matrix = bench['Matrix']()
    matrix.m.url = f'http://127.0.0.1:{args.port}/mcp'
    try:
        matrix.start('font reservation', system=ROOT/'images/C-Flash-512k-MMX-System-Emu80.bin')
        check(matrix.m, out)
    except Exception:
        if matrix.pid:
            matrix.m.screenshot(out/'failure.png')
        raise
    finally:
        matrix.stop()


if __name__ == '__main__':
    main()
