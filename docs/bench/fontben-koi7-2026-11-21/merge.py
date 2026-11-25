"""Combine the FontBen, switch and legacy footprint data."""
import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
out = Path(__file__).resolve().parent
workers = out/'packed-unroll'
profiles = (('Rotate','K7Rotate'),('Packed','K7Packed'),('Fast','K7Hybrid'))


def load(path):
    return json.loads(path.read_text(encoding='utf-8'))


def ms(value):
    return f'{value:.3f}'.replace('.', ',')


corpus = {load(workers/f'worker-{i}/metadata.json')['ordinary'] for i in range(3)}
assert len(corpus)==1
runs = []
for i in range(3):
    worker = workers/f'worker-{i}'
    assert not load(worker/'errors.json')
    runs += load(worker/'results.json')
assert len(runs)==108 and all(row['pixels']=='oracle' for row in runs)
keys = {(r['engine'],r['kind'],r['route']):r['ms'] for r in runs}
assert len(keys)==108
assert Counter(r['engine'] for r in runs) == {
    engine:18 for name,new in profiles for engine in (f'6x8/{name} KOI-7R',f'6x8/{new}')
}

loads = [row for i in range(4) for row in load(workers/f'worker-{i}/loads.json')]
assert len(loads)==9 and all(row['ticks_per_tstate']==1.0 for row in loads)
cpu = load(out/'cpu.json')
legacy = load(out/'legacy-size.json')
assert sum(profile['checks'] for profile in cpu.values())==512
assert sum(profile['checks'] for kernel in legacy.values() for profile in kernel.values())==96
lines = ['# Сравнение KOI-7R на 924 байтах', '',
         'Emu80 MX2, 2 МГц. Штатные KOI-8 модули переключены в KOI-7R.'
         ' На всех 108 этапах совпали экран и курсор. Время в миллисекундах.', '',
         '[Сравнение строковых входов со штатными Монитором-2 и RAMFOS](cpu.md#сравнение-вывода-924-символов)', '',
         '| Профиль | RawLim MAIN, KOI-8 → K7 | RawLim BANK NEAR, KOI-8 → K7 | RawLim BANK Far256, KOI-8 → K7 |',
         '|---|---:|---:|---:|']
for name,new in profiles:
    labels = (f'6x8/{name} KOI-7R',f'6x8/{new}')
    measured = [' → '.join(ms(keys[engine,4,route]) for engine in labels) for route in (0,3,4)]
    lines.append('| '+' | '.join([f'{name} → {new}',*measured])+' |')
lines += ['', '## Байты движка в MXOS', '',
          'Резидентный образ включает код, верхнюю таблицу, выравнивание и служебные данные. '
          'MAIN — постоянная нижняя резервация. Таблицы указаны суммарно в обоих адресных диапазонах.', '',
          '| Профиль | Таблицы KOI-8 → K7 | Резидент KOI-8 → K7 | MAIN KOI-8 → K7 |',
          '|---|---:|---:|---:|']
for name,new in profiles:
    item = cpu[new]
    values = [f"{item['old'][field]} → {item['new'][field]}" for field in ('tables','body','main')]
    lines.append('| '+' | '.join([f'{name} → {new}',*values])+' |')
lines += ['', '## Смена кодировки или загрузка с A:', '',
          'Переключение измерено в модели 8080 по входу setEncoding. '
          'Загрузка измерена в Emu80 от входа FONT до установленного модуля. '
          'Приведён диапазон трёх загрузок на профиль, время в миллисекундах.', '',
          '| Профиль | Переключить KOI-8 → KOI-7R | Установить K7 |',
          '|---|---:|---:|']
for name,new in profiles:
    switch = cpu[new]['switch_cycles']['to7']/2000
    values = [row['new_ticks']/2000 for row in loads if row['profile']==name]
    lines.append(f'| {name} → {new} | {ms(switch)} | {ms(min(values))}–{ms(max(values))} |')
lines += ['', '## Верхняя память legacy', '',
          '| Профиль | Монитор-2, секция с таблицами | RAMFOS-MXOS, секция с таблицами |',
          '|---|---:|---:|']
for name in ('Rotate','Packed','Hybrid','Fast'):
    lines.append(f"| {name} | {legacy['Monitor2'][name]['section']} | {legacy['RAMFOS-MXOS'][name]['section']} |")
(out/'comparison.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
result = {'corpus_sha256':corpus.pop(), 'fontben':runs, 'loads':loads, 'sha256':{
    name:hashlib.sha256((root/name).read_bytes()).hexdigest() for name in (
        'images/SystemRom_emu80_writable.bin','images/SystemRom_512K.bin','images/FlashDisk_emu80_seed.bin',
        *(f'bin/fonts/{name}.i80' for name in ('K7Rotate','K7Packed','K7Hybrid','K7Fast')),
    )}}
(out/'results.json').write_text(json.dumps(result,indent=2)+'\n',encoding='utf-8')
print('\n'.join(lines))
