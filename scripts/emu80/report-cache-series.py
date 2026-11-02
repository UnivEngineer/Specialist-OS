"""Таблицы полной серии кэшей с размером файла из results.json."""
import argparse, hashlib, json, shutil
from pathlib import Path

p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path)
out=p.parse_args().directory
j=json.loads((out/'results.json').read_text());assert len(j['rows'])==24
variants=['baseline','middle','large']
volume=j['fileBytes']*j['passes']
def rate(r,k):return volume*7812.5/1000/r['metrics'][k][0]
def read(r):
    low=f'{rate(r,"fileRead"):.2f}'
    return f'{rate(r,"fileReadHigh"):.2f}/'+low if r['drive']=='A' else low
sizes=[next(r['cacheSlots'] for r in j['rows'] if r['variant']==v)//4 for v in variants]
header=['Диск / драйвер','Режим']+[f'READ, кэш {n} КиБ' for n in sizes]+[f'WRITE, кэш {n} КиБ' for n in sizes]
lines=['| '+' | '.join(header)+' |','|---|---|---:|---:|---:|---:|---:|---:|']
for b in j['rows']:
    if b['variant']!='baseline':continue
    group=[next(r for r in j['rows'] if r['variant']==v and r['drive']==b['drive'] and r['driver']==b['driver'] and r['mode']==b['mode']) for v in variants]
    lines.append('| '+' | '.join([b['drive']+': '+b['driver'],'W' if b['mode']==1 else 'W+F']+[read(r) for r in group]+[f'{rate(r,"fileWrite"):.2f}' for r in group])+' |')
raw=['| Диск / драйвер | Режим | Raw READ | Raw WRITE |','|---|---|---:|---:|']
for r in j['rows']:
    if r['variant']!='middle':continue
    value=f'{rate(r,"rawRead"):.2f}'
    if r['drive']=='A':value=f'{rate(r,"rawReadHigh"):.2f}/'+value
    raw.append('| '+' | '.join([r['drive']+': '+r['driver'],'W' if r['mode']==1 else 'W+F',value,f'{rate(r,"rawWrite"):.2f}'])+' |')
special=j['rows'][0]['tableAddress']==0x1300
if not special:
    artifact=out/'artifacts';artifact.mkdir(exist_ok=True)
    for name in ['DISKBEN.com','DISKBEN.sym']:
        if not (artifact/name).exists():shutil.copyfile(out.parent/'cache-002/artifacts'/name,artifact/name)
layout=('''Для полного кэша 22 КиБ подготовлен отдельный DISKBEN с MAX_FILE_KIB=8:
код заканчивается до 1300h, таблица слотов 1300h–15BFh, исходный файл
1600h–35FFh, кэш от 3600h (до 8DFFh у 22 КиБ). Верхнее окно FB00h–FDFFh
и временный сектор FAT FE00h–FEFFh используют освобождённую штатную область
кэша. Стек SP=8FDFh, резерв 320 байт; CALL-harness 8E00h. Все три размера
измерены с одинаковыми адресами и бинарником. Контрольные байты 15C0h–15FFh.
'''
if special else '''Обычное размещение DISKBEN: файл 2000h–3FFFh, таблица слотов с 1400h,
кэш 4/16 КиБ с 4000h, штатный 1 КиБ — FB00h. Верхнее окно 8200h–8DFFh.
Контрольные байты 1800h–1FFFh. Кэш 16 КиБ не вмещает все 88 секторов H:;
поэтому полная серия 1/4/22 КиБ с переносом буферов выделена в cache-005.
''')
report=f'''# Кэш метаданных, файл {j['fileBytes']//1024} КиБ — 01.11.2026

Emu80 4.0.575, 8080/ВИ53 2 МГц. Файл {j['fileBytes']} байт, {j['passes']}
прохода на сочетание. Скорости десятичные кБ/с (1000 байт/с), рассчитаны из
U32 тиков, а не округлённого экрана. Это измерения эмулятора, не платы.
W — B=1; W+F — запись всего файла, затем полное чтение и побайтовое сравнение.
Пары A: означают «противоположная / своя» половина адресного пространства
относительно ROM-окна: верхняя / нижняя. Файловый WRITE выполняется из LOW.

## Файловый доступ DOS

'''+ '\n'.join(lines)+'''

## Прямой доступ, контроль для кэша 4 КиБ

'''+ '\n'.join(raw)+'''

Raw использует те же выделенные кластеры; FAT-цепочка строится вне замера.
Кэш обычные данные не хранит. Табличные Raw WRITE относятся к нижнему
источнику. Верхняя запись в этой серии не смешивается с эффектом кэша.

## Размещение и проверки

'''+layout+'''
FAT/root A: 24 сектора, B: 40, H: 88. В 22 КиБ помещается весь диапазон
метаданных H:, в 16 КиБ — только 64 сектора. Конфигурации DOS изолированы,
обычный профиль не менялся. Схема поиска/LRU сохранена линейной.
Все 24 сочетания завершены: исходный буфер и сигнальные байты сохранены,
TMP удалён, FAT и существовавшие файлы не изменились. На границе каждого
файла проверено отсутствие dirty-слотов. Дополнительные отсчёты timer_sample
на cacheSector одинаково включены во все три ядра; их стоимость входит
во время. Прогрев и три отдельных прохода доступны в perPass результатов.
Сами по себе эти три повтора не дают оценки погрешности независимых серий.

Сохранение dirty при вытеснении и принудительный saveSectorCache с физическим
барьером в конце файла не отключались. Файловая запись включает FAT/каталог.
Отдельный bios_flushVolume сохраняет кэш и вызывает барьер драйвера.

results.json — исходные U32 тики [ticks, flushTicks, flushCalls], VERIFY,
проходы и проверки; TABLE.md — таблица; */DOS.layout.json и листинги —
размещение ядра; *-manifest.txt — образы; *.png — вывод DISKBEN.
Исходный callback и ограничения описаны в source/dos/README.md.
'''
(out/'RESULTS.md').write_text(report,encoding='utf-8')
(out/'TABLE.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
(out/'hashes.json').write_text(json.dumps({str(p.relative_to(out)):hashlib.sha256(p.read_bytes()).hexdigest() for p in out.rglob('*') if p.suffix in ['.sys','.com']},indent=2)+'\n')
print('\n'.join(lines))
