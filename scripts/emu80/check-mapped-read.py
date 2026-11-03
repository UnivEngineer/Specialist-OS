"""Чтение FlashSys: верхнее ОЗУ, нижнее через регистры и через буфер.

Только отдельный Emu80-flash-test. Тестовые данные в последних секторах
банков восстанавливаются до запуска настоящего меню DISKBEN.
"""
import argparse
import hashlib
import json
import runpy
import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SYSTEM = runpy.run_path(str(ROOT/'scripts/emu80/system-map.py'))['MEMORY_MAP']
helpers = runpy.run_path(str(ROOT / 'source-apps/diskbench/check-emulator.py'))
Bench, symbols = helpers['Bench'], helpers['symbols']


class MappedBench(Bench):
    def subroutine(self, address, **registers):
        # Обвязка не должна попадать в буфер, пересекающий границу 8000h.
        self.stop_at(0x8900)
        self.write(0x8800, bytes((0xCD, address & 255, address >> 8, 0xC3, 0, 0x89)))
        self.call('regs_set', pc=0x8800, sp=0x8F00, **registers)
        before = self.call('cpu_state')['cpu_ticks']
        state = self.until(0x8900)
        return state, state['cpu_ticks'] - before - 27


def main():
    p = argparse.ArgumentParser(description=__doc__)
    p.add_argument('--runtime', required=True, type=Path)
    p.add_argument('--report-directory', type=Path, default=ROOT / 'docs/flash/mapped-read-002')
    p.add_argument('--baseline', type=Path, help='Необязательная прежняя FlashSys.com для сравнения')
    args = p.parse_args()
    if args.runtime.name != 'Emu80-flash-test':
        raise ValueError('Нужен отдельный Emu80-flash-test')
    query = '(Get-Process -Id (Get-NetTCPConnection -State Listen -LocalPort 19781 | Select-Object -First 1 -ExpandProperty OwningProcess)).Path'
    listener = subprocess.run(['powershell.exe', '-NoProfile', '-Command', query],
                              capture_output=True, text=True, check=True).stdout.strip()
    assert Path(listener).resolve() == (args.runtime / 'Emu80qt-mcp.exe').resolve()
    out = args.report_directory.resolve()
    out.mkdir(parents=True, exist_ok=True)
    variants = {}
    for mode, name in enumerate(('byte', 'pair', 'buffer')):
        target = out / name
        command = [str(ROOT / 'tasm/sjasmplus.exe'), '--i8080', '-Wno-rdlow',
                   f'-DFLASHSYS_READ_MODE={mode}', f'--raw={target}.com',
                   f'--sym={target}.sym', f'--lst={target}.lst']
        if mode == 2:
            command.append('-DFLASHSYS_READ_BUFFER=0x8400')
        subprocess.run(command + ['FlashSys.asm'], cwd=ROOT / 'source/flash', check=True)
        variants[name] = target.with_suffix('.com')
    if args.baseline:
        variants = {'before': args.baseline, **variants}
    m = MappedBench('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.stop_at(SYSTEM['NC_LOAD_ADDR'])
    # Сначала реальная холодная загрузка NC, затем независимые секторные вызовы.
    nc = (ROOT / 'bin/NC.i80').read_bytes()
    if m.read(SYSTEM['NC_LOAD_ADDR'], 32) != nc[:32]:
        m.until(SYSTEM['NC_LOAD_ADDR'], 60)
    assert m.read(0xC000, 32) == (ROOT / 'bin/DOS.sys').read_bytes()[:32]
    image_path = args.runtime / 'specialist-os/system-flash.bin'
    original = image_path.read_bytes()
    assert len(original) == 524288
    driver = symbols(ROOT / 'bin/FlashSys.sym')['Driver']
    canonical = (ROOT / 'bin/FlashSys.i80').read_bytes()

    def install(binary):
        m.write(0xFFFC, b'\0')
        m.install_flash_binary(binary)
        m.subroutine(0xC842, af=0, de=1)

    report = {'status': 'emulator-only', 'cpu_hz': 2000000, 'physical_hardware_measured': False,
              'dos_sha256': hashlib.sha256((ROOT / 'bin/DOS.sys').read_bytes()).hexdigest(),
              'variants': {}, 'ui': {}}
    sectors = [bank * 128 + 127 for bank in range(16)]
    changed = []
    try:
        install(canonical)
        # Различающиеся страницы выявляют потерю старших битов/неверный банк.
        for sector in sectors:
            payload = bytes((i * 37 + sector * 13 + (sector >> 7) * 19) & 255 for i in range(256))
            m.write(0x4000, payload)
            changed.append(sector)
            state, _ = m.subroutine(driver, bc=0x100, de=sector, hl=0x4000)
            assert not state['flags']['C']
        expected = image_path.read_bytes()
        for name, path in variants.items():
            install(path.read_bytes())
            counts = {}
            for address in [0, 1, 0x2000, 0x7E00, 0x7F00, 0x7F01, 0x7FFF,
                            0x8000, 0x8001, 0x8200, 0x83FF, 0x8C00]:
                timings = []
                for sector in [0, 1, 128, 256] + sectors:
                    # Маркеры вне массива; для нулевого адреса левого маркера нет.
                    start = address - bool(address)
                    guard = (b'\xDA' if address else b'') + b'\xCC' * 256 + b'\xAD'
                    m.write(start, guard)
                    state, ticks = m.subroutine(0xC863, bc=0x02A7, de=sector, hl=address)
                    assert not state['flags']['C'], (name, address, sector)
                    assert all(state['registers'][k]['dec'] == v for k, v in
                               {'bc': 0x02A7, 'de': sector, 'hl': address, 'sp': 0x8F00}.items())
                    payload = expected[sector * 256:(sector + 1) * 256]
                    assert m.read(start, len(guard)) == (b'\xDA' if address else b'') + payload + b'\xAD', (name, address, sector)
                    timings.append(ticks)
                assert len(set(timings)) == 1, (name, address, timings)
                counts[f'{address:04X}'] = {'cpu_cycles': timings[0],
                                          'kib_s': 500000 / timings[0]}
            report['variants'][name] = {'bytes': path.stat().st_size,
                                       'sha256': hashlib.sha256(path.read_bytes()).hexdigest(),
                                       'buffers': counts, 'checked_reads': len(counts) * 20}
            print(name, counts['2000'], counts['8200'], flush=True)
    finally:
        install(canonical)
        for sector in changed:
            m.write(0x4000, original[sector * 256:(sector + 1) * 256])
            state, _ = m.subroutine(driver, bc=0x100, de=sector, hl=0x4000)
            assert not state['flags']['C']
    assert image_path.read_bytes() == original
    report['system_image_restored'] = True

    # Проходим штатный строковый интерфейс, два чтения и явный пропуск записи.
    s = symbols(ROOT / 'source-apps/diskbench/build/DISKBEN.sym')
    m.write(0, (ROOT / 'bin/DISKBEN.i80').read_bytes())
    m.stop_at(s['menu_drive']);m.call('regs_set',pc=0);m.until(s['menu_drive'])
    m.stop_at(s['run_tests'])
    m.line('a');m.line('0');m.line('0')  # B=1, бюджет 0: только чтение.
    m.until(s['run_tests'])
    assert m.read(s['v_layout'],1)==b'\1'
    m.screenshot(out/'read-summary.png')
    m.stop_at(s['print_result'])
    for case,buffer in enumerate((0x2000,0x8200)):
        m.until(s['print_result'])
        ticks=int.from_bytes(m.read(s['v_elapsed'],4),'little')
        assert ticks>0 and m.read(s['v_case'],1)==bytes([case])
        assert int.from_bytes(m.read(s['io_buffer']+1,2),'little')==buffer
        record={'ticks':ticks,'operations':1024,'kib_s':256/(ticks*256/2000000)}
        report['ui'][f'{buffer:04X}']=[record]
        print('DISKBEN',hex(buffer),record,flush=True)
        m.call('debug_step',mode='into')
    m.stop_at(s['exit_wait']);m.until(s['exit_wait'])
    assert m.read(s['v_case'],1)==b'\4'
    m.screenshot(out/'read-results.png')
    assert image_path.read_bytes() == original
    (out / 'measurements.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')


if __name__ == '__main__':
    main()
