# Emu80: профиль «Специалиста-MX2»

## Самостоятельный RAMFOS с играми

Задача **`run: Emu80 RAMFOS`** собирает один системный ROM RAMFOS и запускает его с заранее заполненными RAM-дисками. Отдельная задача сборки — **`image: RAMFOS games`**. Состав доступен в `images/RAMFOS-games.md` и `images/RAMFOS-games.json`. Все 62 нагрузки из `assets/games/ramfos-candidates.json` распределяются по 16 страницам ДОЗУ; реестр Markdown описывает тот же набор. `images/RAMFOS-system.bin` содержит полноценное ядро, родной ROM-каталог и `Monitor2.sys`; `images/RAMFOS-games.ramdisk` — 16 последовательных страниц по 64 КБ в формате загрузки RAM-диска Emu80. Сейчас игры занимают 12 страниц, оставшиеся четыре пусты. В RAMFOS при старте открывается RAM-страница 0. Короткие имена игр содержат последние три цифры RF-ID; исходные имена, статусы, адреса и хеши сохраняются в отчёте. Это испытательная коллекция: признаки ABI и включение в образ не подтверждают совместимость.

```powershell
python -X utf8 scripts/images/build-ramfos.py
python -X utf8 scripts/images/check-ramfos.py
powershell.exe -NoProfile -File scripts/emu80/run-ramfos.ps1
```

Сборщику нужна внешняя библиотека `D:/Projects/RetroPC/Specialist`; другой корень задаётся через `--library`. Из неё читаются проверенные по хешам игры и `OS/RAMFOS-master/RomDisk/Monitor2.49152.sys`. Исходные файлы не изменяются. Штатные COM/EXE/I80 включаются как исполняемые EXE, RKS — через загрузчик Монитора-2. Для большинства raw-нагрузок адрес 0000h пока предположен реестром.

Профиль использует тот же `SPECIALIST_EMU80_DIR`, но отдельную платформу `specialist-ramfos/`. Профиль, системная flash и RAM-диски MXOS не заменяются. `-PrepareOnly` подготавливает файлы без запуска окна. Параметр `ramDisk.fileName` загружает игры после привязки всех 16 страниц. Каждый запуск из задачи начинает с исходного набора; автосохранение выключено. Для сохранения изменений используйте Save RAM disk As в Emu80 и отдельный файл, затем открывайте его вручную. Хвост каждой страницы с `FFBBh` оставлен свободным: там RAMFOS проверяет память. Старые `RAMFOS-games-NN.bin` больше не используются. Аппаратный вход STD через PUSH/RST обходится тем же начальным режимом MX, что и в профиле MXOS. Переключение на RAM выполняет загрузчик без стека; совместимость игр, сброс после подмены Монитором-2 и реальные аппаратные тайминги требуют отдельной проверки.

Сам Emu80 устанавливается **снаружи проекта**. Репозиторий хранит конфигурацию, скрипты, схемы и собственные расширения, а не Qt DLL, пользовательские настройки и изменяемые диски. Старая установка EMU и задачи `run: emu` сохраняются.

Рекомендуемый вариант — отдельный portable-каталог рядом с прежним Emu80, например `D:\Projects\RetroPC\Specialist\Emulator\Emu80-MXOS`. Не устанавливайте поверх старых профилей MXOS My.

```powershell
powershell.exe -NoProfile -File scripts/emu80/install.ps1 -DestinationDirectory D:\Projects\RetroPC\Specialist\Emulator\Emu80-MXOS
$env:SPECIALIST_EMU80_DIR = 'D:\Projects\RetroPC\Specialist\Emulator\Emu80-MXOS\Emu80qt_40571'
powershell.exe -NoProfile -File scripts/emu80/run.ps1
```

Установка загружает около 28 МБ и проверяет SHA-256 из `upstream.json`. Версия runtime зафиксирована: 4.0.571. Старый установленный 4.0.379 не подходит этому профилю. `referenceCommit` фиксирует исходники, использованные при подготовке интеграции; это не утверждение, что release ZIP собран из этого SHA.

