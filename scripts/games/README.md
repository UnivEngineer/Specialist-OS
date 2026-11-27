# Кампания совместимости игр MXOS2

Стенд загружает систему в видимом Emu80, вводит команду RUN с клавиатуры NC и сверяет нагрузку в памяти при достижении точки входа. Для игр Monitor-2 используется `run H:PROBE.RKS -mon`, для RAMFOS — `run H:PROBE.RKS -rf`. Режимы STD, MX и NATIVE сохраняют самостоятельные строки там, где их указывают реестр или аппаратные признаки. Параметр RUN и фактически введённые клавиши приведены в индивидуальном протоколе. Текущий BIOS MXOS2, Monitor-2 и адаптер RAMFOS можно проверять уже сейчас. Для оценки изменений между сборками нужны два различных системных образа.

## Подготовка под Windows

В корне репозитория нужны Python 3, psutil, Pillow, SjASMPlus, комплект сборки Emu80 и Qt. Образы C:, H: и A: используются как исходные. Пул копирует их в отдельный runtime каждого слота. На H: стенд добавляет испытуемый файл и контрольный маркер, сохраняя остальные файлы с их содержимым. [Сборка системы](../../.agents/skills/mxos-build/SKILL.md) · [сборка и запуск Emu80](../emu80/README.md).

```powershell
python -m pip install psutil "Pillow>=12.1"
.\scripts\build\build-project.ps1 OS
.\scripts\images\build-image.ps1 SystemEmuWritable
.\scripts\images\build-image.ps1 Games2M
.\scripts\images\build-image.ps1 Floppy720
.\scripts\emu80\build.ps1 -Mcp -PoolTemplate
python -X utf8 scripts/emu80/check-mcp-pool.py --size 8
python -X utf8 scripts/emu80/check-mcp-pool.py --size 16
```

Порты 19781–19796 привязаны к loopback. Параметр `--pool-size N` принимает значения от 1 до 16, а `--workers N` ограничивает число одновременно занятых окон. При `--port 19786` используется конкретный экземпляр. Работник держит одно видимое Qt-окно и MCP до конца очереди. Между пробами он подготавливает следующий комплект A:/C:/H: и переключается между двумя именами одной конфигурации Specialist-OS. Emu80 заново создаёт ОЗУ, банки и устройства в том же процессе и окне. Обычный `emu_reset` при проверке выхода сохраняет питание flash и содержимое RAM. После аварии работника следующий владелец сверяет путь и аргументы осиротевшего процесса перед закрытием и повторно создаёт его носители. Проверка `python scripts/emu80/check-cold-boot.py` сверяет HWND, PID и метки в MAIN, банках и H:.

## Инвентаризация и запуск

```powershell
python -X utf8 scripts/games/audit-mxos-compat.py --output docs/compat/game-campaign/audit-before-catalog.json
python -X utf8 scripts/games/audit-mxos-compat.py --catalog --previous-audit docs/compat/game-campaign/audit-before-catalog.json
python -X utf8 scripts/games/run-campaign.py --pilot --pool-size 8
python -X utf8 scripts/games/run-campaign.py --pool-size 16
python -X utf8 scripts/games/control-campaign.py --pool-size 16 --workers 8 --without-card --missing
python -X utf8 scripts/games/control-campaign.py --index-only --without-card
python -X utf8 scripts/games/trace-returns.py --pool-size 16
python -X utf8 scripts/games/diagnose-mon-entry.py
python -X utf8 scripts/games/trace-adskok.py --installed
python -X utf8 scripts/games/record-action-audio.py
python -X utf8 scripts/games/verify-control-campaign.py
python -X utf8 scripts/games/report-campaign.py
python -X utf8 scripts/games/check-campaign.py --require-cold --archive-runtime
```

При наличии исходных пяти ODI команда `python scripts/games/audit-mxos-compat.py --verify-lafan <каталог-ODI>` повторно сверяет адреса, сектора и хеши с сохранённым каталогом Lafan. Для проигрывания одного варианта используйте `--file families/budi/originals/Budi2-std.rks --profile MON` или `--trial <ID>`. Остановленную очередь продолжает та же команда. Принудительный `--force` создаёт новую попытку и сохраняет прежние свидетельства. Проверка `python scripts/tests/check-game-campaign.py --port 19796` воспроизводит остановку работника между двумя пробами и обрыв связи с собственным экземпляром на второй попытке.

