import argparse
from collections import Counter
import hashlib
import json
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument('checkout', type=Path)
root = parser.parse_args().checkout.resolve()
out = Path(__file__).resolve().parent
pairs = (
    ('6×8 Rotate', '6ROTATEROWS', '6ROTATESTREAM', 'RotRows'),
    ('6×8 Packed', '6PACKEDQUADS', '6PACKEDSTREAM', 'FontPack'),
    ('6×8 Fast', '6FULLQUADS', '6FASTSTREAM', 'FontFull'),
    ('4×8 Packed', 'CPM4', '4PACKEDSTREAM', 'Font4'),
    ('4×8 Fast', 'FAST4', '4FASTSTREAM', 'Font4Fst'),
    ('4×8 Compact', '4COMPACT', '4COMPACTSTREAM', 'Font4Cmp'),
)


def load(path):
    return json.loads(path.read_text(encoding='utf-8'))


def save(path, value):
    path.write_text(json.dumps(value, indent=2)+'\n', encoding='utf-8')


rows = []
corpora = set()
for group in range(4):
    worker = out/f'worker-{group}'
    assert not load(worker/'errors.json')
    rows += load(worker/'results.json')
    corpora.add(load(worker/'metadata.json')['ordinary'])

expected = {(engine, inverse):18 for _,old,new,_ in pairs
            for engine in (old,new) for inverse in (False,True)}
assert Counter((r['engine'],r['inverse']) for r in rows) == expected
assert len(corpora)==1 and all(r['pixels']=='oracle' for r in rows)
assert all(r['ms']==round(r['ticks']*.128,3) for r in rows)
lookup = {(r['engine'],r['inverse'],r['kind'],r['route']):r['ms'] for r in rows}
assert len(lookup)==len(rows)


def fmt(value):
    return f'{value:.3f}'.replace('.', ',')


lines = [
    '# Результаты FontBen для Stream', '',
    f'{len(rows)} этапа. На каждом совпали пиксели и курсор. Время в миллисекундах. '
    'Шаг ВИ53 0,128 мс. Нормальный вывод из MAIN в таблице сравнения.', '',
    '| Формат | StringZ прежний → Stream | RawZ прежний → Stream | RawLim прежний → Stream | Δ RawLim |',
    '|---|---:|---:|---:|---:|',
]
for name,old,new,_ in pairs:
    values = [(lookup[old,False,kind,0],lookup[new,False,kind,0]) for kind in (2,3,4)]
    old_lim,new_lim = values[-1]
    diff = f'{(new_lim/old_lim-1)*100:+.1f}'.replace('.', ',')
    times = [f'{fmt(before)} → {fmt(after)}' for before,after in values]
    lines.append('| '+' | '.join([name,*times,diff+'%'])+' |')
lines += [
    '', '## Все маршруты', '',
    'Char и CharRaw указаны в порядке MAIN / BANK CMD / BANK CMD+Z. '
    'StringZ, RawZ и RawLim указаны в порядке MAIN / BANK NEAR / BANK Far256 / BANK Own.', '',
    '| Движок | Инверсия | Char | CharRaw | StringZ | RawZ | RawLim |',
    '|---|---|---:|---:|---:|---:|---:|',
]
for _,old,new,_ in pairs:
    for engine in (old,new):
        for inverse in (False,True):
            cells = [' / '.join(fmt(lookup[engine,inverse,kind,route]) for route in routes)
                     for kind,routes in enumerate([(0,1,2)]*2+[(0,3,4,5)]*3)]
            lines.append('| '+' | '.join([engine,'Да' if inverse else 'Нет',*cells])+' |')
lines += [
    '', '## Модель 8080. RawLim из MAIN', '',
    'Такты включают короткую обвязку CPU-проверки. Полная строка вмещает 64 глифа 6×8 или 96 глифов 4×8.', '',
    '| Формат | 4 глифа, прежний → Stream | 16 глифов, прежний → Stream | Полная строка, прежний → Stream |',
    '|---|---:|---:|---:|',
]
cpu = load(out/'cpu-timing.json')
for name,_,_,module in pairs:
    cells = [f'{before} → {after}' for before,after in cpu[module]['printStringRawLim'].values()]
    lines.append('| '+' | '.join([name,*cells])+' |')
(out/'matrix.md').write_text('\n'.join(lines)+'\n', encoding='utf-8')
save(out/'results.json', rows)
inputs = ['images/SystemRom_emu80_writable.bin','images/FlashDisk_emu80_seed.bin',
          'bin/FONTCOM.i80','bin/FONTBANK.i80']
inputs += [f'bin/fonts/{dll}.i80' for _,_,_,dll in pairs]
save(out/'metadata.json', {'corpus_sha256':corpora.pop(),'sha256':{
    file:hashlib.sha256((root/file).read_bytes()).hexdigest() for file in inputs}})

