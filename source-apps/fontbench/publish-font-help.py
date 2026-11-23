
"""Обновить таблицы в C:\\FONTS\\README.TXT из текущих результатов FontBen."""

from pathlib import Path
import re


ROOT = Path(__file__).resolve().parents[2]
RESULTS = ROOT / 'source-apps/fontbench/RESULTS.MD'
HELP = ROOT / 'assets/system-rom/fonts/README.TXT'
ORDINARY_ENGINES = (
    '6x8/Rotate', '6x8/Fast', '6x8/Packed',
    '8x8/Rotate', '8x8/Fast', '8x8/Packed',
    '4x8/Fast', '4x8/Packed', '4x8/Compact',
)
KOI7_ENGINES = ('6x8/K7Rotate', '6x8/K7Packed', '6x8/K7Hybrid')
PHASE_ENGINES = ('8x8/Rotate', '8x8/Fast', '8x8/Packed')


def rows_in_section(text, heading):
    section = text.split(f'\n## {heading}\n', 1)[1].split('\n## ', 1)[0]
    for line in section.splitlines():
        if line.startswith('| `'):
            yield [cell.strip().strip('`') for cell in line.split('|')[1:-1]]


def main_time(cell):
    time = cell.split(' / ', 1)[0]
    if not re.fullmatch(r'\d+,\d{3}', time):
        raise ValueError(f'Некорректное время FontBen: {cell}')
    return time


def format_table(engines, readings, labels):
    header = f'{"Движок":<14}' + ''.join(f'{label:>10}' for label in labels)
    return [header, *(f'{engine:<14}' + ''.join(f'{time:>10}' for time in readings[engine])
                      for engine in engines)]


def main():
    results = RESULTS.read_text(encoding='utf-8')
    ordinary = {r[0]: tuple(map(main_time, (r[7], r[9])))
                for r in rows_in_section(results, 'Общая таблица')
                if r[1:3] == ['Резидентный', 'Нет']}
    koi7 = {r[0]: tuple(map(main_time, (r[4], r[6])))
            for r in rows_in_section(results, '6×8 в KOI-7R')}
    by_phase = {(r[0], r[3]): main_time(r[8])
                for r in rows_in_section(results, '8×8 при явном позиционировании')
                if r[1:3] == ['bios', 'Нет']}
    phases = {engine: [by_phase[engine, str(x)] for x in (0, 2, 4, 6)]
              for engine in PHASE_ENGINES}
    tables = [
        'Время в мс. Emu80, CPU и ВИ53 на 2 МГц. Путь MAIN',
        'через BIOS, обычное начертание. Установка исключена.',
        'Обычная сетка: 21 строка по 42 глифа с CR/LF, всего',
        '924 байта. Замеры собраны из нескольких серий.',
        '',
        *format_table(ORDINARY_ENGINES, ordinary, ('StringZ', 'RawLim')),
        '',
        'KOI-7R, отдельный корпус той же длины:',
        '',
        *format_table(KOI7_ENGINES, koi7, ('StringZ', 'RawLim')),
        '',
        '8x8, RawLim при явной установке X каждой строки.',
        '22 строки по 42 глифа. X дан в пикселях.',
        '',
        *format_table(PHASE_ENGINES, phases, ('X=0', 'X=2', 'X=4', 'X=6')),
        '',
        'Для своего замера запустите FontBen.com -E=BIOS.',
    ]
    text = HELP.read_bytes().decode('koi8-r')
    heading = '\rЗамеры FontBen\r--------------\r\r'
    start = text.index(heading) + len(heading)
    end = text.index('\rФайлы и импорт\r', start)
    updated = text[:start] + '\r'.join(tables) + '\r' + text[end:]
    for line_number, line in enumerate(updated.split('\r'), 1):
        if len(line) > 62:
            raise ValueError(f'Строка {line_number}: {len(line)} символов')
    HELP.write_bytes(updated.encode('koi8-r'))


if __name__ == '__main__':
    main()
