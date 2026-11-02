"""Отчёт исправленной матрицы: прямой HIGH, полная и посекторная проверка."""
import argparse, csv, hashlib, json, shutil
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('directory', type=Path)
out = p.parse_args().directory
j = json.loads((out / 'raw.json').read_text(encoding='utf-8'))
assert j.get('completedUtc') and len(j['rows']) == 18
tick_ms = 256000 / j['pitHz']
volume = j['fileBytes'] * j['passes']
rows = []
for source in j['rows']:
    r = dict(source, metrics=dict(source['metrics']))
    if r['buffer'] == 'high-direct':
        low = next(x for x in rows if x['id'] == r['id'])
        low['metrics']['rawReadHigh'] = r['metrics']['rawRead']
        low['metrics']['fileReadHigh'] = r['metrics']['fileRead']
    else:
        rows.append(r)

def label(r):
    if r['driver'] in ['ROM', 'ROM native'] or r['mode'] == 'read-only':
        return f"{r['drive']}: {r['driver']} {r['capacityBytes']//1024} КиБ"
    return f"{r['drive']}: {r['driver']}" + (' ' + r['description'] if r['driver'] == 'RAMD' else '')

def mode(r):
    return {'write': 'W', 'write+verify': 'W+S', 'write+file-verify': 'W+F', 'read-only': 'RO'}[r['mode']]

def speed(r, k):
    t = r['metrics'].get(k, [0])[0]
    return volume / (t * tick_ms) if t else None

def fmt(v):
    return '—' if v is None else f'{v:.2f}'

columns = ['rawRead', 'rawReadHigh', 'rawWrite', 'rawWriteHigh', 'fileRead', 'fileReadHigh', 'fileWrite']
def pair(r, key):
    low, high = speed(r, key), speed(r, key+'High')
    return fmt(low) if high is None else fmt(high)+'/'+fmt(low)
table = ['| Диск / драйвер | Режим | Raw READ | Raw WRITE | DOS READ | DOS WRITE |',
         '|---|---|---:|---:|---:|---:|']
flat = []
for r in rows:
    table.append('| ' + ' | '.join([label(r), mode(r)] + [pair(r, k) for k in ['rawRead','rawWrite','fileRead','fileWrite']]) + ' |')
    record = dict(driver=label(r), mode=mode(r))
    for k in columns:
        ticks, flush, count = r['metrics'].get(k, [0, 0, 0])
        record.update({k+'_ticks': ticks, k+'_ms': ticks*tick_ms, k+'_kBps': speed(r, k),
                       k+'_flush_ms': flush*tick_ms, k+'_flush_calls': count})
    record.update(zip(['rawVerify_ms', 'fileVerify_ms'], [v*tick_ms for v in r.get('verifyTicks', [0, 0])]))
    flat.append(record)
with (out / 'results.csv').open('w', encoding='utf-8-sig', newline='') as f:
    w = csv.DictWriter(f, fieldnames=list(flat[0])); w.writeheader(); w.writerows(flat)
(out / 'TABLE.md').write_text('\n'.join(table) + '\n', encoding='utf-8')
flush = ['| Драйвер / режим | Raw flush, мс | Вызовы | DOS flush, мс | Вызовы |', '|---|---:|---:|---:|---:|']
verify = ['| Драйвер | Raw VERIFY, мс | DOS VERIFY, мс |', '|---|---:|---:|']
for r in rows:
    if r['caps'] & 8:
        rw, fw = r['metrics']['rawWrite'], r['metrics']['fileWrite']
        flush.append(f'| {label(r)} / {mode(r)} | {rw[1]*tick_ms:.3f} | {rw[2]} | {fw[1]*tick_ms:.3f} | {fw[2]} |')
    if r['mode'] == 'write+file-verify':
        rv, fv = r['verifyTicks']
        verify.append(f'| {label(r)} | {rv*tick_ms:.3f} | {fv*tick_ms:.3f} |')
art = out / 'artifacts'; art.mkdir(exist_ok=True)
for name, checksum in j['binaries'].items():
    source = art/name if (art/name).exists() else ROOT/'bin'/name
    assert hashlib.sha256(source.read_bytes()).hexdigest() == checksum
    if source != art/name: shutil.copyfile(source, art/name)
for source in [ROOT/'bin/DOS.sym', ROOT/'source-apps/diskbench/build/DISKBEN.sym']:
    if not (art/source.name).exists(): shutil.copyfile(source, art/source.name)
