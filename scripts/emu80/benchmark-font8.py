"""Рендер 8x8: основное ОЗУ и страница 0, одиночные и буферные вызовы."""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
helpers = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-text-bank.py'))
Machine, ps, symbols, screenshot = (helpers[n] for n in ['Machine','ps','symbols','screenshot'])
POWERSHELL = helpers['POWERSHELL']


def font_bytes():
    # Для теста растягиваем штатные шесть бит до восьми. Это подготовка данных,
    # не часть измерения; штатный файл и знакогенератор DOS не изменяются.
    src = (ROOT/'assets/system-rom/FONT.FNT').read_bytes()
    assert len(src) == 2048 and max(src) < 64
    return bytes(sum(((v >> (5-x*6//8)) & 1) << (7-x) for x in range(8)) for v in src)


def corpus(rows):
    return [f'{row:03d} ABCDEFGHIJKLMNOPQRSTUVWXYZ 0123456789 abcdef'.encode()[:48] for row in range(rows)]


def reference(font, lines):
    frame = bytearray(0x3000)
    for y, line in enumerate(lines[-32:]):
        for x, code in enumerate(line):
            at = x*256+y*8
            frame[at:at+8] = font[code*8:code*8+8]
    return bytes(frame)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', required=True, type=Path)
    p.add_argument('--repeats', type=int, default=3)
    p.add_argument('--rows', type=int, nargs='+', default=[8,32,100])
    p.add_argument('--report', type=Path, default=ROOT/'docs/bench/font8-001/results.json')
    args = p.parse_args()
    if args.repeats < 1 or any(n < 1 or n > 100 for n in args.rows):
        p.error('repeats must be positive; rows must be 1..100')
    runtime = args.runtime.resolve()
    exe = runtime/'Emu80qt-mcp.exe'
    port_query = 'Get-NetTCPConnection -LocalPort 19781 -State Listen -ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess; exit 0'
    owner = ps(port_query)
    if owner:
        raise RuntimeError(f'MCP port owned by PID {owner}; no connection made')
    quoted = str(runtime).replace("'", "''")
    busy = ps(f"Get-Process -Name Emu80qt,Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object {{ $_.Path -and (Split-Path -Parent $_.Path) -eq '{quoted}' }} | Select-Object -ExpandProperty Id; exit 0")
    if busy:
        raise RuntimeError(f'Runtime is busy: {busy}')
    dos = (ROOT/'bin/DOS.sys').read_bytes()
    system = (ROOT/'images/SystemRom_emu80_writable.bin').read_bytes()
    if dos not in system:
        raise RuntimeError('Rebuild the system ROM: DOS image is stale')
    subprocess.run([POWERSHELL,'-NoProfile','-File',str(ROOT/'source-apps/textbench/build.ps1')],check=True)
    subprocess.run([POWERSHELL,'-NoProfile','-File',str(ROOT/'source-apps/fontbench/build.ps1')],check=True)
    subprocess.run([POWERSHELL,'-NoProfile','-File',str(ROOT/'scripts/emu80/run.ps1'),
                    '-EmulatorDirectory',str(runtime),'-Flash','-ResetFlash','-Mcp','-PrepareOnly'],check=True)
    # Владеем дескриптором своего процесса даже при ошибке запуска MCP. Qt-окно
    # запускается видимым; существующие процессы не подключаются и не закрываются.
    process = subprocess.Popen([str(exe),'--conf-file','specialist-os/launch.conf',
                                '--platform','specialist-os'],cwd=runtime)
    try:
        for _ in range(50):
            owner = ps(port_query)
            if owner:
                if int(owner) != process.pid:
                    raise RuntimeError(f'Unexpected MCP owner {owner}, expected {process.pid}')
                break
            if process.poll() is not None:
                raise RuntimeError('Owned emulator exited before MCP startup')
            time.sleep(.1)
        else:
            raise RuntimeError('MCP startup timeout')
        m = Machine('http://127.0.0.1:19781/mcp')
        m.call('debug_break')
        m.call('bp_clear')
        ready = symbols('NC')['mainLoop']
        m.call('bp_set',addr=ready,type='exec')
        for _ in range(120):
            m.call('emu_run_for',ms=500)
            if m.call('cpu_state')['registers']['pc']['dec'] == ready:
                break
        else:
            raise RuntimeError('NC boot timeout')
        m.call('bp_clear')
        original_gate = m.read(0xFFC0,32)
        m.write(0xFFFD,b'\0')
        original_page = m.read(0,0xFFC0)
        m.write(0xFFFC,b'\0')
        font = font_bytes()
        render = (ROOT/'bin/FONT8.i80').read_bytes()
        dispatch = (ROOT/'bin/F8DISP.i80').read_bytes()
        gate = (ROOT/'bin/BUFFERGATE.i80').read_bytes()
        entries = symbols('FONT8')
        results = []

        def run(bank, buffered, rows):
            m.call('bp_clear')
            m.write(0xFFFC,b'\0')
            m.write(0xFFF8,b'\xF0')
            m.write(0x9000,bytes(0x3000))
            m.write(0x6000,font)
            m.write(0xEB00,render)
            m.write(0xF880,dispatch)
            m.write(0xFFC0,gate)
            name = 'F8BUF' if buffered else 'F8CHAR'
            sy = symbols(name)
            body = bytearray((ROOT/'bin'/f'{name}.i80').read_bytes())
            targets = {'output':0xFFC0 if bank else entries['buffer8' if buffered else 'draw8'],
                       'scrollOutput':0xFFC0 if bank else entries['scroll8']}
            for label,target in targets.items():
                offset = sy[label]-0x100+1
                body[offset:offset+2] = target.to_bytes(2,'little')
            offset = sy['rowsLeft']-0x100
            body[offset:offset+2] = rows.to_bytes(2,'little')
            lines = corpus(rows)
            data = b''.join(line+b'\0' for line in lines)
            offset = sy['corpus']-0x100
            body[offset:offset+len(data)] = data
            if bank:
                m.write(0xFFFD,b'\0')
            m.write(0x100,body)
            m.call('regs_set',pc=sy['entry'],sp=0x8000,af=0x0097,bc=0x1234,de=0x5678,hl=0x9ABC)
            m.call('bp_set',addr=sy['done'],type='exec')
            before = m.call('cpu_state')['cpu_ticks']
            for _ in range(120):
                m.call('emu_run_for',ms=500)
                state = m.call('cpu_state')
                if state['registers']['pc']['dec'] == sy['done']:
                    break
            else:
                raise RuntimeError('Font benchmark timeout')
            ticks = state['cpu_ticks']-before
            assert state['registers']['sp']['dec'] == 0x8000
            body[sy['rowsLeft']-0x100:sy['rowsLeft']-0x100+2] = b'\0\0'
            assert m.read(0x100,len(body)) == body, 'Caller code/data changed or wrong return page'
            m.write(0xFFFC,b'\0')
            frame = m.read(0x9000,0x3000)
            assert frame == reference(font,lines), (bank,buffered,rows,'Python bitmap reference differs')
            assert m.read(0x6000,len(font)) == font, 'Font was modified'
            return dict(ticks=ticks,seconds_2mhz=ticks/2_000_000,
                        chars_per_second=rows*48*2_000_000/ticks,
                        screen_sha256=hashlib.sha256(frame).hexdigest())

        try:
            for rows in args.rows:
                for buffered in (False,True):
                    samples = [dict(main=run(False,buffered,rows),page0=run(True,buffered,rows)) for _ in range(args.repeats)]
                    assert len({(s['main']['ticks'],s['page0']['ticks']) for s in samples}) == 1
                    a,b = samples[0]['main'],samples[0]['page0']
                    result = dict(rows=rows,width=48,characters=rows*48,scrolls=max(0,rows-32),
                                  buffered=buffered,draw_calls=rows*(3 if buffered else 48),
                                  slowdown_percent=100*(b['ticks']/a['ticks']-1),samples=samples)
                    results.append(result)
                    print(json.dumps(result),flush=True)
            args.report.parent.mkdir(parents=True,exist_ok=True)
            screenshot(m,args.report.with_suffix('.png'))
            report = dict(clock_hz=2_000_000,cell=[8,8],screen_cells=[48,32],
                          font_preparation='FONT.FNT 6-bit rows expanded to 8 bits before measurement',
                          font_sha256=hashlib.sha256(font).hexdigest(),
                          renderer_bytes=len(render),draw8_bytes=entries['buffer8']-entries['draw8'],
                          dispatch_bytes=len(dispatch),shared_gate_code_bytes=12,shared_buffer_bytes=20,
                          artifact_sha256={name:hashlib.sha256((ROOT/'bin'/f'{name}.i80').read_bytes()).hexdigest()
                                           for name in ['FONT8','F8DISP','F8CHAR','F8BUF','BUFFERGATE']},
                          dos_sha256=hashlib.sha256(dos).hexdigest(),results=results)
            args.report.write_text(json.dumps(report,indent=2)+'\n',encoding='utf-8')
            args.report.with_suffix('.font.bin').write_bytes(font)
        finally:
            m.call('bp_clear')
            m.write(0xFFFD,b'\0')
            m.write(0,original_page)
            assert m.read(0,0xFFC0) == original_page
            m.write(0xFFFC,b'\0')
            m.write(0xFFC0,original_gate)
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=10)


if __name__ == '__main__':
    main()
