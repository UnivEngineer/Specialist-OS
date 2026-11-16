"""Refresh user help from the completed, verified resident FontBen matrix."""
from pathlib import Path
import json

R = Path(__file__).resolve().parents[2]
O = R / 'docs/bench/resident-vs-standalone'
names = ['Rotate', 'Compact', 'Packed6K', 'Fast12K', 'Fast8', 'Cpm4']
rows = []
for name in names:
    folder = R / 'docs/bench/window8-grid/Fast8' if name == 'Fast8' else O / name.replace('Fast12K','Fast10K')
    proof = json.loads((folder / 'verified.json').read_text())
    assert proof['pixels'] and proof['residentPreserved']
    data = json.loads((folder / 'results.json').read_text())['results']
    rows.append((name, *[next(r['ms'] for r in data if r['kind'] == k and r['route'] == 0) for k in (2, 4)]))

def read_text(path):
    return path.read_bytes().decode('koi8-r').replace('\r\n', '\n').replace('\r', '\n')

def write_text(path, text):
    path.write_bytes(text.replace('\r', '').replace('\n', '\r').encode('koi8-r'))

p = R / 'assets/system-rom/fonts/README.TXT'
text = read_text(p).replace('2576', '3088').replace('8x8/Win8', '8x8/Fast8').replace('Win8 8x8:', 'Fast8 8x8:').replace('Win8                 0', 'Fast8                0')
start = text.index('Скорость вывода, мс')
end = text.index('Файлы и импорт', start)
table = '\n'.join(f'{name:<12} {z:9.3f}  {lim:9.3f}' for name, z, lim in rows)
text = text[:start] + '''Скорость вывода, мс
------------------
FontBen, Emu80, CPU и ВИ53 2 МГц, 2026-11-13; Fast8: 2026-11-14.
MAIN, прямой BIOS ABI, обычный цвет, без подготовки.
Один корпус README: 24 строки по 46 символов + CRLF,
1152 байтов. StringZ обрабатывает CRLF, RawLim рисует
их глифы. При разной ширине площадь вывода разная.
Это эмуляторный замер, не физическое оборудование.

Движок         StringZ     RawLim
''' + table + '''

Обычные и инверсные ядра раздельны во всех модулях.
В обычном пиксельном цикле нет проверки инверсии
и пустых XOR/NOP. Cpm4 обрабатывает пары через стек;
Fast8 держит указатели в регистрах. Счётчик пакета
8-битный, курсор обновляется после пакета.
FontBen актуальные standalone собирает тот же код, что FONT DLL.

KOI7/8: штатный bios_setFontEncoding, A=0 для KOI8,
A<>0 для KOI7; регистры сохраняются. Переставляются
битмапы и пререндер, у Compact - индексы. В горячем
цикле нет перекодирования. Повторный выбор безопасен.

''' + text[end:]
write_text(p, text)

p = R / 'source/font/README.md'
text = p.read_text(encoding='utf8')
start = text.index('Пользовательская инструкция всех шести движков:')
end = text.index('## Единый код и кодировка', start)
table = '\n'.join(f'| {name} | {z:.3f} | {lim:.3f} |' for name, z, lim in rows)
text = text[:start] + '''Пользовательская инструкция всех шести движков: `A:\\FONTS\\README.TXT`, KOI8-R, окончания строк CR. Ниже — FontBen, локальный ВИ53 Emu80 2 МГц, прямой MAIN ABI, 1152 байтов (24 × 46 + CRLF), без подготовки, обычный вывод. StringZ обрабатывает CRLF, RawLim рисует их. Это эмулятор, не оборудование; при разных ширинах площадь вывода различается. Полная [матрица BIOS / MAIN, включая инверсию](../../docs/bench/resident-vs-standalone/README.md) содержит исходные измерения и условия проверки.

| Движок | StringZ, мс | RawLim, мс |
|---|---:|---:|
''' + table + '\n\n' + text[end:]
p.write_text(text, encoding='utf8')

