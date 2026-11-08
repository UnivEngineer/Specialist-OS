"""Общая таблица FONTBEN из одного прогона и списка измеренных вариантов."""
from pathlib import Path
import argparse
import json
import re

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--results', type=Path, default=ROOT / 'docs/bench/font-matrix-004/results.json')
    parser.add_argument('--out', type=Path, default=ROOT / 'docs/bench/font-matrix-004/table.md')
    args = parser.parse_args()
    result = json.loads(args.results.read_text(encoding='utf-8'))
    source = (ROOT / 'source-apps/fontbench/matrix-main.inc').read_text(encoding='utf-8')
    labels = dict(re.findall(r"^(label\w+): DB '([^']*)',0", source, re.M))
    rows = []
    for case in re.findall(r'^    CASE (.+)$', source, re.M):
        core, kind, flags, main, bank, main_misses, bank_misses, label = case.split(',')
        rows.append((labels[label].strip().rstrip(':'), result['ms'][main], result['ms'][bank]))
    text = '# FONTBEN: 1152 байта README, 48×24, Emu80 2 МГц\n\n'
    text += '| Вариант | Main, мс | DOS, мс |\n|---|---:|---:|\n'
    for label, main, bank in rows:
        text += f'| {label} | {main:.3f} | {bank:.3f} |\n'
    text += '\nBIOS char — неизменённый штатный printChar; Opt — экспериментальная замена только пиксельного бэкенда. Serial сохраняет регистры один раз на строку 48 байт. Win20 передаёт через общее окно 20+20+8 байт с чтением рендером прямо из окна, Far48 копирует строки по 48 байт через memcpy_far в main-буфер, Far1152 копирует весь корпус одним memcpy_far; их main-столбец повторно измеряет соответствующий пакетный вход на 48 байт. Время копирования включено, установка тестового шлюза исключена. У Quad без указания транспорта DOS передаёт по четыре кода регистрами. Во всех случаях один корпус и одна конечная сборка; это модель, не физическое оборудование.\n'
    args.out.parent.mkdir(parents=True, exist_ok=True)
    args.out.write_text(text, encoding='utf-8')
    print('FONTBEN table:', len(rows), 'cases')


if __name__ == '__main__':
    main()
