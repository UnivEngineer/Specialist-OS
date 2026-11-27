# Справочник игровых проб

В карточках указаны логические имена клавиш Emu80 MCP. Выполняйте действия на указанной сцене и после отпускания предыдущей клавиши. Условия входа, раскладки и наблюдения приведены в [описании игровых проб](playbook.md).

Команды с дисковым именем взяты из манифестов H:. В остальных примерах `PROBE` обозначает имя тестового контейнера, а `ARCHIVE` — проверку архивной упаковки. Для переупаковки используют диапазон загрузки и точку входа исходного файла. `RUN` без ключа режима в карточке NATIVE использует флаг MAIN в заголовке EXE.

В справочнике 470 вариантов. У 289 из них описана последовательность действий. Остальные карточки содержат настройки, выход, назначение архивного материала или наблюдавшееся препятствие. Степень проверки эффекта указана в основании карточки.

Фраза «другой работник воспроизвёл» означает, что второй работник получил тот же игровой эффект с чистого старта. Повторы одного работника и сравнение ввода с контрольным ожиданием указаны отдельно, с привязкой к проверенному файлу и режиму.

Справочник собирается командой `python scripts/games/render-control-reference.py` из [карточек](controls.json) и [каталога](catalog.csv).

[Семейства](#семейства) · [Игры «Специалиста»](#игры-специалиста) · [Игры из подборки MX](#игры-из-подборки-mx) · [Остальные архивные варианты](#остальные-архивные-варианты)

## Семейства

### Adskok.rks

- [families/adskok/repaired/Adskok.rks](families/adskok/repaired/Adskok.rks) — `RUN H:ADSKOK.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. Нажмите пробел на заставке ADSKOK, дождитесь первого поля со счётчиком попыток.

**Действие.** `right`. На первом поле нажмите Right.

**Наблюдение.** На игровом поле Right меняет направление. В трассе байт 6B71h меняется с 00h на 02h.

**Выход.** Проверен Reset с восстановлением NC и чтением файла H:.

**Основание.** scripts/games/trace-adskok.py --installed. Проверка 7252h в рабочем варианте сравнивает D6h с D6h.

### Adskok.rks.bin

- [families/adskok/originals/Adskok.rks.bin](families/adskok/originals/Adskok.rks.bin) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** После входа через RUN -MON программа передаёт управление на C800h до опроса заставки в 6BA7h. Пробел в этой точке снова приводит к C800h.

**Действие.** Трасса заканчивается до первого игрового опроса в 6BF1h.

**Наблюдение.** Архивная программа входит в системный монитор. После Reset команда TYPE читает контрольный файл с H:.

**Выход.** Проверен Reset с восстановлением NC и чтением файла H:.

**Основание.** scripts/games/trace-adskok.py. Трасса archive сохраняет переход в Monitor-2 до опроса заставки.

### Almaz2-mx-optimized.exe

- [families/almaz-2/builds/Almaz2-mx-optimized.exe](families/almaz-2/builds/Almaz2-mx-optimized.exe) — `RUN H:BldrDash/Almaz-2/Almaz2MX.exe`, `RUN H:BldrDash/Almaz-2/Almaz2MX.exe -MX` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Дождаться первой отрисовки после вступления.

**Действие.** `right`. При появлении игрока в нижней строке нажать вправо.

**Наблюдение.** Игрок (тип 09h) перемещается по нижней строке из 81C1h вправо.

**Выход.** `clear`. СТР завершает игру. Возврат в NC для RUN -MX описан в README семейства.

**Основание.** source-apps/almaz2/game.inc и families/almaz-2/README.md. Парный прогон штатной сборки NATIVE подтвердил перемещение игрока. Совместимый RUN -MX указан как маршрут отдельной проверки. Парный протокол с контрольным ожиданием выполнил один работник кампании.

### Almaz2-mx.exe

- [families/almaz-2/builds/Almaz2-mx.exe](families/almaz-2/builds/Almaz2-mx.exe) — `RUN H:PROBE.EXE -MX` (исходный контейнер).
- [families/almaz-2/patched/Almaz2-mxos-custom-palette.cpu](families/almaz-2/patched/Almaz2-mxos-custom-palette.cpu) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [families/almaz-2/repaired/Almaz2-mxos-repaired.bin](families/almaz-2/repaired/Almaz2-mxos-repaired.bin) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** Дождаться первой отрисовки после вступления.

**Действие.** `right`. На первом уровне удержать вправо до смены клетки в нижней строке.

**Наблюдение.** Игрок перемещается по нижней строке, в рабочей карте меняется клетка с кодом 09h.

**Выход.** `clear`. СТР завершает игру через C800h.

**Основание.** source-apps/almaz2/game.inc и families/almaz-2/README.md. Парные MX-прогоны этих трёх нагрузок подтвердили перемещение в нижнем ряду по стрелке. Парный протокол с контрольным ожиданием выполнил один работник кампании.

### Almaz2-original.cpu

- [families/almaz-2/originals/Almaz2-original.cpu](families/almaz-2/originals/Almaz2-original.cpu) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).
- [families/almaz-2/originals/Almaz2-original.rks](families/almaz-2/originals/Almaz2-original.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [families/almaz-2/builds/Almaz2.rks](families/almaz-2/builds/Almaz2.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [families/almaz-2/builds/Almaz2-optimized.rks](families/almaz-2/builds/Almaz2-optimized.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [families/almaz-2/builds/Almaz2-8color.rks](families/almaz-2/builds/Almaz2-8color.rks) — `RUN H:BldrDash/Almaz-2/Almaz2MN.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [families/almaz-2/builds/Almaz2-8color-optimized.rks](families/almaz-2/builds/Almaz2-8color-optimized.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [std/Dash2.rks](std/Dash2.rks) — `RUN H:BldrDash/Dash2.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** После вступительной музыки первый уровень открывается автоматически.

**Действие.** `right`. В начале первого уровня, игрок находится в клетке (1,14).

**Наблюдение.** Стрелка вправо перемещает игрока по нижней свободной строке. В рабочей карте младшая тетрада 09h переходит из 81C1h в 81C2h за один шаг.

**Выход.** `clear`. Во время игрового цикла клавиша 1Fh передаёт управление Monitor-2 через C800h.

**Основание.** source-apps/almaz2/game.inc и source-apps/almaz2/README.md. У Dash2 совпадает код 0000h–0AFFh и начальная строка карты с игроком. Другой работник воспроизвёл перемещение с чистого старта у Dash2.rks, Almaz2.rks, Almaz2-optimized.rks, Almaz2-8color.rks и Almaz2-8color-optimized.rks в MON. Пути к протоколам в playbook-independent-protected/workers.json.

### Almaz2-ramfos-optimized.exe

- [families/almaz-2/builds/Almaz2-ramfos-optimized.exe](families/almaz-2/builds/Almaz2-ramfos-optimized.exe) — `RUN H:BldrDash/Almaz-2/Almaz2RF.exe -RF` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [families/almaz-2/builds/Almaz2-ramfos.exe](families/almaz-2/builds/Almaz2-ramfos.exe) — `RUN H:PROBE.EXE -RF` (исходный контейнер).
- [families/almaz-2/builds/archive/Almaz2-ramfos-20261104.cpu](families/almaz-2/builds/archive/Almaz2-ramfos-20261104.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [families/almaz-2/builds/archive/Almaz2-ramfos-optimized-20261102.cpu](families/almaz-2/builds/archive/Almaz2-ramfos-optimized-20261102.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** Первый уровень открывается автоматически после вступления.

**Действие.** `right`. Удержать стрелку вправо до смены клетки в нижней строке.

**Наблюдение.** В рабочей карте игрок (тип 09h) перемещается из 81C1h вправо.

**Выход.** `clear`. Удержать СТР до возврата через F800h.

**Основание.** source-apps/almaz2/game.inc и families/almaz-2/README.md. Парная RF-проба подтверждает перемещение у Almaz2-ramfos.exe и архивной сборки 20261104. Парный протокол с контрольным ожиданием выполнил один работник кампании.

### Almaz2-ramfos-original-broken.cpu

- [families/almaz-2/originals/Almaz2-ramfos-original-broken.cpu](families/almaz-2/originals/Almaz2-ramfos-original-broken.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [families/almaz-2/repaired/Almaz2-ramfos-fixed.cpu](families/almaz-2/repaired/Almaz2-ramfos-fixed.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** После вступительной музыки уровень открывается автоматически. Дождаться первой отрисовки и игрового цикла на 0038h.

**Действие.** `right`. На первом уровне игрок находится в клетке (1,14). Вправо от него несколько свободных клеток.

**Наблюдение.** Стрелка вправо переносит игрока (младшая тетрада 09h) из 81C1h вправо по строке. За один шаг это 81C2h, при удержании в независимом прогоне — 81C4h. Карты и надписи исторических вариантов сверяются отдельно.

**Выход.** `clear`. На игровом поле удерживать СТР до перехода через F800h. В независимой пробе обеих нагрузок также наблюдались возврат в NC и чтение H:.

**Основание.** Правила карты в source-apps/almaz2/game.inc. Для обеих исторических RF-нагрузок другой работник воспроизвёл движение и возврат с чистого старта. Свидетельства основного прогона и almaz-independent. Есть отдельные парные прогоны со сравнением покоя и ввода.

### Budi-ramfos.cpu

- [families/budi/originals/Budi-ramfos.cpu](families/budi/originals/Budi-ramfos.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [families/budi/patched/Budi-ramfos.exe](families/budi/patched/Budi-ramfos.exe) — `RUN H:LodeRun/BUDI/BUDI-RF.EXE -RF` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `0`. После заставки «YOUR RANGE (0…9)?» выбрать 0. От входа 0000h до приглашения запуск идёт автоматически.

**Действие.** `right` → `home`. В первом игровом поле стрелка вправо перемещает игрока. Home копает слева, ПС/LF справа. Соседняя клетка свободна, кирпич под ней цел.

**Наблюдение.** Игрок сдвигается вправо. При успешном копании появляется яма и заводится счётчик её жизни.

**Выход.** `clear`. На игровом поле 1Fh ведёт через 0FC2h на C000h. Результат перехода проверяется отдельно.

**Основание.** Инструкция families/budi/README.md и ввод ранга в 0120h–012Ah. Переупакованную EXE через RUN -RF проверили два работника с чистого старта. После 0, шага вправо и Home оба зафиксировали 8853h: 09h→48h. Артефакты control-discovery/actions-BudiRF-confirm и парная проба EXE. Архивную CPU-нагрузку отдельно проверил один работник в парном протоколе.

### Budi-std.rks

- [families/budi/originals/Budi-std.rks](families/budi/originals/Budi-std.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [families/budi/patched/Budi-std.exe](families/budi/patched/Budi-std.exe) — `RUN H:PROBE.EXE -MON` (исходный контейнер). `RUN H:LodeRun/BUDI/BUDI-STD.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** После вступления игра переходит к полю по своему счётчику. Дождаться появления игрока.

**Действие.** `right` → `q`. На первом уровне сдвинуться на одну клетку вправо и нажать Q для копания слева. Клавиша с символом ^ копает справа.

**Наблюдение.** Поле и положение игрока совпадают с начальным уровнем BUDI2. Успешное копание меняет клетку у ног игрока.

**Выход.** Клавиша 1Fh вновь загружает текущий уровень. Штатный выход из игры требует отдельной проверки.

**Основание.** families/budi/README.md#управление и разбор Budi-std.rks в 0070h, 02A0h и 03A1h. В парных MON-прогонах RKS и EXE удержание Q после шага вправо изменило кирпич в 8853h с 09h на 48h. В контрольных прогонах остался 09h. Другой работник воспроизвёл копание с чистого старта у указанных RKS и EXE. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Budi2-std.rks

- [families/budi/originals/Budi2-std.rks](families/budi/originals/Budi2-std.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -STD` (исходный контейнер).
- [families/budi/patched/Budi2-std.exe](families/budi/patched/Budi2-std.exe) — `RUN H:PROBE.EXE -MON`, `RUN H:PROBE.EXE -STD` (исходный контейнер). `RUN H:LodeRun/BUDI/BUDI2STD.RKS -MON`, `RUN H:LodeRun/BUDI/BUDI2STD.RKS -STD` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [families/budi/patched/Budi2-std-fff8.exe](families/budi/patched/Budi2-std-fff8.exe) — `RUN H:PROBE.EXE -MON`, `RUN H:PROBE.EXE -STD` (исходный контейнер). `RUN H:LodeRun/BUDI/BUD2FFF8.RKS -MON`, `RUN H:LodeRun/BUDI/BUD2FFF8.RKS -STD` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. После заставки и музыки, в цикле ожидания 02A0h.

**Действие.** `right` → `space`. На первом уровне после вывода поля и появления игрока. Сдвинуться вправо, отпустить стрелку и нажать пробел для копания слева.

**Наблюдение.** При опросе 03A1h HL указывает на клетку игрока (вначале 8852h, после одного шага вправо по твёрдому полу через 0485h — 886Eh). Копание слева меняет кирпич в 8853h с 09h на 48h. Признак выполненного копания — остановка в 05FBh после записи и запись о яме в 8D00h.

**Выход.** `clear`. На игровом поле 1Fh ведёт через 03C4h на C800h. В независимой пробе оригинала под MON возврат в NC состоялся после Reset.

**Основание.** Разбор Budi2-std.rks и families/budi/README.md. Другой работник воспроизвёл шаг и копание оригинальной RKS с чистого старта. Свидетельства основного прогона и budi2-independent. Для обеих EXE-публикаций два работника получили тот же переход кирпича в отдельных прогонах с контрольным ожиданием. Пути к протоколам в playbook-independent-protected/workers.json.

### Buran-ramfos.cpu

- [families/buran/originals/Buran-ramfos.cpu](families/buran/originals/Buran-ramfos.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [families/buran/repacked/Buran-ramfos.exe](families/buran/repacked/Buran-ramfos.exe) — `RUN H:Buran/Buran-RF.exe -RF` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `0` → `1`. На вопрос об инструкции ответьте 0 или 1. После инструкции или сразу после ответа 0 выберите сложность от 1 до 3.

**Действие.** `up`. При появлении прицела нажмите стрелку вверх для пуска ракеты. Влево и вправо управляют. Удержание в повторе привязано к чтению ВВ55 по FFE0h–FFE3h.

**Наблюдение.** Пуск увеличивает байт 8808h на 0431h. На цветном поле с синим фоном видна ракета. Автор слышал звук и отметил более медленное обновление экрана.

**Выход.** В коде есть условный переход F800h, доступная клавиша требует отдельной проверки.

**Основание.** families/buran/README.md и разбор Buran-ramfos.cpu. Пуск стрелкой вверх подтверждён парно у CPU-оригинала и переупакованного EXE через RUN -RF. Другой работник воспроизвёл пуск с чистого старта у обеих указанных RAMFOS-версий. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Buran-starwars-std-germanovitsh.rks

- [families/buran/originals/Buran-starwars-std-germanovitsh.rks](families/buran/originals/Buran-starwars-std-germanovitsh.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [families/buran/originals/Buran-starwars-std-tape.rks.bin](families/buran/originals/Buran-starwars-std-tape.rks.bin) — `RUN H:PROBE.RKS -MON` (RKS после удаления ленточного префикса). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.
- [families/buran/originals/Buran-starwars-std.cpu](families/buran/originals/Buran-starwars-std.cpu) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).
- [families/buran/originals/Buran-starwars-std.rks](families/buran/originals/Buran-starwars-std.rks) — `RUN H:Buran/Buran.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `0` → `1`. В Games2M монохромный Buran.rks происходит из STARWARS.RKS. На вопросе об инструкции нажмите 0, затем на запросе сложности нажмите 1.

**Действие.** `up`. После появления корабля, цели и прицела нажмите стрелку вверх для пуска ракеты. Влево и вправо управляют. В отдельной пробе с -STD пробел запуска не дал.

**Наблюдение.** При обработке пуска появляется ракета. В базовом варианте байт 8808h увеличивается при исполнении 0431h.

**Выход.** После проигрыша меню предлагает «ЕЩЕ?». Штатный возврат в оболочку требует отдельной проверки.

**Основание.** Экранная инструкция и разбор Buran-starwars-std.rks. У этой копии отдельно проверены пуск стрелкой вверх и чтение файла после Reset. Варианты описаны в families/buran/README.md. У вариантов Germanovitsh, tape и CPU пуск подтверждён парными MON-прогонами. Другой работник воспроизвёл пуск у Germanovitsh, tape и CPU с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json. Для базовой Buran-starwars-std.rks отдельно сохранён повтор одного работника.

### Buran-std.rks

- [families/buran/originals/Buran-std.rks](families/buran/originals/Buran-std.rks) — `RUN H:Buran/Buran2.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `down` → `down` → `space`. В меню на заставке «Звёздные войны» дважды нажать вниз с отпусканием. Плашка спустится от инструкции через сложность на игру. Нажать пробел.

**Действие.** `space`. Когда появятся корабль, прицел и счётчики целей, нажмите пробел для пуска. Стрелки управляют, по наблюдению автора.

**Наблюдение.** На поле Buran2.rks появляется ракета, 8008h меняется с 00h на 01h. Автор слышал звук игры, полёт ракеты без отдельного звука.

**Выход.** Клавиша штатного возврата в систему требует проверки.

**Основание.** Заставка, текст инструкции и игровой код. Пуск ракеты оригинальной цветной RKS воспроизведён другим работником через RUN -MON с чистого старта. Свидетельства основного прогона и buran-independent. Есть отдельные парные прогоны.

### Elite-jcuken.rks

- [families/elite/originals/Elite-jcuken.rks](families/elite/originals/Elite-jcuken.rks) — `RUN H:Space3D/Elite/Elite.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `n` → `space` → `f1`. На «Load New Commander (Y/N)?» нажать N. На «Press fire or space, Commander» нажать пробел, дождаться «COMMANDER JAMESON / Condition Docked». На станции F1 запускает взлёт.

**Действие.** `space`. Дождаться окончания анимации взлёта и появления Front View. Нажать и удерживать пробел для разгона.

**Наблюдение.** В обеих раскладках скорость корабля в 808Ch при пробеле растёт с 0Ch до 10h. После отпускания сохраняется 10h. В холостых прогонах скорость остаётся 0Ch.

**Выход.** Clear ставит игру на паузу. На паузе АР2 перезапускает игру. Возврат в ОС требует отдельной проверки.

**Основание.** Авторская инструкция families/elite/originals/Readme-original.txt и повторные парные прогоны RKS обеих раскладок через RUN -MON. Для ЙЦУКЕН также проверена EXE-публикация H:. Запись скорости выполняется в 2748h, признак удержания пробела в 8043h. Другой работник воспроизвёл взлёт и разгон этой RKS с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json. Оба работника также воспроизвели эффект в EXE-публикации.

### Elite-qwerty.rks

- [families/elite/originals/Elite-qwerty.rks](families/elite/originals/Elite-qwerty.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `t` → `space` → `f1`. На «Load New Commander (Y/N)?» нажать T. На «Press fire or space, Commander» нажать пробел, дождаться «COMMANDER JAMESON / Condition Docked». На станции F1 запускает взлёт.

**Действие.** `space`. Дождаться окончания анимации взлёта и появления Front View. Нажать и удерживать пробел для разгона.

**Наблюдение.** В обеих раскладках скорость корабля в 808Ch при пробеле растёт с 0Ch до 10h. После отпускания сохраняется 10h. В холостых прогонах скорость остаётся 0Ch.

**Выход.** Clear ставит игру на паузу. На паузе АР2 перезапускает игру. Возврат в ОС требует отдельной проверки.

**Основание.** Авторская инструкция families/elite/originals/Readme-original.txt и повторные парные прогоны RKS обеих раскладок через RUN -MON. Для ЙЦУКЕН также проверена EXE-публикация H:. Запись скорости выполняется в 2748h, признак удержания пробела в 8043h. Другой работник воспроизвёл взлёт и разгон этой RKS с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Jet-set.rks.bin

- [families/jet-set/originals/Jet-set.rks.bin](families/jet-set/originals/Jet-set.rks.bin) — `RUN H:PROBE.RKS -MON` (исходный контейнер с несовпадающей контрольной суммой).
- [families/jet-set/repaired/Jet-set.rks](families/jet-set/repaired/Jet-set.rks) — `RUN H:JET-SET.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/jet-set.cpu](mx/jet-set.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/jet_set.exe.bin](unknown/lafan/jet_set.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.
- [unknown/lafan/jet_set.exi](unknown/lafan/jet_set.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `enter`. На «Press ENTER to Start» нажать Enter.

**Действие.** Перед пробой нужно дойти до поля. У некоторых контейнеров конечный адрес 8DFFh не помещается в штатный RUN.

**Наблюдение.** Если нагрузка размещается, после Enter появляется комната. Раскладку движения следует установить по выбранной редакции.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенное приглашение 6051h и families/jet-set/README.md. Экран PRESS ENTER наблюдался для mx/jet-set.cpu в RF.

### Komanda13-part1.rks.bin

- [families/kom13/originals/Komanda13-part1.rks.bin](families/kom13/originals/Komanda13-part1.rks.bin) — `RUN H:PROBE.RKS -MON` (исходный контейнер с несовпадающей контрольной суммой).
- [families/kom13/repaired/Komanda13-part1.rks](families/kom13/repaired/Komanda13-part1.rks) — `RUN H:KOM13_1.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** В архиве требуется загрузить сначала KOM13_1, затем KOM13_2. Первая часть содержит приглашение «НАЖМИТЕ ЛЮБУЮ КЛАВИШУ И ВКЛЮЧИТЕ».

**Действие.** Запуск комплекта требует способа загрузки обеих частей.

**Наблюдение.** Проверка игровых команд возможна после запуска всего комплекта.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная строка 45DDh и families/kom13/README.md.

### Komanda13-part2.rks

- [families/kom13/originals/Komanda13-part2.rks](families/kom13/originals/Komanda13-part2.rks) — `RUN H:KOM13_2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Загрузить после KOM13_1 по архивной инструкции. Штатный RUN ограничивает верхний адрес и не принимает полный диапазон 0000h–8EFFh.

**Действие.** Перед пробой управления требуется работающий запуск обеих частей.

**Наблюдение.** После установки обеих нагрузок ожидается переход на игровой экран.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** families/kom13/README.md и штатный предел RUN.

### Lines.rks

- [families/lines/repaired/Lines.rks](families/lines/repaired/Lines.rks) — `RUN H:LINES.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. На цветной заставке «ЦВЕТНЫЕ ЛИНИИ» нажать пробел. Дождаться поля 9×9.

**Действие.** `up` → `up` → `up` → `up` → `left` → `left` → `space` → `right` → `space`. В начальной раскладке курсор в центре. Дойти стрелками до бирюзового шара в третьем столбце верхнего ряда, выбрать пробелом, перейти на клетку справа и снова нажать пробел. При другой раскладке выбрать доступный шар и соседнюю свободную клетку.

**Наблюдение.** Бирюзовый шар из третьего столбца переходит в четвёртый, на доску добавляются новые шары и меняется строка «НОВЫЕ ЦВЕТА».

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Другой работник с чистого старта воспроизвёл выделение бирюзового шара в третьем столбце верхней строки и перемещение в четвёртый. В RUN -MON совпали кадры всех шагов и чтение контрольного файла. Повтор через `python scripts/games/replay-screen.py lines`. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Lines.rks.bin

- [families/lines/originals/Lines.rks.bin](families/lines/originals/Lines.rks.bin) — `RUN H:PROBE.RKS -MON` (исходный контейнер с несовпадающей контрольной суммой).

**Начало.** Исторический контейнер с неполной контрольной суммой. Для пробы используйте repaired/Lines.rks.

**Действие.** В исходном контейнере игра должна сначала пройти проверку заголовка и суммы.

**Наблюдение.** Для действий необходим рабочий образ с той же нагрузкой.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** families/lines/README.md и сверка исходного дефекта контейнера.

### StarTrek-std.rks

- [families/startrek/originals/StarTrek-std.rks](families/startrek/originals/StarTrek-std.rks) — `RUN H:StarTrek.rks -STD`, `RUN H:StarTrek.rks -MX -KOI7`, `RUN H:StarTrek.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `n` → `enter` → `s` → `p` → `o` → `k` → `enter` → `d` → `enter`. Между этапами дождитесь нового приглашения. Ответьте Н и Enter на вопрос о пояснениях, введите СПОК и Enter на запрос имени, затем Д и Enter на «ВЫ ГОТОВЫ (Д/Н)?».

**Действие.** `t` → `enter` → `1` → `enter`. Это текстовая игра с командами и координатами. На «КОМАНДА» введите Т и Enter, на «КУРС ТОРПЕДЫ (1–9)» введите 1 и Enter.

**Наблюдение.** Появляется траектория выпущенной торпеды и её результат, число доступных торпед уменьшается.

**Выход.** Клавиша штатного возврата в систему требует проверки.

**Основание.** families/startrek/README.md. Автор проверил MX с KOI-7 и STD. В Emu80 полный сценарий MX с KOI-7 подтвердил расход торпеды с 10 до 9. Повтор STD с раскладкой smart остался на вопросе о пояснениях. В прежних парных MON-прогонах другой работник независимо воспроизвёл расход торпеды в RKS и EXE.

### StarWars-std.rks

- [families/starwars/originals/StarWars-std.rks](families/starwars/originals/StarWars-std.rks) — `RUN H:Space3D/StarWars/StarWars.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [families/starwars/originals/StarWars-std-C800.cpu](families/starwars/originals/StarWars-std-C800.cpu) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).
- [families/starwars/repacked/StarWars-std-C800.rks](families/starwars/repacked/StarWars-std-C800.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `1` → `space`. При «ВАШ РАНГ?» выбрать 1. Дождаться вращающегося логотипа и удержать пробел до его исчезновения. После появления прицела можно управлять кораблём.

**Действие.** `right`. Когда виден прицел и звёздное поле, удержать стрелку вправо.

**Наблюдение.** У классической версии регистр 15B9h меняется с 0 на −2 и −3. У версии C800h он достигает −5. В контроле без движения остаётся 0.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Разбор W1 и families/starwars/README.md. Другой работник воспроизвёл поворот с чистого старта у исходной RKS и переупакованного варианта C800h. Входы логотипа 007Ah, пропуска 0287h и полёта 03E8h. Запись счётчика 15CEh по 0452h. Пути к протоколам в playbook-independent-protected/workers.json.

### StarWars2-mx.cpu

- [families/starwars/originals/StarWars2-mx.cpu](families/starwars/originals/StarWars2-mx.cpu) — `RUN H:Space3D/StarWars/StarWar2.exe -MX` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [families/starwars/originals/StarWars2-mx.rks.bin](families/starwars/originals/StarWars2-mx.rks.bin) — `RUN H:PROBE.RKS -MX` (исходный контейнер с несовпадающей контрольной суммой).
- [families/starwars/repaired/StarWars2-mx.rks](families/starwars/repaired/StarWars2-mx.rks) — `RUN H:PROBE.RKS -MX` (исходный контейнер).

**Начало.** `1` → `space`. На запросе ранга выбрать 1. Во время вращающегося логотипа нажать пробел.

**Действие.** `right`. Когда виден прицел и звёздное поле, удержать стрелку вправо. ПВ отвечает за стрельбу.

**Наблюдение.** Поворот корабля сдвигает звёзды относительно прицела.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Справка W2 и сверка STD/MX в families/starwars/README.md. Другой работник воспроизвёл поворот с чистого старта в EXE-публикации из StarWars2-mx.cpu и исправленном RKS. В полёте регистр 1BA1h по стрелке вправо изменился с 0 до −6. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### StarWars2-ramfos.cpu

- [families/starwars/originals/StarWars2-ramfos.cpu](families/starwars/originals/StarWars2-ramfos.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [families/starwars/repacked/StarWars2-ramfos.exe](families/starwars/repacked/StarWars2-ramfos.exe) — `RUN H:Space3D/StarWars/SW2-RF.exe -RF` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1` → `space`. На «ВАШ РАНГ?» выбрать 1. Дождаться вращающегося логотипа, затем удержать пробел до его исчезновения. Появится игровое поле с прицелом.

**Действие.** `right`. После появления прицела удержать вправо. Пробел на игровом поле используется для выстрела.

**Наблюдение.** У обеих RF-версий в конце пробы с удержанием вправо регистр 1BA1h изменился с 0 на −6, счётчик 1BC1h с 124 на 118. При контрольном ожидании в этом отсчёте сохранились 0 и 124.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Справка W2 1A63h–1B7Fh. Другой работник воспроизвёл поворот с чистого старта у EXE и переупакованной CPU-нагрузки. Логотип 00BBh, пропуск 02C8h, прицел 06F1h. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### StarWars2-std-germanovitsh.rks

- [families/starwars/originals/StarWars2-std-germanovitsh.rks](families/starwars/originals/StarWars2-std-germanovitsh.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [families/starwars/originals/StarWars2-std-tape.rks.bin](families/starwars/originals/StarWars2-std-tape.rks.bin) — `RUN H:PROBE.RKS -MON` (RKS после удаления ленточного префикса). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.
- [families/starwars/repacked/StarWars2-std.rks](families/starwars/repacked/StarWars2-std.rks) — `RUN H:Space3D/StarWars/StarWar2.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1` → `space`. На «ВАШ РАНГ?» выбрать 1. Дождаться вращающегося логотипа «ВЕРСИЯ 2», удержать пробел до его исчезновения. Дождаться прицела.

**Действие.** `right`. Когда виден прицел и звёздное поле, удержать стрелку вправо. ПВ отвечает за стрельбу.

**Наблюдение.** При удержании вправо у Germanovitsh и кассетной версии регистр 1BA1h меняется с 0 на −4. При контрольном ожидании сохраняется 0.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Справка W2 1A63h–1B7Fh. Другой работник воспроизвёл поворот с чистого старта у варианта Germanovitsh и у кассетной нагрузки после снятия префикса. Кассетная проверочная упаковка совпадает с repacked/StarWars2-std.rks. Пути к протоколам обоих работников в playbook-independent-protected/workers.json. RUN отклонил исходную кассетную упаковку до запуска игры. Игровая проверка использует RKS после снятия префикса.

### Zoo-lik-editor.rks.bin

- [families/zoo/originals/Zoo-lik-editor.rks.bin](families/zoo/originals/Zoo-lik-editor.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** Для исторического редактора команда J7000 из нужного монитора LIK.

**Действие.** `right`. В редакторе стрелки двигают курсор по карте.

**Наблюдение.** Положение курсора видно в рабочей области. Для записи уровня нужна отдельная проба.

**Выход.** `m`. М возвращает в монитор.

**Основание.** Характеристики LIK и справка редактора в 7412h–769Fh.

### Zoo-lik-levels.rks.bin

- [families/zoo/originals/Zoo-lik-levels.rks.bin](families/zoo/originals/Zoo-lik-levels.rks.bin) — Набор уровней ZOO, самостоятельной точки входа нет (README семейства).

**Начало.** Файл содержит дополнительные уровни. Самостоятельной точки входа нет.

**Действие.** Для игровых команд используют исполняемую копию Zoo из семейства.

**Наблюдение.** Работа дополнительных уровней требует загрузчика этого набора.

**Выход.** Выход зависит от запущенной игры Zoo.

**Основание.** families/zoo/README.md и формат архивной записи LIK.

### Zoo-std.rks

- [families/zoo/originals/Zoo-std.rks](families/zoo/originals/Zoo-std.rks) — `RUN H:LodeRun/ZOO.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_rks/Zoo.rks](unknown/games_rks/Zoo.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/zoo.exi](unknown/lafan/zoo.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `space`. На заставке «ЗООПАРК» нажать пробел. H на заставке показывает экран управления.

**Действие.** `right`. После появления первого уровня на свободном участке нажать стрелку вправо. Q и S расчищают клетку по диагонали вниз на подходящем участке.

**Наблюдение.** Спрайт игрока меняет клетку, игровая координата хранится в 0352h. Для копания сравнить состояние тайла до и после.

**Выход.** `k` → `e`. K заканчивает игру. На меню E передаёт управление C800h (087Fh → 06CFh).

**Основание.** Встроенная справка 0956h–0ADEh и обработчики Zoo-std.rks. Справка и обработчики совпадают у перечисленных STD-вариантов. В парной пробе RUN -MON исходный Zoo-std.rks принял правую стрелку после начала первого игрового уровня, 0352h сменил 141Eh на 142Eh и 143Eh. Другой работник воспроизвёл шаг именно у Zoo-std.rks с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

## Игры «Специалиста»

### Almaz.rks

- [std/Almaz.rks](std/Almaz.rks) — `RUN H:BldrDash/Almaz.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Первый запуск автоматически проходит приглашение скорости. После проигрыша можно выбрать 9 для медленной игры.

**Действие.** `right`. Когда закончится стартовая анимация, удержать стрелку вправо до игрового шага, если клетка доступна.

**Наблюдение.** Игрок переходит в соседнюю клетку, координаты в 29B9h–29BAh. Рабочая карта 2830h–29AFh.

**Выход.** В оригинале предусмотрен новый раунд после проигрыша. Из игры выйти аппаратным Reset.

**Основание.** source-apps/almaz/README.md и source-apps/almaz/game.inc. В парном RUN -MON при удержании вправо зафиксирован переход координат (15,6) → (17,6). Оба прогона без клавиши сохранили (15,6). Другой работник воспроизвёл шаг с (15,6) до (17,6) с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Almazy.rks

- [std/Almazy.rks](std/Almazy.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `enter`. На заставке выбрать номер пещеры от 1 до 6 (по умолчанию 1), затем Enter.

**Действие.** `right`. Когда появился герой, стрелкой пройти в свободную соседнюю клетку. Пробел со стрелкой взаимодействует с указанной клеткой без шага.

**Наблюдение.** Герой смещается либо изменяется объект в соседней клетке.

**Выход.** `esc`. Esc сдаёт текущую попытку.

**Основание.** Встроенная инструкция на заставке, просмотренная в Emu80 MON.

### AlmazyMX.rks

- [std/AlmazyMX.rks](std/AlmazyMX.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `1` → `enter`. На «ВЫБЕРИ ТИП», для монохрома нажать 1. На заставке выбрать пещеру и начать Enter.

**Действие.** `right`. На проходимом участке нажать стрелку. Пробел со стрелкой взаимодействует без шага.

**Наблюдение.** Место спрайта игрока или состояние соседнего объекта меняется.

**Выход.** `esc`. Esc сдаёт текущую попытку.

**Основание.** Меню выбора видеорежима и встроенная инструкция, просмотренные в Emu80 MON.

### Arcanoid.rks

- [std/Arcanoid.rks](std/Arcanoid.rks) — `RUN H:ARCANOID.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [std/Cooky.rks](std/Cooky.rks) — `RUN H:COOKY.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [std/Cosman.rks](std/Cosman.rks) — `RUN H:COSMAN.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [std/JumpJek.rks](std/JumpJek.rks) — `RUN H:JUMPJEK.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [std/PSST.rks](std/PSST.rks) — `RUN H:PSST.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/arcanoid.cpu](mx/arcanoid.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [mx/cooky.cpu](mx/cooky.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [mx/cosman.cpu](mx/cosman.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [mx/jumpjek.cpu](mx/jumpjek.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** Дождаться окончания заставки пакета. Следовать приглашению конкретной игры.

**Действие.** Справка перечисляет I, T, X, B и @. S — остановка, Q — продолжение. Назначение каждой игровой клавиши и начальный режим требуют отдельной пробы.

**Наблюдение.** Попытка начата при появлении интерактивного поля. Контрольное действие выбирается по конкретной игре.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная общая инструкция пакета Миронченко (например 62DCh в Arcanoid и 2BCCh в PSST).

### Ball.rks

- [std/Ball.rks](std/Ball.rks) — `RUN H:Ball.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1` → `5`. При выборе игры с компьютером или партнёром нажать 1, затем на запросе скорости — 5.

**Действие.** `right` → `up`. Когда игрок появился на поле, нажать вправо и вверх для прыжка.

**Наблюдение.** Игрок перемещается по горизонтали и подпрыгивает.

**Выход.** Клавиша STOP завершает игру. Преобразование подписи STOP в логическую клавишу MCP требует проверки.

**Основание.** Подсказка и меню в 0638h–06E0h, титульный экран просмотрен в Emu80.

### BKT.rks

- [std/BKT.rks](std/BKT.rks) — `RUN H:BKT.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/bkt.cpu](mx/bkt.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** `t`. В меню T открывает режим Tetris. A выбирает «Барьеры», K — «Copter», ПС заканчивает меню.

**Действие.** `right` → `space`. Стрелка вправо смещает фигуру. Пробел двигает вниз. ПВ поворачивает.

**Наблюдение.** Сдвигаются координаты падающей фигуры и её положение в стакане.

**Выход.** `enter`. ВК выходит из Tetris.

**Основание.** Меню BKT и встроенная справка Tetris в 31E3h–3226h. Диспетчер меню и собственно игра Tetris совпадают с MON-копией по коду и тексту.

### Canball.rks

- [std/Canball.rks](std/Canball.rks) — `RUN H:canball.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. На заставке Cannon Ball нажать пробел, дождаться STAGE = 1 и персонажа у нижней границы.

**Действие.** `right`. Удерживать стрелку вправо до перемещения персонажа вдоль нижней границы.

**Наблюдение.** В двух стартовых пробах вправо координата 632Ch меняется 16 → 17 → 19 → 23 → 28. В обоих контрольных ожиданиях она остаётся 16.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Холодные парные пробы RUN -MON. Два других работника воспроизвели адресные значения, кадры и чтение H: после Reset. Команда `python scripts/games/check-controls-extra.py canball --worker probe`.

### Checkers.rks

- [std/Checkers.rks](std/Checkers.rks) — `RUN H:Chess/CHECKERS.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1` → `0`. На запрос времени обдумывания нажать 1. На «КАКУЮ ДАЕШЬ ФОРУ?» нажать 0.

**Действие.** `c` → `3` → `d` → `4` → `enter`. При «ТВОЙ ХОД» ввести исходную и целевую клетку четырьмя знаками, например C3D4, подтвердить Enter.

**Наблюдение.** Шашка перемещается C3→D4. В истории показано C3-D4 и появляется «МОЙ ХОД. ДУМАЮ…».

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Разбор координат в 011Ch–016Bh. Другой работник с чистого старта воспроизвёл ход C3–D4 и приглашение «МОЙ ХОД. ДУМАЮ…». В RUN -MON совпали кадры всех шагов и чтение контрольного файла. Повтор через `python scripts/games/replay-screen.py checkers`. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Chess.rks

- [std/Chess.rks](std/Chess.rks) — `RUN H:Chess/CHESS.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Дождаться завершения отрисовки доски. На «УРОВЕНЬ (1…3)» выбрать 1 по подсказке.

**Действие.** На доске клавишами курсора выбрать исходную и целевую позицию. Подтверждение хода требует проверки.

**Наблюдение.** Принятый ход перемещает фигуру в доступную клетку.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенный BASIC и примечания к вводу в 3925h–3ABDh. Доска наблюдалась в Emu80 MON.

### Chess2.rks

- [std/Chess2.rks](std/Chess2.rks) — `RUN H:Chess/CHESS2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `0` → `enter` → `1` → `enter`. При «LEVEL 0 OR 1?» ввести 0 и Enter. При выборе стороны ввести 1 и Enter (компьютер играет чёрными).

**Действие.** `e` → `2` → `minus` → `e` → `4` → `enter`. На «ВАШ ХОД» ввести E2-E4 и Enter.

**Наблюдение.** Программа принимает E2-E4 и печатает ответный ход, например «МОЙ ХОД: E7-E5». Интерфейс текстовый.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Шаблон координат в 031Fh–04A0h. Другой работник с чистого старта воспроизвёл принятие E2-E4 и ответ E7-E5. В RUN -MON совпали кадры всех шагов и чтение контрольного файла. Повтор через `python scripts/games/replay-screen.py chess2`. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Chess3.rks

- [std/Chess3.rks](std/Chess3.rks) — `RUN H:Chess/CHESS3.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `0`. На «УРОВЕНЬ (0–2)?» нажать 0. H показывает справку.

**Действие.** `up` → `enter` → `up` → `up` → `enter`. На уровне 0 курсор в A1. Перейти вверх на A2 и подтвердить Enter. Дважды вверх до A4, затем Enter.

**Наблюдение.** Белая пешка переходит A2→A4. Чёрные отвечают D7→D5.

**Выход.** СТР начинает новую партию.

**Основание.** Встроенная справка 1502h–15AFh. Другой работник с чистого старта воспроизвёл ход A2–A4 и ответ D7–D5. В RUN -MON совпали кадры всех шагов и чтение контрольного файла. Повтор через `python scripts/games/replay-screen.py chess3`. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Chess4.rks

- [std/Chess4.rks](std/Chess4.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/ahmaty.exi](unknown/lafan/ahmaty.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `0`. На «УРОВЕНЬ (0–2)?» нажать 0. H показывает справку.

**Действие.** Стрелками выбрать белую пешку, Enter подтвердить, стрелками выбрать доступную клетку, Enter выполнить ход.

**Наблюдение.** Пешка меняет клетку, компьютер готовит ответ.

**Выход.** `clear`. СТР выходит в монитор.

**Основание.** Встроенная справка в 1502h–15AFh, экран запроса уровня просмотрен в Emu80. У EXI в 1502h–15AFh та же справка с выходом в монитор.

### Circus.rks

- [std/Circus.rks](std/Circus.rks) — `RUN H:CIRCUS.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `z`. В меню нажать З (логическая z) для запуска. С (логическая s) меняет скорость.

**Действие.** Игровые клавиши уточняются по экрану конкретной версии.

**Наблюдение.** Открывается игровая сцена, дальнейшая проба действия требует отдельной проверки.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенное меню, просмотренное в Emu80 MON.

### Copter.rks

- [std/Copter.rks](std/Copter.rks) — `RUN H:COPTER.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `2`. После инструкции выбрать класс 2 (доступны 0–9). Дождаться, когда исчезнет первоначальная ракета.

**Действие.** `r`. Р (MCP r) пускает новую ракету. З (z) двигает вправо, Д (d) влево, Ж (v) останавливает.

**Наблюдение.** Пуск меняет флаг активности ракеты в 001Eh с 00h на 01h. В контрольном ожидании флаг остаётся нулевым.

**Выход.** `o`. О (MCP o) — выход.

**Основание.** Экран управления COPTER и парная проба RUN -MON. В двух парах чистых запусков клавиша Р включила флаг ракеты только в рукавах с вводом. Другой работник воспроизвёл пуск ракеты с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Dash1.rks

- [std/Dash1.rks](std/Dash1.rks) — `RUN H:BldrDash/Dash1.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/almaz.exi](unknown/lafan/almaz.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Дождаться отрисовки всей карты из кирпичей. В раннем кадре видна её верхняя часть.

**Действие.** Механика управляется словами FORTH с отдельными именами КВВЕРХ, КВНИЗ, КВЛЕВО, КВПРАВО и ПРОБЕЛ. Для пробы героя ещё нужно сопоставить физические клавиши и конкретный шаг.

**Наблюдение.** Заставка и первая карта доступны. Реакцию объекта на клавишу проверяют после полной отрисовки.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Словарь 2AC0h–2B4Ah и ввод 1100h–1147h. Архивная Lafan-копия отличается платформенными байтами, содержит те же слова управления.

### Defence.rks

- [std/Defence.rks](std/Defence.rks) — `RUN H:DEFENCE.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `enter`. После «НАЖМИ ВК» нажать Enter.

**Действие.** Дождаться игровой сцены и сверить её подсказки перед действием.

**Наблюдение.** После приглашения появляется игровое поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Строка в 07F2h и заставка, просмотренная в Emu80 MON.

### Digger.rks

- [std/Digger.rks](std/Digger.rks) — `RUN H:DIGGER.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/digger.cpu](mx/digger.cpu) — `RUN H:Digger.exe -RF` на [H-ROM-64k-Games.txt](../H-ROM-64k-Games.txt).
- [unknown/games_rks/Zemlekop.rks](unknown/games_rks/Zemlekop.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (исходный контейнер).

**Начало.** `space` → `enter`. В «ЗЕМЛЕКОП-1» нажать пробел при первом «НАЖМИТЕ ЛЮБУЮ КЛАВИШУ». Когда появится повторный запрос в таблице, нажать Enter.

**Действие.** Управление землекопом требует проверки на появившемся поле.

**Наблюдение.** Сначала открывается таблица, затем поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Заставки ЗЕМЛЕКОП-1 в MON и RF и повторный экран запроса клавиши.

### Driller-bw.rks

- [std/Driller-bw.rks](std/Driller-bw.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [std/Driller.rks](std/Driller.rks) — `RUN H:DRILLER.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Дождаться игровой сцены или следовать заставке загруженной версии.

**Действие.** `right` → `space`. На первом уровне сместить танк вправо и нажать пробел у доступной клетки.

**Наблюдение.** Сдвигается танк либо меняется клетка действия.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** std/manuals/games_new/driller_bw.txt и mx/manuals/games_mx_new/driller.txt

### Dungeon.rks

- [std/Dungeon.rks](std/Dungeon.rks) — `RUN H:DUNGEON.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Дождаться окончания вступительной инструкции и появления уровня.

**Действие.** `right` → `enter`. Стрелки перемещают персонажа, Enter выполняет действие. Пробел приостанавливает игру, Home продолжает.

**Наблюдение.** Положение персонажа или состояние объекта меняется в ответ на ввод.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная инструкция 3691h.

### Dungeon2.rks

- [std/Dungeon2.rks](std/Dungeon2.rks) — `RUN H:DUNGEON2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/dungeon.cpu](mx/dungeon.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/dungeon2.com.bin](unknown/lafan/dungeon2.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** После инструкции дождаться уровня. В RF-варианте может потребоваться ответ на запрос количества жизней.

**Действие.** `right` → `space`. Стрелки перемещают персонажа, пробел выполняет действие. ПС приостанавливает игру, ВК продолжает.

**Наблюдение.** Спрайт перемещается или появляется игровой результат действия.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Одинаковая встроенная схема управления в 3691h перечисленных вариантов.

### Dushman.rks

- [std/Dushman.rks](std/Dushman.rks) — `RUN H:DUSHMAN.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/dushman.cpu](mx/dushman.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Dushman.rks](unknown/games_rks/Dushman.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (исходный контейнер).
- [unknown/lafan/du-man.exi](unknown/lafan/du-man.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** `i`. В меню нажать И (логическая i). У выбирает уровень, Т — тип.

**Действие.** Игровые клавиши уточняются по инструкции текущей редакции.

**Наблюдение.** Ввод из меню запускает уровень, конкретное игровое действие требует отдельной проверки.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная заставка, просмотренная в Emu80 MON. Во всех трёх копиях сохранены меню и ветвь выбора И. Изменены платформенные обращения к ПЗУ.

### Dushman2.rks

- [std/Dushman2.rks](std/Dushman2.rks) — `RUN H:DUSHMAN2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_lik_rks/dushman3.rks.bin](unknown/games_lik_rks/dushman3.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.
- [unknown/lafan/du-man_2.exe.bin](unknown/lafan/du-man_2.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.
- [unknown/lafan/du-man_2.exi](unknown/lafan/du-man_2.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `i`. В меню нажать И (MCP i). У (MCP u) изменяет уровень.

**Действие.** Перед игровым действием прочитать инструкцию конкретной версии.

**Наблюдение.** Из меню начинается уровень.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Заставка «ДУШМАН-2» и проверка обработчика меню против Dushman.rks. Блоки меню совпадают в вариантах Dushman2 из LIK и Lafan.

### DwnEarth.rks

- [std/DwnEarth.rks](std/DwnEarth.rks) — `RUN H:BldrDash/DwnEarth.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/d_t_e.cpu](mx/d_t_e.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [mx/dte.cpu](mx/dte.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [unknown/games_rks/DownToEarth.rks](unknown/games_rks/DownToEarth.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (исходный контейнер).
- [unknown/lafan/down_e.exi](unknown/lafan/down_e.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** `m`. На подсказке FIRE/M для старта нажать M.

**Действие.** `right`. На первом поле стрелками управлять дозером. 1 включает паузу.

**Наблюдение.** Дозер перемещается по клеткам поля.

**Выход.** `t`. T завершает управление и возвращает к состоянию, в котором можно начать заново.

**Основание.** Встроенная подсказка управления в 0050h–0099h, просмотренная в Emu80 MON. Подсказка 0034h–0099h повторяется в перечисленных вариантах.

### Football.rks

- [std/Football.rks](std/Football.rks) — `RUN H:FOOTBALL.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_lik_rks/football.rks.bin](unknown/games_lik_rks/football.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** Дождаться цветного поля и мяча. Партия открывается автоматически.

**Действие.** `right`. На первом поле удержать стрелку вправо, пока не сместится управляемая ракетка. Мяч имеет независимую траекторию.

**Наблюдение.** При стрелке вправо растёт горизонтальная координата ракетки в 0863h. В парных прогонах она выросла с AAh до ACh. Без ввода ракетка сохранила положение.

**Выход.** `clear`. СТР завершает матч на поле или с экрана окончания.

**Основание.** Парная проба RUN -MON и ветвь обновления координаты 05E9h. СТР обрабатывается в 0775h–077Dh и 0812h–081Fh. У LIK-копии тот же диспетчер поля, изменены платформенные входы вывода и опроса. Механика отдельно доказана для std/Football.rks. Другой работник воспроизвёл движение ракетки в std/Football.rks с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Gold.rks

- [std/Gold.rks](std/Gold.rks) — `RUN H:GOLD.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/gold.cpu](mx/gold.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Gold.rks](unknown/games_rks/Gold.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (исходный контейнер).

**Начало.** Дождаться полной отрисовки первого многоярусного поля. Следить за положением управляемой фигуры.

**Действие.** `right`. Стрелки курсора перемещают игрока на доступном участке. Выбрать свободную клетку вправо.

**Наблюдение.** При нажатии вправо в std/Gold.rks координата X по 179Bh выросла с 48 до 62 при Y=184 по 179Ah. В контрольных прогонах позиция осталась (48,184).

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Таблица раскладки и диспетчер направлений в теле Gold. Другой работник воспроизвёл движение вправо в std/Gold.rks и архивном unknown/games_rks/Gold.rks с чистого старта через RUN -MON. Запись координаты проходила по 1339h, совпали все отсчёты. Пути к протоколам обоих работников в playbook-independent-protected/workers.json. В остальных копиях отличаются загрузка и системное окружение.

### Gr_demo.rks

- [std/Gr_demo.rks](std/Gr_demo.rks) — `RUN H:GR_DEMO.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Демонстрация птиц начинается автоматически после входа.

**Действие.** `space`. После остановки анимации нажать пробел для повторения.

**Наблюдение.** Программа возвращается к входу 0000h, цикл рисунка начинается снова.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Кадры движения птиц и ожидание любой клавиши C337h в 025Fh.

### Harrier.rks

- [std/Harrier.rks](std/Harrier.rks) — `RUN H:HARRIER.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/harrier.cpu](mx/harrier.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** Дождаться «УРОВЕНЬ (1–5)». Выбрать 1. В приглашении «ПРОДОЛЖИТЬ» указана клавиша @.

**Действие.** Справка перечисляет I, T, X, B и @. S — остановка, Q — продолжение. Направления и запуск по заставке нужно проверить.

**Наблюдение.** После выбора появляется кабина или поле текущего уровня.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная инструкция 2E02h и приглашения 2E38h, 34C0h.

### Helicopt.rks

- [std/Helicopt.rks](std/Helicopt.rks) — `RUN H:HELICOPT.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Дождаться вертолёта, счётчика «РАКЕТ» и объекта ПРО внизу.

**Действие.** `up`. Когда виден нижний объект ПРО, стрелкой вверх запустить ракету. Стрелки влево и вправо перемещают объект.

**Наблюдение.** При пуске по стрелке вверх значения 145Eh и 12AAh меняются с 08h/F0h на 00h/01h. На экране появляется ракета.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Коды стрелок 116Bh–1177h и диспетчер пуска 11ACh. Два чистых RUN -MON дали одинаковое состояние после пуска и чтение контрольного файла после Reset. Прогоны control-discovery/actions-Helicopt-rocket и control-discovery/actions-Helicopt-rocket-repeat. Повтор выполнен тем же работником с чистого старта.

### Kamikaze.rks

- [std/Kamikaze.rks](std/Kamikaze.rks) — `RUN H:Kamikaze.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/kamikadz.com.bin](unknown/lafan/kamikadz.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.

**Начало.** `i`. В меню «ИГРА — И, УРОВЕНЬ — У» выбрать И (MCP i). Дождаться реакции программы. У меняет уровень до запуска.

**Действие.** Действия корабля следует проверить отдельно после выбора партии.

**Наблюдение.** Меню меняется на игровое поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Экранное меню, просмотренное в Emu80 MON. Вариант Lafan сохраняет тексты меню и их расположение около 0500h после архивного префикса.

### Klad.rks

- [std/Klad.rks](std/Klad.rks) — `RUN H:KLAD.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/klad.exi](unknown/lafan/klad.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Сначала титр «СКТБ ВТ / ЧЕРНОВЦЫ / ЭЛЕКТРОНМАШ». Дождаться его полного вывода.

**Действие.** Игровой режим следует отделить от анимации титра, прежде чем проверять клавиши текущей редакции.

**Наблюдение.** Наблюдается вывод титра и графической рамки. Достижение карты требует отдельной проверки.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Кадры MON и сравнение нагрузки. Совпадает основной массив 0000h–3F8Fh с платформенными изменениями, стартовая инструкция Klad2 относится к другому файлу.

### Klad2.rks

- [std/Klad2.rks](std/Klad2.rks) — `RUN H:KLAD2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/klad.cpu](mx/klad.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** `space` → `5` → `enter`. После инструкции нажать любую клавишу. На «ТЕМП ИГРЫ?» выбрать 5 и нажать Enter.

**Действие.** `right`. Стрелки управляют персонажем, Я и Ч выполняют действие слева/справа. Плюс/минус меняют этаж, R возвращает к началу игры.

**Наблюдение.** Персонаж сдвигается на соседнюю клетку либо взаимодействует с клеткой действия.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная инструкция в 42DBh–43F0h. В MX-копии и std/Klad2.rks совпадают инструкция, диспетчер клавиш и содержание поля.

### Kolobki.rks

- [std/Kolobki.rks](std/Kolobki.rks) — `RUN H:KOLOBKI.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_lik_rks/kolobki.rks.bin](unknown/games_lik_rks/kolobki.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** В цветном меню выбрать игру после инструкции. Точный путь через меню требует проверки.

**Действие.** `right`. На поле стрелками вести курсор и отсекать участки.

**Наблюдение.** Проведённая игроком линия отсекает участок с Колобком.

**Выход.** `home`. Клавиша «КУРСОР В НАЧАЛО ЭКРАНА» (Home) выходит из партии.

**Основание.** Встроенная инструкция 397Ah–3BDBh.

### KrestNo.rks

- [std/KrestNo.rks](std/KrestNo.rks) — `RUN H:KRESTNO.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1`. На вопрос о первом ходе выбрать 1 для хода человека.

**Действие.** Заполнить свободную клетку способом, показанным на игровом экране.

**Наблюдение.** На доске появляется знак игрока, затем знак ЭВМ.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашение выбора первого хода, просмотренное в Emu80 MON.

### Krot.rks

- [std/Krot.rks](std/Krot.rks) — `RUN H:KROT.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/digger1.cpu](mx/digger1.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/krot.com.bin](unknown/lafan/krot.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.
- [unknown/lafan/krot.exe.bin](unknown/lafan/krot.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.

**Начало.** `space`. На заставке «КРОТ» нажать пробел. Дождаться цветного поля с мешками.

**Действие.** `right`. Когда виден крот и справа свободная клетка, удержать вправо до шага. Остальные стрелки действуют по направлениям.

**Наблюдение.** Координата 0804h меняется. У std/Krot.rks вправо сдвинуло А6E0h → A8E0h на первом поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Два чистых RUN -MON для std/Krot.rks дали одинаковое изменение клетки по 0804h и чтение файла после Reset. Свидетельства в control-discovery/actions-Krot и control-discovery/actions-Krot-repeat. В архивных копиях сохранён диспетчер 0132h–01B9h, изменена запись цвета в порт. Повтор выполнен тем же работником с чистого старта.

### Ladder.rks

- [std/Ladder.rks](std/Ladder.rks) — `RUN H:LADDER.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/ladder.cpu](mx/ladder.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Ladder.rks](unknown/games_rks/Ladder.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (исходный контейнер).

**Начало.** `p`. В меню P начинает игру. I показывает инструкцию, L задаёт сложность и S скорость.

**Действие.** `6`. 6 перемещает персонажа вправо. 4 — влево, 8 и 5 — вверх и вниз по лестнице.

**Наблюдение.** Спрайт персонажа перемещается на соседнюю клетку при наличии прохода.

**Выход.** `e`. В меню E передаёт управление монитору.

**Основание.** Меню в 0C2Bh–0CBFh и инструкция в 0E57h–0EBCh. Одинаковые справка и меню в 0C73h–0EB0h.

### Lamp.rks

- [std/Lamp.rks](std/Lamp.rks) — `RUN H:LAMP.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/lampo-ka.mg2](unknown/lafan/lampo-ka.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.
- [unknown/games_lik_rks/lamp.rks.bin](unknown/games_lik_rks/lamp.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** Дождаться монохромного помещения, персонажа и двигающегося элемента около стены.

**Действие.** `up`. Стрелка вверх сдвигает стрелка к верхней границе. Влево и вправо меняют направление оружия, пробел стреляет.

**Наблюдение.** Координата 151Ah изменяется на восемь по ветви 128Eh, если не достигнут край.

**Выход.** `enter`. После финального сообщения ВК завершает исполнение через RET.

**Основание.** Обработчик 124Eh–12A3h совпадает у архивных «Ламп». ВК на финальном экране вызывает выход через RET. У издания LIK те же команды и координаты. Платформенные вызовы отличаются.

### Land.rks

- [std/Land.rks](std/Land.rks) — `RUN H:LodeRun/LAND.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [std/Land_s.rks](std/Land_s.rks) — `RUN H:LodeRun/LAND_S.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1`. После «YOUR RANGE» выбрать 1 и дождаться уровня.

**Действие.** `right`. Стрелки двигают героя, Q и ^ выполняют действия слева и справа. Дать ветви игрового цикла принять клавишу.

**Наблюдение.** Герой меняет клетку на доступном участке.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Запрос ранга и первое поле в снимках Emu80. Клавиши действий в общей ветви 05E3h–061Ah.

### Land_c.rks

- [std/Land_c.rks](std/Land_c.rks) — `RUN H:LodeRun/LAND_C.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1`. В UNDERLAND на «РАНГ (1–9)» выбрать 1. При приглашении «НАЖМИТЕ ЛЮБУЮ КЛАВИШУ» перейти к полю.

**Действие.** `right`. На уровне стрелки перемещают героя, Я/Ч выполняют действия слева/справа (MCP q и caret).

**Наблюдение.** Игрок меняет клетку или действует на тайл.

**Выход.** `0`. На игровом поле 0 передаёт управление монитору.

**Основание.** Встроенная справка 0E14h–0ECFh и наблюдение стартового запроса в Emu80.

### LodeRun.rks

- [std/LodeRun.rks](std/LodeRun.rks) — `RUN H:LodeRun/LodeRun.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt). `RUN H:LodeRun.rks -MON` на [H-ROM-64k-Games.txt](../H-ROM-64k-Games.txt).
- [unknown/lafan/loderun.exi](unknown/lafan/loderun.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `2` → `0`. В меню 2 выбирает курсорные клавиши, затем 0 начинает игру.

**Действие.** `right`. На первом уровне нажать вправо у свободной соседней клетки.

**Наблюдение.** Персонаж меняет клетку, при входе в клетку с предметом тот подбирается.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Заставка LODE RUNNER, просмотренная в Emu80 MON. У EXI сохранены варианты выбора 1/2/3/0 в 0322h.

### LRunner.rks

- [std/LRunner.rks](std/LRunner.rks) — `RUN H:LodeRun/LRunner.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/land2.cpu](mx/land2.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Peshchera-0FFF.rks](unknown/games_rks/Peshchera-0FFF.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (исходный контейнер).
- [unknown/games_rks/Peshchera-12FF.rks](unknown/games_rks/Peshchera-12FF.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (исходный контейнер).
- [unknown/lafan/pe-era1.exi](unknown/lafan/pe-era1.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.
- [unknown/lafan/pe-era-6f2381.exe.bin](unknown/lafan/pe-era-6f2381.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** `1`. Дождаться полной отрисовки первого поля. Клавиши 1, 2 и 3 выбирают начальный режим. Для пробы нажать 1.

**Действие.** `9`. На поле клавиши 7 и 9 направляют персонажа влево и вправо. После выбора режима 1 удержать 9 до появления игрока и первого шага. Опрос требует несколько одинаковых считываний.

**Наблюдение.** У std/LRunner.rks по клавише 9 появляется игрок и меняется адрес его клетки 2014h с 2130h на 2170h. При равном ожидании без клавиши поле остаётся в начальном состоянии.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Выбор режима 04F1h–050Fh, таблица действий 08B9h–08CAh, ВВ55 в режиме 91h и колонка цифр по 0024h. Первый работник сравнил два холодных запуска с клавишей 9 и два с ожиданием. Другой воспроизвёл оба варианта через RUN -MON. Совпали все отсчёты, чтение контрольного файла и хеши носителей до и после партии. Протоколы LRunner-idle-A, LRunner-idle-B, LRunner-move-A, LRunner-move-B в control-discovery/cold-window-protected. Независимый повтор в recheck-by-coordinator-protected/LRunner-idle-A и LRunner-move-A. В остальных копиях из карточки та же матрица по коду.

### Manager.rks

- [std/Manager.rks](std/Manager.rks) — `RUN H:MANAGER.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** После загрузки видны «PROGRAM ?» и фрагмент «ЖДИТЕ…».

**Действие.** Чтобы проверить команду менеджера, сначала дождаться перехода встроенной программы к рабочему приглашению. В ранних кадрах завершение ожидания не установлено.

**Наблюдение.** В начале пока виден экран ожидания и меняющиеся элементы строки.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Снимки RUN -MON и вход упакованной программы 1A20h. Наличие общего BASIC-опроса 0F75h не указывает клавиши менеджера.

### Mars.rks

- [std/Mars.rks](std/Mars.rks) — `RUN H:MARS.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_rks/Mars.rks](unknown/games_rks/Mars.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (исходный контейнер).
- [unknown/lafan/mars.exe.bin](unknown/lafan/mars.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.
- [unknown/lafan/mars.exi](unknown/lafan/mars.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** `space` → `space`. На надписи «БОРЬБА С МАРСИАНАМИ» нажать пробел и дождаться появления корабля и пришельцев. Второй пробел начинает движение на готовом поле.

**Действие.** `right`. На первом поле нажать вправо, когда корабль свободен для движения. Влево перемещает корабль в другую сторону. Пробел запускает выстрел при истечении задержки.

**Наблюдение.** Для std/Mars.rks при шаге вправо 8C36h–8C38h меняются с E1 A3 00 на E1 A4 01. На экране корабль сдвигается вправо.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Блокирующий ввод на заставке и игровом поле, диспетчер 065Dh–067Fh. Два работника на чистых стартах std/Mars.rks через RUN -MON получили одинаковый шаг корабля. Совпали байты E1 A3 00 → E1 A4 01, чтение контрольного файла и хеши носителей до и после партии. Протоколы control-discovery/cold-window-protected/Mars-two-space и recheck-by-coordinator-protected/Mars-two-space. Архивные версии сохраняют сравнения кодов клавиатуры при других адресах ПЗУ.

### Mars2.rks

- [std/Mars2.rks](std/Mars2.rks) — `RUN H:MARS2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. При «НАЖМИТЕ ПРОБЕЛ» нажать пробел.

**Действие.** `right` → `space`. В боевой сцене нажать вправо, затем пробел для выстрела.

**Наблюдение.** Положение игрока меняется, от орудия появляется снаряд.

**Выход.** `lf`. ПС заканчивает игру.

**Основание.** Встроенная подсказка в 0C0Fh–0C79h, просмотренная в Emu80 MON.

### Music.rks

- [std/Music.rks](std/Music.rks) — `RUN H:MUSIC.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Дождаться появления музыкального меню с бегущим выделением.

**Действие.** `space`. Когда выделена нужная мелодия, нажать пробел. Номер строки хранится в 03D2h.

**Наблюдение.** Указатель использует таблицу 0412h и передаёт управление проигрывателю.

**Выход.** `clear`. В музыкальном меню СТР завершает программу через C800h.

**Основание.** Меню 0068h–00CEh читает произвольную клавишу для выбора, СТР обрабатывается отдельно.

### Nardy.rks

- [std/Nardy.rks](std/Nardy.rks) — `RUN H:NARDY.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/nardy.cpu](mx/nardy.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/nardy-5231b1.exe.bin](unknown/lafan/nardy-5231b1.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.
- [unknown/lafan/nardy.exe.bin](unknown/lafan/nardy.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.

**Начало.** `space` → `2`. На заставке ADD нажать пробел. При «1 — ИГРА С ПАРТНЕРОМ, 2 — ИГРА С КОМПЬЮТЕРОМ» выбрать 2.

**Действие.** `space`. На «БРОСАЙ СНАЧАЛА ТЫ» дождаться запроса броска и нажать пробел. В допустимом ходе используются координаты 0–9 и A–N.

**Наблюдение.** При подтверждении броска записывается выпавший результат в 8824h. В двух чистых прогонах std/Nardy.rks выпало 3, кадры показывают игральную кость.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Ветви заставки 0026h–002Fh, меню 006Eh–0094h и броска 092Dh–0934h. Проба std/Nardy.rks через RUN -MON в control-discovery/actions-Nardy-roll. В Lafan-копиях сохранены меню и игровой разбор. Изменены адреса аппаратного опроса и системного ввода. Возобновлённый прогон в control-discovery/actions-Nardy-roll-repeat совпал по выпавшей кости и чтению файла H: после Reset. Повтор выполнен тем же работником с чистого старта.

### Nardy2.rks

- [std/Nardy2.rks](std/Nardy2.rks) — `RUN H:NARDY2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** В меню выбрать игру с программой. Номер пункта или способ подтверждения требует проверки.

**Действие.** Любой клавишей бросить кости, затем вводить номера исходной и конечной позиции для хода.

**Наблюдение.** После броска видны значения костей, после принятого хода шашка меняет позицию.

**Выход.** `clear`. СТР завершает партию.

**Основание.** Инструкция 3973h–3A47h.

### Ochko.rks

- [std/Ochko.rks](std/Ochko.rks) — `RUN H:OCHKO.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1` → `enter` → `1` → `0` → `0` → `enter`. На вопрос «СКОЛЬКО ИГРОКОВ» ввести 1 и Enter. Дождаться вопроса «СКОЛЬКО ПОЛОЖИТЬ В БАНК», ввести 100 и Enter.

**Действие.** `1` → `0` → `enter`. После появления «В БАНКЕ … ИГРАЕТ N 1» и запроса «НА СКОЛЬКО» ввести ставку 10 и Enter. Для ввода через MCP дать Monitor-2 пройти отпускание клавиш до C1B5h перед каждым нажатием.

**Наблюдение.** Ставка принята, меняется показ расклада или появляется «Е Щ Е», «О Ч К О» либо «П Е Р Е Б О Р». На вопрос «Е Щ Е» ответ D и Enter запрашивает ещё карту. На 0485h адрес текущего оператора в 0237h последовательно принимает значения 23E5h, 2434h, 2547h и, при запросе ещё карты, 2615h.

**Выход.** Команда штатного возврата из игры пока не установлена.

**Основание.** Токенизированные строки 154, 170, 220, 258 и 265. Ставку, запрос карты, ввод D и переход к следующей ставке воспроизвёл другой работник через RUN -MON с чистого старта. Свидетельства основного прогона и ochko-independent.

### Pacman.rks

- [std/Pacman.rks](std/Pacman.rks) — `RUN H:PACMAN.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_rks/Pacman2.rks](unknown/games_rks/Pacman2.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/pac_man.exe.bin](unknown/lafan/pac_man.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** `0` → `p`. На «НАЖМИТЕ <0>» нажать 0 и дождаться меню. P начинает игру, I открывает инструкцию. После P дождаться полной отрисовки лабиринта.

**Действие.** `5`. После полной отрисовки и появления движущихся призраков, пока Колобок стоит у доступного коридора, удержать 5 для движения вниз. 4, 6 и 8 направляют влево, вправо и вверх.

**Наблюдение.** Игрок идёт вниз и собирает точки. В std/Pacman.rks экранная координата 3821h выросла с 78h до 90h при X=AAh и счёт 21C2h увеличился с 0 до 2. В контрольном ожидании двигались призраки.

**Выход.** `e`. В меню E выходит в монитор.

**Основание.** Встроенная инструкция 22F9h и меню 294Eh–297Fh. В четырёх чистых прогонах std/Pacman.rks сравнили действие с покоем. В обоих прогонах с вводом координата Колобка (170,120) перешла в (170,144), счёт вырос 0→2. Коды 4/6/8/5 совпадают в перечисленных версиях. Другой работник воспроизвёл ход и сбор двух точек в std/Pacman.rks с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Pacman2.rks

- [std/Pacman2.rks](std/Pacman2.rks) — `RUN H:PACMAN2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1`. На «RANGE (0–9)» нажать 1.

**Действие.** Управление на игровом поле устанавливается отдельно для этого варианта.

**Наблюдение.** Появляется лабиринт и открывается игровой цикл.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Экран запроса ранга Pacman2.rks, просмотренный в Emu80 MON.

### Pacman3.rks

- [std/Pacman3.rks](std/Pacman3.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** При запросе имени ввести своё имя по подсказке. 0 очищает таблицу рекордов.

**Действие.** `right`. На игровом поле нажать стрелку к свободному коридору. НР приостанавливает, HOME продолжает.

**Наблюдение.** Положение Колобка меняется в направлении доступного коридора, соответствующие точки исчезают.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная справка, просмотренная в Emu80 MON.

### Paket.rks

- [std/Paket.rks](std/Paket.rks) — `RUN H:PAKET.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `t`. На меню Т (MCP t) или У (MCP u) выбирает Tetris, П (p) — Питон, М (m) — Метку.

**Действие.** После выбора прочитать управление конкретной игры пакета.

**Наблюдение.** Появляется меню или поле выбранной игры.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Экран «ПАКЕТ ИГР», просмотренный в Emu80 MON. Предыдущая пользовательская проверка запуска отмечена в catalog.csv.

### Pango.rks

- [std/Pango.rks](std/Pango.rks) — `RUN H:PANGO.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** После заставки дождаться первого поля.

**Действие.** `right` → `space`. Стрелками подплыть к камню и нажать пробел.

**Наблюдение.** После пробела камень сдвигается либо подтверждается взаимодействие с ним.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная инструкция 2D3Fh–2D91h и подсказка 3741h.

### Patrol.rks

- [std/Patrol.rks](std/Patrol.rks) — `RUN H:PATROL.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `5`. На запросе уровня 0–9 выбрать 5.

**Действие.** После выбора уровня проверить управление по появившейся сцене.

**Наблюдение.** Появляется игровое поле после принятого уровня.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Экран запроса уровня PATROL, просмотренный в Emu80 MON.

### Pentax.rks

- [std/Pentax.rks](std/Pentax.rks) — `RUN H:PENTAX.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `a` → `enter` → `0` → `enter`. Ввести короткое имя и Enter. На запросе уровня ввести 0 и Enter.

**Действие.** Рабочие клавиши перечислены как I,T,X,B. S — приостановка, Q — продолжение. Значение отдельных направлений нужно проверить.

**Наблюдение.** После настроек появляется игровое поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная заставка и текст в 24D0h–2540h. Последовательность ввода имени и уровня сверяется с родственным интерфейсом Tetris3.

### Peregon.rks

- [std/Peregon.rks](std/Peregon.rks) — `RUN H:PEREGON.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_rks/Peregon.rks](unknown/games_rks/Peregon.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (исходный контейнер).
- [unknown/lafan/peregon.exi](unknown/lafan/peregon.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** После отрисовки перегородки и движущихся кругов поле идёт автоматически.

**Действие.** Клавиши обоих участников следует подтвердить по аппаратному опросу. Движение кругов происходит по таймеру.

**Наблюдение.** При E на блокирующем приглашении происходит переход в C800h.

**Выход.** `e`. После окончания партии, когда программа ждёт ответ, нажать E.

**Основание.** Опрос 00FAh и ветвь C337h в 02FFh–0307h. Peregon2 имеет отдельное меню.

### Peregon2.rks

- [std/Peregon2.rks](std/Peregon2.rks) — `RUN H:PEREGON2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** В цветном меню открыть «ИНСТРУКЦИЯ» и посмотреть показанные назначения для двух игроков. «ИГРА» открывает поле.

**Действие.** Каждый игрок передвигает перегородку влево и вправо своими клавишами. Перед пробой прочитать их на экране инструкции.

**Наблюдение.** Перегородка смещается после действия, движущиеся объекты продолжают собственное движение.

**Выход.** `lf`. Во время игры ПС завершает попытку и возвращает в меню игры.

**Основание.** Текст меню 016Ah–0196h и справки 031Dh–03B6h. Ветвь ПС проверяет код 0Ah в 085Bh–0861h.

### Pilot.rks

- [std/Pilot.rks](std/Pilot.rks) — `RUN H:PILOT.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/vozd_boy.cpu](mx/vozd_boy.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Pilot.rks](unknown/games_rks/Pilot.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (исходный контейнер).

**Начало.** `s`. На «ВВЕДИТЕ S(ПУСК), Е(ВЫХОД) ИЛИ 0…9(КЛАСС)» нажать S. Цифра меняет класс.

**Действие.** В std/Pilot.rks в MON диагностическая последовательность после S — пробел и затем вправо. Раскладка движения и атаки требует проверки.

**Наблюдение.** После S появляется поле с двумя самолётами. В парной пробе std/Pilot.rks при удержании вправо и после отпускания положение силуэтов отличается от контрольных кадров. Пробел на проверенном интервале дал тот же кадр, что и холостой прогон.

**Выход.** `e`. В меню E означает выход.

**Основание.** Встроенная команда входа в 29C0h и четыре синхронных старта std/Pilot.rks в MON.

### Piton4.rks

- [std/Piton4.rks](std/Piton4.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `5`. На запросе скорости 0–9 нажать 5.

**Действие.** `s`. На первом поле после выбора скорости нажать С (логическая клавиша s), чтобы первый питон повернул вправо. По инструкции Ц, Ч и Я задают вверх, вниз и влево.

**Наблюдение.** Указатель головы 05A7h с A068h проходит вправо до A768h. Без ввода питон с A068h продолжает вверх до A030h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Заставка «ПИТОН-2» и холодные парные пробы RUN -MON. Два других работника воспроизвели адресные значения, кадры и чтение H: после Reset. Команда `python scripts/games/check-controls-extra.py piton4 --worker probe`.

### Pogonya.rks

- [std/Pogonya.rks](std/Pogonya.rks) — `RUN H:POGONYA.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/presled.exi](unknown/lafan/presled.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.
- [unknown/lafan/presled.com.bin](unknown/lafan/presled.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.

**Начало.** Дождаться круглого объекта, шкалы «НАКАЧКА ЛАЗЕРА» и показателя выигрыша.

**Действие.** `right`. Вправо и влево выбирают горизонтальные ветви управления. На первой сцене пробовать вправо до перехода 046Bh.

**Наблюдение.** Выбор вправо считывается как FBh в FF00h при опросе строки направления. Дальнейший эффект зависит от этапа зарядки.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Кадры «НАКАЧКА ЛАЗЕРА», матричный опрос 0013h–003Ch и ветвь 046Bh. У архивного MON-варианта отличается байт 0131h, у COM перенесены платформенные чтения.

### Pool.rks

- [std/Pool.rks](std/Pool.rks) — `RUN H:POOL.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/pool_bas.cpu](mx/pool_bas.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** На «PRESS ANY KEY TO PLAY» нажать любую клавишу. K на меню позволяет выбрать управление.

**Действие.** Проверить поворот указателя направления на столе после настройки клавиш. Пробел участвует в выполнении удара, точный порядок показан во встроенных правилах.

**Наблюдение.** После удара меняется положение шаров. На некоторых путях вместо поля наблюдалось приглашение PROGRAM ?.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Строки токенизированной программы в 262Eh–2A9Bh.

### Popcorn.rks

- [std/Popcorn.rks](std/Popcorn.rks) — `RUN H:POPCORN.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. На поле «СТЕНКА» дождаться мяча и нижней ракетки. Нажать пробел для начала попытки.

**Действие.** `right`. Вправо и влево двигают ракетку по нижнему краю. Нажать вправо, пока центр ракетки вне правого упора.

**Наблюдение.** В двух прогонах стрелка вправо изменила содержимое 05CDh–05CEh с A1 9E на A2 9F.

**Выход.** `enter`. После конца попыток ВК передаёт управление C800h.

**Основание.** Стартовое ожидание 046Fh и диспетчер 0304h–0345h. Два чистых RUN -MON подтвердили сдвиг после старта и чтение файла после Reset. Свидетельства control-discovery/actions-Popcorn-fired и control-discovery/actions-Popcorn-fired-repeat. Повтор выполнен тем же работником с чистого старта.

### Pusher.rks

- [std/Pusher.rks](std/Pusher.rks) — `RUN H:PUSHER.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. После выбора лабиринта нажать пробел.

**Действие.** `right`. Стрелками перемещать толкателя. Чтобы проверить действие, нажать стрелку в сторону ящика со свободной клеткой за ним.

**Наблюдение.** Толкатель и ящик переходят в соседние клетки.

**Выход.** `space` → `m`. Пробел возвращает к выбору лабиринта. Там M выходит в монитор.

**Основание.** Справка в 01E7h–0251h.

### Python1.rks

- [std/Python1.rks](std/Python1.rks) — `RUN H:Python1.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `5`. На «СКОРОСТЬ (0–9)?» нажать 5.

**Действие.** На игровом поле нужно установить конкретные клавиши поворота.

**Наблюдение.** После числа появляется поле, затем можно проверять поворот змеи.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная строка 02F9h и приглашение, просмотренное в Emu80 MON.

### Python2.rks

- [std/Python2.rks](std/Python2.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `5`. На запрос скорости нажать 5.

**Действие.** Команды движения определяются отдельно для этой редакции.

**Наблюдение.** После числа появляется поле для Питона.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Экран запроса скорости, просмотренный в Emu80 MON.

### Python3.rks

- [std/Python3.rks](std/Python3.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `a` → `d` → `5`. Поочерёдно назначить A и D на поворот влево и вправо, 5 задаёт скорость.

**Действие.** `d`. Во время движения нажать D на свободном участке.

**Наблюдение.** Змея меняет направление на поворот вправо.

**Выход.** После окончания партии на «Д,Н» Н прекращает повтор.

**Основание.** Диалог в 0B0Ch–0BB0h (настраиваемые клавиши).

### R-test.rks

- [std/R-test.rks](std/R-test.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `space`. После инструкции перейти к тесту по приглашению на экране.

**Действие.** При появлении белого квадрата нажать любую клавишу. В пробе с цифрами нажать показанную цифру 1–4. В звуковой пробе нажать клавишу при окончании сигнала.

**Наблюдение.** Программа выдаёт измеренный интервал реакции. Ход теста зависит от выбранного задания.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Текст заданий в 05E5h–07AEh.

### Rain.rks

- [std/Rain.rks](std/Rain.rks) — `RUN H:RAIN.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/dovdx.exi](unknown/lafan/dovdx.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `space`. При «НАЖМИ ЛЮБУЮ КЛАВИШУ» нажать пробел.

**Действие.** `right`. На поле стрелками сдвинуть ёмкость по горизонтали к падающей капле.

**Наблюдение.** Ёмкость сдвигается, при попадании капли меняется игровой счёт.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Справка и приглашение в 02CCh–0408h.

### Rally.rks

- [std/Rally.rks](std/Rally.rks) — `RUN H:RALLY.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/rally.cpu](mx/rally.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Rally.rks](unknown/games_rks/Rally.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (исходный контейнер).
- [unknown/lafan/ralli.exe.bin](unknown/lafan/ralli.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.
- [unknown/lafan/ralli.exi](unknown/lafan/ralli.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.
- [unknown/lafan/ralli_2.exe.bin](unknown/lafan/ralli_2.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.

**Начало.** `enter`. Из инструкции Enter продолжает.

**Действие.** `up` → `right`. На трассе стрелкой вверх увеличить скорость, вправо сместиться

**Наблюдение.** После нажатия вверх прибавляется скорость, после вправо машина меняет полосу.

**Выход.** На экране инструкции пробел завершает игру.

**Основание.** Встроенная инструкция Rally 0968h–0A77h и Rally2 09A5h–0AB4h. Встроенная инструкция стрелок и Enter совпадает во всех добавленных копиях.

### Rally2.rks

- [std/Rally2.rks](std/Rally2.rks) — `RUN H:RALLY2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `enter`. На экране «АВТОРАЛЛИ» нажать Enter и дождаться синей машины у нижней части трассы.

**Действие.** `right`. Из начального положения кратко удержать стрелку вправо. Встроенная инструкция также назначает верхней и нижней стрелкам изменение скорости.

**Наблюдение.** После 60 гостевых мс машины в обоих прогонах переходят из B8CCh в B9CCh по указателю 0ABAh. В контролях указатель остаётся B8CCh.

**Выход.** На экране инструкции пробел завершает игру.

**Основание.** Встроенная инструкция 09A5h–0AB4h и холодные парные пробы RUN -MON. Два других работника воспроизвели адресные значения, кадры и чтение H: после Reset. Команда `python scripts/games/check-controls-extra.py rally2 --worker probe`.

### Reversi.rks

- [std/Reversi.rks](std/Reversi.rks) — `RUN H:REVERSI.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1`. На запросе уровня 1–5 выбрать 1.

**Действие.** На доске поставить фишку на допустимую клетку по подсказке игры.

**Наблюдение.** Линия захвата меняет цвет, компьютер делает ответный ход.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашение уровня, просмотренное в Emu80 MON. Клавиши хода требуют отдельной сверки.

### Reversi2.rks

- [std/Reversi2.rks](std/Reversi2.rks) — `RUN H:REVERSI2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. На «НАЖМИТЕ ПРОБЕЛ» нажать пробел. Затем выбрать цвет по приглашению.

**Действие.** На доске выяснить выбор клетки и подтверждение хода.

**Наблюдение.** При допустимом ходе фишки линии захвата меняют цвет.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенные приглашения 01D8h, 0B2Bh и 0B7Fh.

### Reversi3.rks

- [std/Reversi3.rks](std/Reversi3.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** На «FIRST?» выбрать, кто ходит первым, по подсказке программы.

**Действие.** Формат ввода координат клетки требует проверки.

**Наблюдение.** При принятом ходе меняются фишки на доске.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная строка в 5716h и приглашение, просмотренное в Emu80 MON.

### Riseout.rks

- [std/Riseout.rks](std/Riseout.rks) — `RUN H:RISEOUT.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/riseout.com.bin](unknown/lafan/riseout.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.

**Начало.** `1`. При выборе скорости числами 1–6 нажать 1.

**Действие.** Дождаться героя на поле. Указания по клавишам из тусклой заставки требуют отдельной расшифровки.

**Наблюдение.** Игра переходит от запроса скорости к уровню.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Стартовый экран MON в Emu80. В архивной копии совпадает текст запроса и диапазон скорости.

### Robot.rks

- [std/Robot.rks](std/Robot.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** На запросе джойстика выбрать клавиатуру. На «ПУСК? (Y,N)» ответить Y (ожидается код 59h).

**Действие.** В игре нужно удерживать робота под нужной колонной. Раскладка перемещения требует проверки.

**Наблюдение.** Появляется поле падающих блоков, результат виден в счёте.

**Выход.** На вопросе продолжения N (4Eh) завершает игру.

**Основание.** Встроенные запросы и сравнение 59h/4Eh в 003Eh–0045h.

### Saboteur.rks

- [std/Saboteur.rks](std/Saboteur.rks) — `RUN H:SABOTEUR.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/saboteur.cpu](mx/saboteur.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Saboteur.rks](unknown/games_rks/Saboteur.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (исходный контейнер).
- [unknown/lafan/saboteur.exe.bin](unknown/lafan/saboteur.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.
- [unknown/lafan/saboteur.exi](unknown/lafan/saboteur.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `space` → `space` → `space` → `space`. Нажимать пробел после появления каждой сцены по очереди. Сначала заставка, затем описание цели, список предметов и экран с «УРОВЕНЬ / СЕКТОР». После четвёртого пробела дождаться лабиринта.

**Действие.** `up`. В начальном лабиринте нажать вверх. Другие стрелки перемещают героя в проходимые клетки, Enter вызывает ветвь действия.

**Наблюдение.** На первом поле std/Saboteur.rks координата 1207h–1208h сменилась с 31 FE на 31 FD после стрелки вверх.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Ввод 029Ch, 10AEh, 2CFCh, 6F9Ch и диспетчер 1417h. В копиях совпадают коды клавиш, вызовы ПЗУ зависят от платформы. Два чистых RUN -MON std/Saboteur.rks дали одинаковое перемещение и чтение контрольного файла после Reset. Снимки и состояние в control-discovery/actions-Saboteur и control-discovery/actions-Saboteur-repeat. Архивное руководство в unknown/lafan/Saboteur.TXT. Повтор выполнен тем же работником с чистого старта.

### Samfox.rks

- [std/Samfox.rks](std/Samfox.rks) — `RUN H:SAMFOX.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/samantha.cpu](mx/samantha.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [unknown/lafan/sama_fox.exi](unknown/lafan/sama_fox.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** Программа показывает первый кадр иллюстраций автоматически.

**Действие.** `space`. После остановки вывода нажать пробел для перехода к следующему кадру.

**Наблюдение.** Обработчик меняет элемент таблицы иллюстраций 00AAh и рисует следующий кадр.

**Выход.** `clear`. Между кадрами СТР завершает программу через RET.

**Основание.** Таблица кадров в 00AAh и блокирующий ввод 007Ch–0083h. СТР выполняет RET.

### Scrapper.rks

- [std/Scrapper.rks](std/Scrapper.rks) — `RUN H:SCRAPPER.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [std/Scrap2.rks](std/Scrap2.rks) — `RUN H:SCRAP2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/sky_scra.cpu](mx/sky_scra.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/sky_scr.exe.bin](unknown/lafan/sky_scr.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.
- [unknown/lafan/sky_scra.com.bin](unknown/lafan/sky_scra.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.
- [unknown/lafan/sky_skra.exi](unknown/lafan/sky_skra.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `space` → `space`. На экране PygMalion нажать пробел и дождаться полной анимации заголовка SKY SCRAPER. Затем нажать пробел повторно.

**Действие.** `right`. На игровом поле стрелки влево и вправо перемещают персонажа. Вправо пробовать на участке, где он может пройти.

**Наблюдение.** Меняется координата персонажа по горизонтали.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Опрос заставок 029Ch и 18B6h, общие сравнения 08h и 18h в 2120h–212Bh и 2F20h–2F2Bh. Клавиши дополнительных действий в редакциях различаются.

### Skachki.rks

- [std/Skachki.rks](std/Skachki.rks) — `RUN H:SKACHKI.RKS -STD`, `RUN H:SKACHKI.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_rks/Skachki.rks](unknown/games_rks/Skachki.rks) — `RUN H:PROBE.RKS -STD`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (исходный контейнер).
- [unknown/lafan/ska-ki.com.bin](unknown/lafan/ska-ki.com.bin) — `RUN H:PROBE.RKS -STD`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -STD`, `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** `0` → `space`. На «СЛОЖНОСТЬ (0/1)?» выбрать 0. На «СТАРТ?» нажать любую клавишу.

**Действие.** Во время гонки требуется подтвердить раскладку управления наездником.

**Наблюдение.** После подтверждения старта начинается движение лошадей. Оно также может быть автоматическим.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашения 004Eh–007Eh, числовой ввод 01A0h и ожидание клавиши старта 01D5h.

### Skachki2.rks

- [std/Skachki2.rks](std/Skachki2.rks) — `RUN H:SKACHKI2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** На вопросы об участниках «да» — пробел, «нет» — другая клавиша. На «ПРОДОЛЖИТЬ» — пробел.

**Действие.** Перед гонкой прочитать подписи для первого и второго наездников. Коды управления нужно сверить отдельно.

**Наблюдение.** После ответов и подтверждения появляется трасса.

**Выход.** `lf`. ПС выходит из игры.

**Основание.** Встроенная инструкция в 01C8h–024Fh и приглашение 1046h.

### Sonata.rks

- [std/Sonata.rks](std/Sonata.rks) — `RUN H:SONATA.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** После запуска появляется титул «SONATE NR. 14».

**Действие.** Материал представляет музыкальное исполнение одной пьесы. Игровой объект для управления отсутствует.

**Наблюдение.** Первый кадр содержит название сонаты и имя композитора.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Титульный экран музыкального демонстратора в Emu80 MON.

### SPatrol.rks

- [std/SPatrol.rks](std/SPatrol.rks) — `RUN H:spatrol.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. На «PRESS [RUN] FOR GAME» нажать пробел и дождаться STAGE 1.

**Действие.** Дождаться спрайта корабля. Обработчик направления и огня пока не установлен.

**Наблюдение.** Появляется поле первой стадии.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Переход от заголовка к STAGE 1 после пробела в кадрах Emu80 MON.

### Specialist3D.rks

- [std/Specialist3D.rks](std/Specialist3D.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** Дождаться первой отрисовки трёхмерной сцены. Меню выбора игры отсутствует.

**Действие.** `right` → `up`. Нажать вправо для поворота. Когда ракурс изменится, нажать вверх для движения вперёд. На высокой ступеньке движение может быть заблокировано.

**Наблюдение.** Угол 001Ah меняется с 0300h на 030Ch при одном шаге поворота. При удержании вверх меняются координаты в 0016h–0019h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** README и src/engine.asm из локального архива Specialist3D_src.7z. У готового 3d.rks из архива и std/Specialist3D.rks одинаковый SHA-256 f4fbb8a75899e9e8d0bc293f160327dc70444b643f92d484bd10a0b6fedbf946. В двух чистых прогонах через RUN -MON совпали угол после поворота и позиция после движения. После Reset прочитан контрольный файл с H:. Повтор выполнен тем же работником с чистого старта.

### SpKing.rks

- [std/SpKing.rks](std/SpKing.rks) — `RUN H:SpKing.rks -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/afgan.cpu](mx/afgan.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/comti.exi](unknown/lafan/comti.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.
- [unknown/lafan/spaceki.exe.bin](unknown/lafan/spaceki.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.

**Начало.** После «Game start» дождаться игрового поля Spaceking и вертолёта.

**Действие.** `right` → `space`. На свободном участке нажать стрелку вправо. Пробел сбрасывает бомбу.

**Наблюдение.** Вертолёт меняет позицию, бомба отделяется от корпуса по пробелу.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Снимок Spaceking и общий блок игровой нагрузки с SpKing2.rks. В этих выпусках стартовая часть и платформенный опрос отличаются.

### SpKing2.rks

- [std/SpKing2.rks](std/SpKing2.rks) — `RUN H:SPKING2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** На «ВЫБЕРИ СКОРОСТЬ» выбрать значение по экрану.

**Действие.** `right` → `space`. Стрелки двигают вертолёт, пробел сбрасывает бомбу.

**Наблюдение.** По нажатию пробела видно отделение бомбы.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная инструкция 3D70h–3DBBh.

### Square.rks

- [std/Square.rks](std/Square.rks) — `RUN H:SQUARE.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/kwadrat.cpu](mx/kwadrat.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [unknown/lafan/kwadrat.exi](unknown/lafan/kwadrat.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.
- [unknown/lafan/kwadrat.mg2](unknown/lafan/kwadrat.mg2) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON`, `RUN H:ARCHIVE.MG2 -MX` для проверки исходного архива.

**Начало.** В меню «НОВАЯ ЗАДАЧА» создаёт поле, «ИГРА» открывает его. Клавиши самого меню требуют проверки.

**Действие.** `right` → `space`. Стрелкой сдвинуть курсор, пробел выполняет ход по выбранной клетке.

**Наблюдение.** Меняется состояние клеток и увеличивается «СДЕЛАНО ХОДОВ».

**Выход.** `enter`. Enter выходит из партии в меню.

**Основание.** Меню и встроенная инструкция в 3CC6h–3E18h или 04C6h–0618h.

### Stakan.rks

- [std/Stakan.rks](std/Stakan.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/games_rks/Stakan2.rks](unknown/games_rks/Stakan2.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `g`. В меню G начинает игру. Настройки скорости и класса можно менять до старта.

**Действие.** `right`. Во время падения фигуры нажать вправо. P ставит на паузу.

**Наблюдение.** Фигура сдвигается вправо, оставаясь в игровом стакане.

**Выход.** `e`. E завершает игру.

**Основание.** Меню в 0A28h–0B08h и обработчики std/Stakan.rks. У архивного Stakan2.rks совпадает меню 0A28h–0B08h именно со std/Stakan.rks.

### Stakan2.rks

- [std/Stakan2.rks](std/Stakan2.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `space` → `caps` → `i`. На «ИГРА — И, УРОВЕНЬ — У» сначала нажать пробел, чтобы разбудить опрос. При отпущенных клавишах нажать РУС/ЛАТ (MCP caps), затем И (MCP i). Дождаться конца анимации и первой падающей фигуры.

**Действие.** `right`. После конца анимации дождаться первой фигуры и нажать вправо. Пробел выполняет сброс.

**Наблюдение.** В двух чистых пробах D в регистре DE вырос с A6h до A7h по ветви сдвига 0115h–0118h. Фигура переместилась вправо.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Опрос ВВ55 в 072Ch и C337h в 0735h. РУС/ЛАТ переключает 8FECh с 3Ah на BAh, клавиша i возвращает 69h. Два RUN -MON через NC дали одинаковый старт и сдвиг фигуры, после Reset прочитан файл H:. Свидетельства control-discovery/actions-Stakan2-game и control-discovery/actions-Stakan2-game-repeat. Повтор выполнен тем же работником с чистого старта.

### Stena.rks

- [std/Stena.rks](std/Stena.rks) — `RUN H:STENA.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/stena_1.com.bin](unknown/lafan/stena_1.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.
- [unknown/lafan/stena_1.exe.bin](unknown/lafan/stena_1.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.
- [unknown/lafan/stena_1.mg2](unknown/lafan/stena_1.mg2) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON`, `RUN H:ARCHIVE.MG2 -RF`, `RUN H:ARCHIVE.MG2 -MX` для проверки исходного архива.
- [unknown/lafan/stena_cw.exi](unknown/lafan/stena_cw.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** После заставки дождаться поля. Игровое управление требует уточнения по версии.

**Действие.** Нужно зафиксировать реакцию платформы на установленную клавишу до автономного движения мяча.

**Наблюдение.** Сдвиг платформы или отбитый мяч подтверждают игровой ввод.

**Выход.** `clear`. Клавиша стирания экрана, MCP clear (1Fh), завершает игру.

**Основание.** Справка «ВЫХОД ИЗ ИГРЫ» в 0086h. У std/Stena.rks сравнение с 1Fh выполняется на 007Ch.

### Stena2.rks

- [std/Stena2.rks](std/Stena2.rks) — `RUN H:STENA2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/stena2.cpu](mx/stena2.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Stena.rks](unknown/games_rks/Stena.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (исходный контейнер).

**Начало.** `1` → `5`. В меню 1 выбирает курсорное управление, 5 начинает игру.

**Действие.** `right`. Стрелками управлять на доске. Enter приостанавливает падение или продолжает игру.

**Наблюдение.** Игровой объект смещается вправо под действием курсорной клавиши.

**Выход.** `enter` → `clear`. Во время паузы СТР выходит в систему.

**Основание.** Встроенная инструкция 0589h–07B5h.

### Suppiton.rks

- [std/Suppiton.rks](std/Suppiton.rks) — `RUN H:SUPPITON.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/piton.cpu](mx/piton.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/games_rks/SuperPiton.rks](unknown/games_rks/SuperPiton.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (исходный контейнер).
- [unknown/lafan/s.piton.exi](unknown/lafan/s.piton.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.
- [unknown/lafan/s_piton.exi](unknown/lafan/s_piton.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.
- [unknown/lafan/piton_.com.bin](unknown/lafan/piton_.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** В настройках игры задаётся скорость 0–9 и задаётся вопрос «ИЗМЕНИТЬ РЕЖИМЫ ИГРЫ? (Д/Н)». Порядок подтверждений отличается по версии.

**Действие.** После настройки управления вызвать один поворот в свободном направлении.

**Наблюдение.** При повороте голова питона меняет направление. Автоматическое движение пробу клавиш не подтверждает.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенные пункты скорости и настройки в 045Fh–051Ch.

### Tennis.rks

- [std/Tennis.rks](std/Tennis.rks) — `RUN H:TENNIS.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_rks/Tennis2.rks](unknown/games_rks/Tennis2.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** Последовательно назначить клавиши перемещения вправо/влево, остальные движения и подачу по приглашениям. Затем выбрать скорость 1–9.

**Действие.** На корте назначенной клавишей сделать подачу, затем проверить движение ракетки.

**Наблюдение.** Мяч покидает ракетку и начинается розыгрыш.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенные запросы 0057h–038Ch.

### Tennis2.rks

- [std/Tennis2.rks](std/Tennis2.rks) — `RUN H:TENNIS2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/tennis.cpu](mx/tennis.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** `1`. На запросе класса выбрать 1.

**Действие.** На корте определить раскладку ракетки и подачу для этой версии.

**Наблюдение.** На поле появляется ракетка и мяч.

**Выход.** `e`. E — выход.

**Основание.** Встроенная справка 72D8h–72EBh.

### Tennis3.rks

- [std/Tennis3.rks](std/Tennis3.rks) — `RUN H:TENNIS3.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** В меню выбрать игру с партнёром или программой. Клавиши выбора устанавливаются отдельно.

**Действие.** `right`. Правый игрок — стрелки, левый — F/Y/C/Q для влево, вправо, вверх, вниз.

**Наблюдение.** Меняется положение ракетки игрока.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная справка и меню 01FCh–0334h.

### Tetris-m.rks

- [std/Tetris-m.rks](std/Tetris-m.rks) — `RUN H:TETRIS-M.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `space`. После анимированной заставки и появления «ТАБЛИЦА РЕКОРДОВ» нажать пробел.

**Действие.** `right`. При видимой падающей фигуре нажать вправо.

**Наблюдение.** Падает игровая фигура. Сдвиг по столбцам должен возникать по стрелке, а не по таймеру падения.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Один запуск через RUN -MON в control-discovery/actions-Tetris-m подтвердил кнопку старта. Сдвиг требует отдельного сравнения.

### tetris.rks

- [std/tetris.rks](std/tetris.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `enter`. При DEMO или PRESS ENTER нажать Enter.

**Действие.** `space`. На первой только появившейся фигуре нажать пробел. Вправо/влево смещают, вверх поворачивает, вниз ускоряет падение.

**Наблюдение.** Пробел сбрасывает первую фигуру вниз. После укладки появляется следующая фигура. В двух контрольных прогонах без ввода первая фигура осталась в верхних рядах.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Английская подсказка в 2900h–2939h / 298Ah в std/tetris.rks. Парные прогоны RUN -MON подтвердили укладку первой фигуры и появление новой по Space. Другой работник воспроизвёл укладку фигуры с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Tetris1.rks

- [std/Tetris1.rks](std/Tetris1.rks) — `RUN H:TETRIS1.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `enter` → `3` → `enter`. На заставке нажать Enter. На «УРОВЕНЬ СЛОЖНОСТИ» выбрать 3, затем Enter.

**Действие.** `right` → `up`. На падающей фигуре нажать вправо и вверх.

**Наблюдение.** Фигура смещается на столбец и поворачивается, если позволяет поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная подсказка 02C0h и обработчики выбора уровня 034Eh–03B5h. Первый Enter проверяется в 0082h.

### Tetris2.rks

- [std/Tetris2.rks](std/Tetris2.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `5`. При «СКОРОСТЬ (0–9)?» нажать 5.

**Действие.** `right` → `up`. На падающей фигуре нажать вправо, затем вверх. Клавиша вниз ускоряет падение.

**Наблюдение.** Первый обработчик меняет столбец, второй — ориентацию фигуры.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Разбор std/Tetris2.rks, выбор скорости 0580h и коды стрелок 0224h–0235h.

### Tetris3.rks

- [std/Tetris3.rks](std/Tetris3.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/tetris_n.mg2](unknown/lafan/tetris_n.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** `a` → `enter` → `0` → `enter` → `n`. Ввести короткое имя, Enter, на «УРОВЕНЬ (0–F)?» ввести 0 и Enter. На «СМЕНИТЬ ПАРАМЕТРЫ (Y/N)?» нажать N.

**Действие.** `t` → `b`. На поле применить T для поворота и B для сдвига фигуры вправо. S приостанавливает, Q продолжает.

**Наблюдение.** Положение фигуры меняется по горизонтали и меняется её ориентация.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Интерфейс 24D0h–2540h. Другой работник с чистого старта воспроизвёл поворот и сдвиг у std/Tetris3.rks. В RUN -MON совпали кадры всех шагов и чтение контрольного файла. Повтор через `python scripts/games/replay-screen.py tetris3`. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Tetris4.rks

- [std/Tetris4.rks](std/Tetris4.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `5`. На запрос класса нажать 5.

**Действие.** `rpt`. На первой падающей фигуре нажать ПВ (MCP rpt) для поворота. Стрелки смещают фигуру. Enter приостанавливает падение и продолжает его.

**Наблюдение.** В парных RUN -MON код фигуры 60FDh меняется с 28h на 24h и 20h при поворотах через 63F0h. В покое код остаётся 28h.

**Выход.** `clear`. Клавиша гашения экрана (MCP clear) завершает игру.

**Основание.** Встроенная справка Tetris4 и парная проба действительного поворота. Две пары запусков совпали по игровой переменной и кадрам. Другой работник воспроизвёл поворот фигуры с чистого старта. Пути к протоколам обоих работников в playbook-independent-protected/workers.json.

### Tetris5.rks

- [std/Tetris5.rks](std/Tetris5.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `5` → `enter`. На «УСТАНОВИТЕ НАЧАЛЬНЫЙ УРОВЕНЬ (0–9)» выбрать 5 и подтвердить Enter.

**Действие.** `right`. На первом поле вправо двигает фигуру, ПВ (MCP rpt) поворачивает, пробел выполняет сброс.

**Наблюдение.** Стрелка вправо приводит к ветви 13EDh и сдвигает фигуру. Поворот и сброс требуют воспроизводимой игровой пробы.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Стартовый экран в Emu80 и входы 1330h–134Dh, диспетчер 11E7h–1222h, опрос ВВ55 по FFFCh.

### Tip-top.rks

- [std/Tip-top.rks](std/Tip-top.rks) — `RUN H:TIP-TOP.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [std/Tip-top2.rks](std/Tip-top2.rks) — `RUN H:TIP-TOP2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/tip_top.exe.bin](unknown/lafan/tip_top.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.
- [unknown/lafan/tip_top.exi](unknown/lafan/tip_top.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** `1`. На «ВАШ КЛАСС (1–5)?» выбрать 1.

**Действие.** Дождаться игрового поля и сверить управление именно этой редакции.

**Наблюдение.** После выбора класса открывается поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенный запрос 27D5h в вариантах с адресом 2000h и 052Ah в Tip-top2.

### Titan.rks

- [std/Titan.rks](std/Titan.rks) — `RUN H:TITAN.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/titan.exe.bin](unknown/lafan/titan.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** `1`. На «LEVEL 1–9» выбрать 1.

**Действие.** После появления поля проверить точную раскладку платформы.

**Наблюдение.** Появляется игровой уровень 1.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная строка 0947h и запрос уровня, наблюдавшийся в Emu80 RF.

### Treasure.rks

- [std/Treasure.rks](std/Treasure.rks) — `RUN H:TREASURE.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [std/Treasur2.rks](std/Treasur2.rks) — `RUN H:TREASUR2.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/klad_m.exe.bin](unknown/lafan/klad_m.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.
- [unknown/lafan/sokrowi.exi](unknown/lafan/sokrowi.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** `1`. После инструкции выбрать сложность 1 из 1–3.

**Действие.** `right`. На первом поле стрелкой переместить персонажа в ближайшую свободную клетку.

**Наблюдение.** Персонаж перемещается, при сборе сокровища меняется счёт.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная инструкция о стрелках и пункты сложности. Стартовые кадры Treasure/Treasur2 просмотрены в Emu80 MON.

### Tutankh.rks

- [std/Tutankh.rks](std/Tutankh.rks) — `RUN H:TUTANKH.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/tutank.cpu](mx/tutank.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/tutank.exi](unknown/lafan/tutank.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `1`. На «ENTER STARTING TOMB (1–5)?» нажать 1.

**Действие.** `right`. Когда герой появился в первой гробнице, удерживать стрелку вправо до шага. Стрелки задают направление по матрице ВВ55.

**Наблюдение.** В первом уровне по стрелке вправо байты 7D74h–7D75h меняются с 02 2A на 03 2A в std/Tutankh.rks.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Диспетчер 1015h–106Ah и проба `std/Tutankh.rks` RUN -MON в control-discovery/actions-Tutankh. У двух чистых RUN -MON совпали шаг и чтение файла после Reset. Для RF и Lafan отдельно сверены приглашения и платформенные вызовы. Повтор выполнен тем же работником с чистого старта.

### Udav.rks

- [std/Udav.rks](std/Udav.rks) — `RUN H:UDAV.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [mx/udav.cpu](mx/udav.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/udaw_cw1.exi](unknown/lafan/udaw_cw1.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.
- [unknown/lafan/udaw_cw2.exi](unknown/lafan/udaw_cw2.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.
- [unknown/lafan/udaw_cw3.exi](unknown/lafan/udaw_cw3.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** `5`. После запроса скорости выбрать 5. Дождаться начала движения удава.

**Действие.** `right`. Стрелкой изменить направление на доступном участке. При текущем направлении вправо сначала выберите поворот вверх или вниз.

**Наблюдение.** Изменяется направление автоматического движения удава. Смещение направления записывается по 0312h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Запрос скорости и ветвь стрелок 02D4h–030Ch. Платформенный опрос MX/RF меняет вход в монитор, коды направления сохраняются.

### V-perehv.rks

- [std/V-perehv.rks](std/V-perehv.rks) — `RUN H:V-PEREHV.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** Дождаться поля после заставки.

**Действие.** `right` → `space`. Стрелками навести орудие, пробел стреляет. ПВ и клавиша верхнего левого угла меняют скорость.

**Наблюдение.** Прицел сдвигается и появляется выстрел.

**Выход.** `m`. М выходит из игры.

**Основание.** Встроенная инструкция 017Ah–025Dh.

### Vampir.rks

- [std/Vampir.rks](std/Vampir.rks) — `RUN H:VAMPIR.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `2` → `5`. В VAMPIRLAND выбрать 2 для курсорных клавиш, затем 5 для старта.

**Действие.** `right`. На уровне стрелками перемещаться. P останавливает игру, M возвращает в меню.

**Наблюдение.** Спрайт персонажа меняет положение.

**Выход.** `m` → `6`. M возвращает в меню, пункт 6 — выход.

**Основание.** Встроенное меню 085Ah–089Fh и инструкция 0DACh.

### Vertolet.rks

- [std/Vertolet.rks](std/Vertolet.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** Историческая сборка с адресом 5000h печатает «МОНИТОР ?» и «ОШИБКА!» при пробе MON.

**Действие.** Стартовое условие проверки монитора следует выяснить перед пробой действий.

**Наблюдение.** Для сопоставления игры нужен проход проверки исходного монитора.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Наблюдение RUN -MON и строки проверки в историческом RKS.

### Vertolt2.rks

- [std/Vertolt2.rks](std/Vertolt2.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `space`. На «PRESS FIRE TO START GAME» нажать пробел.

**Действие.** После надписи GAME START определить управление на поле текущей редакции.

**Наблюдение.** Игра показывает GAME START и игровой экран.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашение 02B6h и экран перехода в Emu80 MON.

### WWAR.rks

- [std/WWAR.rks](std/WWAR.rks) — `RUN H:WWAR.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1` → `enter`. Дождаться «СЛОЖНОСТЬ». Выбрать 1–3 и подтвердить Enter.

**Действие.** Действие выстрела требует расшифровки матрицы. Enter по игровым ветвям возвращает к выбору новой партии.

**Наблюдение.** После принятия сложности меняются сцена и счётчики марсиан и землян.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Настройка 0070h–0085h, строка сложности 08F2h и ветви возврата 050Ch и 0710h.

### Xonix.rks

- [std/Xonix.rks](std/Xonix.rks) — `RUN H:XONIX.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `enter`. На инструкции нажать ВК и дождаться появления яиц и первого игрового опроса 0284h.

**Действие.** `up`. Удержать стрелку вверх, пока чертик стоит на нижней границе.

**Наблюдение.** Чертик оставляет видимую линию внутрь поля. Указатель 0319h меняется 8BD6h → 8B96h → 8B56h → 8B16h → 8A56h. В контролях он остаётся 8BD6h.

**Выход.** `clear`. СТР заканчивает игру.

**Основание.** Встроенная инструкция 066Ch и холодные парные пробы RUN -MON. Два других работника воспроизвели адресные значения, кадры и чтение H: после Reset. Команда `python scripts/games/check-controls-extra.py xonix --worker probe`.

## Игры из подборки MX

### 36stup.cpu

- [mx/36stup.cpu](mx/36stup.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).
- [unknown/lafan/ao_linx.exe.bin](unknown/lafan/ao_linx.exe.bin) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.
- [unknown/lafan/ao_linx.mg2](unknown/lafan/ao_linx.mg2) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MX`, `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** `1`. На заставке «36 СТУПЕНЕЙ ШАО-ЛИНЯ» нажать 1.

**Действие.** `right` → `up`. Вправо перемещает героя, вверх — прыжок, вниз — приседание. Я/Ч — прыжки в сторону (MCP q/caret).

**Наблюдение.** Спрайт смещается или поднимается в прыжке.

**Выход.** `enter`. ВК прекращает игру.

**Основание.** Встроенная инструкция 0E72h–0FF8h и кадр заставки Emu80.

### ADVENTUR.CPU

- [mx/ADVENTUR.CPU](mx/ADVENTUR.CPU) — `RUN H:ADVENTUR.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/games_rks/Adventur.rks](unknown/games_rks/Adventur.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (исходный контейнер).
- [unknown/lafan/adventur.exe.bin](unknown/lafan/adventur.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.
- [unknown/lafan/adventur.exi](unknown/lafan/adventur.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.

**Начало.** `1`. После инструкции при выборе класса 0–5 нажать 1.

**Действие.** После появления сцены определить управление по загруженной редакции. Цель — собрать три ключа и войти в дверь.

**Наблюдение.** После принятия класса должно появиться поле с ключами и дверью.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Экран запроса класса и цели, просмотренный в Emu80 MON. У всех нагрузок сохранён запрос класса в 1091h.

### barbar.cpu

- [mx/barbar.cpu](mx/barbar.cpu) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** Дождаться главного меню SELECT OPTION. Оно содержит START GAME, GAME KEYS и DEMO.

**Действие.** В GAME KEYS можно выбрать клавиатуру или джойстик и просмотреть FIRE, UP, DOWN, LEFT, RIGHT, PAUSE и QUIT.

**Наблюдение.** При выборе START GAME появляется игровой экран с двумя участниками. Для проверки клавиш нужно зафиксировать раскладку из GAME KEYS.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Таблица меню и названия действий 224Eh–22D0h. Соответствие цифр пунктам пока требует экрана или трассы.

### beat.cpu

- [mx/beat.cpu](mx/beat.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/beat_2.mg2](unknown/lafan/beat_2.mg2) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -RF`, `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** На заставке BEAT дождаться поля с платформой. Способ старта требует проверки для этой редакции.

**Действие.** Только после появления платформы проверить подачу мяча. На наблюдавшейся заставке Space, Right ещё не вызвали перехода.

**Наблюдение.** По принятой команде появляется мяч на поле, затем меняется его положение или счёт.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Снимки заставки и текст игровой нагрузки 089Ah.

### blob_cop.cpu

- [mx/blob_cop.cpu](mx/blob_cop.cpu) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [unknown/lafan/blob_cop.exe.bin](unknown/lafan/blob_cop.exe.bin) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** `1`. На «1: Cursor Key» выбрать курсорное управление. При «Press FIRE to start» нажать ранее указанную клавишу FIRE.

**Действие.** Ориентироваться по показанным назначениям. Клавиша FIRE в режиме курсора ещё требует проверки через MCP.

**Наблюдение.** После FIRE открывается игровое поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Текст настроек 807Fh–8186h. `space` используется только если экран текущей копии указал пробел как FIRE. В Lafan-копии сохранено меню выбора способа управления, адреса экранных строк могут отличаться.

### budi.cpu

- [mx/budi.cpu](mx/budi.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).

**Начало.** `space`. На 02A0h после заставки нажать пробел, дождаться первого уровня.

**Действие.** `right` → `q`. Вправо перемещает героя. Q копает слева, ^ справа. Над целым кирпичом по диагонали должна быть свободная клетка.

**Наблюдение.** Герой меняет клетку, при успешном копании меняется тайл кирпича.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Обработчики 02A0h и 03A1h–03B0h архивной MX-нагрузки BUDI.

### canball.cpu

- [mx/canball.cpu](mx/canball.cpu) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** `space`. Пробел на заставке Cannon Ball.

**Действие.** `right` → `space`. На поле нажать вправо, затем пробел. Для движения также работает влево.

**Наблюдение.** Положение платформы или траектория мяча меняются.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** mx/manuals/games_mx_new/canball.txt. Пуск пробелом виден на заставке STD-версии.

### ColorBall.cpu

- [mx/ColorBall.cpu](mx/ColorBall.cpu) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** Дождаться игровой сцены или следовать заставке загруженной версии.

**Действие.** `right` → `space`. Сыграть одно перемещение курсорной клавишей и действие пробелом.

**Наблюдение.** Перемещается управляемый объект. Действие зависит от состояния поля.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** mx/manuals/games_mx_new/clrball.txt

### destroy.cpu

- [mx/destroy.cpu](mx/destroy.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/destroye.mgk](unknown/lafan/destroye.mgk) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MGK -RF`, `RUN H:ARCHIVE.MGK -MON` для проверки исходного архива.

**Начало.** `2` → `0`. На меню DESTROYER клавиша 2 выбирает курсорные клавиши. 0 начинает игру.

**Действие.** `right`. На открывшемся поле нажать вправо, когда путь свободен.

**Наблюдение.** Игровой персонаж смещается вправо.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Меню ASV Corporation, просмотренное в Emu80 в версиях MX/RAMFOS и Lafan/Monitor-2.

### digger2.cpu

- [mx/digger2.cpu](mx/digger2.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).

**Начало.** `space`. Когда полностью отрисуется название DIGGER, нажать пробел и дождаться игрового цикла.

**Действие.** Клавиши землекопа в этой копии требуют проверки после стартовой перерисовки.

**Наблюдение.** Опрос принимает клавишу в 10C9h и открывается поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Титульный опрос 10C9h–10D6h. Архивный «Крот» и «Землекоп-1» имеют другие диспетчеры.

### driller.cpu

- [mx/driller.cpu](mx/driller.cpu) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** Дождаться игровой сцены или следовать заставке загруженной версии.

**Действие.** `right` → `space`. На уровне нажать вправо и пробел у доступной клетки.

**Наблюдение.** Танк сдвигается либо срабатывает игровое действие.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** mx/manuals/games_mx_new/driller.txt

### exolonmx.i80

- [mx/exolonmx.i80](mx/exolonmx.i80) — `RUN H:EXOLON.EXE -MX` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).

**Начало.** `1`. На меню Exolon нажать 1.

**Действие.** `right`. После появления стартовой площадки удерживать вправо на ровном участке. Короткий пробел стреляет, удержание пробела бросает гранату.

**Наблюдение.** В парных RUN -MX положение героя 2090h сменилось с (16,112) на (28,112) по стрелке. Прежняя пользовательская проверка гранаты зафиксировала изменение GRENADES с 10 на 09.

**Выход.** Клавиша штатного выхода не установлена.

**Основание.** Раздел Exolon в assets/games/README.md и исходная пользовательская проба стрельбы и гранаты в MX. Два работника в отдельных защищённых запусках RUN -MX воспроизвели шаг (16,112) → (28,112) после удержания вправо на 7557h. В контроле герой остаётся на (16,112). Пути к обоим протоколам в playbook-independent-protected/workers.json.

### farmer.cpu

- [mx/farmer.cpu](mx/farmer.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** Ферма открывается после загрузки. Виден счёт, время и год.

**Действие.** Игровое действие фермера ещё требует проверки по BASIC. Таймер и рост растений идут без клавиш.

**Наблюдение.** СТР прерывает BASIC и показывает «STOP IN 310» при ожидании игры.

**Выход.** `clear`. На первом игровом поле СТР останавливает программу и оставляет приглашение BASIC.

**Основание.** Экран RF и результат клавиши СТР в exit-observation для mx/farmer.cpu.

### FireRescue.cpu

- [mx/FireRescue.cpu](mx/FireRescue.cpu) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** Дождаться игровой сцены или следовать заставке загруженной версии.

**Действие.** `right` → `space`. На игровом экране нажать вправо и пробел.

**Наблюдение.** Управляемый объект реагирует на стрелку, пробел вызывает игровое действие.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** mx/manuals/games_mx_new/fire.txt

### FISH_CAT.CPU

- [mx/FISH_CAT.CPU](mx/FISH_CAT.CPU) — `RUN H:FISH_CAT.RKS -MON` на [H-Flash-2M-Games.txt](../H-Flash-2M-Games.txt).
- [unknown/lafan/fishkat.exe.bin](unknown/lafan/fishkat.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** Дождаться игрового поля после заставки Fisher Cat.

**Действие.** `right`. Нажать вправо на доступном участке, курсорные клавиши управляют фигурой.

**Наблюдение.** Положение управляемой фигуры меняется.

**Выход.** `space`. Пробел означает выход по справке.

**Основание.** Встроенная инструкция 06EFh–071Ah.

### hanoi.cpu

- [mx/hanoi.cpu](mx/hanoi.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Hanoi.rks](unknown/games_rks/Hanoi.rks) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (исходный контейнер).
- [unknown/lafan/ba-nq_h.com.bin](unknown/lafan/ba-nq_h.com.bin) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** `space` → `space` → `3`. На заставке с целью игры нажать пробел. Прервать демонстрацию вторым пробелом. На запрос количества колец выбрать 3 (доступны числа 3–7).

**Действие.** `up` → `right` → `down`. Стрелками влево и вправо выбрать стержень, вверх взять верхний диск, вниз положить его. На первом ходу взять верхний диск слева и положить на пустой средний стержень.

**Наблюдение.** В партии с 3 кольцами число колец слева меняется с 3 на 2, на среднем стержне с 0 на 1. Состояние стержней в 0A2Eh–0A30h, выбранный стержень в 0A31h.

**Выход.** `clear`. СТР при игре передаёт управление C800h.

**Основание.** Титул и старт 007Ah–0082h, диспетчер 00ECh–0108h, выбор стержня 019Ah и 01A8h, взятие 01B5h и укладка 01EFh. В Lafan-варианте тот же код выбора стержня и диска, отдельные адреса ПЗУ привязаны к RF. У архивной RKS-копии проверен ход после прерывания демо, снимки и регистры в control-discovery/actions-Hanoi-three. Второй чистый RUN -MON в control-discovery/actions-Hanoi-three-repeat дал такое же состояние стержней и чтение файла после Reset. Повтор выполнен тем же работником с чистого старта.

### kegli.cpu

- [mx/kegli.cpu](mx/kegli.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).
- [unknown/lafan/kegli.ex2](unknown/lafan/kegli.ex2) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EX2 -MX`, `RUN H:ARCHIVE.EX2 -MON` для проверки исходного архива.

**Начало.** `space`. При титуле «КЕГЛИ» нажать любую клавишу. Дождаться ряда шаров, счёта и пирамиды кеглей.

**Действие.** `space`. Когда игровой шар готов к броску, нажать пробел.

**Наблюдение.** Счётчик бросков по 045Bh увеличивается, шар движется к кеглям.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Титульный опрос 0534h–053Ch и обработчик броска 0656h–0665h. Совпадают титульный опрос и обработчик броска 0656h–0665h.

### lode_run.cpu

- [mx/lode_run.cpu](mx/lode_run.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/lode_run.com.bin](unknown/lafan/lode_run.com.bin) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** `2` → `0`. В LODE RUNNER GAME SELECTION сначала выбрать 2 (CURSOR), затем 0 START GAME.

**Действие.** `right`. На поле стрелкой направить героя в доступную клетку справа.

**Наблюдение.** Герой меняет клетку, счёт меняется при подборе предмета.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенное меню 0322h и раскладка 0AA0h, редакция RAMFOS.

### lodka.cpu

- [mx/lodka.cpu](mx/lodka.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** Дождаться, пока закончится синяя заставка и появится игровой объект в воде.

**Действие.** `right`. На свободном участке удержать стрелку вправо. Вправо выбирает код 02h, влево 01h.

**Наблюдение.** Исполняется ветвь горизонтального движения 05C8h, позиция обновляется по 0125h и 0128h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Строки ВВ55 053Ch–0576h и обработка результата 05A8h–05D5h. В RF-пробах показана начальная анимация.

### magie_.cpu

- [mx/magie_.cpu](mx/magie_.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** `2`. На выборе цветного или монохромного режима нажать 2 (монохром).

**Действие.** Клавиши уровня требуют отдельной проверки. У цветного варианта указано использование страницы RAM-диска.

**Наблюдение.** Программа принимает режим и открывает следующий экран.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенное приглашение 8934h–89BFh, просмотренное в Emu80 RF.

### mole.cpu

- [mx/mole.cpu](mx/mole.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** Дождаться заставки MOLE. Способ перехода на игровой экран требует проверки.

**Действие.** `right`. Если показано поле, нажать стрелку вправо в открытом коридоре.

**Наблюдение.** Персонаж смещается по клеткам.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Инструкция о движении курсором в 2B10h и заставка в RF.

### mouses.cpu

- [mx/mouses.cpu](mx/mouses.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/my-i_let.mg2](unknown/lafan/my-i_let.mg2) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -RF`, `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** Дождаться окончания экранной надписи «ЛЕТУЧИЕ МЫШИ» и появления игровой сцены.

**Действие.** `right`. Вправо выбирает код 02h в обработчике, влево 01h. Пробовать на открытом участке справа.

**Наблюдение.** После выбора направления меняется позиция 0125h и 0128h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Матричный опрос 053Ch–0576h, ветвь позиции 05B9h–05D5h. Тело Lafan отличается платформенными байтами и длиной.

### pacman2.cpu

- [mx/pacman2.cpu](mx/pacman2.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** Дождаться отрисовки лабиринта, героя и счётчиков. Если видна таблица после окончания партии, нажать любую клавишу для повтора.

**Действие.** `right`. Стрелки задают направление. Сначала выбрать свободный проход. Направление фиксируется до следующей допустимой команды.

**Наблюдение.** Меняется направление движения героя и положение в лабиринте. В 0C45h хранится принятое направление, курсор вправо даёт 18h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Платформенный опрос F81Bh и игровой диспетчер 00CDh–0110h.

### pikfama.cpu

- [mx/pikfama.cpu](mx/pikfama.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** `f4`. Из экрана справки F2 по надписи вызывает игру. В RUN -RF на матрице Original соответствующий код 01h даёт логическая клавиша F4.

**Действие.** После приглашения «Ваше число» ответить в формате из справки. Формат и длина ответа требуют проверки.

**Наблюдение.** После принятой попытки появляются показатели «Пика» и «Фама».

**Выход.** `f6`. Из экрана справки F6 в RUN -RF передаёт код 03h, выполняет 0446h и возвращает в NC через F800h. В историческом RAMFOS на той же позиции матрицы действует F4.

**Основание.** Встроенные подписи 0465h–04FCh и по два холодных старта на среду с трассой F81Bh. Команда `python scripts/games/check-rf-exit.py --case pikfama --match-matrix --port 19796`.

### poker.cpu

- [mx/poker.cpu](mx/poker.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** `f5`. В RUN -RF на матрице Original F5 начинает игру с кодом 02h. На заставке и в историческом RAMFOS эта позиция обозначена F3.

**Действие.** `1` → `enter`. На запросе ставки нажать 1 (одна монета) и Enter, затем выбрать карты цифрами 1–5 по правилам.

**Наблюдение.** Появляется поле с картами и запросом ставки. Принятие ставки по этой последовательности требует отдельной проверки.

**Выход.** `f6`. На запросе ставки F6 в RUN -RF (F4 в архивном RAMFOS) передаёт код 03h и выходит через 2014h → F800h.

**Основание.** Правила 0A45h–0DCEh и по два холодных старта на среду с трассой F81Bh. Команда `python scripts/games/check-rf-exit.py --case poker --match-matrix --port 19796`.

### rewersi.cpu

- [mx/rewersi.cpu](mx/rewersi.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/rewersi.com.bin](unknown/lafan/rewersi.com.bin) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** `1`. На «УКАЖИТЕ УРОВЕНЬ ИГРЫ (1…4)» выбрать 1.

**Действие.** Формат координат для хода требует проверки.

**Наблюдение.** После выбора уровня появляется доска.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашение в 1530h обеих нагрузок.

### sex0.cpu

- [mx/sex0.cpu](mx/sex0.cpu) — `RUN H:PROBE.EXE -MX`, `RUN H:PROBE.EXE -MON`, `RUN H:PROBE.EXE -RF` (проверочный контейнер с игровым телом).
- [mx/sex1.cpu](mx/sex1.cpu) — `RUN H:PROBE.EXE -MX`, `RUN H:PROBE.EXE -MON`, `RUN H:PROBE.EXE -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/sex1.com.bin](unknown/lafan/sex1.com.bin) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.
- [unknown/lafan/sex2.com.bin](unknown/lafan/sex2.com.bin) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.
- [unknown/lafan/sex_1.mg2](unknown/lafan/sex_1.mg2) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MX`, `RUN H:ARCHIVE.MG2 -MON`, `RUN H:ARCHIVE.MG2 -RF` для проверки исходного архива.

**Начало.** Дождаться первого кадра демонстрации. У CPU-пары точка входа 6200h, у монитора Lafan может использоваться отдельный запускатор.

**Действие.** `space`. Пробел на завершённом кадре перелистывает иллюстрацию.

**Наблюдение.** При принятом вводе срабатывают счётчик кадров и следующая отрисовка.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Диспетчер кадра 000Fh–001Ch и опрос C81Bh в 0013h. MX и Lafan отличаются начальной настройкой экрана.

### shahm_mx.cpu

- [mx/shahm_mx.cpu](mx/shahm_mx.cpu) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** `0`. При «Уровень (0–2)?» выбрать 0.

**Действие.** После выбора уровня дождаться хода человека. Клавиши этой MX-копии требуется проверить отдельно.

**Наблюдение.** Отображается начальная позиция и приглашение хода.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашение по 113Eh в MX-нагрузке.

### shahmat.cpu

- [mx/shahmat.cpu](mx/shahmat.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Chess.rks](unknown/games_rks/Chess.rks) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/ahmaty3.exi](unknown/lafan/ahmaty3.exi) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Дождаться шахматной доски и текстового «ХОД ?». Поле открывается автоматически.

**Действие.** Программа разбирает ввод своего формата через C337h, начальный символ от 1 до 9. Обозначение полной пары координат нужно проверить отдельно.

**Наблюдение.** Появление доски и «ХОД ?» устанавливает готовность. Корректный ход должен изменить позицию фигуры.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Ввод 0E03h–0E2Ch и строка «ХОД ?» в 0FC3h. Общие блоки доски и ввода совпадают у трёх нагрузок.

### shashki.cpu

- [mx/shashki.cpu](mx/shashki.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом).

**Начало.** `1` → `0`. На запрос времени обдумывания нажать 1, при запросе форы 0.

**Действие.** На приглашении хода ввести координаты исходной и целевой клетки. Точный формат MX-копии требует проверки.

**Наблюдение.** Экран передаёт ход человеку.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Стартовые приглашения и доска в пробе MX. Вариант std/Checkers.rks проверен отдельно.

### sup_tan_.cpu

- [mx/sup_tan_.cpu](mx/sup_tan_.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** `4` → `1`. В меню SUPERTANK выбрать 4 для курсора, затем 1 PLAY. Доступен отдельный выбор уровня 2.

**Действие.** `right`. На поле стрелкой сместить игровой танк или прицел.

**Наблюдение.** На ввод меняется положение танка либо прицела.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенное меню 0430h–0490h.

### tetris_1.cpu

- [mx/tetris_1.cpu](mx/tetris_1.cpu) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** `space` → `1`. На «НАЖМИТЕ ЛЮБУЮ КЛАВИШУ» нажать пробел. На «ВАШ УРОВЕНЬ (0–7)?» выбрать 1.

**Действие.** Клавиши сдвига и поворота уточняются по напечатанным символам на игровом экране.

**Наблюдение.** После выбора уровня открывается стакан.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенные подсказки 0550h–05C0h.

### tetris_d.cpu

- [mx/tetris_d.cpu](mx/tetris_d.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** `1`. На запросе уровня 1–9 выбрать 1.

**Действие.** `equals` → `0` → `4`. Равно смещает фигуру вправо, 0 поворачивает, 4 сбрасывает. Влево — 9.

**Наблюдение.** Фигура меняет позицию и ориентацию, затем закрепляется на дне.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Подсказки точной нагрузки mx/tetris_d.cpu, просмотренные в Emu80 MON.

### tetris_s.cpu

- [mx/tetris_s.cpu](mx/tetris_s.cpu) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [unknown/lafan/muztetr1.exi](unknown/lafan/muztetr1.exi) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.
- [unknown/lafan/muztetr2.exi](unknown/lafan/muztetr2.exi) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.
- [unknown/lafan/tetris_s.com.bin](unknown/lafan/tetris_s.com.bin) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.
- [unknown/lafan/tetris2.exe.bin](unknown/lafan/tetris2.exe.bin) — `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.

**Начало.** `5`. На «СКОРОСТЬ (0–9)?» выбрать 5.

**Действие.** После появления фигур проверить совпадение клавиш с Tetris2.rks, включая стрелки. Различаются платформенные обработчики.

**Наблюдение.** После принятой скорости появляется поле. Экран заставки может идти дольше ранних кадров.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашение скорости около 044Bh–0450h в каждой нагрузке.

### vavilon.cpu

- [mx/vavilon.cpu](mx/vavilon.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** В меню «ВАВИЛОНСКАЯ БАШНЯ» доступны «ИГРА», назначение клавиш, уровень и настройки цвета.

**Действие.** Выбрать «ОПРЕДЕЛЕНИЕ КЛАВИШ» для получения точной раскладки. Клавиша подтверждения меню требует проверки.

**Наблюдение.** После выбора игры появляется поле. Назначенные клавиши можно проверить на нём.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Пункты меню и строки назначения клавиш в 0A30h–0ABAh, 1092h–118Ch.

### xonix.cpu

- [mx/xonix.cpu](mx/xonix.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).
- [unknown/games_rks/Xonix.rks](unknown/games_rks/Xonix.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (исходный контейнер).

**Начало.** `enter`. На инструкции ВК начинает игру.

**Действие.** `down`. При появлении игрового поля стрелкой переместить чертика с границы внутрь, когда путь свободен.

**Наблюдение.** Появляется проведённая игроком линия. При соединении двух границ отсекается участок.

**Выход.** `clear`. СТР заканчивает игру.

**Основание.** Встроенная инструкция по адресу 066Ch, просмотренная в Emu80 MON. Инструкция 0692h–06CCh повторяется в этих вариантах.

### zoo_.cpu

- [mx/zoo_.cpu](mx/zoo_.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).
- [families/zoo/originals/Zoo-lik-game.rks.bin](families/zoo/originals/Zoo-lik-game.rks.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -RF`, `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** `space`. В меню H показывает настройку управления, пробел открывает поле.

**Действие.** `right`. На проходимом участке нажать вправо, Q или S расчищают соседнюю клетку.

**Наблюдение.** Персонаж меняет клетку. В LIK-редакции нужен её монитор. В текущей кампании проверяется переупакованная нагрузка.

**Выход.** `e`. В меню E означает выход.

**Основание.** Встроенная справка в 0956h–0ADEh и families/zoo/README.md.

## Остальные архивные варианты

### a-ki.exi

- [unknown/lafan/a-ki.exi](unknown/lafan/a-ki.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.
- [unknown/lafan/a-ki.exe.bin](unknown/lafan/a-ki.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** `1` → `0`. На запрос времени обдумывания нажать 1, на запрос форы 0.

**Действие.** После появления запроса своего хода использовать координаты доски. Формат хода требуется подтвердить.

**Наблюдение.** После принятия параметров видна доска и приглашение хода.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Игровые участки 0005h–1000h близки к mx/shashki.cpu, совпадают приглашения и диспетчер настройки.

### ahmaty.exe.bin

- [unknown/lafan/ahmaty.exe.bin](unknown/lafan/ahmaty.exe.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.
- [unknown/lafan/ahmaty2.exe.bin](unknown/lafan/ahmaty2.exe.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** `0`. На приглашении уровня (0–2) выбрать 0.

**Действие.** Стрелками выбрать клетку, ВК подтверждает позиции. АР2 отменяет ход.

**Наблюдение.** После двух принятых позиций белая фигура перемещается.

**Выход.** `clear`. СТР передаёт управление RAMFOS.

**Основание.** Встроенные справки 1502h–15AFh обеих нагрузок.

### ahmaty1.exi

- [unknown/lafan/ahmaty1.exi](unknown/lafan/ahmaty1.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.
- [unknown/lafan/ahmaty2.exi](unknown/lafan/ahmaty2.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Встроенное меню показывает «1 — НАЧАЛО, 2 — СБРОС, 3 — РОК.Л., 4 — РОК.П.». Дождаться полной отрисовки.

**Действие.** Точную последовательность выбора исходной и целевой клеток нужно подтвердить.

**Наблюдение.** Появляется доска новой партии.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Строки токенизированных программ (3936h и 38CAh).

### almaz_2.com.bin

- [unknown/lafan/almaz_2.com.bin](unknown/lafan/almaz_2.com.bin) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** После вступления дождаться игрока в нижней строке.

**Действие.** `right`. В первом игровом цикле удержать вправо до смены клетки.

**Наблюдение.** Тип игрока 09h перемещается вправо из 81C1h.

**Выход.** `clear`. На игровом поле удерживать СТР до передачи управления F800h.

**Основание.** Начальная карта и игровой код совпадают с архивным Алмазом-2 для RAMFOS. В области 0000h–3400h вне начального перехода отличаются 0886h–0888h.

### almaz_2.exi

- [unknown/lafan/almaz_2.exi](unknown/lafan/almaz_2.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `9`. После приглашения «СКОРОСТЬ(0/9)» выбрать медленный темп 9. Первая попытка может открыться автоматически.

**Действие.** После появления карты проверить направление по диспетчеру текущей редакции.

**Наблюдение.** Приглашение исчезает, появляется поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашение скорости по 29C7h. Загрузка с 1F00h.

### b.tetris.exe.bin

- [unknown/lafan/b.tetris.exe.bin](unknown/lafan/b.tetris.exe.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.
- [unknown/lafan/b.tetris.exi](unknown/lafan/b.tetris.exi) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.
- [unknown/lafan/tetris3.exe.bin](unknown/lafan/tetris3.exe.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** Дождаться «УРОВЕНЬ СЛОЖНОСТИ», выбрать 3, затем Enter при запросе.

**Действие.** `right`. Вправо смещает падающую фигуру. Начальный вход из титульной заставки требует проверки для каждого контейнера.

**Наблюдение.** После принятого уровня фигура появляется в стакане и сдвигается вправо.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенные строки 02A9h–02F0h фонда Tetris1.

### badi.exi

- [unknown/lafan/badi.exi](unknown/lafan/badi.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.
- [unknown/lafan/badi.exe.bin](unknown/lafan/badi.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** После заставки дождаться видимого игрока и кирпичного поля.

**Действие.** `right` → `space`. На проходимой клетке нажать вправо. Рядом со свободной клеткой над целым кирпичом пробел копает по диагонали вниз слева, Enter справа.

**Наблюдение.** Игрок перемещается либо кирпич сменяется ямой.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** У обеих Lafan-нагрузок сравнения Space, CR и кодов стрелок в диспетчере 03A1h. Начальная раскладка требует просмотра каждой карты.

### balbes.mg2

- [unknown/lafan/balbes.mg2](unknown/lafan/balbes.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** Дождаться поля из кирпичей, травы и объектов.

**Действие.** `right`. Стрелкой вправо выбрать шаг на свободный тайл.

**Наблюдение.** Игровой персонаж или курсор меняет клетку. СТР на отдельном приглашении использует ветвь выхода.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Опрос стрелок 03CBh–03EDh и диспетчер ВК/СТР 0901h–0909h. Код этого блока совпадает с дополнительной частью архивного budi-2.exi.

### beat_3.com.bin

- [unknown/lafan/beat_3.com.bin](unknown/lafan/beat_3.com.bin) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.
- [unknown/lafan/beat2.com.bin](unknown/lafan/beat2.com.bin) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.

**Начало.** Дождаться платы с платформой и неподвижным мячом.

**Действие.** `space` → `right`. Когда мяч находится на платформе, нажать пробел и затем вправо.

**Наблюдение.** Мяч отделяется от платформы, разбитый блок исчезает и счёт меняется.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Кадры покоя и движения в MX после пробы Space, Right у beat_3.com.bin и beat2.com.bin. Точное разделение двух клавиш требует повторного прогона.

### Boulder.rks

- [unknown/games_rks/Boulder.rks](unknown/games_rks/Boulder.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `5`. На «СКОРОСТЬ (0/9)» выбрать 5.

**Действие.** Клавиша движения уточняется по загруженному варианту Boulder.

**Наблюдение.** После принятой цифры появляется поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная строка 29C7h.

### boulder1.com.bin

- [unknown/lafan/boulder1.com.bin](unknown/lafan/boulder1.com.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.

**Начало.** В меню выбрать этаж. Точная клавиша запуска требует подтверждения.

**Действие.** `right`. На игровом поле стрелкой сместиться при наличии свободной клетки.

**Наблюдение.** Игрок смещается по карте.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашение этажа 6DE4h и встроенная надпись о курсорных клавишах 6D92h.

### boulder2.com.bin

- [unknown/lafan/boulder2.com.bin](unknown/lafan/boulder2.com.bin) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** `5`. На «СКОРОСТЬ (0/9)» выбрать 5.

**Действие.** После появления первого поля выяснить команды движения. Проба стартового нажатия вызвала отрисовку.

**Наблюдение.** Отобразится игровая карта. Поворот или перемещение требует отдельного действия.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Экран в RF и приглашение в 29C7h.

### budi-2.exi

- [unknown/lafan/budi-2.exi](unknown/lafan/budi-2.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `space`. Дождаться заставки BUDI до ожидания 02A0h и нажать пробел.

**Действие.** `right` → `space`. На первом поле идёт проверка стрелок. Пробел копает слева, Enter справа. Кирпич должен быть ниже свободной клетки сбоку от героя.

**Наблюдение.** Герой меняет клетку и на подходящем участке появляется яма.

**Выход.** `clear`. На игровом поле СТР передаёт управление монитору.

**Основание.** Начальный опрос 02A0h и игровой диспетчер 03A1h–03C7h. Дополнительные ветви после основного уровня отличаются от архивного BUDI2 STD.

### budi_new.exe.bin

- [unknown/lafan/budi_new.exe.bin](unknown/lafan/budi_new.exe.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.
- [unknown/lafan/budi_new.mg2](unknown/lafan/budi_new.mg2) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -RF`, `RUN H:ARCHIVE.MG2 -MX`, `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.
- [unknown/lafan/budi.com.bin](unknown/lafan/budi.com.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX`, `RUN H:ARCHIVE.COM -MON` для проверки исходного архива.

**Начало.** `enter`. После открытия справки ВК возвращает в игру. Дождаться поля после заставки BUDI NEW.

**Действие.** `right` → `q`. На поле вправо — шаг, Q копает слева, ^ справа (MCP caret). СТР выбирает следующее поле, пробел повторяет текущее, F ставит на паузу, K вызывает справку.

**Наблюдение.** Игрок перемещается, при копании на подходящей клетке видна новая яма. Редактор E требует отдельной пробы.

**Выход.** `enter`. ВК указан для выхода из игры. Из редактора или справки ВК возвращает в игру.

**Основание.** Экран управления в Emu80 и байтовая строка Q/^ в 73BAh–73E5h.

### chess_ek.ex2

- [unknown/lafan/chess_ek.ex2](unknown/lafan/chess_ek.ex2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EX2 -MON` для проверки исходного архива.

**Начало.** Подготовленная RKS начинает с предполагаемого адреса 8000h из каталога ODI. Первые восемь байт нулевые, далее таблица адресов.

**Действие.** Для шахматного хода требуется установить вход, использовавшийся родным загрузчиком, и открыть доску.

**Наблюдение.** Вход к экрану игры из переданного диапазона ещё не подтверждён.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Диск Lafan и начальные 8000h–8012h. Обработчики ввода находятся среди подпрограмм 8803h и 8A45h.

### chess_ri.ex2

- [unknown/lafan/chess_ri.ex2](unknown/lafan/chess_ri.ex2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EX2 -MON` для проверки исходного архива.

**Начало.** `0`. На «УРОВЕНЬ 0,1» выбрать 0.

**Действие.** Дождаться запроса хода. Способ передачи хода этой редакции требует проверки.

**Наблюдение.** Появляется игра выбранного уровня.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенное приглашение по 121Ch и 6553h.

### comty_60.exi

- [unknown/lafan/comty_60.exi](unknown/lafan/comty_60.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Дождаться спрайта вертолёта и счётчика после заставки.

**Действие.** `right` → `space`. Стрелка двигает вертолёт, пробел сбрасывает бомбу.

**Наблюдение.** Бомба видимо отделяется от вертолёта.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Тело по смещению 0000h–104Fh побайтно совпадает с телом std/SpKing2.rks после блока инструкции 08D0h.

### destroye.exe.bin

- [unknown/lafan/destroye.exe.bin](unknown/lafan/destroye.exe.bin) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** В пробе переупакованного тела RUN -MX достиг точки входа 1200h. Видимый экран остался на заставке «Специалист MX2».

**Действие.** Перед игровой пробой требуется пройти инициализацию. Для настроек в самом теле есть ветви цифровых клавиш 0–3.

**Наблюдение.** После инициализации ожидается меню Destroyer. Изменение поля по клавише ещё требует наблюдения.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Строка NC, проверенная нагрузка на входе и диспетчер 1990h–19C8h. Поведение mx/destroy.cpu относится к другой нагрузке.

### dungeon.exe.bin

- [unknown/lafan/dungeon.exe.bin](unknown/lafan/dungeon.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** Дождаться окончания вступления.

**Действие.** `space`. Пробел указан в инструкции как игровая команда, ПС продолжает после паузы. Направления в текстовой строке заменены управляющими кодами.

**Наблюдение.** Действие пробела и конкретные движения нужно подтвердить на уровне.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная инструкция 3691h–36C5h.

### dungeon.exi

- [unknown/lafan/dungeon.exi](unknown/lafan/dungeon.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Дождаться окончания вступления.

**Действие.** В строке управления перечислены I,T,X,B,@. S — пауза, Q — продолжение.

**Наблюдение.** Смена игрового состояния после принятой команды требует проверки. Исходная сумма файла расходится с каталогом ODI.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенная инструкция 3691h и блок расхождений исходников в протоколе кампании.

### FishCat.rks

- [unknown/games_rks/FishCat.rks](unknown/games_rks/FishCat.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `space`. После заставки Fisher Cat дождаться экрана, где выполняется блокирующий ввод, и нажать пробел.

**Действие.** По переходу к игровому полю определить конкретную раскладку выпуска 22.rks.

**Наблюдение.** Нажатие на стадии инструкции вызывает переход в 1200h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Наблюдение заставки и `C337h` в 25F3h с переходом к 1200h. Позиции игровых обработчиков отличаются от mx/FISH_CAT.CPU.

### hokkej.mg2

- [unknown/lafan/hokkej.mg2](unknown/lafan/hokkej.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.
- [unknown/lafan/hokkej.exe.bin](unknown/lafan/hokkej.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.
- [unknown/games_lik_rks/hockey.rks.bin](unknown/games_lik_rks/hockey.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** После отрисовки катка наблюдать шайбу и позиции игроков.

**Действие.** Клавиши обоих игроков ещё требуется подтвердить через матричный опрос игры.

**Наблюдение.** СТР передаёт управление C800h. После завершения попытки можно нажать пробел для новой игры.

**Выход.** `clear`. Во время матча или после результата нажать СТР.

**Основание.** Ветки выхода и повтора 0627h–0632h и 06C4h–06D1h в обеих нагрузках. У LIK-копии совпадают сравнения Space и СТР, изменены платформенные адреса.

### jump.mg2

- [unknown/lafan/jump.mg2](unknown/lafan/jump.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** `space`. При открытии заставки дождаться запроса любой клавиши и нажать пробел.

**Действие.** Игровые действия считываются через ВВ55. Точная клавиша прыжка требует парной пробы.

**Наблюдение.** После подтверждения открывается уровень со счётчиком LEVEL.

**Выход.** `n`. На «WILL YOU TRY AGAIN?» выбрать N для выхода в C800h.

**Основание.** Вход 401Eh и ветвь «GAME OVER WILL YOU TRY AGAIN?» на 41F7h–4205h.

### kama_sut.mg2

- [unknown/lafan/kama_sut.mg2](unknown/lafan/kama_sut.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** Дождаться вывода встроенной демонстрационной программы после её заставки.

**Действие.** Операции относятся к просмотру иллюстраций. Клавиша перелистывания требует привязки к сцене после загрузки.

**Наблюдение.** В MON подтверждены точка входа и кадры вывода. Игровой сценарий не установлен.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Архивный формат MG2, вход 1A20h из загрузки и общие BASIC-ветви ввода. По ним нельзя определить клавиши конкретного текста.

### karate.rks.bin

- [unknown/games_lik_rks/karate.rks.bin](unknown/games_lik_rks/karate.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** `0`. В меню выбрать 0 для «DEMONSTRATION GAME».

**Действие.** Для управления участником требуется клавиша игрового режима, показанная на меню конкретной копии.

**Наблюдение.** Запускается демонстрация боя.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенный список меню 0895h–08E1h.

### klad-2.rks.bin

- [unknown/games_lik_rks/klad-2.rks.bin](unknown/games_lik_rks/klad-2.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** Дождаться окончания первого кадра архива LIK.

**Действие.** Редакцию поля следует сверить отдельно. Назначение клавиш из стандартной Klad2.rks здесь требует независимого подтверждения.

**Наблюдение.** Для подготовки пробы требуется игровое поле после вступления.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Сравнение нагрузки LIK с Klad2.rks и Riseout.rks. Общие блоки с Riseout не покрывают весь исполняемый код.

### kosm.boj.exi

- [unknown/lafan/kosm.boj.exi](unknown/lafan/kosm.boj.exi) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `0` → `1`. На запрос инструкции ответить 0, затем выбрать сложность 1.

**Действие.** `up`. После появления прицела нажать вверх для пуска ракеты. Дождаться принятия команды в игровом цикле.

**Наблюдение.** Пуск должен увеличить 8808h через ветвь 0431h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Коды клавиш и ветви запуска в 03FAh–0437h совпадают с Buran-starwars-std.rks. Сам архив проверяется через переупакованный контейнер.

### kosmboj.rks.bin

- [unknown/games_lik_rks/kosmboj.rks.bin](unknown/games_lik_rks/kosmboj.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** В начале переупакованного тела установлен вход 0000h. Переход ведёт на 1A20h во встроенный запускатор BASIC.

**Действие.** Дождаться приглашений этой редакции «Космического боя». Выбор 0/1 из Бурана требует отдельного доказательства здесь.

**Наблюдение.** Первый игровой экран должен показать режим и прицел либо текстовый запрос.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Заголовок LIK, переход с 0003h и общий BASIC-ввод 0F75h. Игрового диспетчера 03FBh–0438h БУРАН/SW в этой нагрузке нет.

### kres_nol.mg2

- [unknown/lafan/kres_nol.mg2](unknown/lafan/kres_nol.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.
- [unknown/lafan/crosser.exe.bin](unknown/lafan/crosser.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** `y`. При «ХОЧЕШЬ ХОДИТЬ ПЕРВЫМ? Y/N» нажать Y.

**Действие.** `7` → `7`. После выбора первого хода ввести две шестнадцатеричные координаты свободной клетки. Для центральной клетки нажать 7 дважды.

**Наблюдение.** На поле 16×16 появляется знак в выбранной клетке. Следом компьютер делает свой ход.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Заставка CROSSER и ветви 02A5h–02C4h, 0558h–0576h. В обеих историях игровых нагрузок меняется первый переход, совпадает обработка координат.

### KrestikiNoliki.rks

- [unknown/games_rks/KrestikiNoliki.rks](unknown/games_rks/KrestikiNoliki.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** Когда видна пустая доска и игра ждёт первый ход, выбрать координаты.

**Действие.** `a` → `1`. Ввести ряд A, B/W или C/S и столбец 1–3. Для верхней левой клетки нажать A, затем 1.

**Наблюдение.** В верхней левой клетке появляется знак человека, затем игра отвечает своим ходом.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Обработчики строки 008Ch–00A8h и столбца 00A9h–00C4h. В чистом RUN -MON ввод A1 записал код 6Fh в 0011h. Артефакт control-discovery/actions-KrestikiNoliki-repeat.

### kriminal.exi

- [unknown/lafan/kriminal.exi](unknown/lafan/kriminal.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `space`. На приглашениях «PRESS ANY KEY...» нажимать пробел с паузой для чтения следующего экрана.

**Действие.** Дождаться последнего экрана вступления. Клавиши игровой сцены требуют отдельной проверки.

**Наблюдение.** Вступительный текст перелистывается.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Повторяющиеся текстовые запросы начиная с 0207h, 02DFh и 03BFh.

### labir_rk.exi

- [unknown/lafan/labir_rk.exi](unknown/lafan/labir_rk.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.
- [unknown/lafan/labirint.exe.bin](unknown/lafan/labirint.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** `1`. При вопросе RANGE выбрать 1, если он появляется в этой редакции.

**Действие.** Управление игроком уточняется по рабочей сцене. Наименование «ЛАБИРИНТ» недостаточно для переноса карты клавиш.

**Наблюдение.** Виден лабиринт после старта.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Строка END GAME в 180Eh и сходство интерфейса с Pacman2.rks.

### labirint.mg2

- [unknown/lafan/labirint.mg2](unknown/lafan/labirint.mg2) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON`, `RUN H:ARCHIVE.MG2 -MX` для проверки исходного архива.
- [mx/labirint.cpu](mx/labirint.cpu) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом).

**Начало.** В редакторе лабиринтов дождаться рабочей сетки.

**Действие.** `right` → `1`. Стрелкой переместить курсор, 1 ставит белую точку, 2 — чёрную. N очищает поле, плюс/минус переключают карту.

**Наблюдение.** После перемещения и нажатия 1 в выбранной клетке появляется белая точка.

**Выход.** `clear`. СТР — выход.

**Основание.** Справка редактора в 0C7Dh–0D30h / 0CD6h–0D79h.

### Land-with-editor.rks

- [unknown/games_rks/Land-with-editor.rks](unknown/games_rks/Land-with-editor.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** Отдельно определить вход в игру и встроенный редактор.

**Действие.** В редакторе пробел сохраняет текущий лабиринт для дальнейшей работы, СТР стирает. Перед пробой зафиксировать исходное поле.

**Наблюдение.** После подтверждённого стирания состояние редактируемой карты меняется.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Справка редактора в 4170h–41AFh.

### land.com.bin

- [unknown/lafan/land.com.bin](unknown/lafan/land.com.bin) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.
- [unknown/lafan/land.exi](unknown/lafan/land.exi) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `1`. При появлении «YOUR RANGE» выбрать 1 и дождаться первой карты.

**Действие.** `right`. Стрелки перемещают героя. Q и ^ копают слева и справа на подходящем участке.

**Наблюдение.** Герой сдвигается на свободную клетку или изменяется соседний кирпич.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Lafan-варианты повторяют игровой диспетчер Land в 05DFh–061Eh с проверкой других кодов регистра Q и ^. Вызовы ПЗУ относятся к RAMFOS.

### land_s.mg2

- [unknown/lafan/land_s.mg2](unknown/lafan/land_s.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** `space`. На «НАЖМИТЕ ЛЮБУЮ КЛАВИШУ» нажать пробел.

**Действие.** `right`. Здесь указаны коды плиток и курсорное управление. На рабочей сетке нажать вправо.

**Наблюдение.** Курсор в рабочей сетке сдвигается в соседнюю клетку.

**Выход.** `m`. M выходит в монитор.

**Основание.** Встроенная инструкция 830Bh–83F0h.

### lestn_rk.exi

- [unknown/lafan/lestn_rk.exi](unknown/lafan/lestn_rk.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.
- [unknown/lafan/lestnica.exe.bin](unknown/lafan/lestnica.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** `p`. P начинает игру, в меню E обозначает выход.

**Действие.** `right` → `space`. Стрелками перемещать героя, прыжок — пробел.

**Наблюдение.** Игрок меняет клетку и/или выполняет прыжок.

**Выход.** `e`. Из меню E передаёт управление монитору.

**Основание.** Встроенная справка 0A9Dh, 0B40h и 0C07h–0C35h.

### Lyagushka.rks

- [unknown/games_rks/Lyagushka.rks](unknown/games_rks/Lyagushka.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/lqgu-ki.exe.bin](unknown/lafan/lqgu-ki.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** Дождаться первого поля и появления лягушки.

**Действие.** `up`. Стрелками выбирать направление. Для пробы удержать вверх на участке без препятствий.

**Наблюдение.** В координате игрока 06C4h смещается вертикальная составляющая при доступном шаге.

**Выход.** `m`. После окончания партии на текстовом запросе M передаёт управление C000h.

**Основание.** Диспетчер 02D4h–02F1h и выход после вопроса по M (4Dh/6Dh) в 0439h–0443h.

### Maestro.rks

- [unknown/games_rks/Maestro.rks](unknown/games_rks/Maestro.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** Дождаться экранного приглашения выбора режима.

**Действие.** На запросе выхода из просмотра нажать любую клавишу. E в меню завершает режим.

**Наблюдение.** Для E меню закрывается.

**Выход.** `e`. В меню режима, когда видна строка «Е — ВЫХОД».

**Основание.** Встроенное меню 0230h–02F5h.

### menedver.exi

- [unknown/lafan/menedver.exi](unknown/lafan/menedver.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `caps`. Нажать РУС при «НАЖМИТЕ КЛАВИШУ РУС».

**Действие.** После исчезновения запроса проверить назначение клавиш игры отдельно.

**Наблюдение.** Исчезает запрос переключения раскладки.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Подсказка «НАЖМИТЕ КЛАВИШУ РУС» в 1F93h. Требуется поддержка клавиши РУС в используемом MCP.

### MiniTetris.rks

- [unknown/games_rks/MiniTetris.rks](unknown/games_rks/MiniTetris.rks) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (исходный контейнер).
- [unknown/lafan/mal_tetr.exi](unknown/lafan/mal_tetr.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -RF`, `RUN H:ARCHIVE.EXI -MX` для проверки исходного архива.
- [unknown/lafan/tetris1.exe.bin](unknown/lafan/tetris1.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** `5`. После «КЛАСС?» выбрать 5.

**Действие.** `right` → `rpt`. Стрелка смещает фигуру, ПВ поворачивает. ВК приостанавливает.

**Наблюдение.** Фигура сдвигается или вращается.

**Выход.** `clear`. ГАШ.ЭКР. (MCP clear) завершает игру.

**Основание.** Встроенная справка 62AEh–62F5h. Клавиша сброса различается в редакциях.

### nupogodi.rks.bin

- [unknown/games_lik_rks/nupogodi.rks.bin](unknown/games_lik_rks/nupogodi.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** `enter`. ВК начинает игру.

**Действие.** Клавиши изменения положения ловца в этой LIK-редакции требуется установить в подходящем мониторе.

**Наблюдение.** После нажатия Enter начинается падение яиц. Перемещение ловца подтверждается только отдельной игровой клавишей.

**Выход.** `clear`. СТР — выход в монитор.

**Основание.** Встроенная инструкция 7472h и требования к монитору LIK из LIST.TXT.

### o-ko.ex2

- [unknown/lafan/o-ko.ex2](unknown/lafan/o-ko.ex2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EX2 -MON` для проверки исходного архива.

**Начало.** `1` → `enter` → `1` → `0` → `0` → `enter`. На запрос числа игроков ввести 1 и Enter, на запрос банка 100 и Enter.

**Действие.** `1` → `0` → `enter` → `d` → `enter`. На запрос ставки ввести 10 и Enter. D и Enter означают «да» на запрос ещё карты.

**Наблюдение.** После принятой ставки ожидается раздача, затем меняется запрос игрового хода.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Совпадают токенизированные строки BASIC запроса числа игроков и банка в 23E5h–2449h и ветвь запроса ставки. Прежний подтверждённый RUN относится к std/Ochko.rks.

### pacman-2.rks.bin

- [unknown/games_lik_rks/pacman-2.rks.bin](unknown/games_lik_rks/pacman-2.rks.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON`, `RUN H:ARCHIVE.RKS -RF`, `RUN H:ARCHIVE.RKS -MX` для проверки исходного архива.
- [unknown/lafan/pacman_2.mg2](unknown/lafan/pacman_2.mg2) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON`, `RUN H:ARCHIVE.MG2 -RF`, `RUN H:ARCHIVE.MG2 -MX` для проверки исходного архива.
- [unknown/lafan/pakman2.com.bin](unknown/lafan/pakman2.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.

**Начало.** На запросе «ИМЯ» заполнить имя по приглашению. 0 очищает таблицу.

**Действие.** Справка перечисляет I,T,X,B. S приостанавливает, Q продолжает. Назначение направлений нужно доказать в коридоре.

**Наблюдение.** Персонаж меняет направление после команды. Открытие экрана имени подтверждает только доступ к настройке.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Инструкции 1353h и 1392h у LIK и 24D0h/250Fh у двух Lafan-нагрузок.

### Paket1.rks

- [unknown/games_rks/Paket1.rks](unknown/games_rks/Paket1.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/blok.exe.bin](unknown/lafan/blok.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** В меню ПАКЕТ ИГР выбрать вариант по подписи, затем следовать подсказке конкретной игры. У одного варианта указано два ВК для начала.

**Действие.** Игровые направления меняются с выбранной игрой. Для Tetris в строке 02C1h показаны влево и вправо.

**Наблюдение.** Появляется экран выбранной игры. Команды должны подтверждаться на её поле.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашения 02A9h, 1014h, 13DAh и 1400h. Меню пакета просмотрено в Emu80.

### pe-era.exe.bin

- [unknown/lafan/pe-era.exe.bin](unknown/lafan/pe-era.exe.bin) — `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** Увидеть первое поле и дождаться выбора начального режима.

**Действие.** Значения клавиш оригинальной «Пещеры» нужно проверять по отдельной матрице этой копии. Выбор `2Bh` записывается в FFF8h.

**Наблюдение.** Диспетчер ждёт стабильный вход. Для перемещения вправо внутренний код 01h задаётся при считанном FDh.

**Выход.** Штатный выход требует проверки.

**Основание.** В pe-era.exe.bin отличаются запись по 0024h и таблица действий 08B9h–08CAh. Остальные варианты из карточки LRunner выбирают колонку цифр маской BCh.

### piton.exi

- [unknown/lafan/piton.exi](unknown/lafan/piton.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `space` → `caps` → `i`. На меню пробел переводит матричный опрос к блокирующему C337h. Если действует латинская раскладка, нажать РУС/ЛАТ (MCP caps). И (MCP i) запускает поле по коду 69h. Иные клавиши в этом меню повышают уровень.

**Действие.** Дождаться игрового поля после клавиши И. Раскладка управления змеёй требует отдельной пробы.

**Наблюдение.** Уровень меняется в 13FDh по ветви настройки. При коде 69h исполняется вход 10ACh.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Опрос FF01h и C337h в 107Fh–10ACh. Режим буквы И выведен из штатной таблицы Monitor-2 и сравнения 69h. Полный прогон этой копии требует проверки.

### Piton.rks

- [unknown/games_rks/Piton.rks](unknown/games_rks/Piton.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/piton_2.exi](unknown/lafan/piton_2.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `5`. На запросе скорости 0–9 нажать 5.

**Действие.** При появлении поля определить клавиши поворота для данной редакции.

**Наблюдение.** После принятой цифры появляется поле. Изменение направления требует прогона.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Приглашения 4255h, 02F9h и экран «УДАВ» в Emu80.

### piton_1.exi

- [unknown/lafan/piton_1.exi](unknown/lafan/piton_1.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Сначала выбрать фон по приглашению, затем клавиши направления и скорость 0–9.

**Действие.** После старта применить назначенную клавишу поворота в свободную сторону.

**Наблюдение.** Игровой питон поворачивает при нажатии настроенной клавиши.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенный диалог 0AE7h–0BB0h. Формат выбора фона требует проверки.

### piton_3.exi

- [unknown/lafan/piton_3.exi](unknown/lafan/piton_3.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `5`. При запросе скорости выбрать 5.

**Действие.** На поле сравнить раскладку с Piton4. Она зависит от действующего кода таблицы и платформенного опроса.

**Наблюдение.** После принятия скорости начинается движение змеи.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Вариант Lafan наследует игровое тело std/Piton4.rks с изменённым платформенным вводом.

### piton_4.exi

- [unknown/lafan/piton_4.exi](unknown/lafan/piton_4.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `enter`. При настройке этой версии нажать ВК для начала игры.

**Действие.** Ждать игровую змею и символы поля. Клавиши направления нужно вывести из отдельного опроса этой версии.

**Наблюдение.** ВК ведёт к подготовке игровой сцены 4052h. СТР имеет отдельную ветвь.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Опрос 4045h и переход 4052h. В отличие от std/Piton4.rks нагрузка размещена с 4000h.

### PodvodnayaLodka.rks

- [unknown/games_rks/PodvodnayaLodka.rks](unknown/games_rks/PodvodnayaLodka.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** В MON после `PROGRAM ?` требуется дождаться карты «ПОДВОДНАЯ ЛОДКА». В зафиксированных кадрах первая видимая сцена не дошла до карты.

**Действие.** `right`. На поле стрелки влево и вправо должны управлять горизонтальным движением.

**Наблюдение.** После входа в игровой цикл по стрелке изменится позиция 0125h или 0128h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Нагрузка 0100h–0F5Fh повторяет mx/lodka.cpu с 29 различиями, включая платформенный опрос клавиатуры. У RKS дополнительный префикс 0000h–00FFh.

### poker.com.bin

- [unknown/lafan/poker.com.bin](unknown/lafan/poker.com.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON` для проверки исходного архива.

**Начало.** `f3`. F3 начинает игру, F1 показывает правила и F2 комбинации.

**Действие.** `1` → `enter`. На запросе ставки нажать 1 (одна монета) и Enter, затем выбрать карты цифрами 1–5 по правилам.

**Наблюдение.** Ставка принята и на экране появляются карты или результат раздачи.

**Выход.** `f4`. F4 — выход в меню программы.

**Основание.** Архивные правила 0A45h–0DCEh. Коды в другой сборке Poker проверены отдельно.

### pqtna-ki.mg2

- [unknown/lafan/pqtna-ki.mg2](unknown/lafan/pqtna-ki.mg2) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON`, `RUN H:ARCHIVE.MG2 -RF`, `RUN H:ARCHIVE.MG2 -MX` для проверки исходного архива.
- [unknown/lafan/pqtna-ki.com.bin](unknown/lafan/pqtna-ki.com.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -MON`, `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.
- [unknown/lafan/pqtna-k.exe.bin](unknown/lafan/pqtna-k.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** После перемешивания дождаться поля «Пятнашек» с пустой клеткой.

**Действие.** `right`. Стрелка двигает плитку, если ход в данном направлении допустим. По положению пустой клетки выбрать доступную стрелку.

**Наблюдение.** Один номер переходит в соседнюю пустую клетку и меняется счётчик ходов.

**Выход.** `m`. На поле нажать M для выхода.

**Основание.** Диспетчер 0134h–0168h и счётчик 040Eh. Выход M сравнивается с 4Dh. MON передаёт управление C000h, RF — F800h.

### pusher.mgk

- [unknown/lafan/pusher.mgk](unknown/lafan/pusher.mgk) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MGK -MON`, `RUN H:ARCHIVE.MGK -RF` для проверки исходного архива.
- [unknown/lafan/pusher.exe.bin](unknown/lafan/pusher.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.

**Начало.** `space` → `enter`. После титула PUSHER нажать пробел. На экране выбора стрелки меняют уровень. Enter открывает выбранную схему.

**Действие.** `right`. На игровом поле стрелки перемещают героя. Если соседняя клетка занята ящиком, он перемещается, когда клетка дальше свободна.

**Наблюдение.** Герой сдвигается, на подходящем участке сдвигается и ящик.

**Выход.** `esc`. На выборе уровня Esc передаёт управление монитору.

**Основание.** Опрос титула в 0020h, выбор уровня 0038h и 0496h–04ADh, сравнения стрелок в 018Ah, 0218h, 02A6h и 0333h.

### samantha.com.bin

- [unknown/lafan/samantha.com.bin](unknown/lafan/samantha.com.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.

**Начало.** Дождаться окончания первого кадра иллюстраций.

**Действие.** `space`. После остановки вывода нажать пробел для следующего кадра.

**Наблюдение.** На очередное действие меняется кадр.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Таблица кадров 00AAh. У RAMFOS-архива чтение клавиши 007Ch перенаправлено через C803h, затем переход в 6570h.

### sex_2.exi

- [unknown/lafan/sex_2.exi](unknown/lafan/sex_2.exi) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** Нагрузка начинается с C7h по адресу 0000h, то есть RST 0. Вход RUN по этому адресу зацикливается до первого кадра.

**Действие.** Для просмотра нужен установленный архивный вход после 0000h. Ветка следующего кадра читает F81Bh.

**Наблюдение.** Отсутствие изображения связано со стартовым переходом. Опрос по 0013h расположен в теле после него.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Адрес из каталога ODI, байт входа C7h, ветвь F81Bh и отсутствие кадра после проверенной точки входа RF.

### snajper.com.bin

- [unknown/lafan/snajper.com.bin](unknown/lafan/snajper.com.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.
- [unknown/lafan/wertole.exe.bin](unknown/lafan/wertole.exe.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -RF`, `RUN H:ARCHIVE.EXE -MX` для проверки исходного архива.

**Начало.** Дождаться конца таблицы «ZAB, OWN, SIG» и игрового экрана после «COME START».

**Действие.** `space`. На игровой сцене пробел проходит в ветвь расходования боезапаса, если счётчик больше единицы.

**Наблюдение.** Обработчик 51CFh меняет состояние объекта и счётчик 601Bh при принятой игровой операции.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Меню или «COME START» в архивных RF/MX-кадрах. Общая ветвь 51CFh–51E0h в обеих нагрузках. Вызовы монитора и прочие коды команд различаются.

### sokoban.exe.bin

- [unknown/lafan/sokoban.exe.bin](unknown/lafan/sokoban.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.
- [unknown/lafan/sokoban.mg2](unknown/lafan/sokoban.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** Дождаться открытия уровня после инструкции. Плюс/минус выбирают этаж, пробел возвращает текущий этаж.

**Действие.** `right`. Стрелками подойти к сундуку и сдвинуть его, если за ним свободна клетка.

**Наблюдение.** Сундук меняет клетку. При установке на цель меняется символ сундука.

**Выход.** `enter`. ВК завершает игру.

**Основание.** Токенизированная встроенная инструкция 24A1h–264Ch.

### sokrov_c.exe.bin

- [unknown/lafan/sokrov_c.exe.bin](unknown/lafan/sokrov_c.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.
- [unknown/lafan/sokrov_c.mg2](unknown/lafan/sokrov_c.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** `1`. После инструкции выбрать первый уровень сложности.

**Действие.** `right`. На поле сдвинуть героя стрелкой на свободную клетку.

**Наблюдение.** Игрок меняет положение, собираемые объекты исчезают при подборе.

**Выход.** `clear`. СТР выходит из игры.

**Основание.** Встроенная инструкция 0A5Ah–0BCCh.

### sonnik.exi

- [unknown/lafan/sonnik.exi](unknown/lafan/sonnik.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Дождаться каталога слов.

**Действие.** `right` → `space`. На списке стрелками и пробелом выбрать слово.

**Наблюдение.** Появляется текст толкования выбранного слова.

**Выход.** `clear`. СТР — выход.

**Основание.** Встроенные строки 0343h–03F7h. Это справочник с интерактивным выбором.

### sprint.com.bin

- [unknown/lafan/sprint.com.bin](unknown/lafan/sprint.com.bin) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF` для проверки исходного архива.

**Начало.** Дождаться начального поля из текущей копии.

**Действие.** `right`. По стрелке выполняется ветвь перемещения на доступную клетку. СТР возвращает к RAMFOS.

**Наблюдение.** Меняется положение управляемого объекта на карте.

**Выход.** `clear`. На игровом поле удержать СТР до реакции.

**Основание.** Диспетчер 1193h–11ACh сопоставляет четыре стрелки. СТР передаёт управление F800h.

### stakan.exe.bin

- [unknown/lafan/stakan.exe.bin](unknown/lafan/stakan.exe.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON` для проверки исходного архива.

**Начало.** В меню «ИГРА — И» вход требует код 69h, а перед ним запись клавиши в матрицу FFE1h.

**Действие.** Для пробы требуется рабочая раскладка И и совместимый доступ к матрице. Управление фигурами проверять после 091Ch.

**Наблюдение.** На принятую букву программа входит в код 091Ch. Иные распознанные знаки меняют класс игры.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Опрос FFE1h на 072Ch, C337h на 0735h и сравнение 69h на 0738h. Историческая среда клавиатуры требует проверки.

### stakan.exi

- [unknown/lafan/stakan.exi](unknown/lafan/stakan.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** На «ВАШЕ ИМЯ?» ввести имя. На «ВАШ РАНГ (0–9)?» выбрать цифру.

**Действие.** Справка описывает влево, вправо, поворот и показ следующей фигуры. Символы клавиш графические и требуют сверки.

**Наблюдение.** После принятого имени и ранга начинается игра в стакане.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Меню 0432h–0520h.

### Strana.rks

- [unknown/games_rks/Strana.rks](unknown/games_rks/Strana.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/land_1.exi](unknown/lafan/land_1.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.
- [unknown/lafan/land_2.mg2](unknown/lafan/land_2.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** Дождаться первого поля или выйти из редактора по инструкции этой редакции.

**Действие.** `right`. На игровом поле и свободном участке стрелка вправо перемещает героя. Q и ^ применяются для копания.

**Наблюдение.** Меняется клетка игрока или состояние кирпича.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Коды стрелок и операций над кирпичом в 05DFh–061Eh совпадают с Land. Стартовая часть имеет отдельный диспетчер.

### Tank.rks

- [unknown/games_rks/Tank.rks](unknown/games_rks/Tank.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).
- [unknown/lafan/tamagawk.exi](unknown/lafan/tamagawk.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `1`. На «ВАШ КЛАСС (0–9)» нажать 1.

**Действие.** `r`. Подсказки показывают ПУСК РАКЕТЫ и клавиши влево/вправо. Для пуска используется Р (MCP r).

**Наблюдение.** От игровой машины отделяется ракета.

**Выход.** `o`. О — выход.

**Основание.** Встроенное меню COPTER в 0600h–06B5h. Клавиши направления могут различаться с std/Copter.rks.

### tenis.exi

- [unknown/lafan/tenis.exi](unknown/lafan/tenis.exi) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `1`. После «КЛАСС?» выбрать 1. Допустимые цифры уточняются по отображению.

**Действие.** Управление ракеткой требует проверки на игровом поле.

**Наблюдение.** Запрос класса должен исчезнуть после принятия цифры.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенный запрос класса 02C8h.

### tetris-d.rks.bin

- [unknown/games_lik_rks/tetris-d.rks.bin](unknown/games_lik_rks/tetris-d.rks.bin) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.RKS -MON` для проверки исходного архива.

**Начало.** `5`. На запросе скорости 0–9 выбрать 5.

**Действие.** `6` → `5` → `8`. 6 двигает фигуру вправо, 5 поворачивает, 8 бросает. Влево — 4.

**Наблюдение.** Фигура сначала сдвигается и поворачивается, затем опускается на дно.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Подсказки варианта LIK, просмотренные в Emu80 MON после переупаковки нагрузки.

### tetris.exi

- [unknown/lafan/tetris.exi](unknown/lafan/tetris.exi) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** Дождаться выбора уровня. Способ первого старта уточняется.

**Действие.** На поле клавиша РУС сбрасывает, в экранном приглашении «ДАЛЬШЕ» пробел продолжает.

**Наблюдение.** Наблюдается изменение уровня или сброс поля после подтверждённой клавиши.

**Выход.** `clear`. СТР обозначает выход.

**Основание.** Встроенная инструкция 01B8h–01F7h и приглашение 054Ch.

### utka_.com.bin

- [unknown/lafan/utka_.com.bin](unknown/lafan/utka_.com.bin) — `RUN H:PROBE.RKS -RF`, `RUN H:PROBE.RKS -MX` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.COM -RF`, `RUN H:ARCHIVE.COM -MX` для проверки исходного архива.

**Начало.** `0`. На титуле «ТЕСТ-ШУТКА» выбрать один из двух вариантов 0 или 1.

**Действие.** `1`. После первого вопроса выбрать 0 или 1 по подсказке. Для пробы установить ответ 1.

**Наблюдение.** Печатается следующий вопрос, ответ записывается в таблицу по 0AE0h.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Титульный текст 0700h, разбор варианта 0029h–0045h и двоичный ввод ответов 004Ch–0066h.

### viznx.mg2

- [unknown/lafan/viznx.mg2](unknown/lafan/viznx.mg2) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON` для проверки исходного архива.

**Начало.** По окончании инструкции перейти к редактированию поля.

**Действие.** `right` → `space`. Стрелками выбрать клетку, пробел фиксирует организм, СТР запускает эволюцию. Во время эволюции пробел удерживает расчёт, Enter возвращает к редактированию.

**Наблюдение.** В выбранной клетке появляется организм. При запуске рисунок меняется по правилам колонии.

**Выход.** `m`. M возвращает в монитор.

**Основание.** Встроенная инструкция 0319h–0774h. Это режим редактора и модель клеточного автомата.

### Vulkan.rks

- [unknown/games_rks/Vulkan.rks](unknown/games_rks/Vulkan.rks) — `RUN H:PROBE.RKS -MON` (исходный контейнер).

**Начало.** `p`. В меню P начинает игру, K настраивает управление, S меняет звук.

**Действие.** `right`. После появления вулкана и вертолёта удержать вправо. Меню также назначает АР2 (MCP esc) выстрелу.

**Наблюдение.** Вертолёт меняет координату 0F9Dh с 58 на 59 и 61. В обоих контрольных ожиданиях она остаётся 58.

**Выход.** `e`. В меню E завершает игру.

**Основание.** Встроенное меню и холодные парные пробы RUN -MON. Два других работника воспроизвели адресные значения, кадры и чтение H: после Reset. Команда `python scripts/games/check-controls-extra.py vulkan --worker probe`.

### war_v2.exe.bin

- [unknown/lafan/war_v2.exe.bin](unknown/lafan/war_v2.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.
- [unknown/lafan/war_v2.mg2](unknown/lafan/war_v2.mg2) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MG2 -MON`, `RUN H:ARCHIVE.MG2 -RF` для проверки исходного архива.
- [mx/war.cpu](mx/war.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** `enter`. На «НАЖМИ ВК» нажать Enter. Если показано «ВАШ УРОВЕНЬ? (1–3)», выбрать 1.

**Действие.** Клавиши движения требуется определить на первом поле.

**Наблюдение.** Игра переходит к выбору уровня или полю.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенные приглашения 07F2h и, в вариантах war_v2, 1FBCh.

### wertolet.exi

- [unknown/lafan/wertolet.exi](unknown/lafan/wertolet.exi) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `1`. На «LEVEL [0–9]?» выбрать 1.

**Действие.** `r`. После появления поля нажать R для пуска ракеты (FIRE).

**Наблюдение.** При FIRE появляется выстрел.

**Выход.** `o`. O завершает игру.

**Основание.** Встроенная инструкция в 0471h–04D8h.

### wozd.boj.exi

- [unknown/lafan/wozd.boj.exi](unknown/lafan/wozd.boj.exi) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.

**Начало.** `1`. На «LEVEL (0–9)?» выбрать 1.

**Действие.** Раскладка управления полётом требует отдельной пробы.

**Наблюдение.** После выбора открывается игровой уровень.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Встроенное приглашение 07C8h.

### wremq_2.exi

- [unknown/lafan/wremq_2.exi](unknown/lafan/wremq_2.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** Дождаться отрисовки счётчика времени.

**Действие.** Игровое действие отсутствует. Счётчик часов изменяется по циклу программы.

**Наблюдение.** Пробел сразу передаёт управление C800h.

**Выход.** `space`. На экране счётчика нажать пробел.

**Основание.** Ветвь выхода 1229h–122Eh. Десятичные разряды счётчика обновляются в 1239h–126Bh.

### xonix-a.exe.bin

- [unknown/lafan/xonix-a.exe.bin](unknown/lafan/xonix-a.exe.bin) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXE -MON`, `RUN H:ARCHIVE.EXE -MX`, `RUN H:ARCHIVE.EXE -RF` для проверки исходного архива.
- [unknown/lafan/xonix-a.exi](unknown/lafan/xonix-a.exi) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON`, `RUN H:ARCHIVE.EXI -MX`, `RUN H:ARCHIVE.EXI -RF` для проверки исходного архива.
- [unknown/lafan/xonix.mgk](unknown/lafan/xonix.mgk) — `RUN H:PROBE.RKS -MON`, `RUN H:PROBE.RKS -MX`, `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.MGK -MON`, `RUN H:ARCHIVE.MGK -MX`, `RUN H:ARCHIVE.MGK -RF` для проверки исходного архива.
- [mx/xonix_.cpu](mx/xonix_.cpu) — `RUN H:PROBE.RKS -RF` (проверочный контейнер с игровым телом).

**Начало.** Дождаться титульного счёта или запроса YOUR RANGE. Версии с загрузкой на 5000h отличаются от Xonix STD.

**Действие.** При «Press any key» нажать любую клавишу. Управление после появления поля нужно проверить по экрану конкретной нагрузки.

**Наблюдение.** Игровое поле наблюдается после принятой команды, положение курсора меняется при установленной стрелке.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Надписи Score/Level, Game over и Press any key в диапазоне 5000h–5E00h либо по адресу 0003h–0E00h.

### xonix.exi

- [unknown/lafan/xonix.exi](unknown/lafan/xonix.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** `1`. На «YOUR RANGE (0–9?)» выбрать 1. S начинает игру с текущими параметрами.

**Действие.** `right`. После появления границы поля выбрать стрелку вправо. Младшие два бита 16E0h задают направление.

**Наблюдение.** Игрок направляется вправо и строит линию при уходе с границы.

**Выход.** Штатный возврат в ОС требует проверки.

**Основание.** Старт 111Bh–1135h и обработчики 1204h–123Dh. Историческая программа использует собственные входы клавиатуры 0003h и 001Bh.

### xonixp.exi

- [unknown/lafan/xonixp.exi](unknown/lafan/xonixp.exi) — `RUN H:PROBE.RKS -MON` (проверочный контейнер с игровым телом). `RUN H:ARCHIVE.EXI -MON` для проверки исходного архива.

**Начало.** При видимом поле LEVEL и счётчиках дождаться положения игрока на границе.

**Действие.** `right`. Стрелки задают направление. Переместить игрока по открытой начальной границе.

**Наблюдение.** Позиция игрока меняется и появляется проложенный им участок линии.

**Выход.** `clear`. СТР перезапускает игру с 1100h.

**Основание.** Диспетчер 117Bh–11B6h и подписи игрового поля 164Eh.
