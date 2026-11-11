"""COM/EXE/RKS: контейнеры, стандартный размер FAT и запуск в отдельном Emu80."""
import argparse
import json
from pathlib import Path
import runpy
import struct
import subprocess
import sys
import tempfile
import base64

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
sys.path.insert(0, str(ROOT / 'scripts/images'))
from executable import Image, convert, convert_batch, decode, encode


def host_checks():
    count = 0
    for size in (1, 239, 240, 241, 255, 256, 257, 4096):
        image = Image(bytes(i & 255 for i in range(size)), 0x1000, 0x1000 + size - 1)
        for kind in ('com', 'exe'):
            packed = encode(image, kind)
            assert len(packed) == size + 16 and decode(packed, kind) == image
            for offset in (0, 6, 8, 10, 12, 14, 16, len(packed)-1):
                bad = bytearray(packed)
                bad[offset] ^= 1
                try:
                    decode(bad, kind)
                except ValueError:
                    pass
                else:
                    raise AssertionError((size, kind, offset))
                count += 1
        rks = Image(image.data, image.load, image.load)
        assert decode(encode(rks, 'rks'), 'rks') == rks
    with tempfile.TemporaryDirectory() as tmp:
        cpu = Path(tmp) / 'game.cpu'
        cpu.write_bytes(b'\xc9')
        assert convert(cpu, 'com')[1] == Image(b'\xc9')
        assert convert(cpu, 'exe', load=0x4000)[1] == Image(b'\xc9')
        cpu.write_text('1000\n1010\n', encoding='ascii')
        cpu.with_suffix('.i80').write_bytes(bytes(17))
        assert convert(cpu, 'exe')[1] == Image(bytes(17), 0x1000, 0x1010)
        driver = Image(b'\xc9', 0x9000, 0x9000, 1)
        assert decode(encode(driver, 'com'), 'com') == driver
        cpu.write_text('9000\n9000\n01\n', encoding='ascii')
        cpu.with_suffix('.i80').write_bytes(driver.data)
        assert convert(cpu, 'com')[1] == driver
        bank = Image(b"\xc9", 0x100, 0x100, 2)
        for kind in ("com", "exe", "dll"):
            assert decode(encode(bank, kind), kind) == bank
        cpu.write_text("0100\n0100\n02\n", encoding="ascii")
        cpu.with_suffix(".i80").write_bytes(bank.data)
        assert convert(cpu, "dll")[1] == bank
        main = Image(b"\xc9", 0x100, 0x100, 4)
        assert decode(encode(main, "exe"), "exe") == main
        for flags in (3, 5, 6, 7, 8, 0x80, 0xff):
            try:
                encode(Image(b'\xc9', 0x9000, 0x9000, flags), 'com')
            except ValueError:
                pass
            else:
                raise AssertionError(('Unknown flags accepted', flags))
        bad = Path(tmp) / 'bad.exe'
        packed = bytearray(encode(Image(b'\xc9'), 'exe'))
        packed[4] = 2
        bad.write_bytes(packed)
        try:
            convert(bad, 'com')
        except ValueError:
            pass
        else:
            raise AssertionError('Unsupported version treated as raw EXE')
        # Пакетный путь сохраняет порядок и результаты одиночной конвертации.
        unicode_cpu = Path(tmp) / 'проверка.cpu'
        unicode_cpu.write_bytes(b'\xc9')
        requests = [dict(source=str(unicode_cpu), format='EXE', load=0x4000),
                    dict(source=str(cpu), format='com', load=0)]
        expected = [base64.b64encode(convert(r['source'],r['format'],r['load'])[0]).decode('ascii')
                    for r in requests]
        assert convert_batch([]) == []
        assert convert_batch(requests[:1]) == expected[:1]
        assert convert_batch(requests) == expected
        batch = Path(tmp) / 'запрос.json'
        batch.write_text(json.dumps(requests,ensure_ascii=False),encoding='utf-8-sig')
        result = subprocess.run([sys.executable,str(ROOT/'scripts/images/executable.py'),
                                 '--batch',str(batch)],capture_output=True,text=True,check=True)
        assert json.loads(result.stdout) == expected
        batch.write_text(json.dumps([dict(source=str(bad),format='com')]),encoding='utf-8')
        result = subprocess.run([sys.executable,str(ROOT/'scripts/images/executable.py'),
                                 '--batch',str(batch)],capture_output=True,text=True)
        assert result.returncode != 0 and not result.stdout and 'bad.exe' in result.stderr
    return {'corruptions_rejected': count, 'round_trips': 24, 'cpu_pair_and_bare': True}


