"""Publish a completed DiskBen matrix into the stable README section."""
import argparse,hashlib,json,statistics,re,csv
from pathlib import Path
R=Path(__file__).resolve().parents[2]
p=argparse.ArgumentParser(description=__doc__);p.add_argument('matrix',type=Path);p.add_argument('--readme',type=Path,default=R/'source-apps/diskbench/README.md');a=p.parse_args()
d=json.loads(a.matrix.read_text(encoding='utf-8'));assert d['complete'] and len(d['rows'])==114
rows=d['rows'];median=statistics.median
assert len({(r['key'],r['context']) for r in rows})==114 and all(r['returnNC'] for r in rows)

def choose(driver,contexts,alignment=0,fragment=None,check=None):
 return [r for r in rows if r['driver']==driver and r['context'] in contexts and r['alignment']==alignment and (fragment is None or r['fragment']==fragment) and (check is None or r['check']==check)]
def metric(rs,key):
 values=[r['rates'][key] for r in rs if key in r['rates']]
 return f'{median(values):.2f}' if values else '—'
def raw(driver,contexts,alignment=0,fragment=None,high=False):
 rr=choose(driver,contexts,alignment,fragment);read=rr
 if driver=='AT512KS':read+=choose('ROM-SYS',contexts,alignment,fragment)
 suffix='High' if high else ''
 return [metric(read,'rawRead'+suffix)]+[metric(choose(driver,contexts,alignment,fragment,check),'rawWrite'+suffix) for check in (0,2,1)]
def file_row(driver,context,alignment,fragment,high=False):
 rs=choose(driver,[context],alignment,fragment)
 read_key='fileReadHigh' if high else 'fileRead'
 write_key='fileWriteHigh' if high else 'fileWrite'
 read=metric(rs,read_key)
 if driver=='AT512KS':read=metric(choose('ROM-SYS',[context],alignment,fragment),read_key)+' / '+read
 return [read]+[metric(choose(driver,[context],alignment,fragment,check),write_key) for check in (0,2,1)]

sha=hashlib.sha256((a.matrix.parent/'input/bin/DOS.sys').read_bytes()).hexdigest()
date=d.get('date','13.11.2026')
text=[f'Матрица от **{date}**. Emu80, Intel 8080, 2 МГц; локальный ВИ53 2 МГц; кэш метаданных DOS 8 КиБ; файл 8 КиБ; один проход записи на вариант, ROM — три чтения. Все числа — **КиБ/с**. Это результаты модели эмулятора, отдельно от измерений реальной платы.','',f'Сборка: SHA-256 DOS.sys `{sha}`. Полный прогон содержит 114 строк MAIN/DOS/BANK, включая оба выравнивания H:, оба размещения и все три режима проверки. Проверены возврат в NC, восстановление FAT после удаления временных файлов и неизменность ROM-образов.','', '### RAW: подготовленная серия секторов', '', '| Диск / драйвер | Буфер | Исполнитель / размещение | READ | WRITE | WRITE + секторная проверка | WRITE + перечитывание блока |','|---|---|---|---:|---:|---:|---:|']
for contexts,label,high in [(['MAIN'],'MAIN HIGH',True),(['MAIN'],'MAIN LOW',False),(['DOS','BANK'],'DOS / BANK',False)]:
 text.append('| '+' | '.join(['A: ROM-SYS / AT512KS','256-byte aligned',label]+raw('AT512KS',contexts,high=high))+' |')
text.append('| '+' | '.join(['B: CPM32','256-byte aligned','MAIN / DOS / BANK']+raw('CPM32',['MAIN','DOS','BANK']))+' |')
for driver in ('AT2MX','AT2MXF'):
 for alignment,buffer in [(0,'aligned'),(1,'+1, unaligned')]:
  for frag,label in ([(None,'оба размещения')] if driver=='AT2MX' else [(0,'последовательное'),(1,'случайное')]):
   text.append('| '+' | '.join(['H: '+driver,buffer,'MAIN / DOS / BANK; '+label]+raw(driver,['MAIN','DOS','BANK'],alignment,frag))+' |')
