"""Compare baseline and optimized NC/DOS PIT measurements."""
import json
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
OLD = ROOT/'docs/nc/sort-benchmark-2026-11-11'
NEW = ROOT/'docs/nc/sort-optimized-2026-11-11'


def read(folder,name):
    return json.loads((folder/name).read_text())


def main():
    before, after = read(OLD,'results.json'), read(NEW,'results.json')
    assert all(value == before['datasets'][name] for name,value in after['datasets'].items())
    lines = ['# NC и DOS: оптимизация сортировки и списков', '',
             '11.11.2026. Emu80, Intel 8080 и ВИ53 2 МГц. Медианы трёх повторов; единица — миллисекунда. Это измерения модели, не реального компьютера. Исходные данные совпадают побайтно с предыдущим стендом; чтение каталога, подготовка ключей и сортировка измерены раздельно.', '',
             'Рабочий NC теперь использует стабильную LSD radix-сортировку индексов. FILE_INFO и исходный регистр имён не меняются. Режимы 8+3 и 3+8, группировка каталогов, выбранный файл и общий кэш трёх каталогов сохранены. Пререндер отключён.', '',
             '## Штатный NC: весь buildFileIndex', '',
             'Интервал включает заполнение индекса, подготовку ключей и сортировку внутри самого загруженного NC. Чтение DOS, смена диска, шрифт и рисование исключены. До изменений был Shell, после — оптимизированный radix.', '',
             '| Записей | Режим | До, мс | После, мс | Ускорение |', '|---:|---|---:|---:|---:|']
    for name in ('actual-nc.json','actual-nc-250.json'):
        old = {r['mode']:r for r in read(OLD,name)}
        for r in read(NEW,name):
            previous = old[r['mode']]['median_ms']
            lines.append(f"| {r['count']} | {'8+3' if r['mode']==1 else '3+8'} | {previous:.1f} | {r['median_ms']:.1f} | {previous/r['median_ms']:.2f}× |")
    lines += ['', '## Только сортировка на общем стенде', '',
              'Сравнительные методы используют одинаковые оптимизированные компараторы, прямой CALL которых подменяется перед запуском по режиму. В таблицах raw означает исходные имена с преобразованием регистра только при различии байтов; upper — отдельный нормализованный буфер. Заполнение индекса, определение базы, подготовка рангов и проверка пробельных хвостов выполняются перед чистым интервалом сравнительного метода. Поэтому полное время рабочего NC дано отдельно выше. Для radix подготовка ключей измерена отдельно и указана в последнем столбце.', '']
    for mode,title in ((1,'По имени: 8+3'),(2,'По расширению: 3+8')):
        lines += ['### '+title, '',
                  '| Набор | Shell raw / upper | Вставки raw / upper | Слияние raw / upper | Quicksort raw / upper | Radix | Подготовка radix |',
                  '|---|---:|---:|---:|---:|---:|---:|']
        for dataset in after['datasets']:
            current = [r for r in after['results'] if r['dataset']==dataset and r['mode']==mode]
            values = []
            for algorithm in ('shell','insertion','merge','quick'):
                pair = {r['prepared']:r['median_ms'] for r in current if r['algorithm']==algorithm}
                values.append(f"{pair[False]:.1f} / {pair[True]:.1f}")
            radix = next(r for r in current if r['algorithm']=='radix-opt')
            lines.append('| '+dataset+' | '+' | '.join(values)+f" | {radix['median_ms']:.1f} | {radix['preparation_ms']:.1f} |")
        lines.append('')
    lines += ['### До и после оптимизации radix', '',
              '| Набор, режим 3+8 | Прежний radix по разрядам | Новый radix | Подготовка нового |',
              '|---|---:|---:|---:|']
    for dataset in after['datasets']:
        previous = next(r for r in before['results'] if r['dataset']==dataset and r['mode']==2 and r['algorithm']=='radix-columns')
        current = next(r for r in after['results'] if r['dataset']==dataset and r['mode']==2 and r['algorithm']=='radix-opt')
        lines.append(f"| {dataset} | {previous['median_ms']:.1f} | {current['median_ms']:.1f} | {current['preparation_ms']:.1f} |")
    lines += ['', '## Реализованные изменения NC', '',
              '- База FILE_INFO определяется один раз, адреса записей вычисляются локально. Ранги готовятся один раз; сравнительный метод не вызывает ncFileRank на каждой паре.',
              '- Компараторы имени и расширения раздельные. Подменяется операнд прямого CALL в Shell, quicksort и слиянии; вставки используют тот же Shell-проход с шагом 1. Проверки режима внутри сравнения нет.',
              '- Равные сырые байты не вызывают BIOS upperCase. При различии оба кода приводятся штатным способом; сравнение без учёта регистра сохранено.',
              '- Перед сравнительными методами поля 8 и 3 проверяются на канонические пробельные хвосты. Только для такого набора два равных пробела заканчивают сравнение поля, указатели продвигаются к его границе и сравнение продолжается по следующему ключу. При внутреннем пробеле ранний выход отключается для всего набора. A B/A C и расширения A B/A C входят в CPU-регрессию.',
              '- Radix использует 12 массивов по 256 байт: 11 нормализованных разрядов и ранг. Постоянный разряд пропускается. Счётчики очищаются и префиксно суммируются только для фактического min..max; гистограмма строится прямым последовательным чтением разряда. Распределение обходится без компаратора, вызова адресного помощника и умножения индекса на 16.',
              '- Индексы панелей выровнены на 256 байт. Radix чередует входную и временную таблицы, а итоговое копирование делает только при необходимости. Равные ключи сохраняют исходный порядок.', '',
              'У корректных коротких имён пробелы не входят в имя; поля на диске дополнены ими до ширины 8 и 3. Проверка перед ранним выходом позволяет сохранить полное сравнение и для нетипичной записи внешнего образа. [Ограничения 8.3 Microsoft](https://learn.microsoft.com/en-us/openspecs/windows_protocols/ms-fscc/18e63b13-ba43-4f5f-a5b7-11e871b71f14).', '',
              '## Получение списка DOS', '',
              'DOS уже читал сектор каталога один раз и обходил записи по указателю. Затем компактные FILE_INFO собираются в KD_DATA на 4096 байт и передаются клиенту через kdStore / bankCopyFar. При 250 записях это одна пачка 4001 байт; при 300 — две пачки 4096+705. Сырые 32-байтные FAT-записи прямо в NC не копируются.', '',
              'Оптимизированы убывающие skip/capacity, счётчик остатка записей корня и копирование 12 байтов имени/атрибутов без цикла. Обход после исчерпания capacity продолжается для возврата полного числа записей. Границы корня, фильтры, E5/05 и формат FILE_INFO сохранены.', '',
              '| Записей | Кэш корня | До, мс | После, мс |', '|---:|---|---:|---:|']
    old = read(OLD,'list-results.json')['results']
    new = read(NEW,'list-results.json')
    for r in new['results']:
        if r['operation']!='list' or r['count'] not in (127,250):
            continue
        previous = next(x for x in old if x['operation']=='list' and x['count']==r['count'] and x['cache']==r['cache'])
        lines.append(f"| {r['count']} | {'холодный' if r['cache']=='cold-root' else 'тёплый'} | {previous['median_ms']:.1f} | {r['median_ms']:.1f} |")
    memory = read(NEW,'nc-memory.json')
    lines += ['', 'Холодный корень получен remount выбранного H: вне интервала; тёплый — повторным реальным LIST. На тесте 300 записей корень больше общего кэша 8 КиБ, поэтому повторный проход не гарантирует ускорения. Переключение A:→H: и SET отдельно остаются около 32,5 мс. У NC сохраняется собственный кэш, при попадании в который LIST не вызывается.', '',
              '## Проверки и память', '',
              '- 60 CPU-случаев рабочего radix и 60 сравнительного Shell: оба режима, оба столбца/панели, 0/1/255 записей, совпадающие ключи, KOI-8, пробельные хвосты и внутренние пробелы. Отдельно пройдены 15 случаев общего кэша, 24 проверки счётчиков/выбора и проверки инверсии прокрутки.',
              '- 72 конфигурации сортировки по три PIT-повтора на побайтно одинаковых входах. После каждого проверены индекс, FFh и неизменность FILE_INFO. Штатный NC замерен отдельно для 127/250 записей и обоих режимов.',
              '- DOS: три корня 127/250/300 записей, три операции и холодный/тёплый кэш, по три повтора. Дополнительно девять skip/capacity на каждый корень: capacity 0/1/256/300, skip за концом, защитные байты после FFh. Проверены две FAR-пачки, удалённая запись, LFN и неполный последний сектор корня с RootEntCnt=303; BADROOT.TXT за законной границей не выдаётся.',
              '- SRTDIR00.ASM открыт и закрыт штатными клавишами NC, корень восстановил 250 записей. Сохранены снимки и directory-ui.json.',
              '- OS и SystemEmuWritable собраны без ошибок/предупреждений; проверены листинг NC, фиксированное размещение BIOS/DOS и SDK ABI. Тестовые эмуляторы видимые, без звука, закрыты в finally.', '',
              f"Ключи занимают 3072 байта, счётчики и временный индекс — 512 байт. NC_END={memory['nc_end']:04X}h, APP_MEMORY_END={memory['memory_end']:04X}h; свободны {memory['free_bytes']} байт. Пререндер BIOS после запуска отключён. Compile-time NC_SORT_RADIX=0 оставляет сравнительный Shell, UI по-прежнему выбирает только порядок имени/расширения/диска.", '',
              '## Воспроизведение', '', '```powershell',
              'python scripts/emu80/benchmark-nc-sort.py --sizes 32 64 128',
              'python scripts/emu80/benchmark-nc-sort.py --append --fill-h --h-only',
              'python scripts/emu80/benchmark-nc-sort.py --append --baseline-only',
              'python scripts/emu80/benchmark-nc-sort.py --append --baseline-only --fill-h',
              'python scripts/emu80/benchmark-nc-list.py',
              'python scripts/emu80/report-nc-sort-optimized.py', '```', '',
              'Результаты — results.json, actual-nc*.json, list-results.json; исходники до изменений и бинарники — before/. Входные образы тестовых дисков отдельные, системный набор игр не изменён. Все запуски используют собственный runtime .local/nc-sort-benchmark-runtime и требуют свободный MCP-порт 19781.', '']
    (NEW/'report.md').write_text('\n'.join(lines),encoding='utf-8')
    print(NEW/'report.md')


if __name__=='__main__':
    main()
