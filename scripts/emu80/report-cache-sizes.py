"""Сводка трёх размеров кэша: 1/4/22 КиБ."""
import json, shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2];out=ROOT/'docs/diskbench/cache-003'
old=json.loads((ROOT/'docs/diskbench/cache-002/results.json').read_text())
new=json.loads((out/'results.json').read_text());assert len(new['rows'])==8
def rate(r,k):return 3072*7812.5/1000/r['metrics'][k][0]
def read(r):
    low=f'{rate(r,"fileRead"):.2f}'
    return f'{rate(r,"fileReadHigh"):.2f}/'+low if r['drive']=='A' else low
lines=['| Диск / драйвер | Режим | READ, кэш 1 КиБ | READ, кэш 4 КиБ | READ, кэш 22 КиБ | WRITE, кэш 1 КиБ | WRITE, кэш 4 КиБ | WRITE, кэш 22 КиБ |',
       '|---|---|---:|---:|---:|---:|---:|---:|']
for middle in new['rows']:
    def find(v):return next(r for r in old['rows'] if r['variant']==v and r['drive']==middle['drive'] and r['driver']==middle['driver'] and r['mode']==middle['mode'])
    group=[find('baseline'),middle,find('large')]
    lines.append('| '+' | '.join([middle['drive']+': '+middle['driver'],'W' if middle['mode']==1 else 'W+F']+[read(r) for r in group]+[f'{rate(r,"fileWrite"):.2f}' for r in group])+' |')
text='''# Кэши 1 / 4 / 22 КиБ — 01.11.2026

Файловый доступ DOS, десятичные кБ/с. Файл 1 КиБ, три прохода,
Emu80 4.0.575, CPU/ВИ53 2 МГц. W — запись, W+F — запись с полной проверкой.
Пары у A: означают **противоположная / своя** половина адресного пространства
относительно ROM-окна: верхняя / нижняя. WRITE DOS здесь из нижней памяти.
Эмуляторные результаты не заменяют измерение реальной платы.

'''+ '\n'.join(lines)+'''

## Выводы

4 КиБ (16 слотов) — лучший из трёх проверенных размеров для записи RAM B:
14,47 против 8,41 и 12,88 кБ/с без проверки. Это не доказательство глобального
оптимума. На системной flash A: и внешней H: запись с 4 КиБ медленнее 1 КиБ.
На H: чтение с 4 КиБ также немного медленнее 1 КиБ; целиком FAT/root не влезают.
22 КиБ сильно ускоряют чтение H:, но увеличивают стоимость записи.

Размер кэша сам по себе не решает задачу. Текущие поиск слота и поиск LRU
линейны, FAT обходится по отдельным элементам. До переноса NC/ядра в другую
страницу полезно отдельно проверить прямое индексирование резидентных
метаданных или быстрый повторный доступ к текущему сектору FAT. Это пока
предложения, такие оптимизации в данном опыте не реализованы.

## Условия

1/22 КиБ взяты из cache-002, 4 КиБ — из этой серии cache-003. Исполняется
тот же сохранённый DISKBEN.com и те же 3×1 КиБ; исходный бинарник и символы
скопированы в artifacts/. Дополнительный timer_sample в cacheSector включён
во все три изолированных ядра. Накладные расходы измерителя включены;
статистическая погрешность по независимым сериям не оценивалась.

Кэш 4 КиБ: 2800h–37FFh, таблица 2400h–247Fh. Payload 2000h–23FFh,
верхнее окно 8200h–8DFFh. Пересечений нет, контрольные байты сохранены.
После каждого прохода dirty-флаги обнулены штатным завершением файла,
после комбинации FAT и прежние файлы сохранены, TMP удалён. Все 8 сочетаний
прошли проверки. Обычный профиль остаётся с кэшем 1 КиБ.

Подробности устройства кэша, flush и проверки полного размещения всех
88 секторов метаданных H: см. ../cache-002/RESULTS.md.

Воспроизведение: pack-dos.py --output <новый отчёт>/middle/DOS.sys
--define=FAT_CACHE_ADDR=02800h --define=FAT_CACHE_SIZE=4096
--define=FAT_CACHE_TABLE_ADDR=02400h --define=FAT_CACHE_BENCH_SAMPLE=1.
Затем benchmark-cache.py --sampled --variants middle --report-directory
<новый отчёт> --benchmark-binary docs/diskbench/cache-002/artifacts/DISKBEN.com.
Тестовый Emu80 закрыт, основной runtime не менялся.
'''
art=out/'artifacts';art.mkdir(exist_ok=True)
for name in ['DISKBEN.com','DISKBEN.sym']:
    shutil.copyfile(ROOT/'docs/diskbench/cache-002/artifacts'/name,art/name)
(out/'RESULTS.md').write_text(text,encoding='utf-8')
(out/'TABLE.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
print('\n'.join(lines))