write_text(R / 'assets/benchmarks/fontben/Readme.txt', '''Бенчмарки FontBen и TextBen
==========================

FontBen.com - MAIN/BANK сетка пяти текстовых API.
FontBank.com - самостоятельный банковый участник.
DLL движков самостоятельно не запускаются.

Char и CharRaw: MAIN, BANK COMMAND, BANK TEXT.
StringZ, StringRawZ, StringRawLim: MAIN, BANK Win20,
Far256 и собственный MAIN-буфер. MXCALL входит в замер.
Строковые банковые вызовы автоматически режут текст.

Параметры (регистр не важен):
-E=BIOS        установленный движок, по умолчанию
-K             то же, что -E=BIOS
-E=STANDALONE  шесть актуальных актуальные standalone-движков
-E=ALL         BIOS и шесть актуальные standalone-движков
-E=LEGACY      прежние контрольные реализации
-E=имя,имя     отдельные движки
-I             инверсия для BIOS и актуальные standalone
-P=OFF|ON|BOTH  состояния пререндера только для LEGACY
-S             только посимвольные API
-N             не сохранять отчёт
-Q             без ожидания Enter
-?             справка
Вместо = можно использовать пробел.

Актуальные standalone-движки:
ROTATE6  - Rotate 6x8, вращение, четвёрки
PACK6K - Packed6K 6x8, упакованные фазы
FAST12K - Fast12K 6x8, полные фазы
COMPACT6  - Compact 6x8, 162 глифа FONTKOI
FAST8    - Fast8 8x8, байты без масок
CPM4    - Cpm4 4x8, пары повторенных полубайтов

актуальные standalone и устанавливаемые FONT DLL собираются из
одних исходников. Все имеют отдельные обычные и
инверсные ядра. Исправление движка меняет обе сборки.
FontBen временно подменяет BIOS/MXCALL-входы на DLL
в MAIN и восстанавливает перед отчётом и выходом.
BIOS измеряет установленный модуль без подмены.

BIOS/актуальные standalone всегда испытывают одно состояние модуля:
Packed6K/Fast12K с таблицей, остальные без неё.
Для standalone нужна свободная MAIN: предварительно
установите FONT 6x8/Rotate. Сам FontBen не отключает
и не освобождает резидентный пререндер.

Примеры:
FONT 4x8/Cpm4
FontBen.com -E=BIOS -I
FONT 6x8/Rotate
FontBen.com -E=CPM4,FAST8 -Q
FontBen.com -E=STANDALONE -I -Q

Исторические имена доступны явно, например:
WIN8 ROW6 COL6PRE SCALAR12 TRIPACK ROW8 4DUP MON2.
WIN8 - оригинальное ядро оконного монитора.
FAST8 - ускоренный движок, FONT 8x8/Fast8.
ROW8 - прежнее экспериментальное ядро 8x8.
ROW6 (бывший PAGED) - построчные страницы 6x8.
COL6PRE (COLPRE) - посимвольная запись столбцов.
SCALAR12 (FULL) - посимвольная таблица 12 КиБ.
Коллекция LEGACY сохранена для сравнения прежнего
кода и поддерживает только обычный вывод.
FontLab/FontMap/FontBlk заменены общей сеткой и DLL.

Текст: A:\\README.TXT, 24 блока по 46 байтов и CRLF,
всего 1152 байта. Char/StringZ обрабатывают CRLF.
Raw рисует их глифы и переносится по краю экрана.
CharRaw рисует даже код 00h. RawZ завершается нулём,
RawLim печатает заданное количество байтов.

18 этапов на модуль. Время измеряет локальный ВИ53
в MAIN и BANK. Подготовка, загрузка DLL, очистка и
печать отчёта исключены из измеряемого интервала.
Результат: B:\\FONTBEN.TXT. Этап, название и время
остаются на экране. После серии - таблица и Enter.

TextBen.com проверяет штатную печать MAIN/BANK и
файловые функции DOS. После завершения - Enter.
''')
print('Updated README.md and KOI8-R / CR help from verified FontBen runs')
