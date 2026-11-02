"""Проверка DISKBEN в изолированном Emu80; запись только на тестовый RAM B:."""
import argparse
import base64
import hashlib
import json
import re
import runpy
import statistics
import struct
import subprocess
import urllib.request
from pathlib import Path

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
Machine = runpy.run_path(str(ROOT / 'scripts/emu80/check-flash.py'))['Machine']


def symbols(path):
    return {k: int(v, 16) for k, v in re.findall(r'^(\w+): EQU 0x([0-9A-Fa-f]+)', path.read_text(), re.M)}


class Bench(Machine):
    def until(self, pc, seconds=60):
        for _ in range(seconds):
            state = self.call('cpu_state')
            if state['registers']['pc']['dec'] == pc:
                return state
            self.call('emu_run_for', ms=1000)
        raise RuntimeError(self.call('cpu_state'))

    def stop_at(self, pc):
        self.call('bp_clear')
        self.call('bp_set', addr=pc, type='exec')

    def tap(self, key):
        result=self.call('kbd_send', keys=[key], action='tap', duration_ms=100)
        if not result.get('stoppedByBreakpoint'):
            self.call('emu_run_for', ms=120)

    def line(self, value):
        # bios_input ожидает строку и Enter; отдельный символ не завершает ввод.
        self.call('emu_run_for', ms=250)
        for key in value:
            self.tap(key)
        self.tap('ENTER')

    def screenshot(self, path):
        request = urllib.request.Request(self.url, json.dumps({'jsonrpc':'2.0','id':999,'method':'tools/call',
            'params':{'name':'screen','arguments':{}}}).encode(), {'Content-Type':'application/json'})
        reply = json.load(urllib.request.urlopen(request, timeout=30))
        block = next(b for b in reply['result']['content'] if b['type'] == 'image')
        path.write_bytes(base64.b64decode(block['data']))

    def sector(self, number):
        self.subroutine(0xC863, bc=0x0200, de=number, hl=0x6000)
        return self.read(0x6000, 256)


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', type=Path, required=True)
    p.add_argument('--variant', choices=['Legacy', 'Buffered'], required=True)
    p.add_argument('--repeats', type=int, default=3)
    p.add_argument('--report-directory',type=Path)
    p.add_argument('--url', default='http://127.0.0.1:19781/mcp')
    args = p.parse_args()
    if args.runtime.name != 'Emu80-flash-test' or args.url != 'http://127.0.0.1:19781/mcp':
        raise ValueError('Use isolated Emu80-flash-test at local port 19781')
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    listener = subprocess.run(['powershell.exe','-NoProfile','-Command',query],capture_output=True,text=True,check=True).stdout.strip()
    assert Path(listener).resolve() == (args.runtime/'Emu80qt-mcp.exe').resolve()
    assert 'SYSTEM_FLASH' not in (args.runtime/'specialist-os/launch.conf').read_text()
    m = Bench(args.url)
    m.call('debug_break')
    m.stop_at(0xE800)
    if m.read(0xE800, 32) != (ROOT/'bin/NC.com').read_bytes()[:32]:
        m.until(0xE800, 30)
    # Проверяем текущую сборку; full throttle выявляет последующая сверка ВИ53/CPU.
    assert m.read(0xC000, 32) == (ROOT/'bin/DOS.sys').read_bytes()[:32]
    s = symbols(HERE/'build/DISKBEN.sym')
    ds = symbols(ROOT/'bin/DOS.sym')
    out = args.report_directory or HERE/'build'/args.variant
    out.mkdir(parents=True,exist_ok=True)
    for src in [ROOT/'bin/DOS.sys', ROOT/'bin/DOS.lst', ROOT/'bin/DOS.sym', ROOT/'bin/NC.com',
                HERE/'build/DISKBEN.com']:
        (out/src.name).write_bytes(src.read_bytes())
    boot_rom = (args.runtime/'specialist-os/system.bin').read_bytes()
    (out/'SystemRom_emulator.bin').write_bytes(boot_rom)
    m.stop_at(0x8100)
    if args.variant == 'Buffered':
        # Штатный DOS загружается с байтовым RAMD; R4 устанавливается отдельно.
        m.write(0x7000,b'A:RAMD.COM\0')
        assert not m.subroutine(0xC85D,hl=0x7000)['flags']['C']
    m.subroutine(0xC842, af=0x0100, de=1)
    m.subroutine(0xC86F, bc=0)
    count = m.read(0x8FF8, 1)[0]
    assert count == 16
    cycles = {}
    for page in [0, 1, count-1]:
        sector = page*256+254
        payload = bytes((i*37+page*17+13)&255 for i in range(256))
        m.write(0x4000, b'\xDA'+payload+b'\xAD')
        state = m.subroutine(0xC863, bc=0x0100, de=sector, hl=0x4001)
        assert not state['flags']['C']
        for mode in [1, 2]:
            before = m.call('cpu_state')['cpu_ticks']
            state = m.subroutine(0xC863, af=0x5A02, bc=mode*256+0xA7, de=sector, hl=0x4001)
            elapsed = state['cpu_ticks']-before-27  # CALL + JMP обвязки, RET входит в драйвер.
            assert not state['flags']['C']
            assert state['registers']['af']['dec'] >> 8 == 0x5A
            assert all(state['registers'][k]['dec'] == v for k,v in {
                'bc':mode*256+0xA7,'de':sector,'hl':0x4001,'sp':0x8F00}.items())
            assert m.read(0x4000, 258) == b'\xDA'+payload+b'\xAD'
            cycles[str(mode)] = elapsed
        assert m.sector(sector) == payload
    for sector in [0x00FF, 0x01FF, 0x0FFF, count*256]:
        for mode in [1, 2]:
            assert m.subroutine(0xC863, bc=mode*256, de=sector, hl=0x4000)['flags']['C']
    if args.variant == 'Buffered':
        for addr in [0xFE01, 0xFF00]:
            assert m.subroutine(0xC863, bc=0x100, de=0x100, hl=addr)['flags']['C']
        rs = symbols(ROOT/'bin/RAMD.sym')
        tail_length = rs['ramdBankTailEnd']-0xFF00
        tail = m.read(0xFF00, tail_length)
        for page in range(count):
            m.write(0xFFFD, bytes([page]))
            assert m.read(0xFF00, tail_length) == tail
            m.write(0xFFFC,b'\0')
        # Переключение режимов не переписывает хвост; сервисный ABI переносит один байт.
        m.subroutine(0xC86F, bc=0x100)
        m.subroutine(0xFFD0, af=0, bc=0x69, hl=0xFFBD)
        assert m.subroutine(0xFFC0, af=0, hl=0xFFBD)['registers']['bc']['dec']&255 == 0x69
        m.subroutine(0xC86F, bc=0)
        assert m.read(0xFF00, tail_length) == tail
    boot = m.sector(0)
    size,spc,res,nf,root,total,fatsz = (struct.unpack_from('<H',boot,11)[0],boot[13],
        struct.unpack_from('<H',boot,14)[0],boot[16],struct.unpack_from('<H',boot,17)[0],
        struct.unpack_from('<H',boot,19)[0],struct.unpack_from('<H',boot,22)[0])
    data_start = res+nf*fatsz+(root*32+size-1)//size
    metadata = b''.join(m.sector(i) for i in range(data_start))
    report = {'status':'emulator-only','physical_hardware_measured':False,'variant':args.variant,
        'cpu_hz':2_000_000,'pit_hz':2_000_000,'divider':256,'driver_cycles':cycles,
        'dos_bytes':(ROOT/'bin/DOS.sys').stat().st_size,
        'dos_sha256':hashlib.sha256((ROOT/'bin/DOS.sys').read_bytes()).hexdigest(),
        'rom_sha256':hashlib.sha256(boot_rom).hexdigest(),
        'packaged_rom_sha256':hashlib.sha256((ROOT/'images/SystemRom_emulator.bin').read_bytes()).hexdigest(),
        'bench_sha256':hashlib.sha256((HERE/'build/DISKBEN.com').read_bytes()).hexdigest(),
        'metadata_sha256':hashlib.sha256(metadata).hexdigest(),'data_start':data_start,'runs':[]}
    names = ['read','write']
    for repeat in range(args.repeats):
        m.write(0,(HERE/'build/DISKBEN.com').read_bytes())
        m.stop_at(s['menu_drive'])
        m.call('regs_set',pc=0)
        m.until(s['menu_drive'])
        m.call('emu_run_for',ms=200)
        m.line('b')
        m.stop_at(s['print_result'])
        m.line('3')
        records = {}
        for case,name in zip((0,2),names):
            state=m.until(s['print_result'],60)
            assert m.read(s['v_case'],1)[0] == case, m.call('cpu_state')
            ticks=int.from_bytes(m.read(s['v_elapsed'],4),'little')
            ops=int.from_bytes(m.read(s['v_operations'],2),'little')
            nominal=cycles['2' if case<2 else '1']+198
            delta=ticks*256-ops*nominal
            assert ticks>0 and abs(delta)<ops*nominal*0.02, (ticks,ops,nominal,delta)
            records[name]={'ticks':ticks,'operations':ops,'driver_cycles_per_op':nominal-198,
                'pit_minus_nominal_cycles':delta,'kib_s':ops/4/(ticks*256/2_000_000)}
            print(args.variant,repeat+1,name,records[name],flush=True)
            if case==0:
                m.call('debug_step',mode='into')
            else:
                m.stop_at(s['exit_wait'])
                m.until(s['exit_wait'])
        report['runs'].append(records)
        assert m.read(s['v_budget'],1) == b'\3'
        start=int.from_bytes(m.read(s['v_start'],2),'little')
        assert start>=data_start and start+32<=total
        report['start_sector']=start
        m.screenshot(out/f'run-{repeat+1}.png')
        m.stop_at(0x8100)
        assert b''.join(m.sector(i) for i in range(data_start)) == metadata
        (out/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
    report['median']={n:{'ticks':statistics.median(r[n]['ticks'] for r in report['runs']),
        'kib_s':statistics.median(r[n]['kib_s'] for r in report['runs'])} for n in names}
    report['checks']=['booted DOS/NC','16 pages','unaligned buffer and guards','data readback',
        'registers and SP','invalid sector/page rejected','metadata unchanged after every run',
        'PIT checked against full driver CPU cycles','all writes verified by DISKBEN']
    if args.variant=='Buffered': report['checks']+=['all 16 tail replicas identical','service byte ABI','mode switch preserves replicas']
    (out/'measurements.json').write_text(json.dumps(report,indent=2)+'\n')
    print(json.dumps(report['median'],indent=2),flush=True)


if __name__=='__main__':
    main()
