"""COM/EXE/RKS через обычную командную строку NC и kbd_send, без смены PC/SP."""
import argparse
import base64
import json
from pathlib import Path
import runpy
import struct
import subprocess
import sys
import time
import urllib.request

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
sys.path.insert(0, str(ROOT / 'scripts/images'))
from executable import Image, encode
Machine = runpy.run_path(str(Path(__file__).with_name('check-flash.py')))['Machine']
OUT = ROOT / 'docs/compat/sector-load-001/ui'
CASES = [('NATIVE.COM', None, None), ('RUNCOM.COM', 'MX', None),
         ('DEFAULT.RKS', 'MON', None)]
CASES += [(f'MENU{i}.EXE', mode, str(i))
          for i, mode in enumerate(('MX', 'STD', 'RF', 'MON', 'MON'), 1)]
CASES += [('EXPLICIT.EXE', 'STD', None), ('CUSTOM.EXE', 'MON:Monitor2', None)]
CASES += [(f'ORDER{i}.EXE', mode, None)
          for i, mode in enumerate(('MX', 'STD', 'RF', 'MON', 'MON:Monitor2'))]
CASES += [('UPPER.EXE', 'MX', None), ('RFHIGH.EXE', 'RF', None),
          ('EDGE.EXE', 'STD', None), ('SPLIT.RKS', 'MON', None)]