Задача VS Code `run: Emu80 H-ROM64k` предварительно собирает системный образ и ROM-диск H:. `run: Emu80 H-ROM64k (no build)` использует существующие образы. Скрипт принимает также `-EmulatorDirectory`, `-SystemImage`, `-PrepareOnly` и `-Hidden` (для фоновой диагностики; обычный запуск открывает окно). Он копирует только файлы в `specialist-os/` внутри runtime и не редактирует `emu80.conf`, штатные профили или сохранённые настройки. Закройте экземпляр этой установки перед повторным запуском. Переменную можно сохранить на уровне пользователя Windows; скрипт читает её и оттуда. Если окружение VS Code ещё содержит путь к обычной сборке, flash-запуск проверяет также актуальную пользовательскую переменную и выбирает установку с маркером `SpecialistFlashDisk`. Явный `-EmulatorDirectory` не подменяется.

Профиль задаёт CPU 8080 2 МГц, 16 страниц RAM-диска, непереключаемые 32 байта, две страницы ROM по 32 КБ, стандартный режим и переключение 8/16 цветов. Сброс начинается сразу с MX-страницы ROM (режим 2). Аппаратный вход через `LXI SP,F7FFh; RST 0` в STD-странице не используется: Cpu8080 Emu80 реализует PUSH через запись младшего байта перед старшим, тогда как загрузочная последовательность MX2 требует обратного порядка для переключения через F7FEh. Загрузчик MX-страницы копирует DOS без использования стека; вход DOS на C000h сам устанавливает SP. Исходный системный образ не изменяется. Это обход дефекта порядка шинных операций ядра Emu80, а не полная эмуляция аппаратного сброса; после исправления ядра можно вернуть начальный режим 3. Проверка загрузочного пути без стека: `python scripts/emu80/check-boot.py`. Она исполняет ограниченный набор команд загрузчика до установки SP в DOS; это проверка обхода при старте, а не тест загрузки NC или всего Emu80. Обычный профиль H: использует тот же 64-КБ образ и протокол ROM.COM, что и EMU. Отдельный flash-профиль поддерживает запись на четыре AT29C040A (см. ниже). Порты второго ВВ55 не заняты SD-контроллером или мышью. Файлы `.rks` запускаются из NC через RUN/Monitor2; штатный загрузчик Commander не используется как замена загрузчика MXOS.

Источник аппаратных моделей: `source-emulators/emu80/`. Полный upstream хранится в отдельном checkout `emu80-src`.

## Сборка и запуск flash-профиля

Собственная сборка содержит также `SpecialistMx2VideoRam` для независимых MX/STD-цветов. `run.ps1` определяет наличие модели по манифесту сборки и создаёт `specialist-os/launch.conf` с `MX2_MODELS`. Это работает и с обычным ROM-профилем на собственной сборке. Прямое подключение клавиатурного ВВ55 к штатному SpecVideoRam может затирать MX-цвет при сканировании клавиатуры; официальная сборка без модели проекта не получает это исправление. Проверка: `python scripts/emu80/check-interface.py --runtime <Emu80-flash-test>` (для анализа PNG требуется Pillow). Она проверяет независимость MX-защёлки, STD-цвета, возврат MX-цвета после STD и курсор NC после удаления файлов.

