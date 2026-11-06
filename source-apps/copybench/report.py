"""Таблица из сохранённого result.json; повторный запуск эмулятора не нужен."""
import argparse
import json
from pathlib import Path

def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('directory', type=Path)
    args = parser.parse_args()
    data = json.loads((args.directory/'result.json').read_text(encoding='utf-8'))
    lines = ['# Сравнение межстраничных копировщиков', '',
             'Emu80, CPU 2 МГц; три одинаковых повтора, данные и границы назначения проверены. Время — мс, скорость — десятичные кБ/с. Это не замер физического оборудования.', '',
             'Установка алгоритма исключена. Включены подготовка параметров и цикл вызовов; FAR дополнительно проверяет запрос и сохраняет/восстанавливает окно. CPM16 требует размер, кратный 16, и не сохраняет прежний профиль окна.', '',
             '| Алгоритм | Источник → назначение; инициатор | Байт | мс | кБ/с |',
             '|---|---|---:|---:|---:|']
    for r in data['rows']:
        lines.append(f"| {r['name']} | {r['source'][0]} → {r['destination'][0]}; {r['caller']} | {r['size']} | {r['ms']:.4f} | {r['kB_s']:.2f} |")
    lines += ['', 'Страницы: 0 — main, 1…16 — ДОЗУ 0…15. Сырые такты, адреса и SHA-256 бинарников находятся в result.json.', '']
    (args.directory/'RESULTS.md').write_text('\n'.join(lines), encoding='utf-8')

if __name__ == '__main__':
    main()
