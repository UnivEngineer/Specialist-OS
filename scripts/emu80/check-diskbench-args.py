"""Запуск DISKBEN с аргументами через клавиатуру NC и пропущенные вопросы."""
from pathlib import Path
import argparse
import json
import runpy
import urllib.request

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, default=ROOT/'docs/diskbench/args-001/ui')
    args = parser.parse_args()
    out = args.report.resolve()
    out.mkdir(parents=True, exist_ok=True)
    h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
    h['Matrix'].__init__.__globals__.update(OUT=out, RUNTIME=ROOT/'.local/monitor2-001')
    x = h['Matrix']()
    m = x.m
    s = x.s
    nc = h['symbols'](ROOT/'bin/NC.sym')
    binary = (ROOT/'bin/DISKBEN.i80').read_bytes()
    questions = ('menu_drive','menu_verify','menu_writes','menu_size','prepare','argument_error')
    rows = []

    def expect(label):
        if m.call('cpu_state')['registers']['pc']['dec'] == s[label]:
            return
        m.call('bp_clear')
        for name in questions:
            m.call('bp_set', addr=s[name], type='exec')
        # Не ловить повторно текущий breakpoint перед продолжением.
        state = m.call('cpu_state')
        if state['registers']['pc']['dec'] in [s[n] for n in questions]:
            m.call('debug_step', mode='into')
        m.call('emu_run_for', ms=5000)
        state = m.call('cpu_state')
        assert state['registers']['pc']['dec'] == s[label], (label,state)

    def launch(command, name):
        m.call('bp_clear')
        m.call('emu_run_for', ms=500)
        m.tap('esc')
        keys = {' ': 'space', ':': 'colon', '-': 'minus', '.': 'period'}
        for c in command:
            if c == '=':
                # В матрице «Специалиста» = получается из Shift и минуса.
                m.call('kbd_send',keys=['shift'],action='press')
                try:
                    m.call('emu_run_for',ms=250)
                    m.tap('minus')
                finally:
                    m.call('kbd_send',keys=['shift'],action='release')
                m.call('emu_run_for',ms=250)
            else:
                m.tap(keys.get(c,c.lower()))
        assert m.read(nc['CMD_LINE'],len(command)).upper() == command.upper().encode(), m.read(nc['CMD_LINE'],len(command))
        m.screenshot(out/(name+'-command.png'))
        m.stop_at(s['entry'])
        m.tap('enter')
        m.until(s['entry'],120)
        assert m.read(0,32) == binary[:32]

    def finish(name):
        m.stop_at(s['exit_wait'])
        for label in ('error', 'file_exists'):
            m.call('bp_set', addr=s[label], type='exec')
        m.until(s['exit_wait'],120)
        if name not in ('readonly', 'bad-size'):
            # Полный READ/WRITE прошёл, собственный тестовый файл удалён.
            assert m.read(s['v_fileOwned'],1) == b'\0'
            for stat in ('statFileRead', 'statFileWrite'):
                assert int.from_bytes(m.read(s[stat],4),'little') > 0
        m.screenshot(out/(name+'-result.png'))
        m.call('bp_clear')
        m.call('emu_run_for',ms=500)
        m.tap('space')
        m.stop_at(nc['mainLoop'])
        m.until(nc['mainLoop'],120)
        assert m.read(0,32) == (ROOT/'bin/NC.i80').read_bytes()[:32]

    try:
        x.start('diskbench-args')
        request = urllib.request.Request(m.url, json.dumps({'jsonrpc':'2.0','id':999,'method':'tools/list','params':{}}).encode(), {'Content-Type':'application/json'})
        reply = json.load(urllib.request.urlopen(request,timeout=30))
        (out/'tools.json').write_text(json.dumps(reply,indent=2)+'\n',encoding='utf-8')
        cases = [
            ('all', 'DISKBEN B: -C=1 -W=1 -S=1', [], (1,1,6,1)),
            ('partial', 'DISKBEN B: -W=1', [('menu_verify','0'),('menu_size','2')], (1,1,1,2)),
            ('drive', 'DISKBEN -C=0 -W=1 -S=1', [('menu_drive','b')], (1,1,1,1)),
            ('writes', 'DISKBEN B: -C=2 -S=1', [('menu_writes','1')], (1,1,5,1)),
            ('readonly', 'DISKBEN B: -W=0', [], (1,0,1,8)),
        ]
        for name,command,answers,expected in cases:
            print('CASE',name,flush=True)
            launch(command,name)
            for label,value in answers:
                expect(label)
                m.call('bp_clear')
                m.call('emu_run_for',ms=500)
                m.screenshot(out/(name+'-'+label+'.png'))
                for c in value:
                    m.tap(c)
                # Ввод идёт через клавиатуру; после Enter ловим следующий
                # вопрос либо начало теста, не подменяя PC/SP и буфер строки.
                m.call('bp_clear')
                for target in questions:
                    m.call('bp_set',addr=s[target],type='exec')
                m.tap('enter')
            expect('prepare')
            actual = tuple(m.read(s[n],1)[0] for n in ('v_drive','v_budget','v_writeMode','v_fileKiB'))
            assert actual == expected, (name,actual,expected)
            finish(name)
            rows.append(dict(case=name,command=command,values=actual,completed=True))
            print('PASS',name,flush=True)
        launch('DISKBEN B: -S=8192','bad-size')
        expect('argument_error')
        finish('bad-size')
        rows.append(dict(case='bad-size',rejected=True))
        (out/'result.json').write_text(json.dumps(rows,indent=2)+'\n',encoding='utf-8')
        print('DISKBEN ARGS KEYBOARD PASS',len(rows),flush=True)
    finally:
        x.stop()


if __name__ == '__main__':
    main()