Профиль `run: Emu80 H-Flash2M` использует собственную сборку Emu80 с моделью `SpecialistFlashDisk` и `SpecialistSystemFlash`. Встроенный драйвер читает все банки A:; DOS устанавливает FLASHSYS.COM для разрешения записи до чтения AUTOEXEC; AUTOEXEC устанавливает FLASH.COM для H:. По умолчанию системный A: — AT29C040A 512 КБ, внешний H: — четыре таких чипа общим объёмом 2 МБ. FLASH/FLASHFST определяют число установленных AT29C040A через software ID; FLASHSYS определяет системный тип C010A/C020/C040A тем же способом. Установщики flash-COM загружаются с 0000h, затем копируют резидент в прежние DC00h/DE00h. ROM.COM сохраняет загрузку в DE00h. Подробности и ограничения: [драйверы flash](../../source/flash/README.md). Модель H: уже поддерживает `flashDisk.chipCount = 1..4`; размер её образа должен совпадать с количеством чипов по 512 КиБ. Основной профиль остаётся четырёхчиповым. В двухчиповой тестовой конфигурации драйвер возвращает 1 МиБ. Системная модель принимает также полные линейные образы 128 КБ (AT29C010A, страница 128 байт) и 256 КБ (AT29C020, страница 256 байт). Например:

```powershell
.\scripts\emu80\run.ps1 -Flash -ResetFlash -SystemImage SystemRom_128K.bin
```

`-ResetFlash` заменяет оба рабочих образа с предварительным резервированием. Без него существующий A: сохраняет данные и свой фактический размер независимо от выбранного seed. Для проверки ROM без записи добавьте `-ProtectSystemFlash`: та же модель запрещает /WE через J4; чтение банков остаётся доступным. На любом диске DOS отклоняет удаление файлов System или Read-only, а также замену Read-only. System без Read-only можно изменять: атрибут сохраняется. При удалении NC выводит предупреждение. При ошибке запуска AUTOEXEC или его команды DOS возвращает встроенный драйвер A: и запускает ROM.COM, затем NC. Это не обнаруживает зависание программы или её внутреннюю ошибку. `FLASH.COM` последовательно записывает сектор и ждёт готовности по DQ7. `FLASHFST.COM` выполняет конвейерную запись; DOS распределяет новые кластеры по чипам через обычную FAT-цепочку. Оба драйвера читают одни и те же образы.

Нужны Python, Git, MSVC 2022 Community с Windows SDK и Qt 5.15.2 MSVC x64. Воспроизводимая подготовка Qt (около 36 МБ, проверка SHA-256; нужен 7-Zip):

```powershell
git clone https://github.com/vpyk/emu80v4.git D:\Projects\RetroPC\Specialist\Emulator\emu80-src
git -C D:\Projects\RetroPC\Specialist\Emulator\emu80-src checkout e5cfece5ce16e8da01d0fc68b97cc510234246b7
powershell.exe -NoProfile -File scripts/emu80/setup-build.ps1 -ToolsDirectory D:\Projects\RetroPC\Specialist\Emulator\Emu80-build
$env:SPECIALIST_EMU80_SOURCE_DIR = 'D:\Projects\RetroPC\Specialist\Emulator\emu80-src'
$env:SPECIALIST_EMU80_QT_DIR = 'D:\Projects\RetroPC\Specialist\Emulator\Emu80-build\qt\5.15.2\msvc2019_64'
powershell.exe -NoProfile -File scripts/emu80/build.ps1
powershell.exe -NoProfile -File scripts/emu80/deploy-build.ps1 -SourceDirectory $env:SPECIALIST_EMU80_SOURCE_DIR -QtDirectory $env:SPECIALIST_EMU80_QT_DIR -BuildDirectory D:\Projects\RetroPC\Specialist\Emulator\Emu80-build\flash-gui -DestinationDirectory D:\Projects\RetroPC\Specialist\Emulator\Emu80-MXOS-flash
$env:SPECIALIST_EMU80_DIR = 'D:\Projects\RetroPC\Specialist\Emulator\Emu80-MXOS-flash'
```

После этого запускайте задачу `run: Emu80 H-Flash2M`. Для обновления собственной сборки закройте её окно и выполните `build: Emu80 adapter`; эта задача пересобирает модель и обновляет только runtime с маркером собственной сборки. Скрипты читают эти переменные также с уровня пользователя Windows.

