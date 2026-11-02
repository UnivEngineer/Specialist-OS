"""Сводная таблица из исходных тиков benchmark-disks.py, без округлений экранного вывода."""
import argparse,csv,hashlib,json,shutil
from pathlib import Path
ROOT=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('directory',type=Path,nargs='?',default=ROOT/'docs/diskbench/matrix-001');a=p.parse_args();out=a.directory
j=json.loads((out/'raw.json').read_text(encoding='utf-8'));assert j.get('completedUtc') and len(j['rows'])==15
volume=j['fileBytes']*j['passes'];tick_ms=256000/j['pitHz']
def speed(r,key):
 t=r['metrics'][key][0];return volume/(t*tick_ms) if t else None
def text(value):return '—' if value is None else f'{value:.2f}'
def label(r):
 if r['driver']=='ROM':return f"{r['drive']}: ROM {r['capacityBytes']//1024} КиБ"
 return f"{r['drive']}: "+('RAMD '+r['description'] if r['driver']=='RAMD' else r['driver'])
def sort(r):
 order={'A':0,'B':1,'H':2}
 return (order[r['drive']],0 if r['driver']=='ROM' else 1,r['capacityBytes'],r['driver'],r['variant']=='Legacy',r['mode'],r['buffer']=='high-staged')
rows=sorted(j['rows'],key=sort)
header='| Диск / драйвер | Режим | Буфер | Raw READ | Raw WRITE | DOS READ | DOS WRITE |\n|---|---|---|---:|---:|---:|---:|'
lines=[header];flat=[]
for r in rows:
 mode={'write':'W','write+verify':'W+V','read-only':'RO'}[r['mode']];buf='HIGH*' if r['buffer']=='high-staged' else 'LOW'
 values=[speed(r,key) for key in ['rawRead','rawWrite','fileRead','fileWrite']]
 lines.append('| '+' | '.join([label(r),mode,buf]+[text(v) for v in values])+' |')
 d={'disk_driver':label(r),'mode':mode,'buffer':buf,'file_bytes':j['fileBytes'],'passes':j['passes']}
 for key in ['rawRead','rawWrite','fileRead','fileWrite']:
  ticks,flush,count=r['metrics'][key]
  d.update({key+'_ticks':ticks,key+'_ms':ticks*tick_ms if ticks else '',key+'_kBps':speed(r,key) or '',key+'_flush_ms':flush*tick_ms,key+'_flush_calls':count})
 flat.append(d)
with (out/'results.csv').open('w',encoding='utf-8-sig',newline='') as f:
 w=csv.DictWriter(f,fieldnames=list(flat[0]));w.writeheader();w.writerows(flat)
(out/'TABLE.md').write_text('\n'.join(lines)+'\n',encoding='utf-8')
flush=['| Драйвер / режим | Raw flush, мс | Вызовы | DOS flush, мс | Вызовы |','|---|---:|---:|---:|---:|']
for r in rows:
 if not r['caps']&8:continue
 rw,fw=r['metrics']['rawWrite'],r['metrics']['fileWrite']
 flush.append(f"| {label(r)} / {r['mode']} | {rw[1]*tick_ms:.3f} | {rw[2]} | {fw[1]*tick_ms:.3f} | {fw[2]} |")
def find(driver,mode='write',buffer='low',variant='Buffered'):
 return next(r for r in rows if r['driver']==driver and r['mode']==mode and r['buffer']==buffer and r['variant']==variant)
flash=find('FLASH');fst=find('FLASHFST');fstv=find('FLASHFST','write+verify');flashv=find('FLASH','write+verify');r4=find('RAMD');legacy=find('RAMD',variant='Legacy')
spread=max((max(p[2][0] for p in r['perPass'])-min(p[2][0] for p in r['perPass']))/(sum(p[2][0] for p in r['perPass'])/j['passes'])*100 for r in rows if 'perPass' in r)
build_path=Path('D:/Projects/RetroPC/Specialist/Emulator/Emu80-flash-test/specialist-build.json')
build=json.loads(build_path.read_text(encoding='utf-8-sig'));build['currentSourcesMatch']=all(hashlib.sha256((ROOT/'source-emulators/emu80'/v['path']).read_bytes()).hexdigest()==v['sha256'] for v in build['sources'])
assert build['currentSourcesMatch'];(out/'emulator-build.json').write_text(json.dumps(build,indent=2)+'\n')
art=out/'artifacts';art.mkdir(exist_ok=True)
for name in ['DOS.sys','DOS.sym','DISKBEN.com','Flash.com','FlashFst.com','FlashSys.com','Rom.com']:
 assert name not in j['binaries'] or hashlib.sha256((ROOT/'bin'/name).read_bytes()).hexdigest()==j['binaries'][name]
 shutil.copyfile(ROOT/'bin'/name,art/name)
