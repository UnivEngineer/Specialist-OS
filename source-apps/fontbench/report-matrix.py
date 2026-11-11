"""Общая таблица FONTBEN из одного прогона и списка измеренных вариантов."""
from pathlib import Path
import argparse
import json
import re

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--results', type=Path, default=ROOT / 'docs/bench/module-api/full/results.json')
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/module-api/full/table.md')
    args = parser.parse_args()
    result = json.loads(args.results.read_text(encoding='utf-8'))
    source = (ROOT / 'source-apps/fontbench/matrix-data.inc').read_text(encoding='utf-8')
    label_source = source + (ROOT / 'source-apps/fontbench/engines/kernel-api.inc').read_text(encoding='utf-8')
    labels = dict(re.findall(r"^(label\w+): DB '([^']*)',0", label_source, re.M))
    rows = []
    for case in re.findall(r'^    CASE (.+)$', source, re.M):
        core, kind, flags, main, bank, main_misses, bank_misses, label = case.split(',')
        if result.get('cases') is not None and label not in result['cases']:
            continue
        banks = result.get('options', [7, 7, 3])[2]
        supported = label not in result.get('unsupportedBankCases', [])
        rows.append((labels[label].strip().rstrip(':'), result['ms'].get(main) if banks & 1 else None, result['ms'].get(bank) if banks & 2 and supported else None))
    text = '# FONTBEN: 1152 байта README, 48×24, Emu80 2 МГц\n\n'
    column = 'BANK' if result.get('nativeBankLaunch') or result.get('publicModuleABI') else 'DOS'
    text += f'| Вариант | Main, мс | {column}, мс |\n|---|---:|---:|\n'
    for label, main, bank in rows:
        fmt = lambda v: '—' if v is None else f'{v:.3f}'
        text += f'| {label} | {fmt(main)} | {fmt(bank)} |\n'
    if result.get('crlfInText'):
        text += '\n1152 печатных байта + 24 CRLF + NUL (1201 байт), шаг строк 10 пикселей. MAIN: один MXOS_CALL PRINT_STRING_MAIN на поток; CHAR — MXOS_CALL PRINT_CHAR на символ. BANK: штатный PRINT_STRING_BANK с порциями до 256 байт, Far48 — 24 переноса по 50 байт и 24 строковых вызова, Far1152 — перенос 1201 байта и один строковый вызов. Standalone временно подключён к BIOS-входам и таблице MXOS_CALL; после этапа они восстановлены. Таймер читается локально. Подготовка и запуск BANK COM исключены. Это замеры Emu80, не физического оборудования.\n'
    elif result.get('publicModuleABI'):
        text += '\nBANK — отдельный EXEC-банк, публичный MODULE_CALL и штатные службы MEM. Win20 — PRINT_STRING_BANK штатного BIOS; для экспериментальных ядер этот банковый путь недоступен. Serial переносит коды через GET8; Far48 и Far1152 — через mx_memcpyFar. Загрузка и вход/возврат основного модуля исключены; вложенные вызовы экспериментальных ядер, чтение таймера и перенос включены. Main у транспортов — контрольный повтор без переноса. Старые DOS-прогоны используют другую методику. Это модель Emu80, не физическое оборудование.\n'
    else:
        text += '\nИсторический прогон DOS с частным шлюзом. Загрузка исключена, перенос текста включён. Это модель Emu80, не физическое оборудование.\n'
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding='utf-8')
    print('FONTBEN table:', len(rows), 'cases')


if __name__ == '__main__':
    main()