`run: Emu80 H-Flash2M` собирает SystemEmuWritable, Games2M и seed, затем заменяет рабочие A: и H: новыми образами. Прежние `system-flash.bin`, `flash-disk.bin` и их файлы `.sdp` предварительно сохраняются вместе в отдельном каталоге `specialist-os/flash-backups/`. Новые образы начинают с начальным состоянием SDP.

`run: Emu80 H-Flash2M (no build)` использует существующие образы без сборки и сохраняет содержимое обоих дисков между запусками. Если образа ещё нет, он создаётся из имеющегося seed. Для резервирования копируйте оба `.bin` и оба `.bin.sdp` при закрытом эмуляторе. Прямой вызов `run.ps1 -Flash` сохраняет диски; ключ `-ResetFlash` явно заменяет оба с резервной копией. Опция `-ProtectSystemFlash` моделирует J4 с запрещённой записью на A:.

Для диагностики можно собрать `build.ps1 -Mcp` и развернуть его с `-Mcp` в отдельный каталог **Emu80-flash-test**. MCP upstream слушает localhost:19781. Никогда не направляйте тесты на рабочий процесс: они меняют ОЗУ и диск!

```powershell
powershell.exe -NoProfile -File scripts/emu80/run.ps1 -EmulatorDirectory <Emu80-flash-test> -Flash -Mcp -Hidden
python scripts/emu80/check-flash.py --runtime <Emu80-flash-test> --drivers Flash FlashFst
python scripts/emu80/check-flash.py --runtime <Emu80-flash-test> --pipeline-only
# Закрыть и запустить этот тестовый процесс заново, затем:
python scripts/emu80/check-flash.py --runtime <Emu80-flash-test> --verify-persisted
python scripts/emu80/check-system-flash.py --runtime <Emu80-flash-test>
```

Flash, FlashFst и FlashSys поддерживают B=5: синхронная запись, чтение и сравнение сектора общим кодом DOS. Обычный B=1 полного сравнения не делает; DATA polling и тайм-аут остаются обязательными. В DISKBEN режим выбирается при запуске. Проверочное чтение самого бенчмарка вне измерения сохраняется.

Начальный прогон требует неизменённого seed и установленного загрузкой FLASH.COM. FlashSys устанавливается загрузкой ОС; тест его повторно не устанавливает. После перезапуска проверка `check-system-flash.py --verify-persisted` сверяет сектора и SDP. Для отказа записи запустите профиль с `-ProtectSystemFlash` и выполните `check-system-flash.py --locked`. Ограничения модели и отдельный C++ тест описаны в `source-emulators/emu80/README.md`.

Проверка буфера BAT и его границ: `python scripts/emu80/check-bat-buffer.py --runtime <Emu80-flash-test>`. Она использует отформатированный тестовый RAM-диск B: и изменяет его содержимое.

Проверка файлового пути: `python scripts/emu80/check-copy.py --runtime <Emu80-flash-test>`. Проверка запрета удаления и замены с атрибутами R/S/RS на A: и H:. `python scripts/emu80/check-protected-files.py --runtime <Emu80-flash-test>`. Запускайте её на свежем тестовом образе: она создаёт тестовые файлы. Она создаёт файл через DOS и исполняет процедуру F5 из NC для копирования FONT.FNT с A: на H:, проверяет каталог, FAT-цепочку и данные в образе, затем читает файлы через DOS после сброса кэша. После перезапуска тестового процесса повторите с `--verify-only`. Перед проверкой дождитесь загрузки NC. `check-system-files.py` после `check-copy.py` проверяет копирование H: → A: без отдельного подтверждения, а после перезапуска с `--verify-persisted` — содержимое скопированного файла. Старый `check-readonly.py` относится к профилю с 64-КБ ROM и не подходит профилю с автоматически загруженным FlashSys. Старый отчёт `reports/flash-2026-10-29.json` содержит имя Flash, использовавшееся для прежнего варианта с полным сравнением; его контрольные суммы относятся к тому прогону.