def prepare(path):
    # Меняется только копия H: внутри собственного остановленного runtime.
    raw = bytearray(path.read_bytes())
    word = lambda n: struct.unpack_from('<H', raw, n)[0]
    sector = word(11)
    fat = word(14)*sector
    root = (word(14)+raw[16]*word(22))*sector
    data = root + ((word(17)*32+sector-1)//sector)*sector
    slots = [p for p in range(root, data, 32) if raw[p] in (0, 0xE5, 0xFF)]
    free = [c for c in range(2, (len(raw)-data)//sector+2) if word(fat+c*2)==0]
    assert len(slots) >= len(CASES)+2
    existing = {bytes(raw[p:p+11]).upper() for p in range(root,data,32)
                if raw[p] not in (0,0xE5,0xFF)}
    for name, _, _ in CASES:
        stem,ext=name.split('.')
        assert (stem.ljust(8)+ext.ljust(3)).encode() not in existing, 'Восстановите чистую тестовую копию H:'
    for i, (name, _, _) in enumerate(CASES):
        marker = 0x7100+i
        marker = 0x7100+i*4
        # Метка и ожидание клавиши позволяют увидеть результат до холодного
        # старта: начальный тест ОЗУ DOS может затереть метку после RET.
        tag = b'MX'+bytes((i,0x5A))
        code = bytes((0x21,marker&255,marker>>8))
        for byte in tag:
            code += bytes((0x36,byte,0x23))
        getch = 0xF803 if name in ('MENU3.EXE', 'ORDER2.EXE', 'RFHIGH.EXE') else 0xC803
        code += bytes((0xCD,getch&255,getch>>8,0xC9))
        rks = name.endswith('.RKS')
        size = 240 if name=='EDGE.EXE' else 251 if name=='SPLIT.RKS' else 1025
        payload = code+bytes(size-len(code)) if rks else bytes([0x76])*16+code+bytes(size-16-len(code))
        load = 0xEA00 if name=='UPPER.EXE' else 0xD300 if name=='RFHIGH.EXE' else 0x8EE0 if name=='EDGE.EXE' else 0x1000
        image = Image(payload, load, load if rks else load+16)
        packed = encode(image, name.split('.')[1])
        count = (len(packed)+sector-1)//sector
        chain, free = free[:count], free[count:]
        assert len(chain) == count
        for n, cluster in enumerate(chain):
            struct.pack_into('<H',raw,fat+cluster*2,chain[min(n+1,count-1)])
            offset = data+(cluster-2)*sector
            chunk = packed[n*sector:(n+1)*sector]
            raw[offset:offset+sector] = chunk + bytes([255])*(sector-len(chunk))
        stem, ext = name.split('.')
        entry = bytearray((stem.ljust(8)+ext.ljust(3)).encode()+bytes(21))
        struct.pack_into('<H',entry,26,chain[0])
        struct.pack_into('<I',entry,28,len(packed))
        raw[slots[i]:slots[i]+32] = entry
    # Нулевой файл нужен для проверки обновлённого редактора и DIR_FileSize.
    entry = b'EMPTY   TXT'+bytes(21)
    raw[slots[len(CASES)]:slots[len(CASES)]+32] = entry
    # Две короткие команды: каждая затирает нижний буфер BAT при загрузке.
    batch=b'run RUNCOM.COM\rrun -STD EXPLICIT.EXE\r'
    cluster=free[0]
    struct.pack_into('<H',raw,fat+cluster*2,cluster)
    offset=data+(cluster-2)*sector
    raw[offset:offset+sector]=batch+bytes([255])*(sector-len(batch))
    entry=bytearray(b'SHORT   BAT'+bytes(21))
    struct.pack_into('<H',entry,26,cluster)
    struct.pack_into('<I',entry,28,len(batch))
    raw[slots[len(CASES)+1]:slots[len(CASES)+1]+32]=entry
    path.write_bytes(raw)


def check(runtime):
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    owner = subprocess.run(['powershell.exe','-NoProfile','-Command',query],
                           check=True,capture_output=True,text=True).stdout.strip()
    assert Path(owner).resolve() == runtime/'Emu80qt-mcp.exe'
    m = Machine('http://127.0.0.1:19781/mcp')
    OUT.mkdir(parents=True,exist_ok=True)
    def tap(key):
        r=m.call('kbd_send',keys=[key],action='tap',duration_ms=100)
        assert not r.get('stoppedByBreakpoint'), r
        m.call('emu_run_for',ms=120)
    def text(s):
        mapping={' ':'space',':':'colon','.':'period','-':'minus','/':'slash'}
        for char in s:
            tap(mapping.get(char,char.lower()))
    def screen(name):
        request=urllib.request.Request(m.url,json.dumps(dict(jsonrpc='2.0',id=1,
            method='tools/call',params=dict(name='screen',arguments={}))).encode(),
            {'Content-Type':'application/json'})
        content=json.load(urllib.request.urlopen(request))['result']['content']
        block=next(c for c in content if c['type']=='image')
        (OUT/(name+'.png')).write_bytes(base64.b64decode(block['data']))
    def wait_nc():
        expected=(ROOT/'bin/NC.i80').read_bytes()[:32]
        for _ in range(30):
            m.call('emu_run_for',ms=500)
            if m.read(SYSTEM['NC_LOAD_ADDR'],32)==expected:
                # Наличие кода NC ещё не означает, что закончены каталог
                # и первый вход в getch. Не посылать первую букву при загрузке.
                m.call('emu_run_for',ms=2000)
                return
        raise AssertionError('NC did not return')
    results=[]
    m.call('bp_clear')
    wait_nc()
    for i,(name,mode,choice) in enumerate(CASES):
        marker=0x7100+i*4
        before=m.read(marker,4)
        tag=b'MX'+bytes((i,0x5A))
        assert before!=tag, 'Повторный запуск требует подготовки свежих тестовых данных'
        command=f'H:{name}' if mode is None else (
            f'A:RUN.COM H:{name}' if choice or name.endswith(('.COM','.RKS')) else
            f'A:RUN.COM -{mode} H:{name}')
        if name.startswith('ORDER'):
            command=f'run H:{name} /x -{mode} one'
        tap('esc')
        m.call('emu_run_for',ms=250)
        text(command)
        symbols=runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))['symbols'](ROOT/'bin/NC.sym')
        typed=m.read(symbols['vars.cmdLine'],len(command))
        assert typed.upper()==command.upper().encode(), (command,typed)
        screen(f'{i:02}-command')
        tap('enter')
        if choice:
            m.call('emu_run_for',ms=2000)
            screen(f'{i:02}-menu')
            tap(choice)
            if choice=='5':
                m.call('emu_run_for',ms=500)
                text('Monitor2')
                screen(f'{i:02}-custom')
                tap('enter')
        for _ in range(30):
            m.call('emu_run_for',ms=500)
            if m.read(marker,4)==tag:
                break
        after=m.read(marker,4)
        assert after==tag, (command,choice,before,after,m.call('cpu_state'))
        if name.startswith('ORDER'):
            assert m.read(SYSTEM['SYSTEM_PROGRAM_ADDR'],7).upper()==b'/X ONE\0', (command,m.read(SYSTEM['SYSTEM_PROGRAM_ADDR'],7))
        m.call('emu_run_for',ms=700)
        screen(f'{i:02}-entry')
        tap('space')
        wait_nc()
        screen(f'{i:02}-return')
        results.append(dict(command=command,choice=choice,marker_before=before.hex(),
                            marker_after=after.hex(),returned_to_nc=True))
        (OUT/'results.json').write_text(json.dumps(results,indent=2)+'\n')
        print('OK:',command,choice or '',flush=True)
    m.call('emu_run')


if __name__=='__main__':
    p=argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime',required=True,type=Path)
    p.add_argument('--prepare',action='store_true')
    args=p.parse_args()
    runtime=args.runtime.resolve()
    assert runtime==ROOT/'.local/monitor-compat-001'
    if args.prepare:
        query="Get-Process Emu80qt-mcp -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Path"
        active=subprocess.run(['powershell.exe','-NoProfile','-Command',query],capture_output=True,text=True).stdout
        assert str(runtime/'Emu80qt-mcp.exe').lower() not in active.lower()
        prepare(runtime/'specialist-os/flash-disk.bin')
    else:
        check(runtime)
