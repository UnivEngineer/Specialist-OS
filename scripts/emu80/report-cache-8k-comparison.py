"""8 КиБ: прежние 1/4 КиБ, отдельный 22 КиБ и контроль Raw из matrix-002."""
import hashlib, json
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2];out=ROOT/'docs/diskbench/cache-005'
small=json.loads((ROOT/'docs/diskbench/cache-004/results.json').read_text())
big=json.loads((out/'results.json').read_text())
raw=json.loads((ROOT/'docs/diskbench/matrix-002/raw.json').read_text())
assert len(small['rows'])==24 and len(big['rows'])==8
assert small['fileBytes']==big['fileBytes']==raw['fileBytes']==8192
def rate(r,k,source):return source['fileBytes']*source['passes']*7812.5/1000/r['metrics'][k][0]
def find(source,driver,mode,variant=None):
    return next(r for r in source['rows'] if r['driver']==driver and r['mode']==mode and
                (variant is None or r['variant']==variant))
drivers=['FLASHSYS','FLASH','FLASHFST','RAMD']
def table(key,mode):
    title='READ' if key=='fileRead' else 'WRITE'
    lines=[f'| Диск / драйвер | Raw {title} | DOS, кэш 1 КиБ | DOS, кэш 4 КиБ | DOS, кэш 22 КиБ |',
           '|---|---:|---:|---:|---:|']
    for driver in drivers:
        original=find(raw,driver,'write' if mode==1 else 'write+file-verify','Buffered')
        a=find(small,driver,mode,'baseline');b=find(small,driver,mode,'middle');c=find(big,driver,mode,'large')
        rawkey='rawRead' if key=='fileRead' else 'rawWrite'
        values=[rate(original,rawkey,raw),rate(a,key,small),rate(b,key,small),rate(c,key,big)]
        lines.append('| '+' | '.join([a['drive']+': '+driver]+[f'{v:.2f}' for v in values])+' |')
    return '\n'.join(lines)
read=table('fileRead',1);write=table('fileWrite',1);verify=table('fileWrite',6)
report='''# Кэши 1 / 4 / 22 КиБ, файл 8 КиБ — 01.11.2026

Файл 8192 байта, три прохода, объём каждой операции 24576 байт. Emu80
4.0.575, CPU/ВИ53 2 МГц. Скорости десятичные кБ/с (1000 байт/с), из U32
тиков, без округления промежуточных величин. Это измерения модели.

**Для 1 и 4 КиБ использованы уже завершённые замеры cache-004** на файле
8 КиБ. 22 КиБ измерены отдельно в cache-005.
Контрольные Raw скорости взяты из прежней matrix-002, также 3×8192 байта;
они не пересчитаны из новой серии. У Raw нет работы с кэшем метаданных.

Для A: в этой сравнительной таблице показана только «своя» нижняя половина
адресного пространства CPU: верхние окна двух опытов имеют разный размер,
поэтому их показатели здесь не смешиваются. Полные пары «противоположная /
своя» из прежней серии сохранены в ../matrix-002/RESULTS.md.

На 8-КиБ файле выигрыш кэша 4 КиБ при записи RAM B: невелик — около 5%,
в отличие от опыта 1 КиБ. Кэш 22 КиБ улучшает чтение H: примерно на 19–24%,
но запись H: оказывается примерно вдвое медленнее кэша 1 КиБ. Большой кэш
не даёт общего ускорения нынешнего алгоритма. Линейный поиск/LRU и обход
FAT сохранены; прежде архитектурного переноса нужен отдельный опыт с быстрым
доступом к резидентным метаданным. Стандартный кэш остаётся 1 КиБ.

## Чтение после обычной записи

'''+read+'''

## Обычная запись

'''+write+'''

## Запись всего файла с последующей проверкой

В Raw и DOS ниже проверка данных включена во время WRITE. Это W+F,
а не синхронная посекторная B=5. В READ выше дополнительных проверок нет.

'''+verify+'''

## Сопоставимость и память

cache-004: файл 2000h–3FFFh, таблица слотов с 1400h, кэши 4/16 КиБ с
4000h; кэш 1 КиБ — штатные FB00h–FEFFh. Верхнее окно 8200h–8DFFh.
Серия 1/4/16 КиБ целиком сохранена в ../cache-004/RESULTS.md.

Чтобы вместить файл 8 КиБ и весь кэш 22 КиБ под экраном, cache-005 использует
отдельный DISKBEN: код до 1300h, таблица 1300h–15BFh, оригинал 1600h–35FFh,
кэш 3600h–8DFFh. Освобождённые FB00h–FDFFh — верхнее окно на три сектора,
FE00h–FEFFh — временный сектор FAT. Стек SP=8FDFh, резерв 320 байт; CALL-
harness 8E00h. Контрольные байты 15C0h–15FFh, пересечения исключены ASSERT.

Таким образом, сравнение 1/4 с 22 КиБ имеет различие размещения. Для A:
оно меняет также физический путь загрузки метаданных: прежний кэш 1 КиБ
был в верхней RAM, 22 КиБ — в нижней. Это не идеальное выделение одного
фактора «размер». Верхние чтения исключены из сводки. Повторные baseline-
замеры с новым размещением остановлены по просьбе пользователя; их начало
сохранено как aborted-baseline.json, в итоговые числа не включено.

Обычный DOS и штатный DISKBEN побайтово прежние. В измерительных ядрах
одинаково включён дополнительный timer_sample на cacheSector; цена callback
входит во время. Внутренние линейные поиск/LRU кэша не оптимизировались.
Увеличение размера не равносильно прямой резидентной FAT без поиска.

Восемь сочетаний 22 КиБ завершены, каждый по три прохода. Оригинал и
сигнальные байты сохранены, TMP удалён, FAT и существовавшие файлы не изменены.
На каждой границе файла dirty-слоты сброшены штатным завершением. Кэш 88×256
вмещает всю FAT (64 сектора) и root (24 сектора) H:. На A: нужно 24 слота,
на B: — 40. Обычные данные файла через кэш не проходят.

## Воспроизведение и исходные данные

results.json содержит новые U32 тики, VERIFY, три прохода, адреса и проверки;
сравнительные данные — ../cache-004/results.json и ../matrix-002/raw.json.
artifacts/ содержит отдельный DISKBEN, large/ — ядро с параметрами и листингом.
Тестовый runtime Emu80-flash-test закрывается сценарием, основной не меняется.

Команда опыта: benchmark-cache.py --sampled --variants large --file-kib 8
--report-directory docs/diskbench/cache-005
--benchmark-binary docs/diskbench/cache-005/artifacts/DISKBEN.com.
Для повтора нужен новый каталог и сборки с теми же параметрами.
'''
residency=out/'residency/results.json'
if residency.exists():
    r=json.loads(residency.read_text());assert r['allFatAndRootSectorsResident'] and r['explicitVolumeFlushClearsDirty']
    report+='\nОтдельно проверено одновременное присутствие всех секторов 1–88 H: в кэше.\nПринудительный C899h очистил dirty-флаг сектора 88; образ побайтово прежний.\n'
(out/'RESULTS.md').write_text(report,encoding='utf-8')
(out/'TABLE.md').write_text(read+'\n\n'+write+'\n\n'+verify+'\n',encoding='utf-8')
(out/'hashes.json').write_text(json.dumps({str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.suffix in ['.sys','.com']},indent=2)+'\n')
print(read+'\n\n'+write+'\n\n'+verify)