def emulator_checks(runtime):
    runtime = runtime.resolve()
    assert runtime == ROOT / '.local/monitor-compat-001'
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    owner = subprocess.run(['powershell.exe', '-NoProfile', '-Command', query],
                           check=True, capture_output=True, text=True).stdout.strip()
    assert Path(owner).resolve() == runtime / 'Emu80qt-mcp.exe'
    Machine = runpy.run_path(str(ROOT / 'scripts/emu80/check-copy.py'))['FileMachine']
    m = Machine('http://127.0.0.1:19781/mcp')
    def reset():
        m.call('bp_clear')
        m.call('emu_reset')
        m.call('emu_run_for', ms=2000)
        assert m.read(SYSTEM['NC_LOAD_ADDR'], 32) == (ROOT / 'bin/NC.i80').read_bytes()[:32]
        m.call('debug_break')
    def until(addresses):
        for _ in range(40):
            m.call('emu_run_for', ms=250)
            state = m.call('cpu_state')
            if state['registers']['pc']['dec'] in addresses:
                return state
        raise AssertionError(state)
    def return_input(**registers):
        state = m.call('cpu_state')['registers']
        sp = state['sp']['dec']
        pc = int.from_bytes(m.read(sp, 2), 'little')
        m.call('regs_set', pc=pc, sp=sp+2, **registers)
    def create(name, data):
        m.subroutine(0xC842, af=0x0100, de=1)
        stem, ext = name.split('.')
        desc = bytearray((stem.ljust(8)+ext.ljust(3)).encode()+bytes(21))
        struct.pack_into('<I', desc, 28, len(data))
        m.write(0x7200, desc)
        m.write(0x3000, data)
        state = m.subroutine(0xC845, hl=0x7200, de=0x3000)
        assert not state['flags']['C'], (name, state)
    reset()
    m.call('bp_set', addr=0x8100, type='exec')
    payload = bytearray((i*17 & 255) for i in range(257))
    payload[16] = 0xC9
    image = Image(bytes(payload), 0x1000, 0x1010)
    for ext in ('COM', 'EXE'):
        create('PROBE.'+ext, encode(image, ext))
    rks_image = Image(bytes(payload), 0x1000, 0x1000)
    create('PROBE.RKS', encode(rks_image, 'rks'))
    create('EMPTY.TXT', b'')
    m.write(0x7200, b'EMPTY   TXT'+bytes(21))
    assert not m.subroutine(0xC84B, hl=0x7200)['flags']['C']
    results = []
    cases = [('B:PROBE.COM', '1', 'MX'),
             ('A:RUN.COM B:PROBE.COM /X', '1', 'MX'),
             ('A:RUN.COM -MX B:PROBE.EXE /X', None, 'MX'),
             ('A:RUN.COM -STD B:PROBE.EXE /X', None, 'STD'),
             ('A:RUN.COM -RF B:PROBE.EXE /X', None, 'RF'),
             ('A:RUN.COM B:PROBE.RKS /X', None, 'MON'),
             ('A:RUN.COM -MON:Monitor2 B:PROBE.EXE /X', None, 'MON')]
    cases += [('A:RUN.COM B:PROBE.EXE /X', str(i), mode)
              for i, mode in enumerate(('MX','STD','RF','MON','MON'), 1)]
    for command, choice, mode in cases:
        reset()
        entry = 0x1000 if '.RKS' in command else 0x1010
        m.write(0x1101, b'\xa5')
        m.write(0x7300, command.encode()+b'\0')
        m.write(0x8000, bytes.fromhex('21 00 73 cd 5d c8 c3 00 81'))
        m.call('bp_set', addr=entry, type='exec')
        m.call('bp_set', addr=0x8100, type='exec')
        if choice:
            m.call('bp_set', addr=0xC803, type='exec')
            m.call('bp_set', addr=0xC80F, type='exec')
        m.call('regs_set', pc=0x8000, sp=SYSTEM['STACK_ADDR'])
        if choice:
            state = until({0xC803, 0x8100})
            assert state['registers']['pc']['dec'] == 0xC803, (command, state)
            # В этой модели text_screen не поддерживается: меню проверяется
            # по достижению getch, выбор и ввод — через возврат штатного ABI.
            return_input(af=ord(choice)<<8)
            m.call('bp_clear')
            m.call('bp_set', addr=entry, type='exec')
            m.call('bp_set', addr=0x8100, type='exec')
            if choice == '5':
                m.call('bp_set', addr=0xC80F, type='exec')
                state = until({0xC80F, 0x8100})
                assert state['registers']['pc']['dec'] == 0xC80F, state
                m.write(state['registers']['hl']['dec'], b'Monitor2\0')
                return_input()
        state = until({entry, 0x8100})
        assert state['registers']['pc']['dec'] == entry, (command, choice, state)
        assert m.read(0x1000, len(payload)) == payload, (command, choice)
        assert m.read(0x1101, 1) == b'\xa5', (command, 'tail overwritten')
        if mode == 'RF':
            kernel = (ROOT / 'bin/RAMFOS.mon').read_bytes()
            assert m.read(0xF800, 3) == kernel[0x1300:0x1303]
        if mode == 'STD':
            assert m.read(0x8FEA, 1) == b'\xff'
        results.append({'command': command, 'menu': choice, 'mode': mode,
                        'payload_and_entry': True, 'tail_preserved': True})
        print('OK:', command, choice or '', mode, flush=True)
    reset()
    m.call('emu_run')
    return results


if __name__ == '__main__':
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path)
    args = parser.parse_args()
    report = {'host': host_checks()}
    if args.runtime:
        report['emulator'] = emulator_checks(args.runtime)
    output = ROOT / 'docs/compat/executable-v2-001/checks.json'
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    print(json.dumps(report, indent=2))