build = json.loads(Path('D:/Projects/RetroPC/Specialist/Emulator/Emu80-flash-test/specialist-build.json').read_text(encoding='utf-8-sig'))
assert all(hashlib.sha256((ROOT/'source-emulators/emu80'/x['path']).read_bytes()).hexdigest() == x['sha256'] for x in build['sources'])
(out/'emulator-build.json').write_text(json.dumps(build, indent=2)+'\n')
report = '''# Исправленная матрица DISKBEN — 01.11.2026

Emu80 4.0.575, 8080 и ВИ53 2 МГц, tWC AT29 = 10 мс. Это результаты модели,
не физического компьютера. Измеряется эмулируемое время ВИ53; RDP и MCP
не входят в интервал. Файл 8 КиБ, три прохода: 24576 байт на операцию.
Скорости десятичные кБ/с (1000 байт/с), рассчитаны из исходных U32 тиков.
Один тик = 0,128 мс; расходы таймера и обёртки не вычитаются.

## Скорости, кБ/с

W — обычная запись. W+S — B=5, проверка каждого записанного сектора.
W+F — обычная запись всего файла с завершением, затем полное чтение
и побайтовое сравнение с оригиналом. RO — только чтение.
Запись A: выполняется один раз из LOW; чтение A: отдельно LOW и HIGH.
Для B:/H: разделения адресов нет. Прочерк означает отсутствие операции.
Пара `51.12/38.30` означает «противоположная / своя»: буфер соответственно
в верхней (8000h–FFFFh) или нижней (0000h–7FFFh) половине адресного пространства
относительно нижнего ROM-окна A:. Это половины адресного пространства CPU,
а не номера банков ROM или страниц RAM-диска. Одиночная запись A: в этой
серии измерена только из LOW; отдельный WRITE HIGH добавлен в следующий опыт.

''' + '\n'.join(table) + '''

## Что исправлено

Старый HIGH* в matrix-001 включал перенос из промежуточного верхнего буфера
обратно в нижнюю память. Он не измерял прямую загрузку в верхнюю память.
Теперь HIGH читает непосредственно в 8200h–8DFFh порциями до 3 КиБ, без
копирования обратно. Нормальное сравнение с оригиналом вне READ; в W+F
оно входит в WRITE и VERIFY. Исходный файл остаётся в 2000h–7FFFh.

A: FLASHSYS Raw READ HIGH = 51,12 кБ/с, LOW = 38,30 кБ/с;
RAMD R4 Raw READ = 42,70 кБ/с. Прямое копирование A: действительно быстрее.
DOS READ HIGH = 34,93 кБ/с, у R4 = 35,83 кБ/с: это уже файловые операции,
с разной геометрией тома, цепочками FAT и дополнительными вызовами.

FlashFst Raw WRITE: W = 18,57; W+S = 6,84; W+F = 8,62 кБ/с.
Проверка после записи сохраняет часть выигрыша конвейера. Через DOS разница
W+S/W+F мала: 2,47/2,51 кБ/с; основной расход находится вне финального flush.
Для последовательных Flash/FlashSys W+F может уступать W+S из-за отдельного
прохода чтения и сравнения. W+F проверяет данные файла; W+S в DOS проверяет
также записываемые сектора метаданных. Их покрытие различается.

## Flush

Времена за три прохода, уже включены в WRITE. Строки без DISK_CAP_FLUSH
не выводятся. Raw вызывает B=4 один раз в конце пакета, DOS fileCreate —
один раз после данных и один раз после сохранения FAT/каталога.
В секторном цикле DOS flush отсутствует. NC передаёт партии до 32 КиБ.

''' + '\n'.join(flush) + '''

## Отдельное время полной проверки

Суммы за три файла, уже включены в WRITE режима W+F. Искусственное заполнение
буфера исключено, чтение и побайтовое сравнение включены.

''' + '\n'.join(verify) + '''

## Проверки и ограничения

Все 18 сочетаний завершены; после объединения LOW/HIGH ROM в таблице 16 строк.
Существовавшие файлы и FAT сохранены, TMP удалён; RO-образы не изменились.
На RAM-диске проверены исходные Boot/FAT/root до и после теста. В host-harness
исправлена команда DCR C вместо ошибочной DCR D в цикле чтения метаданных;
результаты этой серии получены уже с исправлением. Измерительный CALL-harness
перенесён в 8E00h, за пределы окна HIGH. Legacy DOS собран отдельно в отчёте,
основное ядро остаётся R4. Диски C:–G: не установлены (нулевая ёмкость).

Native ROM A: проверен с 64/512 КиБ, внешний ROM H: с 64 КиБ. Для ROM64
использован минимальный набор файлов, для ROM512 полный: разницу DOS-чтения
нельзя объяснять только размером чипа. RO замер прогрет одним чтением.

Отдельная проверка readback-negative-001 ловит порчу байтов 255, 3071, 3327,
8191 обоими путями W+F (DOS/Raw), подтверждает сохранность исходного буфера
и отказ HIGH при несовпадении. Парсер размера проверен на 311 строках,
сравнение — на 25 случаях (1/8/9/16/24 КиБ). Дополнительный полный цикл
1/24 КиБ на A: и B: прошёл: исходный буфер сохранён, TMP удалён,
FAT и существовавшие файлы не изменились (readback-boundary-001).
Сценарий проверки: check-diskbench-readback-boundary.py.
Это не проверка реальной flash и её таймингов.

## Воспроизведение

`python scripts/emu80/benchmark-disks.py --report-directory docs/diskbench/matrix-003`

Нужен новый каталог; существующая серия защищена от перезаписи. Используется
только отдельный Emu80-flash-test, принадлежащий сценарию процесс закрывается.
`python scripts/emu80/report-diskbench-readback.py docs/diskbench/matrix-002`
строит отчёт из raw.json, без повторного замера.

Артефакты: raw.json — исходные тики [ticks, flushTicks, flushCalls] и проверки;
results.csv — вычисленные значения; TABLE.md — таблица; artifacts/ — бинарники
и символы с проверкой SHA-256; emulator-build.json — сборка модели; *.png —
вывод DISKBEN; legacy/ и rom*/ — изолированные сборки и фикстуры.
'''
(out / 'RESULTS.md').write_text(report, encoding='utf-8')
print('\n'.join(table))
