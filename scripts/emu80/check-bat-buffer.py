"""Потоковые BAT: длинный файл, вложенность, границы и ошибки; свой Emu80."""
import argparse
import json
import re
import runpy
import subprocess
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--report', type=Path, default=ROOT/'.local/bat-check')
    p.add_argument('--port', type=int, choices=range(19781, 19785), required=True)
    args = p.parse_args()
    out = args.report.resolve()
    out.mkdir(parents=True, exist_ok=True)
    h = runpy.run_path(str(ROOT/'source-apps/diskbench/check-emulator.py'))
    m = h['Bench'](f'http://127.0.0.1:{args.port}/mcp')
    s = h['symbols'](ROOT/'bin/banked/KERNEL.sym')
    exe = runpy.run_path(str(ROOT/'scripts/images/executable.py'))
    word = lambda n: struct.pack('<H', n)
    def invoke(operation, reads=None, **registers):
        stub = b'\xCD'+word(s['mx_callRegs'])+word(s[operation])+b'\xC3'+word(0x8E10)
        m.write(0x8E00, stub)
        m.stop_at(0x8E10)
        m.call('regs_set', pc=0x8E00, sp=0x8FDF, **registers)
        if reads is None:
            return m.until(0x8E10, 180, page=0)
        m.call('bp_set', addr=s['batRead'], type='exec')
        for _ in range(240):
            m.call('emu_run_for', ms=1000)
            state = m.call('cpu_state')
            pc = state['registers']['pc']['dec']
            page = m.read(s['bankPage'], 1)[0]
            if pc == 0x8E10 and page == 0:
                return state
            if pc in (0x8E10, s['batRead']):
                if pc == s['batRead'] and page == s['DOS_IO_PAGE']:
                    reads.append(struct.unpack('<HH', m.read(s['BAT_HANDLE']+s['FH_BUFFER'], 4)))
                m.call('debug_step', mode='into')
        raise RuntimeError(('BAT trace timed out', state))

    rows = []

    def dos_read(addr, count):
        m.write(0xFFFD, b'\0')
        result = m.read(addr, count)
        m.write(0xFFFC, b'\0')
        return result

    def create(name, data):
        assert not invoke('MXOS_DOS_FILE_SELECT_DRIVE', af=0x100, de=1)['flags']['C']
        stem, ext = name.split('.')
        obj = bytearray(64)
        obj[:11] = (stem.ljust(8)+ext.ljust(3)).encode('ascii')
        obj[40:42] = bytes((1, 15))
        m.write(0x100, obj)
        assert not invoke('MXOS_DOS_FILEHANDLE', af=0, hl=0x100)['flags']['C']
        for start in range(0, len(data), 8192):
            chunk = data[start:start+8192]
            m.write(0x2000, chunk)
            m.write(0x100+49, struct.pack('<HH', 0x2000, len(chunk)))
            assert not invoke('MXOS_DOS_FILEHANDLE', af=0x0200, hl=0x100)['flags']['C']
            assert m.read(0x100+51, 2) == struct.pack('<H', len(chunk))
        assert not invoke('MXOS_DOS_FILEHANDLE', af=0x0500, hl=0x100)['flags']['C']

    def execute(name, expected=0, count=0, reads=None):
        m.write(0x8800, b'\0')
        m.write(0x8500, ('B:\\'+name+'\0').encode('ascii'))
        state = invoke('MXOS_DOS_FILE_EXEC', hl=0x8500, reads=reads)
        got = state['registers']['af']['dec'] >> 8
        assert state['flags']['C'] == bool(expected), (name, state)
        if expected:
            assert got == expected, (name, got)
        assert m.read(0x8800, 1) == bytes((count,)), (name, m.read(0x8800, 1))
        assert dos_read(s['v_batDepth'], 1) == b'\0'
        assert dos_read(s['v_batFrame'], 2) == struct.pack('<H', s['BAT_BUFFER']-s['BAT_FRAME_SIZE'])
        assert state['registers']['sp']['dec'] == 0x8FDF
        assert m.read(s['bankPage'], 1) == b'\0'
        assert dos_read(s['BAT_HANDLE']+s['FH_STATE'], 1) == b'\0'
        assert dos_read(s['batWindowEnd'], 2) == word(0)
        rows.append(dict(file=name, error=expected, executed=count, bankAndStackPreserved=True))
        print('PASS', name, flush=True)

    # run-mcp-slot verifies the newly started PID owns the listener before returning.
    launch = subprocess.run(['powershell.exe', '-NoProfile', '-File',
        str(ROOT/'scripts/emu80/run-mcp-slot.ps1'), '-Port', str(args.port)],
        capture_output=True, check=True)
    pid = int(re.search(rb'\(PID (\d+)\)', launch.stdout).group(1))
    try:
        core = h['symbols'](ROOT/'bin/NC_CORE.sym')
        cli = h['symbols'](ROOT/'bin/CLI.sym')
        poll = core['ncBankInputPoll']
        def wait_nc():
            for _ in range(160):
                m.until(poll, 60)
                bank = m.read(s['bankPage'], 1)[0]
                if bank > s['DOS_IO_PAGE']:
                    return bank
                m.call('debug_step', mode='into')
            raise AssertionError('NC did not reach its keyboard poll in EXEC')
        m.stop_at(poll)
        wait_nc()
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        m.stop_at(cli['cliPrompt'])
        m.tap('f10')
        m.until(cli['cliPrompt'], 120, page=0)
        # Подготовить файл через файловый ABI, пока настоящая CLI остановлена
        # у приглашения. После подготовки вернуть только затронутые байты
        # приложения и его регистры. Затем EXIT штатно возвращает UI Commander.
        ui_file = b'@REM keyboard test\r\nMD B:/UIMARK'
        regs = m.call('cpu_state')['registers']
        scratch = [(at,m.read(at,n)) for at,n in
                   ((0x100,64),(0x2000,len(ui_file)),(0x8E00,0x1DF))]
        create('UITEST.BAT',ui_file)
        for at,data in scratch:
            m.write(at,data)
        m.call('bp_clear')
        m.call('regs_set', **{k:regs[k]['dec'] for k in ('af','bc','de','hl','pc','sp')})
        def type_line(command):
            m.call('bp_clear')
            m.call('emu_run_for', ms=500)
            m.tap('esc')
            m.call('emu_run_for', ms=250)
            for ch in command.lower():
                m.tap({':':'colon', '/':'slash', '.':'period'}.get(ch,ch))
        type_line('EXIT')
        assert m.read(cli['CMD_LINE'],4).upper()==b'EXIT'
        m.stop_at(poll)
        m.tap('enter')
        nc_page = wait_nc()
        command = 'C:/TESTS/DOSTEST.BAT'
        type_line(command)
        m.stop_at(poll)
        m.until(poll, 30, page=nc_page)
        assert m.read(core['CMD_LINE'],len(command)).upper() == command.encode()
        m.screenshot(out/'dos-test-command.png')
        m.stop_at(s['cliPauseWait'])
        m.tap('enter')
        m.until(s['cliPauseWait'],60,page=s['DOS_IO_PAGE'])
        frame = int.from_bytes(m.read(s['v_batFrame'],2),'little')
        assert m.read(frame,11)==b'DOSTEST BAT'
        assert m.read(frame+s['BAT_LINE'],2)==word(18)
        m.screenshot(out/'dos-test-first-pause.png')
        m.call('debug_step',mode='into')
        m.call('emu_run_for',ms=250)
        page = m.read(s['bankPage'],1)[0]
        line = m.read(frame+s['BAT_LINE'],2) if page==s['DOS_IO_PAGE'] else dos_read(frame+s['BAT_LINE'],2)
        assert line==word(18)
        m.tap('space')
        m.until(s['cliPauseWait'],60,page=s['DOS_IO_PAGE'])
        assert m.read(frame+s['BAT_LINE'],2)==word(23)
        m.screenshot(out/'dos-test-finished.png')
        m.stop_at(s['batDone'])
        m.call('emu_run_for',ms=250)
        m.tap('space')
        state = m.until(s['batDone'],60,page=s['DOS_IO_PAGE'])
        assert not state['flags']['C'], state
        rows.append(dict(case='C-DOSTEST',completedLines=23,pauses=2))
        print('PASS DOSTEST from C, two keyboard pauses',flush=True)
        m.stop_at(poll)
        nc_page = wait_nc()
        command = 'B:/UITEST.BAT'
        type_line(command)
        m.stop_at(poll)
        m.until(poll, 30, page=nc_page)
        assert m.read(core['CMD_LINE'],len(command)).upper() == command.encode()
        m.screenshot(out/'nc-command.png')
        m.stop_at(s['batDone'])
        m.tap('enter')
        state = m.until(s['batDone'],60,page=s['DOS_IO_PAGE'])
        frame = int.from_bytes(m.read(s['v_batFrame'],2),'little')
        assert not state['flags']['C'] and m.read(frame,11)==b'UITEST  BAT'
        assert m.read(frame+s['BAT_LINE'],2)==word(2)
        rows.append(dict(case='NC-keyboard-command',completedLines=2))
        print('PASS NC keyboard BAT',flush=True)
        m.stop_at(poll)
        wait_nc()
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        m.stop_at(cli['cliPrompt'])
        m.tap('f10')
        m.until(cli['cliPrompt'],120,page=0)
        create('MARK.COM', exe['encode'](exe['Image'](bytes.fromhex('21008834C9'), 0, 0, 4), 'com'))
        create('CHILD.BAT', b'B:\\MARK.COM\nB:\\MARK.COM\r')
        create('PARENT.BAT', b'@echo off\r\n'+b'REM pad\n'*20+b'B:\\MARK.COM\r\nB:\\CHILD.BAT\r\nB:\\MARK.COM')
        execute('PARENT.BAT', count=4)
        # Верхние 128 байтов образа DLL попадают в первое окно сценария.
        # LOAD.COM вызывает реальный MODULE_LOAD/UNLOAD. Покинутый сценарий
        # должен дочитать ровно оставшийся хвост даже после записи тела DLL.
        top = s['DOS_BANK_MODULE_END']
        def make_dll(name, start, end):
            image = exe['Image'](b'\xC9', start, start, 1, bss=end-start-1)
            create(name, exe['encode'](image, 'dll'))

        def module_call(name):
            return b'\xCD'+word(s['mx_callRegs'])+word(s['MXOS_DOS_MODULE_'+name])
        loader = (b'\x3E'+bytes([s['MODULE_TARGET_DOS']])+b'\x21'+word(0x20)
                  +module_call('LOAD')+b'\xD8'+module_call('UNLOAD')+b'\xC9')
        create('LOAD.COM', exe['encode'](exe['Image'](loader.ljust(0x20,b'\0')+b'B:/TOP.DLL\0',0,0,4),'com'))
        make_dll('TOP.DLL', top-128, top)
        content = b'B:/MARK.COM\r\nB:/LOAD.COM\r\n'+b'REM data\n'*35+b'B:/MARK.COM'
        create('EVICT.BAT', content)
        reads = []
        execute('EVICT.BAT', count=2, reads=reads)
        assert reads[0] == (top-len(content),len(content)), reads
        assert reads[1:] and all(r==(s['BAT_INPUT'],128) for r in reads[1:]), reads
        def load_dll(name):
            m.write(0x8500, name.encode()+b'\0')
            result = invoke('MXOS_DOS_MODULE_LOAD', hl=0x8500, af=s['MODULE_TARGET_DOS']<<8)
            assert not result['flags']['C'], result
            return result['registers']['hl']['dec']
        plain = b'REM '+b'x'*123+b'\r\n'+b'REM data\n'*35+b'B:/MARK.COM'
        create('PLAIN.BAT', plain)
        for name, start, expected in [('TOP.DLL',top-128, (top-128-len(plain),len(plain))),
                                       ('FULL.DLL',s['DOS_BANK_DRIVER_ADDR'], (s['BAT_INPUT'],128))]:
            if name=='FULL.DLL':
                make_dll(name,start,top)
            handle = load_dll('B:/'+name)
            reads = []
            execute('PLAIN.BAT', count=1, reads=reads)
            assert reads[0] == expected, reads
            if name=='TOP.DLL':
                assert reads == [expected], reads
            assert not invoke('MXOS_DOS_MODULE_UNLOAD',hl=handle)['flags']['C']
        # Штатный верхний установщик вытесняет окно BAT так же, как DLL.
        install = b'B:/MARK.COM\r\nC:/DRIVERS/AT2MXF.COM\r\n'+b'REM pad\n'*20+b'B:/MARK.COM'
        create('DRVTEST.BAT', install)
        reads = []
        execute('DRVTEST.BAT', count=2, reads=reads)
        assert reads[0] == (top-len(install),len(install)), reads
        assert reads[1:] and all(r==(s['BAT_INPUT'],128) for r in reads[1:]), reads
        # COPY перезаписывает общую область команды. Соседние строки переживают это.
        create('CPTEST.BAT',b'COPY B:/PLAIN.BAT B:/CLONE.BAT\r\nB:/CLONE.BAT')
        execute('CPTEST.BAT',count=1)
        for command in (b'MD B:/SUB\0', b'CD B:/SUB\0'):
            m.write(0x8500, command)
            assert not invoke('MXOS_DOS_FILE_EXEC', hl=0x8500)['flags']['C']
        create('LOCAL.BAT', b'B:\\MARK.COM\n')
        m.write(0x8500, b'CD B:/\0')
        assert not invoke('MXOS_DOS_FILE_EXEC', hl=0x8500)['flags']['C']
        execute('SUB/LOCAL.BAT', count=1)
        large = b'@echo off\n'+(b'REM '+b'x'*91+b'\r\n')*750+b'B:\\MARK.COM'
        assert len(large) > 65536
        create('LARGE.BAT', large)
        execute('LARGE.BAT', count=1)
        assert dos_read(s['BAT_BUFFER']+s['BAT_OFFSET'], 4) == struct.pack('<I', len(large)-len(b'B:\\MARK.COM'))
        create('EMPTY.BAT', b'')
        execute('EMPTY.BAT')
        create('EDGE.BAT', b'\n'*511+b'\r\nB:\\MARK.COM')
        execute('EDGE.BAT', count=1)
        create('MAXLINE.BAT', b'REM '+b'x'*123+b'\nB:\\MARK.COM')
        execute('MAXLINE.BAT', count=1)
        create('LONG.BAT', b'REM '+b'x'*124+b'\nB:\\MARK.COM')
        execute('LONG.BAT', expected=7)
        prefix = b'@echo off\r\nREM ok\r\n'
        create('ERROR.BAT', prefix+b'NoSuchCommand\r\nB:\\MARK.COM')
        execute('ERROR.BAT', expected=2)
        assert dos_read(s['BAT_BUFFER']+s['BAT_LINE'], 2) == struct.pack('<H', 3)
        assert dos_read(s['BAT_BUFFER']+s['BAT_OFFSET'], 4) == struct.pack('<I', len(prefix))
        m.screenshot(out/'line-error.png')
        create('NUL.BAT', b'REM x\0\nB:\\MARK.COM')
        execute('NUL.BAT', expected=7)
        create('KOI.BAT', b'REM \xFF\nB:\\MARK.COM')
        execute('KOI.BAT', count=1)
        target = 'MARK.COM'
        for depth in range(1, s['BAT_MAX_DEPTH']+1):
            name = f'DEPTH{depth}.BAT'
            create(name, ('B:/'+target).encode())
            target = name
        execute(target, count=1)
        create('SELF.BAT', b'B:\\SELF.BAT\n')
        execute('SELF.BAT', expected=7)
        assert dos_read(s['BAT_BUFFER']+s['BAT_LINE'],2)==word(1)
        create('CYCLEA.BAT',b'B:/CYCLEB.BAT')
        create('CYCLEB.BAT',b'REM nested\r\nB:/CYCLEA.BAT')
        execute('CYCLEA.BAT',expected=7)
        assert dos_read(s['BAT_BUFFER']+s['BAT_FRAME_SIZE']+s['BAT_LINE'],2)==word(2)
        execute('PARENT.BAT', count=4)
        create('DELSELF.BAT',b'DEL B:/DELSELF.BAT\r\nB:/MARK.COM')
        execute('DELSELF.BAT',expected=6)
        execute('DELSELF.BAT',expected=6)  # Запись и содержимое остались доступны.
        create('DELCHILD.BAT',b'DEL B:/DELPAR.BAT')
        create('DELPAR.BAT',b'B:/DELCHILD.BAT\r\nB:/MARK.COM')
        execute('DELPAR.BAT',expected=6)
        execute('DELPAR.BAT',expected=6)
        create('GONE.BAT',b'B:/MARK.COM')
        create('BEFORE.BAT',b'DEL B:/GONE.BAT\r\nB:/GONE.BAT')
        execute('BEFORE.BAT',expected=2)
        create('AFTER.BAT',b'B:/CHILD.BAT\r\nDEL B:/CHILD.BAT\r\nB:/MARK.COM')
        execute('AFTER.BAT',count=3)
        create('MISSING.BAT',b'B:/CHILD.BAT')
        execute('MISSING.BAT',expected=2)
        (out/'result.json').write_text(json.dumps(rows, indent=2)+'\n', encoding='utf-8')
    finally:
        subprocess.run(['powershell.exe', '-NoProfile', '-Command',
            f"$p=Get-Process -Id {pid} -ErrorAction SilentlyContinue; "
            f"if ($p -and $p.Path -eq (Join-Path $env:LOCALAPPDATA 'Specialist-OS/emu80/{args.port}/Emu80qt-mcp.exe')) "
            "{ Stop-Process -InputObject $p }"], check=True)


if __name__ == '__main__':
    main()