shutil.copyfile(ROOT/'source-apps/diskbench/build/DISKBEN.sym',art/'DISKBEN.sym')
report='''# Матрица DISKBEN — 01.11.2026

Emu80 4.0.575, Intel 8080 2 МГц, ВИ53 2 МГц. Это измерения модели эмулятора.
Собственные модели совпадают по SHA-256 с записью сборки Emu80; tWC AT29
в этой сборке равен 10 мс. Время берётся из эмулируемого ВИ53, а не часов
Windows. Задержки интерфейса, HTTP/MCP и RDP не входят в измеряемые интервалы.

Файл 8 КиБ (8192 байта), три прохода: объём каждой строки — 24576 байт.
Скорость = общий объём / суммарное время; кБ/с здесь десятичные (1000 байт/с).
Числа вычислены из исходных U32 тиков, без округления до целых мс или кБ/с
в экранном форматтере. Цена таймера и обёртки DISKBEN включена, не вычитается.

## Скорость, кБ/с

W — обычная запись, W+V — B=5 (запись с проверкой), RO — только чтение.
HIGH* использует промежуточный сектор 8200h; его копирование включено в замер.
Полный файл хранится в основном буфере 2000h. Этот вариант включает staging;
загрузка NC непосредственно в верхнюю память требует отдельного замера.

'''+ '\n'.join(lines)+'''

## Flush

Суммы за три файла, уже включённые во время WRITE. Без DISK_CAP_FLUSH
барьер не вызывается и в таблицу flush драйвер не включается.

'''+ '\n'.join(flush)+f'''

## Выводы

- FlashFst ускоряет прямую запись против Flash в {speed(fst,'rawWrite')/speed(flash,'rawWrite'):.2f} раза:
  {speed(fst,'rawWrite'):.2f} против {speed(flash,'rawWrite'):.2f} кБ/с.
  В файловом пути выигрыш составляет {(speed(fst,'fileWrite')/speed(flash,'fileWrite')-1)*100:.1f}%:
  {speed(fst,'fileWrite'):.2f} против {speed(flash,'fileWrite'):.2f} кБ/с.
- При B=5 преимущество конвейера почти исчезает: Raw {speed(fstv,'rawWrite'):.2f}
  против {speed(flashv,'rawWrite'):.2f} кБ/с. B=5 ждёт запись и читает каждый сектор,
  поэтому загрузка разных чипов не перекрывается как при B=1.
- Через DOS FlashFst с проверкой даёт {speed(fstv,'fileWrite'):.2f} кБ/с, Flash —
  {speed(flashv,'fileWrite'):.2f}. Различия размещения кластеров и обращений к FAT
  остаются, хотя ожидания записи уже синхронны. Из одного этого замера нельзя
  выделить стоимость каждой внутренней процедуры DOS.
- R4 быстрее Legacy: Raw READ в {speed(r4,'rawRead')/speed(legacy,'rawRead'):.2f} раза,
  Raw WRITE в {speed(r4,'rawWrite')/speed(legacy,'rawWrite'):.2f}; файловое чтение в
  {speed(r4,'fileRead')/speed(legacy,'fileRead'):.2f}, файловая запись в {speed(r4,'fileWrite')/speed(legacy,'fileWrite'):.2f} раза.
- У FlashFst в режиме W время последнего/метаданных барьеров составляет
  {fst['metrics']['rawWrite'][1]/fst['metrics']['rawWrite'][0]*100:.2f}% Raw WRITE и
  {fst['metrics']['fileWrite'][1]/fst['metrics']['fileWrite'][0]*100:.2f}% DOS WRITE.
  Основная разница Raw/DOS не объясняется только заключительным flush.
- Разброс времени файловой записи между тремя проходами не превысил {spread:.2f}%
  от среднего. Это разброс внутри данной серии в модели, не оценка погрешности
  реального оборудования.

## Охват и сопоставимость

- A: текущая системная flash 512 КиБ — FLASHSYS W/W+V, LOW/HIGH.
- B: RAM 1 МиБ — RAMD R4 и Legacy. B=5 ими не поддерживается.
- H: четыре AT29C040A, 2 МиБ — Flash и FlashFst, W/W+V.
- Встроенный ROM-драйвер A: проверен на 64 и 512 КиБ, LOW/HIGH.
- ROM.COM для H: проверен с отдельным 64-КиБ ROM-профилем. Его протокол
  отличается от внешней AT29-платы; запуск ROM.COM на flash-профиле не является
  проверкой этого драйвера.
- C:–G: возвращают нулевую ёмкость, драйверы не установлены. Копии RAM B:
  под другими буквами не создавались.
- У ROM запись и B=5 не поддерживаются; оба отказа подтверждены вне замера.
  Прочерки в таблице означают неподдерживаемую операцию, а не нулевую скорость.

Для RO подготовлены отдельные образы с DISKBEN.TMP того же размера и шаблона.
ROM 64 КиБ использует минимальный загрузочный набор, ROM 512 КиБ — полный
системный набор. Каталоги и FAT различаются; разницу файлового чтения этих
образов нельзя приписывать только размеру ROM. Перед RO-замером выполнено
одно прогревающее файловое чтение. Сам RO-замер вызывает тот же bios_fileLoad2,
measure_begin/end и raw_transfer DISKBEN; дополнительный 8080 harness сохранён.

В записываемых тестах исполняется обычный цикл DISKBEN: DOS WRITE → DOS READ →
Raw WRITE → Raw READ → удаление. File WRITE включает выделение цепочки, FAT,
каталог и оба барьера DOS. Raw использует те же выделенные секторы и один
барьер после пакета записи. Режим W+V выбирает B=5 для данных и метаданных DOS;
в Raw проверяются записываемые секторы данных. Перед каждым READ буфер меняется
на противоположный шаблон вне замера; после чтения сверяются все 8192 байта.
Сравнение, печать, разбор цепочки для Raw и удаление не входят в время операций.

После каждого записываемого сочетания проверены возврат FAT к исходному виду,
сохранность существовавших записей каталога и, для flash, содержимого занятых
кластеров. На B: использована отдельно отформатированная RAM-фикстура.
RO-образы проверены на неизменность SHA-256. Все проверки прошли.

## Воспроизведение и артефакты

Сценарий: `python -X utf8 scripts/emu80/benchmark-disks.py --report-directory docs/diskbench/matrix-002`.
Существующий отчёт защищён от перезаписи; для новой серии нужен новый каталог.
Он работает только в отдельном `Emu80-flash-test` на localhost:19781,
создаёт изолированный DOS с RAMD_BUFFERED=0 и отдельные RO-образы в отчёте.
Основной runtime пользователя не запускается и не изменяется. Измерительные
параметры задаются до старта цикла; пользовательский ввод не входит в таймер.

- `raw.json` — U32 тики, времена/числа flush, фазы, ёмкости, SHA-256 бинарников;
  элемент статистики имеет вид [ticks, flushTicks, flushCalls].
- `results.csv` — скорости и времена без округления для последующего анализа.
- `TABLE.md` — компактная таблица скоростей.
- `artifacts/` — бинарники и таблицы символов измеренной конфигурации.
- `legacy/` — изолированная сборка Legacy DOS с листингом и символами.
- `emulator-build.json` — версия исходников аппаратных моделей и контрольные суммы.
- `*.png` — вывод DISKBEN для записываемых сочетаний.
- `rom64/`, `rom512/`, `external-rom64/` — RO-фикстуры с TMP.
'''
(out/'RESULTS.md').write_text(report,encoding='utf-8');print('\n'.join(lines));print('\n'.join(flush))
