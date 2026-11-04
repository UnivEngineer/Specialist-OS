"""Изолированный бенчмарк BIOS: основное ОЗУ против страницы 0 через общее окно."""
import argparse
import base64
import hashlib
import json
import re
import runpy
import shutil
import subprocess
import time
import urllib.request
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(ROOT / 'scripts/emu80/check-flash.py'))['Machine']
POWERSHELL = shutil.which('pwsh') or shutil.which('powershell')


def ps(code):
    return subprocess.check_output([POWERSHELL, '-NoProfile', '-Command', code], text=True).strip()


def symbols(name):
    return {k: int(v, 16) for k, v in re.findall(r'^([\w.]+): EQU 0x([0-9A-Fa-f]+)',
            (ROOT / 'bin' / (name + '.sym')).read_text(encoding='utf-8-sig'), re.M)}


def screenshot(m, path):
    request = urllib.request.Request(m.url, json.dumps({
        'jsonrpc': '2.0', 'id': 99999, 'method': 'tools/call',
        'params': {'name': 'screen', 'arguments': {}}}).encode(),
        {'Content-Type': 'application/json'})
    result = json.load(urllib.request.urlopen(request, timeout=30))['result']
    block = next(x for x in result['content'] if x['type'] == 'image')
    path.write_bytes(base64.b64decode(block['data']))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path,
                        help='Отдельная свободная установка Emu80 с MCP и моделями MX2')
    parser.add_argument('--report', type=Path, default=ROOT / 'docs/bench/text-bank-001/results.json')
    parser.add_argument('--repeats', type=int, default=3)
    parser.add_argument('--buffer', action='store_true', help='Дополнительно проверить общий буфер на 20 байт')
    args = parser.parse_args()
    if args.repeats < 1:
        parser.error('--repeats must be positive')
    runtime = args.runtime.resolve()
    exe = runtime / 'Emu80qt-mcp.exe'
    # Подключение к уже запущенному экземпляру запрещено: стенд владеет всем сеансом.
    owner = ps("Get-NetTCPConnection -LocalPort 19781 -State Listen -ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess; exit 0")
    if owner:
        raise RuntimeError(f'MCP port already owned by PID {owner}; no connection made')
    quoted = str(runtime).replace("'", "''")
    busy = ps(f"Get-Process -Name Emu80qt,Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object {{ $_.Path -and (Split-Path -Parent $_.Path) -eq '{quoted}' }} | Select-Object -ExpandProperty Id; exit 0")
    if busy:
        raise RuntimeError(f'Runtime already in use by PID {busy}')
    dos = (ROOT/'bin/DOS.sys').read_bytes()
    system = (ROOT/'images/SystemRom_emu80_writable.bin').read_bytes()
    if dos not in system:
        raise RuntimeError('System image has stale DOS; rebuild the system ROM first')
    subprocess.run([POWERSHELL, '-NoProfile', '-File', str(ROOT / 'source-apps/textbench/build.ps1')], check=True)
    subprocess.run([POWERSHELL, '-NoProfile', '-File', str(ROOT / 'scripts/emu80/run.ps1'),
                    '-EmulatorDirectory', str(runtime), '-Flash', '-ResetFlash', '-Mcp'], check=True)
    pid = None
    try:
        for _ in range(100):
            value = ps("Get-NetTCPConnection -LocalPort 19781 -State Listen -ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess; exit 0")
            if value:
                candidate = int(value)
                path = ps(f'(Get-Process -Id {candidate}).Path')
                if Path(path).resolve() != exe.resolve():
                    raise RuntimeError(f'Unexpected MCP owner: {path}')
                pid = candidate
                break
            time.sleep(.1)
        if pid is None:
            raise RuntimeError('Owned MCP instance did not start')
        m = Machine('http://127.0.0.1:19781/mcp')
        m.call('debug_break')
        m.call('bp_clear')
        ready = symbols('NC')['mainLoop']
        m.call('bp_set', addr=ready, type='exec')
        for _ in range(120):
            m.call('emu_run_for', ms=500)
            if m.call('cpu_state')['registers']['pc']['dec'] == ready:
                break
        else:
            raise RuntimeError('NC boot timeout')
        m.call('bp_clear')
        syms = symbols('TEXTBEN')
        gate_syms = symbols('GATE')
        program = bytearray((ROOT / 'bin/TEXTBEN.i80').read_bytes())
        gate = bytearray((ROOT / 'bin/GATE.i80').read_bytes())
        assert len(gate) == 32
        original_vars = m.read(0x8FDF, 33)
        original_gate = m.read(0xFFC0, 32)
        m.write(0xFFFD, b'\0')
        original_page = m.read(0, 0xFFC0)
        m.write(0xFFFC, b'\0')
        results = []

        def run(bank, width, rows, noop, buffered=False):
            active_syms = symbols('TEXTBUF') if buffered else syms
            m.call('bp_clear')
            m.write(0xFFFC, b'\0')
            m.write(0x8FDF, original_vars)
            # Экран и исходное состояние одинаковы; очистка вне измеряемого участка.
            m.write(0x9000, bytes(0x3000))
            m.write(0x8FEA, b'\0\xff')  # KOI-8, нет незаконченной ESC-команды
            m.write(0x8FFA, b'\0\0\x08\0')  # нормальный текст, начальная базовая линия 8
            m.write(0xF880, b'\xc9')      # контрольная процедура RET без вывода
            target = 0xF880 if noop else 0xC809
            body = bytearray((ROOT/'bin/TEXTBUF.i80').read_bytes()) if buffered else bytearray(program)
            payload = (b'ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789 abcdefghijk'[:width] + b'\r\n\0')
            offset = active_syms['text'] - 0x100
            body[offset:offset + len(payload)] = payload
            call = 0xFFC0 if bank else (symbols('DISPATCH')['bufferService'] if buffered else target)
            offset = active_syms['output'] - 0x100 + 1
            body[offset:offset + 2] = call.to_bytes(2, 'little')
            gate_body = bytearray((ROOT/'bin/BUFFERGATE.i80').read_bytes()) if buffered else bytearray(gate)
            if buffered:
                m.write(0xF880, (ROOT/'bin/DISPATCH.i80').read_bytes())
            else:
                offset = gate_syms['service'] - 0xFFC0 + 1
                gate_body[offset:offset + 2] = target.to_bytes(2, 'little')
            m.write(0xFFC0, gate_body)
            if bank:
                m.write(0xFFFD, b'\0')
            m.write(0x100, body)
            m.call('regs_set', pc=active_syms['entry'], sp=0x8000, de=rows, bc=0x1234, hl=0x5678, af=0x0202)
            m.call('bp_set', addr=active_syms['done'], type='exec')
            before = m.call('cpu_state')['cpu_ticks']
            for _ in range(120):
                m.call('emu_run_for', ms=500)
                state = m.call('cpu_state')
                if state['registers']['pc']['dec'] == active_syms['done']:
                    break
            else:
                raise RuntimeError('Benchmark timeout')
            ticks = state['cpu_ticks'] - before
            assert state['registers']['sp']['dec'] == 0x8000, state
            # Здесь всё ещё доступна память вызывающей программы: RET вернул страницу.
            assert m.read(0x100, len(body)) == body, 'Caller code/data corrupted or wrong page'
            m.write(0xFFFC, b'\0')
            frame = m.read(0x9000, 0x3000)
            return {'ticks': ticks, 'seconds_2mhz': ticks / 2_000_000,
                    'screen_sha256': hashlib.sha256(frame).hexdigest(),
                    'cursor_hex': m.read(0x8FFC, 2).hex()}

        try:
            cases = [(True,47,100,False),(False,20,8,False),(False,47,8,False),
                     (False,20,100,False),(False,47,100,False)]
            if args.buffer:
                cases += [(False,47,8,True),(False,47,100,True)]
            for noop, width, rows, buffered in cases:
                samples = [dict(main=run(False,width,rows,noop,buffered), page0=run(True,width,rows,noop,buffered)) for _ in range(args.repeats)]
                calls = (((width+2+19)//20) if buffered else width+2)*rows
                for sample in samples:
                    a, b = sample['main'], sample['page0']
                    assert a['screen_sha256'] == b['screen_sha256'], 'Different screens'
                    assert a['cursor_hex'] == b['cursor_hex'], 'Different cursor positions'
                    if not buffered:
                        assert b['ticks'] - a['ticks'] == 166*calls, (calls,sample)
                    else:
                        reference = next(x for x in results if not x['noop'] and not x['buffered'] and x['width']==width and x['rows']==rows)['samples'][0]['main']
                        assert b['screen_sha256'] == reference['screen_sha256'], 'Buffered output differs from character output'
                        assert b['cursor_hex'] == reference['cursor_hex']
                assert len({(x['main']['ticks'], x['page0']['ticks']) for x in samples}) == 1
                a, b = samples[0]['main'], samples[0]['page0']
                result = dict(noop=noop, buffered=buffered, width=width, rows=rows, calls=calls, samples=samples,
                              overhead_ticks_per_call=(b['ticks']-a['ticks'])/calls,
                              slowdown_percent=100*(b['ticks']/a['ticks']-1))
                results.append(result)
                print(json.dumps(result, ensure_ascii=False), flush=True)
            args.report.parent.mkdir(parents=True, exist_ok=True)
            screenshot(m, args.report.with_suffix('.png'))
            report = dict(clock_hz=2_000_000, gate_bytes=len(gate), gate_code_bytes=29,
                          program_sha256=hashlib.sha256(program).hexdigest(),
                          bios_sha256=hashlib.sha256((ROOT/'bin/DOS.sys').read_bytes()).hexdigest(),
                          system_image_sha256=hashlib.sha256(system).hexdigest(),
                          gate_sha256=hashlib.sha256(gate).hexdigest(),
                          buffered_gate_code_bytes=12, shared_buffer_bytes=20,
                          cp_m_reference='syabr-cpm-0.1.zip: CPM.RKS, FF00 copier and FFC0 shared scratch',
                          results=results)
            args.report.write_text(json.dumps(report, ensure_ascii=False, indent=2)+'\n', encoding='utf-8')
        finally:
            m.call('bp_clear')
            m.write(0xFFFD, b'\0')
            m.write(0, original_page)
            assert m.read(0, 0xFFC0) == original_page
            m.write(0xFFFC, b'\0')
            m.write(0xFFC0, original_gate)
    finally:
        if pid is not None:
            # Закрываем только свой подтверждённый экземпляр, даже при провале проверки.
            ps(f"$p = Get-Process -Id {pid} -ErrorAction SilentlyContinue; if ($p -and $p.Path -eq '{str(exe).replace(chr(39), chr(39)*2)}') {{ Stop-Process -Id {pid} }}")


if __name__ == '__main__':
    main()
