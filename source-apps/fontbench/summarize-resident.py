"""Publish only complete, pixel-verified FontBen/PIT runs of one image."""
from pathlib import Path
import json,hashlib
R=Path(__file__).resolve().parents[2];O=R/'docs/bench/resident-vs-standalone'
pairs=[('Rotate','SharedRotate'),('Packed6K','SharedPacked'),('Fast12K','SharedFull'),('Compact','SharedCompact'),('Win8','Shared8'),('Cpm4','Shared4')]
legacy=['PagedOff','Tripack','PagedOn','Standalone8','Dup4','Colpre']
image_hash=hashlib.sha256((O/'tested-system.bin').read_bytes()).hexdigest()
def load(name):
 p=O/name.replace('Fast12K','Fast10K');proof=json.loads((p/'verified.json').read_text());assert proof['imageSha256']==image_hash,(name,'different image')
 rows=json.loads((p/'results.json').read_text())['results'];assert len(rows)==18,name
 return {(r['kind'],r['route']):r['ms'] for r in rows}
data={n:load(n) for pair in pairs for n in pair}
data.update({n:load(n) for n in legacy})
data.update({n+'-inverse':load(n+'-inverse') for pair in pairs for n in pair})
# A comparison must use byte-identical input, not merely an equal byte count.
corpora={(O/n.replace('Fast12K','Fast10K')/'corpus.bin').read_bytes() for n in data};assert len(corpora)==1
names=['Char','CharRaw','StringZ','StringRawZ','StringRawLim']
routes=['MAIN','BANK CMD','BANK TEXT','BANK Win20','BANK Far256','BANK Own']
def matrix(suffix=''):
 text='| API / вызов | '+' | '.join(a for a,b in pairs)+' |\n|---|'+('---:|'*6)+'\n'
 for k in range(5):
  for rt in ([0,1,2] if k<2 else [0,3,4,5]):
   text+='| '+names[k]+' / '+routes[rt]+' | '+' | '.join(f"{data[a+suffix][k,rt]:.3f} / {data[b+suffix][k,rt]:.3f}" for a,b in pairs)+' |\n'
 return text
text="""# FontBen: резидентные и MAIN-сборки, 2026-11-13

Один образ, один корпус 1152 байтов (24 × 46 символов + CRLF), CPU и ВИ53 Emu80 2 МГц. Время — локальный ВИ53, квант 0,128 мс; накладные расходы MXCALL включены. Загрузка, подготовка, очистка экрана и отчёт исключены. Это эмулятор, не физическое оборудование. В каждой клетке **резидент через BIOS / тот же публичный код в MAIN**, миллисекунды. BIOS-прогон не подменяется. FONT*-прогон использует полные фронтенды из source/font, без старой обвязки standalone. При разных ширинах одинаковое число символов занимает разную площадь.

Проверены все пиксели, курсор, восстановление публичных ABI, сохранность резидентных таблиц и возврат в NC. KOI7/8, повторный Attach и совпадение MAIN/резидента с инверсией дополнительно проверены CPU-тестами.

## Обычный вывод

"""
text+=matrix()
text+='\n## Инверсный вывод\n\n'+matrix('-inverse')
text+='\n## Старые standalone-ядра и текущий резидент, MAIN\n\nСтарые ядра измерены заново на том же корпусе и образе; это не подстановка исторических 153/198 мс. Для Win8 и старого 8X8 используются одинаковые битмапы оконного монитора. Для Fast12K скаляр сравнивается с COLPRE, пакетные строки — с PAGED ON.\n\n| Текущий / контроль | CharRaw: старый → новый | RawLim: старый → новый | Изменение RawLim |\n|---|---:|---:|---:|\n'
for current,old,scalar in [('Rotate','PagedOff','PagedOff'),('Packed6K','Tripack','Tripack'),('Fast12K','PagedOn','Colpre'),('Win8','Standalone8','Standalone8'),('Cpm4','Dup4','Dup4')]:
 before=data[old][4,0];after=data[current][4,0]
 text+=f'| {current} / {old} | {data[scalar][1,0]:.3f} → {data[current][1,0]:.3f} | {before:.3f} → {after:.3f} | {(after/before-1)*100:+.1f}% |\n'
text+='\nУ Compact нет тождественного исторического компактного модуля для парного сравнения. Старые контрольные ядра не изменялись ради поддержки инверсии; в FontBen это отдельная коллекция LEGACY. Все шесть актуальных FONT*-движков имеют обычные и инверсные ядра.\n'
text+=f'\nВсего {len(data)*18} этапов. SHA-256 сохранённого tested-system.bin: `{image_hash}`. Исходные results.json, verified.json, corpus.bin, FONTBEN.TXT и снимки находятся в подпапках этого каталога. Воспроизведение: check-resident.py; для инверсии — имена шести BIOS и шести Shared вариантов с --inverse. Последующая пересборка справки на диске не меняет измеренный код.\n'
# Keep archived measurements, but publish the current engine names.
import re
for before, after in [('Win8','FAST8'),('PAGED','ROW6'),('FULL','SCALAR12'),('COLPRE','COL6PRE'),('8X8','ROW8')]:
 text=re.sub(r'\b'+before+r'\b',after,text)
(O/'README.md').write_text(text,encoding='utf8')
# Keep old material explicitly historical; replace only our generated section.
p=R/'docs/bench/engine-grid/README.md';old=p.read_text(encoding='utf8')
start='<!-- RESIDENT-MATRIX-BEGIN -->';end='<!-- RESIDENT-MATRIX-END -->'
if start in old:
 old=old[:old.index(start)]+start+'\n'+text+'\n'+end+old[old.index(end)+len(end):]
else:
 old=start+'\n'+text+'\n'+end+'\n\n'+old
p.write_text(old,encoding='utf8')
print(len(data)*18,'verified PIT stages published')