text+=['', 'RAW: для объединённых исполнителей/размещений приведены медианы. READ объединяет повторы режимов проверки; WRITE берётся из V=0, секторная проверка — V=2, перечитывание блока — V=1. В последнем случае это прямое чтение и сравнение массива секторов, без файлового API и повторного обхода FAT. Запись A: относится только к AT512KS; ROM-SYS доступен только для чтения. HIGH отдельно измерен только для MAIN A:.', '']
for fragment,title in [(0,'Последовательное размещение'),(1,'Случайное размещение')]:
 text += ['### FILE: '+title.lower(), '', '| Диск / драйвер | Буфер | Исполнитель | READ | WRITE | WRITE + секторная проверка | WRITE + проверка файла |','|---|---|---|---:|---:|---:|---:|']
 for driver in ('AT512KS','CPM32','AT2MX','AT2MXF'):
  for alignment in ((0,1) if driver in ('AT2MX','AT2MXF') else (0,)):
   for context in ('MAIN','DOS','BANK'):
    name={'AT512KS':'A: ROM-SYS / AT512KS','CPM32':'B: CPM32'}.get(driver,'H: '+driver)
    buf='+1, unaligned' if alignment else 'aligned'
    if context=='MAIN' and driver=='AT512KS':
     assert all('fileReadHigh' in r['rates'] for r in choose(driver,[context],alignment,fragment))
     text.append('| '+' | '.join([name,buf,'MAIN HIGH']+file_row(driver,context,alignment,fragment,True))+' |')
    label='MAIN LOW' if context=='MAIN' and driver=='AT512KS' else context
    text.append('| '+' | '.join([name,buf,label]+file_row(driver,context,alignment,fragment))+' |')
 text.append('')
text+=['FILE: READ — медиана повторов режимов проверки; WRITE — V=0; секторная проверка — V=2; полное чтение и сравнение файла — V=1. В READ A: два числа означают ROM-SYS / AT512KS; запись относится только к AT512KS. Они читают разные заранее подготовленные цепочки, поэтому различие FILE READ не приписывается только коду физического драйвера. MAIN A: включает отдельные LOW и HIGH. Для FILE HIGH целый буфер 8 КиБ находится в экране с 9000h; его сохранение/восстановление исключены из таймера. MAIN, DOS и BANK показаны отдельно, без скрытого усреднения между страницами.', '', 'Последовательное/случайное означает способ выделения кластеров; во всех случаях читается и записывается весь файл по порядку. Создание и размещение цепочки, удаление и подготовка RAW исключены из замера; FILE включает открытие, передачу целого буфера, метаданные и закрытие, WRITE — также финальный flush. Для V=1 один запуск сохраняет и обычный WRITE, и WRITE+VERIFY; таблица использует V=0 как независимую контрольную скорость обычной записи. Подробные временные интервалы, отдельный flush и все исходные значения остаются в matrix.json локального отчёта.','']
if d.get('supplements'):
 text+=['Дополнение MAIN HIGH для A: измерено отдельным прогоном той же даты. Все строки A: обновлены; результаты B:/H: сохранены из исходной полной матрицы. BIOS, DOS, IO, MEM и физические драйверы в обоих прогонах побайтно совпадали. Изменён только измеритель и его вывод.', '']
body='\n'.join(text)
s=a.readme.read_text(encoding='utf-8');s,n=re.subn(r'(?s)(<!-- DISKBEN_MATRIX_BEGIN -->\n).*?(<!-- DISKBEN_MATRIX_END -->)',lambda m:m[1]+body+m[2],s);assert n==1
a.readme.write_text(s,encoding='utf-8');(a.matrix.parent/'README-tables.md').write_text(body,encoding='utf-8')
print('Published 114 measurements into',a.readme)

columns=['drive','driver','alignment','fragment','check','context','readonly','rawRead','rawWrite','rawReadHigh','rawWriteHigh','fileRead','fileWrite','fileReadHigh','fileWriteHigh','rawWritePlain','fileWritePlain','rawWriteHighPlain','fileWriteHighPlain']
with (a.matrix.parent/'results.csv').open('w',encoding='utf-8',newline='') as stream:
 writer=csv.DictWriter(stream,fieldnames=columns);writer.writeheader()
 for r in rows:writer.writerow({k:r.get(k,r['rates'].get(k,'')) for k in columns})
