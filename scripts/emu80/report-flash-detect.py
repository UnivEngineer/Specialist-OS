"""Отчёт и сохранение бинарников проверки software ID AT29C."""
import hashlib, json, runpy, shutil
from pathlib import Path

ROOT=Path(__file__).resolve().parents[2];out=ROOT/'docs/flash/detect-001'
rows=json.loads((out/'results.json').read_text())
files=json.loads((out/'filesystem/results.json').read_text())
boot=json.loads((out/'boot/results.json').read_text())
assert sum('chips' in r for r in rows)==8
assert sum('physicalKiB' in r for r in rows)==3
assert sum(r.get('noIdRejected',False) for r in rows)==3
assert files['twoChipDosFile']['afterProcessRestart'] and boot['autoInstalledFromLoadAddressZero']
symbols=runpy.run_path(str(ROOT/'scripts/build/pack-dos.py'))['symbols']
art=out/'artifacts';art.mkdir(exist_ok=True)
table=['| Драйвер | COM, байт | Резидент, байт | Окно | Было резидентно |', '|---|---:|---:|---|---:|']
old={'Flash':354,'FlashFst':506,'FlashSys':348}
hashes={}
for name in old:
    s=symbols(ROOT/'bin'/f'{name}.sym');size=s['resident_end']-s['Driver']
    assert size<=512 and s['resident_file']==3
    table.append(f"| {name} | {(ROOT/'bin'/f'{name}.com').stat().st_size} | {size} | {s['Driver']:04X}h–{s['resident_end']-1:04X}h | {old[name]} |")
    for suffix in ['com','sym','lst']:
        source=ROOT/'bin'/f'{name}.{suffix}';shutil.copyfile(source,art/source.name)
        hashes[source.name]=hashlib.sha256(source.read_bytes()).hexdigest()
(out/'hashes.json').write_text(json.dumps(hashes,indent=2)+'\n')
report='''# Идентификация AT29C в драйверах — 02.11.2026

Реализовано определение физической ёмкости без пробной записи данных.
FLASH/FLASHFST распознают 1–4 AT29C040A (512/1024/1536/2048 КиБ),
FLASHSYS — AT29C010A/C020/C040A (128/256/512 КиБ). Три COM загружаются
с 0000h; после успешного ID резидент копируется в прежнее верхнее окно.
Манифест SystemRom.txt, build-project.ps1 и задачи VS Code обновлены.
ROM.COM остаётся прежним, с загрузкой в DE00h.

## Размеры

'''+ '\n'.join(table)+'''

Установщик входит в файл, но не занимает постоянную память DOS. Общий код
источников — flash-install.inc и flash-map.inc. У FlashFst убрано скрытое
кэширование режима ВВ55: каждая операция явно задаёт режим, в том числе
после ID. Таблица выбора логического чипа занимает четыре байта. MapSector:
21 байт, 107 тактов без CALL, одно чтение таблицы и четыре обращения к стеку.
Рабочие циклы загрузки страниц не менялись.

## Алгоритм и ограничения

Используются AA/55/90 по адресам 5555h/2AAAh/5555h, manufacturer по 0,
device по 1; выход AA/55/F0. Паузы — цикл 30720 тактов (15,36 мс при
2 МГц, 12,288 мс при 2,5 МГц), до и после чтения ID. Данные и SDP при ID
не меняются, после выхода режим чтения массива восстановлен. Источники:
[Atmel, Programming Family of Flash Memories](https://ww1.microchip.com/downloads/en/appnotes/doc0510.pdf),
[AT29C010A](https://media.digikey.com/pdf/Data%20Sheets/Atmel%20PDFs/AT29C010A.pdf),
[AT29C020](https://media.digikey.com/pdf/Data%20Sheets/Atmel%20PDFs/AT29C020.pdf).

Системный ID выполняет временный код в 8200h–82FFh при верхнем стеке.
Для H: проверяются четыре физических гнезда; manufacturer FFh считается
пустым. Неизвестный непустой ID отклоняется, прежний резидент/регистрация
сохраняются. Системная защита J4 / внешняя readOnly блокирует /WE, поэтому
software ID недоступен; подмена на запись не выполняется. Возврат CF=1.

AT29C040 без A (ID 5Bh, страница 512 байт) и меньшие чипы на внешней плате
не поддерживаются; системные C010A/C020/C040A поддерживаются. Разделение
256-байтного логического сектора на две записи по 128 выбрано по ID C010A,
не по BPB. B=3 сообщает физические секторы; чтение/запись B=1/2/5 за границу
отклоняются. Файловая система и её исторический формат не изменены.

## Выполненные проверки Emu80

- Холодный старт до NC с новыми COM, манифестом загрузки 0, 1–4 чипами H:.
  Для каждого количества оба драйвера возвращают физическую ёмкость,
  правильное число чипов в описании и таблице выбора.
- Команды ID не меняют образ и SDP. Проверены границы 32/64/512 КиБ,
  последний сектор, B=5 с чтением, восстановление исходных секторов.
  Отклонены первый сектор за концом и FFFFh через B=1/2/5.
- FORMAT для двух чипов создаёт том 1 МиБ / 4096 секторов, несмотря на
  прежний BPB от 2-МиБ фикстуры. Само усечение образа не использовалось
  как доказательство корректности файловой системы: том переформатирован.
- Файл 4096 байт создан через DOS с FlashFst, FAT-цепочка использует оба
  чипа. Файл прочитан, после перезапуска прочитан обычным Flash, затем
  удалён с восстановлением FAT. Отдельно проверены автоматические записи
  v_drives: A=DC00h, H=DE00h после загрузки COM с 0000h.
- На A: 128/256/512 КиБ проверены ID, описание, физическая страница
  128/256/256, запись с чтением и физические границы.
- Физический A: 512 КиБ при BPB тома 128 КиБ возвращает 2048 секторов
  и успешно пишет все 256 байт физической страницы. Прежняя зависимость
  от BPB в выборе страницы удалена.
- Без software ID три установщика возвращают CF=1, не меняют резидент
  и адрес прежнего драйвера. Для физически пустых промежуточных гнёзд
  в модели нет отдельной конфигурации: преобразование проверено синтетически
  таблицей [0,16], HL/B сохранены. Электрический детект таких гнёзд не проверен.

Сценарии: check-flash-detect.py, check-flash-detect-file.py. JSON результатов:
results.json, filesystem/results.json, boot/results.json. Эмулятор — отдельный
Emu80-flash-test, процессы закрыты. На реальной плате не проверялось.

## Сборка и образы

build: OS — 0 ошибок/предупреждений. Просмотрены .lst и .sym: резиденты
не выходят за свои 512-байтные окна, временный код ID не выходит за 82FFh.
Собраны SystemEmuWritable, SystemHardware 128/256/512; каждый из 23 файлов
в каждом образе побайтово совпадает с манифестом, flash loadAddress=0.
Бинарники/символы/листинги сохранены в artifacts/, контрольные суммы hashes.json.

Системный образ 64 КиБ не собирается из текущего общего манифеста из-за
переполнения данных. Это воспроизведено до изменения драйверов (на LLM80.EXE)
и после; журнал system-rom-64.log. Состав не сокращался, старые образы не
удалялись. Рабочий flash-профиль использует 512 КиБ; аппаратные 128+ собраны.
'''
(out/'RESULTS.md').write_text(report,encoding='utf-8')
print('\n'.join(table))
