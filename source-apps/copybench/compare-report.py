"""Rebuild the compact README comparison from verified CopyBen measurements."""
from pathlib import Path
import argparse, hashlib, json

HERE = Path(__file__).resolve().parent
ROOT = HERE.parents[1]
BEGIN = '<!-- COPYBEN-COMPARISON-BEGIN -->'
END = '<!-- COPYBEN-COMPARISON-END -->'


def rate(row):
    return row['KiB_s']


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--report', type=Path, required=True)
    parser.add_argument('--update-readme', action='store_true')
    args = parser.parse_args()
    near = json.loads((args.report/'near.json').read_text())
    far = json.loads((args.report/'far.json').read_text())
    reference = json.loads((HERE/'reference.json').read_text(encoding='utf-8'))
    assert all(r['verified'] and len(set(r['ticks'])) == 1 for r in near+far)
    expected = {'MEM': 100, 'CPM original': 45, 'local16': 100, 'local32': 100,
                'local32 aligned256': 30, 'local32 aligned batch': 40}
    assert len(near) == 126
    assert {name: sum(r['algorithm'] == name for r in far) for name in expected} == expected, 'Incomplete far matrix'
    lines = [BEGIN, '', '## Сводка near/far — 13.11.2026', '',
             'Все скорости в следующих таблицах — **КиБ/с (1024 байта)** при 2 МГц. Near измерен на CPU-модели Intel 8080 z88dk-ticks, far — на модели Emu80; это не замеры физического оборудования. По три повтора с одинаковыми тактами; проверены данные, неизменность источника, защитные байты и SP. Для far также проверено восстановление COMMAND после серии. Чтения таймера внутри передающего цикла нет: учитывается целиком интервал исполнения CPU, включая подготовку каждого запроса.', '',
             '### Near: один адресный контекст', '',
             'Основная таблица: источник 2000h, назначение 5000h, области не пересекаются. Вызовы серии развёрнуты в стенде; каждый заново загружает параметры, поэтому цена CALL/RET и подготовки не скрыта. У специальных 256-байтных входов нет универсальных проверок и сохранения регистров данных; SP сохраняется. Знак «—» означает, что контракт не поддерживает один вызов на 8 КиБ.', '',
             '| Алгоритм | 256 Б | 8 КиБ одним вызовом | 32 × 256 Б | Ограничение |',
             '|---|---:|---:|---:|---|']
    for name, comment in [('byte', 'Обычный побайтовый цикл'),
                          ('BIOS memmove', 'Универсальный, выбор направления'),
                          ('BIOS forward', 'Непересекающиеся области'),
                          ('stack256', 'Фиксированные 256 Б, любой адрес'),
                          ('stack256 aligned', 'Назначение xx00h'),
                          ('byte256 aligned x8', 'Назначение xx00h, разворот ×8')]:
        subset = [r for r in near if r['algorithm'] == name and r['direction'] == 'LOW -> LOW'
                  and r['source'] == 0x2000 and r['destination'] == 0x5000]
        def cell(size, batch):
            row = next((r for r in subset if r['size'] == size and r['batch256'] == batch), None)
            return f'{rate(row):.2f}' if row else '—'
        lines.append(f'| {name} | {cell(256, False)} | {cell(8192, False)} | {cell(8192, True)} | {comment} |')
    high = next(r for r in near if r['algorithm'] == 'BIOS memmove' and r['direction'] == 'HIGH -> LOW'
                and r['source'] == 0x9000 and r['destination'] == 0x3000 and r['size'] == 8192 and not r['batch256'])
    lines += ['', f'Общий BIOS memmove при назначении выше источника выбирает обратное направление даже без перекрытия. При копировании 9000h → 3000h он идёт вперёд и даёт {rate(high):.2f} КиБ/с на 8 КиБ. Прямой forward-вход исключает этот выбор. Смещения 00h/01h и F0h/FCh проверены отдельно: выравнивание само по себе не ускоряет POP/PUSH или MOV на 8080. Специализированный alignedStack256 экономит счётчик: INC L стоит те же 5 тактов, что INX H, но формирует Z; на 16 байт исчезает DEC B. Разбиение большого near-переноса на отдельные вызовы по 256 байт не является безусловным ускорением.', '',
              '### Far: подготовка, порции и время жизни COPY', '',
              'Основная таблица: MAIN → DATA, 2000h → 4000h. «COPY» — профиль сохраняется между 32 вызовами, COMMAND восстанавливается один раз в конце; «COMMAND» — восстановление после каждого сектора. Построение всех FAR-пакетов и финальное восстановление входят в замер. Установка экспериментального кода в страницы выполняется до измерения, но дублирование рабочих параметров CPU в оба хвоста входит в каждый вызов.', '',
              '| Алгоритм | 256 Б | 8 КиБ одним вызовом | 32 × 256, COPY | 32 × 256, COMMAND |',
              '|---|---:|---:|---:|---:|']
    for name in ['MEM', 'CPM original', 'local16', 'local32', 'local32 aligned256', 'local32 aligned batch']:
        subset = [r for r in far if r['algorithm'] == name and r['direction'] == 'MAIN -> DATA'
                  and r['source'][1] == 0x2000 and r['destination'][1] == 0x4000]
        assert subset, name
        def cell(size, mode):
            row = next((r for r in subset if r['size'] == size and r['mode'] == mode), None)
            return f'{rate(row):.2f}' if row else '—'
        lines.append(f'| {name} | {cell(256, "whole")} | {cell(8192, "whole")} | {cell(8192, "copy256")} | {cell(8192, "command256")} |')
    lines += ['', 'Контракты различаются. MEM — действующий проверяющий FAR-вход, ленивое сохранение COMMAND и точный размер. Для физического ROM используется доверенный внутренний вход, а не публичный FAR-код. CPM original — исторический CP/M, положительный размер кратен 16, окно не сохраняется; поэтому колонки его серии показывают сырую цену без COMMAND. local16/local32 — приватные прототипы: размер кратен порции, границы и страницы гарантирует стенд; параметры находятся в хвостах источника/назначения. Начальный source selector также записывается в хвост инициатора, если он третий. local32 aligned256 требует ровно 256 байт и выровненный источник. local32 aligned batch принимает целые 256-байтные секторы одним вызовом: младший C завершает сектор, B увеличивается один раз на сектор, отдельный счётчик считает секторы. Эти прототипы не установлены в MEM.SYS.', '',
              'Проверены MAIN↔DATA, DATA→DATA с третьим инициатором, ROM→DATA и ROM→верхняя MAIN; младшие адреса 00h/01h/F0h/FCh, пересечение страницы и пакеты на 256 Б/8 КиБ. У байтового счётчика aligned-варианта ограничение выравнивания обязательно; это отдельный контракт, а не ускорение любого адреса. Полные матрицы не смешивают направления, даже когда показатели совпадают.', '',
              '### Исторические алгоритмы: сохранённые контрольные данные', '',
              'Ниже сохранён автономный повтор исторических исходников: 8 КиБ, MAIN↔RAMD, все скорости заново вычислены из тактов в КиБ/с. У секторных вариантов это 32 вызова по 256 байт; исходный CP/M и исторический FAR получают один запрос на 8 КиБ. Установка кода исключена. Поэтому таблица сохраняет историю, но не подменяет сравнение одинаковых контрактов выше.', '',
              '| Алгоритм | Чтение | Запись | Контракт/порция |',
              '|---|---:|---:|---|']
    legacy = reference['legacyFar']
    for name, label in [('Builtin byte', 'CALL на каждый байт'), ('Builtin block', 'Побайтовый блок 256 Б'),
                        ('stack', 'Буфер 18 Б'), ('loop', 'Буфер 12 Б, обычный цикл'),
                        ('unrolled', 'Буфер 13 Б, разворот'), ('direct', 'Прямой, разворот по 7'),
                        ('regs2', 'Два байта в регистрах'), ('RAMD R4', 'Четыре байта, BC/DE'),
                        ('regs6', 'Шесть байт в регистрах'), ('CPM16', 'Окно 16 Б, запрос 8 КиБ'),
                        ('FAR', 'Исторический универсальный снимок')]:
        readings = []
        for read in [True, False]:
            row = next(r for r in legacy['rows'] if r['size'] == 8192 and r['caller'] == 0
                       and r['name'] == (name if name in ['CPM16', 'FAR'] else name+('-read' if read else '-write'))
                       and r['source'][0] == (3 if read else 0))
            readings.append(row['size']*2000000/row['ticks'][0]/1024)
        lines.append(f'| {name} | {readings[0]:.2f} | {readings[1]:.2f} | {label} |')
    for name in ['stack', 'loop', 'unrolled', 'direct', 'regs2', 'regs4', 'regs6', 'CPMCOPY']:
        assert hashlib.sha256((ROOT/'bin/copybench'/f'{name}.bin').read_bytes()).hexdigest() == legacy['binaries'][name]
    old = next(r for r in reference['stockNear']['copy'] if r['bytes'] == 16384 and r['source'] == 0)
    speed = old['bytes']*2000000/old['stock']['callTicks']/1024
    lines += ['', f'Исторический near memcpy из 5e83e32, совпадающий по инструкциям с вариантом 2022 года, давал {speed:.2f} КиБ/с на 16 КиБ (CPU-модель 04.11.2026, CALL/RET включены, загрузка регистров исключена). Это контроль медленного цикла с CALL сравнения на каждом байте, не нынешний byteCopy из первой таблицы. В полном DOS-замере от 31.10.2026 R4 давал 44,675/42,287 КиБ/с чтения/записи; отличие от автономной строки выше связано с обёрткой DOS и измерением ВИ53 с дополнительными 198 тактами на операцию. Эти цифры не следует смешивать или объявлять регрессией чистого движка.', '',
              'Исходные исторические такты, методика и SHA-256 сохранены в [reference.json](reference.json); сборка восьми локальных исторических образов проверена на совпадение с этими SHA-256. Десятичные кБ/с старых отчётов здесь не используются. Старые таблицы и комментарии README выше оставлены как история прежнего стенда; единицы и контракты актуального сравнения задаёт этот раздел.', '',
              '### Практический выбор', '',
              'Для непересекающегося near-диапазона сначала использовать готовый прямой стековый копировщик и передавать весь диапазон одним вызовом. Не заменять его побайтовым циклом только ради выравнивания. Специальный секторный вход сравнивать с прямым входом на том же размере, включая подготовку. Для far передавать максимальную доступную пачку и сохранять COPY до конца серии; стоимость COMMAND нельзя прятать за границей замера. Увеличение полезной порции окна проверять отдельно от цены дублирования параметров: local16/local32 отличаются именно шириной порции при одной обвязке. Ограниченные aligned-входы не подменяют универсальный memcpy_far.', '',
              '### Воспроизведение', '',
              '```powershell', 'python source-apps/copybench/build.py',
              'python source-apps/copybench/check-local.py --out docs/bench/copyben-new/cpu',
              'python source-apps/copybench/compare.py --report docs/bench/copyben-new --runtime .local/rom-pio-19782 --port 19782',
              'python source-apps/copybench/compare-report.py --report docs/bench/copyben-new --update-readme', '```', '',
              'Для повторного входа после прерывания far-матрицы используется --stage far --resume; исходники и замороженные входы должны оставаться теми же, SHA-256 бинарников проверяются по far-builds.json. --algorithms ограничивает список вариантов. --port задаёт клиентский адрес уже настроенного MCP-сервера, не меняет порт EXE Emu80. Эмулятор собственный, видимый, muted; занятый порт не используется, процесс закрывается в finally. После подмены хвостов запрещены файловые вызовы и возврат в NC: отдельный процесс закрывается целиком. Это ABI-стенд, не проверка интерфейса запуска COPYBEN.COM. Near-часть запускается отдельно через --stage near и не открывает эмулятор.', '', END]
    section = '\n'.join(lines)+'\n'
    (args.report/'SUMMARY.md').write_text(section, encoding='utf-8')
    if args.update_readme:
        path = HERE/'README.md'; text = path.read_text(encoding='utf-8')
        if BEGIN in text:
            first, rest = text.split(BEGIN, 1); _, last = rest.split(END, 1)
            text = first+section.rstrip()+last
        else: text = text.rstrip()+'\n\n'+section
        path.write_text(text, encoding='utf-8')
    print('CopyBen summary generated:', len(near), 'near rows;', len(far), 'far rows')


if __name__ == '__main__': main()
