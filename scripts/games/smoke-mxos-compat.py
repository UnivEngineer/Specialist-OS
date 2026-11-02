"""Короткий запуск библиотеки без ввода в изолированном Emu80.

Нужен отдельный runtime .local/monitor-compat-001 с MCP localhost:19781.
Это проверка старта и наблюдение зависимостей, не функциональная приёмка игр.
"""
import argparse
import hashlib
import json
from pathlib import Path
import runpy
import subprocess

ROOT = Path(__file__).resolve().parents[2]
Machine = runpy.run_path(str(ROOT / 'scripts/emu80/check-flash.py'))['Machine']
audit = runpy.run_path(str(ROOT / 'scripts/games/audit-mxos-compat.py'))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--runtime', type=Path, required=True)
    parser.add_argument('--limit', type=int)
    parser.add_argument('--ms', type=int, default=250)
    args = parser.parse_args()
    runtime = args.runtime.resolve()
    if runtime != ROOT / '.local/monitor-compat-001':
        raise ValueError('Разрешена только изолированная monitor-compat-001')
    image = (runtime / 'specialist-os/system.bin').read_bytes()
    if image != (ROOT / 'images/SystemRom_emulator.bin').read_bytes():
        raise ValueError('Образ тестовой машины отличается от текущей сборки')
    # До любых записей убедиться, что единственный слушатель — наш экземпляр.
    command = "$ErrorActionPreference='Stop'; @(Get-NetTCPConnection -State Listen -LocalPort 19781 | ForEach-Object { (Get-CimInstance Win32_Process -Filter ('ProcessId = '+$_.OwningProcess)).ExecutablePath }) | ConvertTo-Json -Compress"
    answer = subprocess.run(['powershell.exe', '-NoProfile', '-Command', command],
                            check=True, capture_output=True, text=True).stdout
    owners = json.loads(answer)
    owners = [owners] if isinstance(owners, str) else owners
    expected = str(runtime / 'Emu80qt-mcp.exe').lower()
    if len(owners) != 1 or owners[0].lower() != expected:
        raise ValueError(f'MCP принадлежит другой установке: {owners}')
    m = Machine('http://127.0.0.1:19781/mcp')
    m.call('debug_break')
    m.call('bp_clear')
    m.call('emu_reset')
    # Проверить именно ROM в памяти процесса, а не только файл на диске.
    actual_rom = b''.join(m.read(i, 4096) for i in range(0, 32768, 4096))
    if actual_rom != image[32768:]:
        raise ValueError('Загружен другой ROM; проверить абсолютные пути профиля')
    m.call('emu_run_for', ms=1500)
    m.call('debug_break')
    baseline = b''.join(m.read(i, min(4096, 0xFFE0-i)) for i in range(0, 0xFFE0, 4096))
    assert baseline[0xC000] == 0xC3 and baseline[0xC800] == 0xC3
    reference = (ROOT / 'assets/programs/Monitor2.mon').read_bytes()
    kernel_size = (ROOT / 'bin/DOS.sys').stat().st_size
    records = []
    corpus = list(audit['inventory']())
    if args.limit:
        corpus = corpus[:args.limit]
    for index, (record, payload) in enumerate(corpus):
        result = {'file': record['file'], 'sha256': record['sha256'], 'runs': [],
                  'compatibility': 'unconfirmed'}
        if payload is None or record['load'] + len(payload) > 0x9000:
            result['skip'] = record.get('error', 'not_a_low_memory_application')
            records.append(result)
            continue
        for mode in ('mxos_native', 'monitor2_std'):
            m.call('debug_break')
            m.call('bp_clear')
            # Reset возвращает MX-маппинг; сохранённая память даёт одинаковый
            # исходный код, переменные и стек без зависимости от прошлой игры.
            m.call('emu_reset')
            # Reset выбирает ROM-страницу 2: перед записью обязательно вернуть
            # основную RAM. Иначе записи ниже 8000h попали бы в ROM и пропали.
            m.write(0xFFFC, b'\x00')
            m.write(0, baseline)
            m.write(record['load'], payload)
            observed = b''.join(m.read(record['load']+i, min(4096,len(payload)-i))
                                for i in range(0,len(payload),4096))
            if observed != payload:
                raise RuntimeError(f'Нагрузка не записана в RAM: {record["file"]}')
            m.write(0x9000, bytes(0x3000))
            reached = True
            if mode == 'monitor2_std':
                m.write(0xC000, reference)
                m.write(0x8FD0, record['entry'].to_bytes(2, 'little'))
                m.call('regs_set', pc=0xC000, sp=0x8FD0, af=0, bc=0, de=0, hl=0)
                m.call('bp_set', addr=record['entry'], type='exec')
                m.call('emu_run_for', ms=250)
                state = m.call('cpu_state')
                reached = state['registers']['pc']['dec'] == record['entry']
                m.call('bp_clear')
            else:
                m.write(0xFFBE, b'\x00\xC8')
                m.call('regs_set', pc=record['entry'], sp=0xFFBE, af=0, bc=0, de=0xFF00, hl=0)
                for addr in range(0xF800, 0xF804):
                    m.call('bp_set', addr=addr, type='access')
                for addr in (0xC000, 0xC800):
                    m.call('bp_set', addr=addr, type='exec')
            if reached:
                m.call('emu_run_for', ms=args.ms)
            state = m.call('cpu_state')
            screen = m.read(0x9000, 4096) + m.read(0xA000, 4096) + m.read(0xB000, 4096)
            changes = []
            if mode == 'mxos_native':
                actual = b''.join(m.read(0xC000+i, min(4096,kernel_size-i)) for i in range(0,kernel_size,4096))
                changes = [0xC000+i for i, (a,b) in enumerate(zip(actual,baseline[0xC000:])) if a!=b]
            result['runs'].append({'mode': mode, 'entry_reached': reached,
                                   'requested_ms': args.ms, 'state': state,
                                   'screen_nonzero_bytes': sum(x!=0 for x in screen),
                                   'screen_sha256': hashlib.sha256(screen).hexdigest(),
                                   'kernel_changed_addresses': changes})
        records.append(result)
        if (index+1)%12 == 0:
            print(f'Обработано {index+1}/{len(corpus)}', flush=True)
    m.call('debug_break')
    m.call('bp_clear')
    m.call('emu_reset')
    m.call('emu_run_for', ms=1500)
    m.call('debug_break')
    output = ROOT / 'docs/compat/monitor-001-smoke.json'
    report = {'experiment':'monitor-001', 'date':'2026-10-31',
              'method':'Emu80 8080, explicit RAM mapping and full payload readback, isolated ROM profile, no keyboard input',
              'limits':['250 ms is not a functional test; breakpoints may stop earlier.',
                        'No games declared compatible solely from this smoke test.'],
              'system_image_sha256':hashlib.sha256(image).hexdigest(),
              'rom_verified_in_emulator':True,
              'files':records}
    output.write_text(json.dumps(report,ensure_ascii=False,indent=2)+'\n',encoding='utf-8')
    print(f'Сохранено {len(records)} результатов: {output}', flush=True)


if __name__ == '__main__':
    main()
