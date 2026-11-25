"""Refresh the current FontBen matrix while preserving earlier raw measurements."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
here = Path(__file__).resolve().parent
baseline_report = here.parent/'fontben-alternating-2026-11-21'
stream_report = here.parent/'fontben-stream-2026-11-21'
orion_report = here.parent/'fontben-orion-m1'


def load(path):
    return json.loads(path.read_text(encoding='utf-8'))


def save(name, data):
    (here/name).write_text(json.dumps(data, indent=2)+'\n', encoding='utf-8')


def key(row):
    return tuple(row[k] for k in ('mode','engine','inverse','phase','pre','kind','route'))


def display(row):
    return f"{row['ms']:.3f}".replace('.', ',')


def unique(rows):
    entries = {key(row):row for row in rows}
    assert len(entries) == len(rows)
    return entries


baseline = unique(load(baseline_report/'results.json'))
streams = unique(load(stream_report/'results.json'))
corpus = {name:load(baseline_report/'metadata.json')[name] for name in ('ordinary','phase','pseudo')}
assert load(stream_report/'metadata.json')['corpus_sha256'] == corpus['ordinary']
workers = [here/f'worker-{i}' for i in range(4)]
for worker in workers:
    assert not load(worker/'errors.json'), worker
    for name,digest in load(worker/'metadata.json').items():
        assert corpus[name] == digest, worker

measured = unique([row for worker in workers for row in load(worker/'results.json')])
assert Counter((row['engine'],row['mode']) for row in measured.values()) == {
    ('6x8/Rotate','bios'):36, ('6x8/Rotate','pseudo'):4, ('6ROTATECOL4','standalone'):36}
assert all(row['pixels']=='oracle' for row in measured.values())
replaced = {k for k in measured if k in baseline}
assert len(replaced) == 40 and all(k[1]=='6x8/Rotate' for k in replaced)

merged = {**streams, **baseline, **measured}
assert len(merged) == len(baseline) + len(streams.keys()-baseline.keys()) + 36
assert all(row['ms']==round(row['ticks']*.128,3) for row in merged.values())
profiles = {}
for signature, row in merged.items():
    profiles.setdefault(signature[:5], []).append(row)
for (mode, _, _, phase, _), rows in profiles.items():
    relevant = 2 if mode=='pseudo' else 5 if phase is not None else 18
    assert len(rows)==relevant, rows


def values(rows, kinds):
    ordered = sorted(rows, key=lambda r:r['route'])
    return [' / '.join(display(row) for row in ordered if row['kind']==kind) for kind in kinds]


def sorted_groups(profiles):
    return sorted(profiles, key=lambda k:(k[0],k[1],k[2],-1 if k[3] is None else k[3],k[4]))


def check(rows):
    outcomes = {row['pixels'] for row in rows}
    assert len(outcomes) == 1, outcomes
    return {'oracle':'Пиксели и курсор','routes':'Равенство маршрутов'}[outcomes.pop()]


# Keep this series separate because the text on the system volume changed.
orion_series = {}
orion_hashes = set()
orion_profiles = (
    ('6x8/Rotate', 'bios', 0, 'bios.json', 5),
    ('6x8/Packed', 'bios', 1, 'packed-scalar.json', 2),
    ('6x8/Fast', 'bios', 1, 'fast-scalar.json', 2),
    ('ORIONM1', 'legacy', 0, 'orionm1.json', 5),
)
for engine, mode, pre, filename, kinds in orion_profiles:
    data = load(orion_report/filename)
    digest = data['corpus_sha256']
    orion_hashes.add(digest)
    assert data['corpus_bytes'] == 924, filename
    expected = {(kind, route) for kind in range(kinds)
                for route in ((0, 1, 2) if kind < 2 else (0, 3, 4, 5))}
    original = data['rows']
    assert len(original) == len(expected), filename
    assert {(r['kind'], r['route']) for r in original} == expected, filename
    assert all(r['ms'] == round(r['ticks']*.128, 3) for r in original), filename
    orion_series[engine] = [
        dict(row, engine=engine, mode=mode, width=6, inverse=False, phase=None, pre=pre,
             pixels='oracle', corpus_sha256=digest, origin=f'fontben-orion-m1/{filename}')
        for row in original
    ]
assert len(orion_hashes) == 1 and corpus['ordinary'] not in orion_hashes
orion_rows = [row for rows in orion_series.values() for row in rows]


lines = [
    '# Текущая матрица FontBen на корпусе 924 байта', '',
    'Emu80 MX2, CPU и ВИ53 2 МГц. Шаг таймера 0,128 мс. '
    'Обычная сетка использует 21 строку по 42 глифа с CRLF. '
    'Фазовая сетка использует 22 явно позиционированные строки по 42 глифа.', '',
    'Для Char и CharRaw порядок значений MAIN / BANK CMD / BANK CMD+Z. '
    'Для StringZ, RawZ и RawLim порядок MAIN / BANK NEAR / BANK Far256 / BANK Own.', '',
    f'Всего {len(merged)+len(orion_rows)} результатов. Основные таблицы содержат {len(merged)} результатов '
    f'предыдущих серий. Ещё {len(orion_rows)} результатов на корпусе с системного C: приведены в отдельном разделе.', '',
    '[Новые и архивные серии](../fontben-rotate-col4-2026-11-21/README.md) · '
    '[Сводные отсчёты](../fontben-rotate-col4-2026-11-21/results.json) · '
    '[Хеши, корпус и происхождение строк](../fontben-rotate-col4-2026-11-21/metadata.json)', '',
]
for group, title in (('bios','Резидентные движки'),('standalone','Актуальные standalone'),
                     ('legacy','Исторические контрольные ядра')):
    lines += [f'## {title}', '',
              '| Движок | Инв. | PRE | Char | CharRaw | StringZ | RawZ | RawLim | Сверка |',
              '|---|---|---:|---:|---:|---:|---:|---:|---|']
    for mode, engine, inverse, phase, pre in sorted_groups(profiles):
        if mode==group and phase is None:
            rows = profiles[(mode,engine,inverse,phase,pre)]
            cells = [f'`{engine}`','Да' if inverse else 'Нет',str(pre),*values(rows,range(5)),check(rows)]
            lines.append('| '+' | '.join(cells)+' |')
    lines.append('')

lines += ['## 8×8 при явном позиционировании', '',
          '| Движок | Сборка | Инв. | X, пикс. | Char | CharRaw | StringZ | RawZ | RawLim | Сверка |',
          '|---|---|---|---:|---:|---:|---:|---:|---:|---|']
for mode, engine, inverse, phase, pre in sorted_groups(profiles):
    if phase is not None and mode!='pseudo':
        rows = profiles[(mode,engine,inverse,phase,pre)]
        cells = [f'`{engine}`',mode,'Да' if inverse else 'Нет',str(phase*2),*values(rows,range(5)),check(rows)]
        lines.append('| '+' | '.join(cells)+' |')
lines += ['', '## Псевдографика в MAIN', '',
          '| Движок | Инв. | RawLim | RawPseudo | Сверка |', '|---|---|---:|---:|---|']
for mode, engine, inverse, phase, pre in sorted_groups(profiles):
    if mode=='pseudo':
        rows = profiles[(mode,engine,inverse,phase,pre)]
        cells = [f'`{engine}`','Да' if inverse else 'Нет',*values(rows,(4,5)),check(rows)]
        lines.append('| '+' | '.join(cells)+' |')
lines += [
    '', '## Orion M1 и 6×8 на корпусе с системного C:', '',
    'Этот корпус также содержит 924 байта, но его SHA-256 отличается от корпуса основных таблиц. '
    'Строковые замеры Rotate и М1 взяты из той же серии. Для Packed и Fast на этом корпусе '
    'измерены посимвольные входы. Прочерк обозначает отсутствие замера в этой серии. '
    '[Отчёт и исходные отсчёты](../fontben-orion-m1/README.md)', '',
    '| Движок | Инв. | PRE | Char | CharRaw | StringZ | RawZ | RawLim | Сверка |',
    '|---|---|---:|---:|---:|---:|---:|---:|---|',
]
for engine, _, pre, _, _ in orion_profiles:
    rows = orion_series[engine]
    cells = [f'`{engine}`', 'Нет', str(pre), *[v or '—' for v in values(rows, range(5))], check(rows)]
    lines.append('| ' + ' | '.join(cells) + ' |')
(baseline_report/'matrix.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')

# The project table includes the latest KOI-7R series on its own corpus.
koi = load(here.parent/'fontben-koi7-2026-11-21/results.json')
koi_profiles = {}
for row in unique(koi['fontben']).values():
    assert row['mode'] == 'bios' and row['phase'] is None and not row['inverse']
    assert row['ms'] == round(row['ticks']*.128, 3)
    koi_profiles.setdefault(row['engine'], []).append(row)
assert len(koi_profiles) == 6 and all(len(rows) == 18 for rows in koi_profiles.values())
assert koi['corpus_sha256'] not in {*corpus.values(), *orion_hashes}
koi_lines = [
    '', '## 6×8 в KOI-7R', '',
    'Корпус из 924 байт приведён к KOI-7R. Сравниваются штатные модули после смены кодировки и '
    'фиксированные K7Rotate, K7Packed и K7Hybrid. '
    '[Условия и исходные данные](../fontben-koi7-2026-11-21/README.md)', '',
    '| Движок | PRE | Char | CharRaw | StringZ | RawZ | RawLim | Сверка |',
    '|---|---:|---:|---:|---:|---:|---:|---|',
]
for engine, rows in koi_profiles.items():
    pre = {row['pre'] for row in rows}
    assert len(pre) == 1, engine
    cells = [f'`{engine}`', str(pre.pop()), *values(rows, range(5)), check(rows)]
    koi_lines.append('| ' + ' | '.join(cells) + ' |')

# Merge by API and route, so a new scalar run keeps the last string values.
ordinary = {signature: row for signature, row in merged.items() if row['phase'] is None}
ordinary.update({key(row): row for row in orion_rows})
by_engine = {}
for signature, row in ordinary.items():
    by_engine.setdefault(signature[:5], []).append(row)
labels = {'bios':'Резидентный', 'standalone':'Standalone', 'legacy':'Исторический'}
ordinary_lines = [
    '## Общая таблица', '',
    'Колонка «Корпус» указывает серию замера. У смешанных строк Char и CharRaw взяты из серии C:, '
    'строковые входы — из серии A:. Все корпуса обычной сетки занимают 924 байта. '
    '[Отсчёты серии C:](../fontben-orion-m1/README.md)', '',
    '| Движок | Сборка | Инв. | PRE | Корпус | Char | CharRaw | StringZ | RawZ | RawLim | Сверка |',
    '|---|---|---|---:|---|---:|---:|---:|---:|---:|---|',
]
for mode, engine, inverse, phase, pre in sorted_groups(by_engine):
    rows = by_engine[(mode,engine,inverse,phase,pre)]
    assert len(rows) == 18, (engine,inverse,pre)
    corpora = {'C:' if 'corpus_sha256' in row else 'A:' for row in rows}
    if len(corpora) > 1:
        assert {row['kind'] for row in rows if 'corpus_sha256' in row} == {0,1}
    corpus_label = 'C: посимв., A: строки' if len(corpora) > 1 else corpora.pop()
    cells = [f'`{engine}`', labels[mode], 'Да' if inverse else 'Нет', str(pre), corpus_label,
             *values(rows, range(5)), check(rows)]
    ordinary_lines.append('| ' + ' | '.join(cells) + ' |')

main_start = lines.index('## Резидентные движки')
phase_start = lines.index('## 8×8 при явном позиционировании')
orion_start = lines.index('## Orion M1 и 6×8 на корпусе с системного C:')
total = len(ordinary) + sum(row['phase'] is not None for row in merged.values()) + len(koi['fontben'])
header = [
    '# Результаты FontBen', '',
    *lines[2:6],
    f'В таблицах {total} отсчётов Emu80. Для каждого сочетания движка, режима и API '
    'взяты последние доступные измерения. Отдельные таблицы содержат сетку KOI-7R, фазы X и псевдографику.', '',
    *lines[8:main_start],
]
project_lines = [*header, *ordinary_lines, *koi_lines, '', *lines[phase_start:orion_start]]
results_path = here.parents[2]/'source-apps/fontbench/RESULTS.MD'
results_path.write_text('\n'.join(project_lines).replace('](../', '](../../docs/bench/').rstrip()+'\n',
                        encoding='utf-8')


comparison = ['# Штатный 6×8 Rotate до и после', '',
              'RawLim на корпусе 924 байта, время в миллисекундах.', '',
              '| Режим | Маршрут | Было | Стало |',
              '|---|---|---:|---:|']
for inverse in (False,True):
    for route,title in ((0,'MAIN'),(3,'BANK NEAR'),(4,'BANK Far256'),(5,'BANK Own')):
        sig = ('bios','6x8/Rotate',inverse,None,0,4,route)
        comparison.append('| '+' | '.join(['Инверсия' if inverse else 'Обычный',
                                            title,display(baseline[sig]),display(measured[sig])])+' |')
(here/'comparison.md').write_text('\n'.join(comparison)+'\n', encoding='utf-8')
save('measured.json', list(measured.values()))
save('results.json', [*merged.values(), *orion_rows])

files = ('images/SystemRom_emu80_writable.bin','images/SystemRom_512K.bin',
         'images/FlashDisk_emu80_seed.bin',
         'bin/fonts/Rotate.i80','bin/fonts/Rot6Col4.i80','bin/FONTCOM.i80','bin/FONTBANK.i80')
metadata_path = here/'metadata.json'
# The supplements were recorded with other binaries. Preserve the archived
# hashes rather than attributing the old Col4 measurements to today's build.
recorded_hashes = (load(metadata_path)['sha256'] if metadata_path.exists()
                   else {name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in files})
save('metadata.json', {
    'corpus':corpus,
    'sha256':recorded_hashes,
    'additional_corpus':{'orion-m1':next(iter(orion_hashes))},
    'origins':{
        'alternating-carried': len(baseline)-len(replaced),
        'stream-carried': len(streams.keys()-baseline.keys()),
        'rotate-col4-measured': len(measured),
        'orion-m1-carried': len(orion_rows),
    },
})
print(f'{len(measured)} checked measurements, {len(merged)+len(orion_rows)} matrix rows')
