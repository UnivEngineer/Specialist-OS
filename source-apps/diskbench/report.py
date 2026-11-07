"""Публикует измерения RAM-драйверов в локальные рабочие материалы docs/."""
import hashlib
import json
import shutil
from pathlib import Path

HERE=Path(__file__).resolve().parent
ROOT=HERE.parents[1]
OUT=ROOT/'docs/diskbench/ramd-001'


def main():
    OUT.mkdir(parents=True,exist_ok=True)
    data={name:json.loads((HERE/'build'/name/'measurements.json').read_text()) for name in ['Legacy','Buffered']}
    assert data['Legacy']['bench_sha256']==data['Buffered']['bench_sha256']
    for name,report in data.items():
        (OUT/f'{name.lower()}.json').write_text(json.dumps(report,indent=2)+'\n')
        shutil.copyfile(HERE/'build'/name/'run-3.png',OUT/f'{name.lower()}.png')
    safety=json.loads((HERE/'build/safety.json').read_text())
    measured=(HERE/'build/Legacy/DISKBEN.com').read_bytes()
    assert measured==(HERE/'build/Buffered/DISKBEN.com').read_bytes()
    final=(HERE/'build/DISKBEN.com').read_bytes()
    assert hashlib.sha256(measured).hexdigest()==data['Legacy']['bench_sha256']
    assert len(measured)==len(final)
    differences=[i for i,(a,b) in enumerate(zip(measured,final)) if a!=b]
    assert not differences or (len(differences)==1 and measured[differences[0]]==0x0C and final[differences[0]]==0x1F)
    shutil.copyfile(HERE/'build/Legacy/DISKBEN.com',OUT/'DISKBEN-measured.com')
    shutil.copyfile(HERE/'build/safety.json',OUT/'safety.json')
    shutil.copyfile(HERE/'build/flash-budget.png',OUT/'flash-budget.png')
    names={'read':'Чтение','write':'Запись',
           'read_seq':'Последовательное чтение','read_random':'Псевдослучайное чтение',
           'write_seq':'Последовательная запись','write_random':'Псевдослучайная запись'}
    rows=[]
    assert data['Legacy']['median'].keys()==data['Buffered']['median'].keys()
    for key in data['Legacy']['median']:
        label=names[key]
        old=data['Legacy']['median'][key];new=data['Buffered']['median'][key]
        rows.append(f"| {label} | {old['ticks']:.0f} | {new['ticks']:.0f} | {old['kib_s']:.4f} | {new['kib_s']:.4f} | {(new['kib_s']/old['kib_s']-1)*100:.2f}% |")
    old=data['Legacy'];new=data['Buffered']
    errors={n:max(abs(v['pit_minus_nominal_cycles']) for r in d['runs'] for v in r.values()) for n,d in data.items()}
    hashes=[]
    for name,d in data.items():
        hashes += [f"- {name} DOS.SYS: `{d['dos_sha256']}`.",
                   f"- {name} ROM запуска: `{d['rom_sha256']}`.",
                   f"- {name} упакованный ROM при измерении: `{d['packaged_rom_sha256']}`."]
    for name in ['legacy','buffered']:
        path=ROOT/f'images/SystemRom_ramd-{name}.bin'
        hashes.append(f"- Финальный SystemRom_ramd-{name}.bin: `{hashlib.sha256(path.read_bytes()).hexdigest()}`.")
    filelist=['source/include/mxos.inc','source/dos/DOS.asm','source/dos/DOSChecked.asm',
        'source/bios/driverFFC0.inc','source/dos/driverBuffered.inc','source/dos/driverChecked.inc',
        'source/bios/reboot2.inc','source/dos/reboot3.inc','source/nc/saveLoadState.inc',
        'scripts/build/build-if-needed.ps1','scripts/build/build-project.ps1','scripts/emu80/check-boot.py',
        'scripts/images/manifests/A-System.txt','source-apps/diskbench/','source-apps/README.md',
        'Specialist-OS.code-workspace','README.md','docs/MXOS-banked-kernel.md']
    text=f'''# RAM-драйверы MXOS: DISKBEN / ramd-001

30.10.2026. **Только Emu80 4.0.575 и его модель ВИ53; аппаратных измерений нет.**

Буферный стековый драйвер реализован и работает, но ускорения не дал.
Поэтому по умолчанию оставлен Legacy, Buffered выбирается явно настройкой сборки.

## Результаты

Медиана трёх полных повторов каждого случая. CPU=CLK1=2 МГц,
канал 1 делит на 256, тик=128 мкс. Wall-clock хоста не используется.
Прочитано 1024 сектора на случай; записано 96 (N=3) с обязательной
проверкой содержимого после каждой записи. Сектор 256 байт.
Общий тестовый участок: {old['start_sector']:04X}h–{old['start_sector']+31:04X}h;
порядок RND — полный воспроизводимый LCG-период на 32 секторах.

| Случай | Legacy, тики | Buffered, тики | Legacy, КиБ/с | Buffered, КиБ/с | Изменение скорости |
|---|---:|---:|---:|---:|---:|
{chr(10).join(rows)}

Сырые повторы, число операций, сверка CPU/PIT, контрольные суммы и список проверок:
[Legacy](legacy.json), [Buffered](buffered.json), [дополнительные проверки](safety.json).
Скриншоты: [Legacy](legacy.png), [Buffered](buffered.png), [нулевой бюджет flash](flash-budget.png).

## Что входит в измерение

Полный физический вызов через C863h и 198 тактов обвязки на операцию.
Развёртывание драйвера, форматирование, подготовка адреса, поиск свободного
места, вывод и проверочное чтение в эти интервалы не входят.
Коррекция накладных расходов скрыто не вычитается.
Результат — скорость секторного ABI, без файлового кэша DOS.
В режиме flash дополнительно учитывается B=4 для завершения очереди FlashFst;
в настоящем сравнении выбран тип R, и эта обёртка не выполнялась.

Номинальные CPU-такты полного вызова, независимо считанные счётчиком Emu80:

| Вызов C863h | Legacy | Buffered |
|---|---:|---:|
| B=2, чтение | {old['driver_cycles']['2']} | {new['driver_cycles']['2']} |
| B=1, запись | {old['driver_cycles']['1']} | {new['driver_cycles']['1']} |

Это обычный допустимый путь с буфером ниже FE00h. Проверка последнего
допустимого буфера FE00h дала Buffered: чтение
{safety['worst_buffer_driver_cycles']['2']}, запись {safety['worst_buffer_driver_cycles']['1']} тактов.
Размер сектора и направление фиксированы, поэтому данные и выравнивание
буфера не меняют число итераций. Для Legacy сохранён прежний байтовый цикл;
обе сборки дополнены общей проверкой страницы/сектора, Buffered также
защищает область FF00h от перекрытия буфером.

Для каждой строки каждого повтора сравнивались `ticks×256` и
`operations×(driver_cycles+198)`. Максимальная абсолютная разница:
Legacy {errors['Legacy']} тактов, Buffered {errors['Buffered']} тактов
за весь соответствующий случай. Разница включает квантование накопленных
измерений и дискретность обслуживания ВИ53 звуковым трактом Emu80.
Нулевые или несовместимые с CPU показания отвергаются; full throttle запрещён.

## Реализация и цена

Буферный цикл: 14×18+4 байта, 15 порций. Полные порции развёрнуты в три
стековые группы по шесть байт, только BC/DE/HL. Переходник стека и 18 байт
буфера находятся в общем окне. Код в FF00h–FF9Eh одинаков в основной RAM
и 16 страницах RAM-диска. Установка выполняется один раз при полном старте,
через сервисный байтовый драйвер; проверочного чтения в MXOS нет.
При переключении B=0/B=1 и тёплом возврате хвосты не переустанавливаются.

Legacy делает 512 смен банка. Buffered делает 30 смен банка и ещё
15 повторных включений основной RAM, всего 45 записей маппера.
Цена полезного копирования увеличивается с 512 до 1024 обращений к данным
памяти, поскольку каждый байт проходит промежуточный буфер. Дополнительно
выполняются сохранение указателей, перестройка стекового переходника и
управление порциями. Именно полный цикл оказался дороже; однократная
установка в этот вывод не включена.

DOS.SYS: Legacy {old['dos_bytes']} байт, Buffered {new['dos_bytes']} байт,
разница +{new['dos_bytes']-old['dos_bytes']} байт. Конец Buffered —
{0xC000+new['dos_bytes']:04X}h, до DA00h свободно {0xDA00-0xC000-new['dos_bytes']} байт.
DA00h–DBFFh зарезервированы под командную строку и стек;
состояние NC вынесено в FFB0h–FFBBh. Файловые секторы/ёмкость не менялись:
xxFFh уже были помечены bad. Границы фиксированных ORG_PAD сохранились.

## Проверки

Холодная загрузка DOS/NC в обоих ROM-профилях; 16 страниц; чтение/запись
в начале, середине и конце RAM-диска; невыровненный буфер со стражами;
сохранность A/BC/DE/HL/SP; отказ для xxFFh, страницы за концом диска и
перекрытия хвоста буфером; идентичность 16 копий; сервисный байтовый ABI;
переключение режима без повторной записи хвостов.

После каждого полного повтора все {old['data_start']} секторов загрузочной области,
FAT и корня сравнивались побайтно. Они не менялись. Начальная контрольная
сумма SHA-256: `{old['metadata_sha256']}`.
Запись шла в свободные кластеры данных, без выделения файла и правок FAT.

Отдельно проверены BPB для 256–4096 байт, одной/двух FAT и неполного
сектора корня; отказ для FAT12 и неверной геометрии; 17-битное смещение
FAT16 кластера 32768; реальный интерфейс F с нулевым бюджетом и пропуском
записи; завершение через B=4 на синхронном драйвере; сохранность хвостов
после FORMAT B: Y R; штатная загрузка A:DISKBEN.COM через fileExec в 0000h.
Разбор BPB будущих секторов не является проверкой несуществующего драйвера
дисковода. Реальную flash в этих тестах не программировали.

COM в обоих сравнениях загружался через MCP из одного измеряемого бинарника;
штатный fileExec проверен отдельно. JSON различает ROM фактического запуска
и итоговый упакованный ROM; это существенно, когда COM пересобран после запуска
эмулятора. Оба используют соответствующий записанный DOS.SYS.

После замеров исправлен единственный байт интерфейса: начальный код 0Ch
(курсор домой) заменён на 1Fh (очистка экрана), чтобы результаты предыдущего
запуска не оставались на экране при нулевом бюджете записи. Измеряемый код
и все адреса остались побайтно прежними; отличие двух COM проверено скриптом.
Финальный интерфейс с нулевым бюджетом и штатная загрузка проверены отдельно.
Исходный измеренный COM сохранён [здесь](DISKBEN-measured.com),
его SHA-256 приведён ниже. Финальный COM:
`{hashlib.sha256(final).hexdigest()}`.

## Воспроизведение и артефакты

[Команды и ограничения DISKBEN](../../../source-apps/diskbench/README.md).
Выполнены сборки OS с `-RamDiskDriver Legacy` и `Buffered`, сборки SystemEmu,
`check-emulator.py --repeats 3` для обоих вариантов, `check-safety.py`,
`check-boot.py` для обоих ROM. Ошибок и предупреждений SjASMPlus нет.
Финальные проверки и выбранный вариант сборки указаны в сообщении о завершении.

Генерируемые выходы: `bin/DISKBEN.i80`, `images/SystemRom_ramd-legacy.bin`,
`images/SystemRom_ramd-buffered.bin`; архивы листингов/символов/бинарников
находятся в `source-apps/diskbench/build/Legacy/` и `Buffered/`.

SHA-256 DISKBEN.COM: `{old['bench_sha256']}`.

{chr(10).join(hashes)}

Изменённые файлы этой работы:

{chr(10).join('- `'+f+'`.' for f in filelist)}

Это проверка функциональности и времени модели. Тайминги реального ВМ80,
CLK1/GATE реальной платы, запись AT29C040A и аппаратное переключение страниц
требуют проверки на оборудовании. Перенос DOS в банк и менеджер выделения
страниц расширенной памяти не реализовывались.
'''
    (OUT/'RESULTS.md').write_text(text,encoding='utf-8')
    print(OUT/'RESULTS.md')


if __name__=='__main__':main()