Инвентаризация сохраняет отдельно SHA-256 источника, исполняемой нагрузки и точных байтов контейнера. Дубликаты делят пробу лишь при том же типе загрузчика, совпадении байтов контейнера и профиле. Для CPU-дескриптора берётся его I80, для RKS проверяется диапазон и сумма. Архивные расширения проверяются и в исходном виде, и через доступную RUN упаковку. У ряда Lafan известен лишь адрес из ODI. Вход с начала их тела помечен как предположение. В статическом протоколе раздельно доступны улики ABI, аппаратные обращения и загрузки указателей с адресом инструкции и основанием. Разведочный режим записан в причине выбора профиля.

## Свидетельства

Основной протокол содержит байты команды, достигнутые адреса загрузчика и игры, цветные PNG, записи PCM и хеши сравниваемых областей со списками различий. Ожидание проходит в гостевом времени со скоростью до 32×. Для захвата звука Emu80 переключается на 1× и записывает выход звуковой схемы MX до отключения динамиков на хосте. Отчёт приводит амплитуду, RMS, частоту и число цифровых предельных отсчётов. Прослушивание и сравнение с эталоном требуют собственной отметки. Сходство палитры с оригиналом оценивается по сцене при наличии эталона.

Четыре старта в парной проверке сравнивают две машины без ввода и две с исследуемым действием на совпадающих тактах. Кадры до действия и после него, область различий, ограничивающий прямоугольник и, где известен, адрес объекта сохраняются в отдельном протоколе. Отчёт различает изменение экрана, ввод в меню и действие на уровне. Происхождение каждой карточки и независимые повторы отмечены в [справочнике управления](../../assets/games/playbook.md).

После игры фиксируются адрес вектора или адрес возврата и SP при входе и выходе. Если NC не обнаружен, отдельно регистрируется Reset. После появления NC стенд вызывает CLI клавишей F10, сверяет участки BIOS в MAIN, вводит `type H:CHECK.TXT` и проверяет точные байты маркера в процедуре печати DOS. Проверяемые диапазоны BIOS, Monitor-2, RAMFOS и банкового DOS приведены в записи каждой пробы вместе со списком различающихся адресов. Полная запись вектора и установка шрифта учитываются для соответствующего профиля.

## Результаты и повтор на другой сборке

Чтобы точно повторить ввод старой попытки, передайте `python scripts/games/replay-trial.py <путь-к-observation.json>`. Сценарий берётся из этого протокола, изображения A:/C:/H: и символы — из его fixtures. Повтор сохраняется в отдельном каталоге replays. Для выхода со стека `trace-returns.py` подбирает окно по гостевым тактам и затем пошагово сверяет опкод RET, прежний стек и фактический адрес перехода.

Для дополнительной проверки случаев с отказом TYPE или изменившейся flash используйте `python scripts/games/replay-trial.py --anomalies docs/compat/game-campaign --repeat 3 --diagnose-recovery --out docs/compat/game-campaign/rechecks`. Каждый повтор получает исходные файлы и стартовые флаги программной защиты AT29. У системной flash C: флаг равен 01, у H: файл защиты отсутствует. При изменении носителя сохраняются его конечное содержимое, оба хеша и смещения изменившихся байтов. При отказе TYPE после первого сброса записывается результат ещё одного Reset, а при необходимости и холодной загрузки на тех же байтах носителей. Команда `check-campaign.py` повторно читает RGB, параметры WAV, ссылки и маркер каждого завершённого результата.

С параметром `--archive-runtime` проверка сохраняет в fixtures точный EXE, DLL Qt, плагины и данные шаблона. SHA-256 архива и путь находятся в verification.json. Для повтора на другом компьютере с Windows распакуйте архив в отдельный каталог и присвойте его полный путь переменной `SPECIALIST_EMU80_POOL_TEMPLATE`.

Для звука действия `record-action-audio.py` повторяет пилот или указанную `--trial` пробу с уже установленным игровым объектом. Две контрольные записи идут без клавиши, две опытные — с удержанием. Сверяются регистры, RAM, цветные кадры и такты перед действием, затем сохраняются WAV на 1× по фазам удержания и отпускания. Источник записи — динамик модели MX2 после логики PC5 и ВИ53, до регулировки громкости хоста. Магнитофонный PC7 на этом профиле в динамик не выведен.

Основная сводка находится в [RESULTS.md](../../docs/compat/game-campaign/RESULTS.md). Машиночитаемые отчёты, таблицы вариантов, PNG, WAV и закреплённые копии входных образов обычно находятся в локальной `docs/compat/`. Для новой кампании используется отдельный `--out`, для парного исследования — `--campaign`. Другую сборку передайте через `--system <C.bin> --build <каталог бинарников и символов>`. Воспроизводимый набор с сохранёнными C:, H:, A: и таблицами символов указан в карточке каждой пробы.

