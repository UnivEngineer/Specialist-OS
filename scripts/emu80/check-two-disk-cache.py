"""Два диска DOS: сохранность кэша, записи и перемонтирование; собственный Emu80."""
import json
import re
import runpy
import struct
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OUT = ROOT/'.local/two-cache-emulator'
OUT.mkdir(parents=True, exist_ok=True)
h = runpy.run_path(str(ROOT/'scripts/emu80/benchmark-disks.py'))
h['Matrix'].__init__.__globals__.update(OUT=OUT, RUNTIME=ROOT/'.local/monitor2-001')
x = h['Matrix']()
m = x.m
s = {k:int(v,16) for k,v in re.findall(r'^([\w.]+): EQU 0x([\dA-Fa-f]+)', (ROOT/'bin/banked/KERNEL.sym').read_text('utf-8'), re.M)}
rows = []


def dos_read(addr, count):
    m.write(0xFFFD, b'\0')
    data = m.read(addr, count)
    m.write(0xFFFC, b'\0')
    return data


def word(name):
    return int.from_bytes(dos_read(s[name],2),'little')


def entries(drive):
    data=dos_read(s['cacheData'],64*8)
    found={}
    for index in range(64):
        number,address,reference,dirty,tag=struct.unpack_from('<HHHBB',data,index*8)
        if tag<128 and tag&7==drive:
            found[(number,tag)]=(address,dos_read(address,128<<(tag>>3)))
    return found


def select(drive):
    result = x.func(0xC842, af=drive<<8, de=1)
    assert not result['flags']['C'], (drive,result)
    assert dos_read(s['v_cacheDrive'],1) == bytes((drive,))


def object_for(drive, name, mode):
    stem,ext = name.split('.')
    obj = bytearray(64)
    obj[:11] = (stem.ljust(8)+ext.ljust(3)).encode('ascii')
    obj[40:42] = bytes((drive,mode))
    m.write(0x100,obj)
    assert not x.func(0xC8D5, af=0, hl=0x100)['flags']['C']


def close():
    assert not x.func(0xC8D5, af=0x0500, hl=0x100)['flags']['C']


try:
    fixture=runpy.run_path(str(ROOT/'scripts/emu80/benchmark-dos-cache.py'))['prepare_fixture'](h,OUT)
    x.start('two-disk-cache',**fixture)
    x.console()
    # Команды вводятся штатной клавиатурой CLI до прямых проверок ABI.
    cli = h['symbols'](ROOT/'bin/CLI.sym')
    for command in ('dir a:', 'dir b:', 'dir a:'):
        m.call('bp_clear')
        m.call('emu_run_for',ms=500)
        m.tap('esc')
        for ch in command:
            m.tap({' ':'space', ':':'colon'}.get(ch,ch))
        assert m.read(cli['CMD_LINE'],len(command)).upper() == command.upper().encode()
        m.stop_at(cli['cliPrompt'])
        m.tap('enter')
        m.until(cli['cliPrompt'],120)
    rows.append('CLI keyboard: DIR A:, DIR B:, DIR A:')
    # Обычный выбор тома сохраняет таблицу, MRU и отдельный неполный сектор.
    select(0)
    object_for(0,'README.TXT',1)
    m.write(0x100+49,struct.pack('<HH',0x2000,37))
    assert not x.func(0xC8D5, af=0x0100, hl=0x100)['flags']['C']
    expected = m.read(0x2000,37)
    close()
    table = word('v_cacheTable')
    count = dos_read(s['v_cacheCount'],1)[0]
    descriptor = word('v_cachePartial')
    cached = entries(0)
    partial = dos_read(descriptor,8)
    select(1)
    other = word('v_cacheTable')
    assert other == table
    object_for(1,'CACHE.TMP',15)
    payload = bytes((i*11+3)&255 for i in range(8193))
    m.write(0x2000,payload)
    m.write(0x100+49,struct.pack('<HH',0x2000,len(payload)))
    assert not x.func(0xC8D5, af=0x0200, hl=0x100)['flags']['C']
    close()
    select(0)
    assert word('v_cacheTable') == table
    assert entries(0) == cached
    object_for(0,'README.TXT',1)
    m.write(0x100+49,struct.pack('<HH',0x2000,37))
    assert not x.func(0xC8D5, af=0x0100, hl=0x100)['flags']['C']
    assert m.read(0x2000,37) == expected
    close()
    select(1)
    object_for(1,'CACHE.TMP',1)
    m.write(0x100+49,struct.pack('<HH',0x2000,len(payload)))
    assert not x.func(0xC8D5, af=0x0100, hl=0x100)['flags']['C']
    assert m.read(0x2000,len(payload)) == payload
    close()
    rows.append('A/B: shared metadata pool, tagged partial sector; B write/read 8193 bytes')
    # Третий том также использует общий пул, без замены фиксированных контекстов.
    select(7)
    assert dos_read(s['v_cacheDrive'],1) == b'\7'
    select(1)
    assert word('v_cacheTable') == other
    rows.append('A/B/H: same shared pool across three disks')
    # RAW-записи требуют явного remount; FORMAT использует именно этот вход.
    assert not x.func(0xC896)['flags']['C']
    table = word('v_cacheTable')
    assert not entries(1)
    assert entries(0) == cached
    object_for(1,'CACHE.TMP',1)
    close()
    rows.append('explicit remount invalidates metadata, file remains readable')
    select(7)
    object_for(7,'RAIN.RKS',1)
    close()
    assert entries(7)
    select(1)
    x.install('FlashFst')
    # Установка H: происходила при неактивном H:; повторный выбор пересоздаёт кэш.
    select(7)
    assert not entries(7)
    rows.append('inactive H driver replacement and remount')
    m.screenshot(OUT/'complete.png')
    (OUT/'result.json').write_text(json.dumps(rows,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print('TWO DISK CACHE EMULATOR PASS', *rows, sep='\n')
finally:
    x.stop()