Звук ВИ53 использует `SpecMxPit8253SoundSource`. Сверка XML crossboard: GATE0/1/2 подтянуты к единице, CLK0/1 получают 2 МГц, OUT1 тактирует CLK2. Два элемента ЛЕ1 формируют `NOT PC5 AND (OUT0 OR OUT2)`. OUT2 не подключён к GATE0. Именно эту логику реализует штатный источник звука MX в Emu80; источник `RkPit8253SoundSource` ей не соответствует. Магнитофонный выход PC7 не подмешивается в динамик. Причина слышимых шумов/разрывов пока не установлена; схема не является основанием для замены модели ВИ53. Точное соответствие схеме, цветовая палитра, запись системной flash, тайминги реального оборудования требуют отдельной проверки. Этот профиль не заявляет полную аппаратную эквивалентность MX2.

Проверка выделения кластеров и конвейера: `python -X utf8 scripts/emu80/check-flash-allocation.py --runtime <Emu80-flash-test>`. Она создаёт файлы на тестовом H:, читает их Flash/FlashFst и исполняет F5 NC; отчёты сохраняются в `docs/flash/interleave-001/`. Проверяется также B=5 у Flash, FlashFst и FlashSys. `check-driver-api.py` проверяет дескрипторы, инфо-панель NC, сохранность стека и отказ при ошибке чтения/сравнения. `check-diskbench-flash.py` проверяет оба режима меню DISKBEN. Эти сценарии меняют только отдельную установку Emu80-flash-test; результаты и снимки экрана сохраняются в `docs/flash/driver-api/`.

Проверка встроенного ROM-драйвера: `scripts/emu80/check-rom-banks.py`. Сценарий работает только с отдельным Emu80-flash-test; подготовительные фазы `pattern` и `relocate` меняют его системный образ при закрытом эмуляторе. Фаза `read` проверяет тома 128/256/512 КБ с запрещённой записью, `boot` — загрузку перенесённых выше 64 КБ файлов, `rom64` — старую раскладку MX2. Для варианта без Checked нужен `bin/DOS-rom-legacy.sys/.lst`, собранный из DOS.asm с `-DDISK_DRIVER_CHECKED=0`. Отчёты сохраняются в `docs/rom-banks/`.

Проверка ёмкости системной модели: `python -B scripts/emu80/check-system-capacity.py --runtime <Emu80-flash-test>`. Она записывает последний сектор тестового A: и проверяет остальные байты, длину образа, чтение встроенным драйвером и software ID. После перезапуска используйте `--verify-persisted` для проверки данных и SDP. На отдельно запущенном профиле с `-ProtectSystemFlash` используйте `--locked`. Проверены все три ёмкости без дополнения образов до 512 КБ.


### COM/EXE MXOS2 и пересборка образов

Утилиты теперь собираются в CPU/I80. COM и EXE на дисках имеют общий 16-байтный заголовок, RKS сохраняет стандартный контейнер. EXE и RKS передаются через NC.EXT в RUN.COM; EXE без ключа спрашивает режим. RAMFOS.MON загружается ключом -RF. Прежние сырые EXE/COM из старых образов не подходят новому DOS.

При миграции пересоберите SystemEmuWritable и Games2M, затем выполните `scripts/emu80/seed-flash.ps1`. Само обновление четырёх FlashDisk_chip*.bin не заменяет FlashDisk_emu80_seed.bin. Только после этого запускайте собственный профиль с -Flash -ResetFlash; старые диски будут сохранены скриптом запуска. Основной набор сейчас проверяется в большом системном flash-образе; в 64-КБ ROM весь состав не помещается.

Проверка запуска через клавиатуру NC находится в `scripts/emu80/check-executable-ui.py`; он использует отдельный `.local/monitor-compat-001` и подтверждает владельца MCP-порта. Режим --prepare добавляет контрольные файлы только в остановленную тестовую копию H:. Ввод выполняется kbd_send без активации окна Windows. Хостовая проверка контейнеров — `scripts/tests/check-executable-v2.py`.