```powershell
python -X utf8 scripts/games/run-campaign.py --system D:\MXOS2-test\C.bin --build D:\MXOS2-test\bin --out docs/compat/game-candidate --pool-size 16
python -X utf8 scripts/games/control-campaign.py --system D:\MXOS2-test\C.bin --build D:\MXOS2-test\bin --campaign docs/compat/game-candidate --pool-size 16 --workers 8 --without-card
python -X utf8 scripts/games/report-campaign.py --out docs/compat/game-candidate --reference docs/compat/game-campaign
```

Прежний `smoke-mxos-compat.py` моделирует старт посредством отладчика. Эксперимент monitor-001 требует прежнего 64-КиБ ROM и отдельного ROM-runtime. Параметр `--port` принимает 19781–19796. Текущая раздельная сборка BIOS/DOS использует линейную системную flash, для неё штатный RUN и возврат проверяются кампанией выше.

Для проверки исторической дискеты через ROM RAMFOS используйте `python scripts/games/check-lafan-boot.py <каталог-ODI> --platform <исходники-Emu80>/dist/specmx`. Команда копирует дискету во временный каталог, после ввода даты нажимает F6, останавливается на инструкции проверки статуса ВГ93 и сравнивает первые пять секторов с гостевой памятью. В протоколе остаются полный SHA-256, адрес перехода, кадры RAMFOS и конечный хеш дискеты.

## Первоначальные машины и загрузчики

В [таблице исторических путей](../../docs/compat/game-campaign/historical.md) указаны исходный Monitor-2 с файловым загрузчиком Emu80 и командами R/G, архивный RAMFOS на оригинальном MX и MX-DOS V3.6 с реальным выбором записи на ODI. Для повторов данной кампании исходные конфигурации, ПЗУ и пять сверенных дискет лежат в `docs/compat/game-campaign/fixtures/native`. Команда `check-native-campaign.py --archive <каталог-ODI> --platforms <исходники-Emu80>/dist` проверяет дискеты и фиксирует эти входы для новой рабочей копии.

```powershell
$native = 'docs/compat/game-campaign/fixtures/native'
python -B -X utf8 scripts/games/check-monitor-pilot.py --source "$native/dist/spec"
python -B -X utf8 scripts/games/check-lafan-ramfos.py --archive "$native/odi" --source "$native/dist/specmx"
python -B -X utf8 scripts/games/check-almaz2-ramfos.py --source "$native/dist/specmx"
python -B -X utf8 scripts/games/check-rf-ramfos.py --source "$native/dist/specmx"
powershell -NoProfile -File scripts/games/build-lafan-fdc.ps1
python -B -X utf8 scripts/games/check-lafan-mxdos.py "$native/odi"
python -B -X utf8 scripts/games/verify-lafan-mxdos.py "$native/odi"
```

Параметр `--resume attempt-…` продолжает остановленный проход MX-DOS. Индексы содержат точные SHA-256 заголовков, носителей и фактических нагрузок. Для проверки каждого сектора, чтения из ВГ93 и повторного запуска отдельной записи используйте [инструкцию дискетного стенда](../../docs/compat/game-campaign/native-fdc-run/README.md). Для Poker, Pikfama и StarWars2 есть [сопоставление клавиш, кодов, игровых переменных и команд повтора](../../docs/compat/game-campaign/native-ramfos/differences.md).

Для записи динамика оригинального STD используйте `powershell -NoProfile -File scripts/games/build-lafan-fdc.ps1 -MonitorSpeaker`, затем `python -B -X utf8 scripts/games/record-monitor-audio.py`. Отдельная сборка с параметром `beepSoundSource.mcpCaptureSpeaker = yes` снимает канал PC5 до выключения громкости. Сначала реальная программа 8080 переключает этот выход с расчётной частотой 988,14 Гц. Затем BUDI2 и «Очко» проходят по четыре холодных старта с теми же действиями и интервалами, что их пробы в MON. WAV, такты, регистры и отдельные результаты двух физических динамиков находятся в [протоколе звука STD](../../docs/compat/game-campaign/native-monitor/speaker/index.json).

В авторской коллекции Александра Болтаевского сохранены 14 исходных контейнеров из каталога, `MON-BAI2.RKS`, снимок `LIK3.RSS` и указания к выбору монитора. Воспроизводимая копия архива имеет SHA-256 `766be678e8a0e3467ba2d27ffcc12a641d7d7bc8227f9e91daed124babffe592`. [Архив Emu80](https://emu80.org/files/?id=22).
