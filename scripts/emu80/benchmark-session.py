"""Общий изолированный сеанс экранных бенчмарков MX2."""
from contextlib import contextmanager
from pathlib import Path
import runpy
import subprocess
import time

ROOT=Path(__file__).resolve().parents[2]
h=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-text-bank.py'))
Machine,ps,symbols,screenshot=(h[n] for n in ['Machine','ps','symbols','screenshot'])
POWERSHELL=h['POWERSHELL']


@contextmanager
def session(runtime, restore_memory=True):
    runtime=runtime.resolve()
    query='Get-NetTCPConnection -LocalPort 19781 -State Listen -ErrorAction SilentlyContinue | Select-Object -ExpandProperty OwningProcess; exit 0'
    owner=ps(query)
    if owner:
        raise RuntimeError(f'MCP port occupied by PID {owner}; no connection made')
    quoted=str(runtime).replace("'","''")
    busy=ps(f"Get-Process -Name Emu80qt,Emu80qt-mcp -ErrorAction SilentlyContinue | Where-Object {{ $_.Path -and (Split-Path -Parent $_.Path) -eq '{quoted}' }} | Select-Object -ExpandProperty Id; exit 0")
    if busy:
        raise RuntimeError(f'Runtime occupied by PID {busy}')
    dos=(ROOT/'bin/DOS.sys').read_bytes()
    if dos not in (ROOT/'images/SystemRom_emu80_writable.bin').read_bytes():
        raise RuntimeError('System image has stale DOS')
    for component in ['textbench','fontbench']:
        subprocess.run([POWERSHELL,'-NoProfile','-File',str(ROOT/f'source-apps/{component}/build.ps1')],check=True)
    subprocess.run([POWERSHELL,'-NoProfile','-File',str(ROOT/'scripts/emu80/run.ps1'),'-EmulatorDirectory',str(runtime),
                    '-Flash','-ResetFlash','-Mcp','-PrepareOnly'],check=True)
    process=subprocess.Popen([str(runtime/'Emu80qt-mcp.exe'),'--conf-file','specialist-os/launch.conf','--platform','specialist-os'],cwd=runtime)
    try:
        for _ in range(50):
            owner=ps(query)
            if owner:
                if int(owner)!=process.pid:
                    raise RuntimeError(f'Unexpected MCP owner: {owner}')
                break
            if process.poll() is not None:
                raise RuntimeError('Emulator exited before MCP startup')
            time.sleep(.1)
        else:
            raise RuntimeError('MCP timeout')
        m=Machine('http://127.0.0.1:19781/mcp')
        m.call('debug_break');m.call('bp_clear')
        ready=symbols('NC')['mainLoop']
        m.call('bp_set',addr=ready,type='exec')
        for _ in range(120):
            m.call('emu_run_for',ms=500)
            if m.call('cpu_state')['registers']['pc']['dec']==ready:
                if m.read(0,32)==(ROOT/'bin/NC.i80').read_bytes()[:32]:
                    break
                # FORMAT и другие COM также работают с нуля: адреса совпадают.
                m.call('bp_clear');m.call('emu_run_for',ms=50)
                m.call('bp_set',addr=ready,type='exec')
        else:
            raise RuntimeError('NC boot timeout')
        m.call('bp_clear')
        saved_gate=m.read(0xFFC0,32)
        m.write(0xFFFD,b'\0');saved_page=m.read(0,0xFFC0);m.write(0xFFFC,b'\0')
        try:
            yield m
        finally:
            m.call('bp_clear')
            # Смена ядра/STD не допускает восстановления через регистры MX.
            # Такой сеанс завершается закрытием собственного процесса ниже.
            if restore_memory:
                m.write(0xFFFD,b'\0');m.write(0,saved_page)
                assert m.read(0,0xFFC0)==saved_page
                m.write(0xFFFC,b'\0');m.write(0xFFC0,saved_gate)
    finally:
        if process.poll() is None:
            process.terminate()
        process.wait(timeout=10)
