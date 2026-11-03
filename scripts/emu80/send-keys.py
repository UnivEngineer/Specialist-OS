"""Ввод через клавиатуру отдельного Emu80, без перехвата фокуса Windows."""
import argparse
import base64
import json
from pathlib import Path
import runpy
import subprocess
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(Path(__file__).with_name('check-flash.py')))['Machine']

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', required=True, type=Path)
    parser.add_argument('--text')
    parser.add_argument('--key')
    parser.add_argument('--repeat', type=int, default=1)
    parser.add_argument('--screenshot', type=Path)
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    assert runtime == ROOT / '.local/monitor-compat-001', 'Нужен отдельный тестовый runtime'
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    owner = subprocess.run(['powershell.exe', '-NoProfile', '-Command', query],
                           check=True, capture_output=True, text=True).stdout.strip()
    assert Path(owner).resolve() == runtime / 'Emu80qt-mcp.exe'
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('bp_clear')
    m.call('emu_run_for', ms=500)
    mapping = {':':'colon', '.':'period', '-':'minus', ' ':'space', '/':'slash'}
    for key in ([args.key]*args.repeat if args.key else []) + [mapping.get(c, c.lower()) for c in args.text or '']:
        result = m.call('kbd_send', keys=[key], action='tap', duration_ms=100)
        if not result.get('stoppedByBreakpoint'):
            m.call('emu_run_for', ms=400 if args.repeat>1 else 120)
        if key == 'esc':
            m.call('emu_run_for', ms=250)
    m.call('emu_run')
    if args.screenshot:
        time.sleep(.5)
        request = urllib.request.Request(m.url, json.dumps(dict(jsonrpc='2.0',id=1,
            method='tools/call', params=dict(name='screen',arguments={}))).encode(),
            {'Content-Type':'application/json'})
        result = json.load(urllib.request.urlopen(request))
        content = next(x for x in result['result']['content'] if x['type']=='image')
        args.screenshot.parent.mkdir(parents=True, exist_ok=True)
        args.screenshot.write_bytes(base64.b64decode(content['data']))

if __name__ == '__main__':
    main()
