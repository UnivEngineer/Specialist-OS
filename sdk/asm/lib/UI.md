# Общая графика MAIN

`ui.inc` подключается первым проходом для макросов списков объектов и вторым с `DEFINE UI_IMPLEMENTATION` для процедур. Библиотека не использует состояние NC и подходит для MAIN-программы или её MAIN DLL. Нужен `mxos.inc`. Существующие окна, заливки и байтовая инверсия сохранены; попиксельные функции не заменяют быстрый путь панелей и диалогов.

```asm
    INCLUDE "mxos.inc"
    INCLUDE "lib/ui.inc"         ; G_WINDOW / G_LINE / G_VLINE
    ; ... ORG, код приложения ...
    DEFINE UI_BYTE_RECT
    DEFINE UI_PIXEL_RECT
    DEFINE UI_BITMAP         ; сохранение/восстановление областей экрана
    DEFINE UI_TEXT_RECT      ; если нужен перенос 8 строк глифа из ячейки 10px
    DEFINE UI_TEXT_RECT_COLORS ; цветной перенос, требует UI_TEXT_RECT
    DEFINE UI_IMPLEMENTATION
    INCLUDE "lib/ui.inc"
```

Ненужные группы можно не подключать. Основные `uiDraw`, `uiInverseRect` и `uiCopyRect` входят в обычную реализацию; `UI_NO_TOOLBAR` исключает общий рендер тулбара. В функциях используются абсолютные координаты экрана; смещение панели, списки надписей и выбор палитры задаёт приложение.

| Вход | Координаты и размеры | Действие |
|---|---|---|
| `uiToolbar` | HL — список UI_TOOLBAR_FIELD, B/C — цвета клавиш/подписей | Нижняя панель Y=247..255, 48 экранных байтов; используется активный шрифт 4/6/8 px, курсор и инверсия сохранены |
| `uiDraw` | HL — список объектов, A — сдвиг X в экранных байтах | Байтовые окна и линии; макрос `G_WINDOW` выравнивает границы по байтам |
| `uiFillRectBytes` | HL — адрес VRAM, B — ширина в байтах, C — высота в пикселях, A — узор | Быстрая заливка без масок крайних байтов |
| `uiFrameRectBytes` | То же | Однопиксельный контур прямоугольника с байтовыми границами X; внутри битмап сохранён |
| `uiInverseRect` | HL — адрес VRAM, B — ширина в байтах, C — высота в пикселях | Быстрая инверсия полных экранных байтов; DE сохранён |
| `uiCopyRect` | HL — источник VRAM, BC — назначение VRAM, D — ширина в байтах, E — высота в пикселях | Перенос с безопасным направлением при вертикальном перекрытии |
| `uiFillRectPixels` | HL — X, D — Y, BC — ширина в пикселях, E — высота в пикселях, A — узор | Попиксельная заливка с масками краёв |
| `uiInputFieldPixels` | HL — X, D — нижняя линия глифа, BC — ширина поля в пикселях, A — высота глифа | Сплошной фон поля ввода с отступом по одному пикселю сверху и снизу; положение текста и курсора не меняется |
| `uiFrameRectPixels` | То же | Попиксельный контур толщиной один пиксель; внутри битмап сохранён |
| `uiInverseRectPixels` | То же, A не используется | Инверсия прямоугольника; повторный вызов восстанавливает битмап |
| `uiCopyTextRect` | Как `uiCopyRect`, высота кратна 10 | Развёрнутый стековый перенос строк 1–8 каждой ячейки, пропуск пустых строк 0 и 9 |
| `uiCopyTextRectColors` | HL — адрес 10-байтового запроса, описанного ниже | Перенос текста вместе с программной таблицей цветов строк |
| `uiBitmapSave` / `uiBitmapRestore` | HL — VRAM, DE — буфер, B — ширина в байтах, C — высота | Сохранение/восстановление произвольной области без пропуска межстрочных полей |
| `uiBitmapSaveColors` / `uiBitmapRestoreColors` | HL — адрес 12-байтового запроса | Битмап вместе с сериями известных приложению цветовых атрибутов |

Попиксельные X находятся в пределах 0–383, Y — 0–255. Прямоугольник целиком помещается в экран; обрезания за его границами нет. Высота нового API — 0–255, нулевая ширина или высота у заливки/рамки/пиксельной инверсии ничего не рисует. Старым `uiInverseRect` и `uiCopyRect` передаются ненулевые размеры. Для байтовых адресов младший байт — Y, старший — 90h плюс номер байтового столбца. Узор 00h очищает, FFh заполняет; прочие узоры привязаны к битам экранного байта, а не к началу прямоугольника. AF/BC/DE/HL разрушаются, кроме явно оговорённых исключений. Подробности переноса и ограничения перекрытия описаны в исходниках.

Цвет устанавливает вызывающий код. Цветовой регистр не читается: атрибут относится ко всему экранному байту даже при маскированной записи одного пикселя. Приложение должно знать палитру соседних пикселей и выбирать совместимый атрибут; три независимых цвета в одном байте не представимы. NC хранит текущий цвет программно и обрабатывает кромку выделения отдельно. Внутри одноцветного диалога заливка, контур и инверсия используют его общую палитру.

Пиксельные заливки и контуры используют обычный стек и собственные малые рабочие переменные, поэтому нереентерабельны. `uiCopyTextRect` временно использует SP для данных: IRQ должны быть запрещены; код, обычный стек и рабочие данные не пересекаются с переносимыми областями. Высота `uiCopyTextRect` — 0 либо 10–250 с шагом 10, ширина ненулевая; нулевая высота ничего не меняет. Вертикальное и горизонтальное перекрытие разрешено в обоих направлениях. Пропуск межстрочных полей допустим только при одинаковом пустом фоне; выделение, меняющее поля 0/9, нужно снять или исключить. Инверсия только восьми строк глифа допустима. Стековый копировщик находится в `ui-text-rect.inc`, байтовые операции — в `ui-byte-rect.inc`, пиксельные — в `ui-pixel-rect.inc`.

`uiCopyTextRectColors` из `ui-text-colors.inc` принимает HL с адресом запроса: источник VRAM (word), назначение VRAM (word), ширина в байтах (byte), высота в пикселях (byte), адрес цветов источника (word), адрес цветов назначения (word). Ширина ненулевая, высота 0 либо 10–200 с шагом 10; нулевая высота ничего не меняет. Таблицы содержат по одному байту цвета на ячейку, от верхней строки к нижней. Прямоугольники полностью находятся в VRAM; вертикальное перекрытие разрешено в обоих направлениях, горизонтальное — только влево. Фон межстрочных полей одинаковый и пустой, выделение заранее снято или исключено из переноса. API сохраняет исходные цвета в собственном временном буфере 20 байт, поэтому таблицы могут перекрываться, а затем обновляет таблицу назначения. Длинные одноцветные группы переносятся пакетами; при трёх и более группах используется один развёрнутый проход с выбором цвета внутри каждой строки. Цветовой регистр только записывается, промежуточного перекрашивания текста нет. AF/BC/DE/HL разрушаются, A возвращает последний записанный цвет, SP восстановлен. Для нулевой высоты значение A не определено. Функция нереентерабельна и требует запрета IRQ, как `uiCopyTextRect`; программное зеркало текущего цвета вызывающая программа обновляет из A.

## Снимки экранных областей

`UI_BITMAP` подключает `ui-bitmap.inc`. Это локальный API без резервирования памяти: буферы предоставляет приложение. У `uiBitmapSave` и `uiBitmapRestore` одинаковые аргументы: HL — верхний левый экранный байт, DE — буфер, B — ширина в байтах (1–48), C — высота в пикселях (1–255). Нулевая ширина или высота ничего не делают. X выровнен на экранный байт; прямоугольник целиком находится в VRAM, Y+height≤256. Буфер не пересекается с VRAM, кодом, стеком или рабочими данными библиотеки. Его размер — width×height байт; сначала хранятся все строки первого байтового столбца, затем второго и так далее. BC сохранён; HL указывает на следующий экранный столбец с прежним Y, DE — за данные буфера, AF разрушен. `Save` не пишет в VRAM и не читает цветовой регистр; `Restore` пишет с текущим цветом. Не пропускаются ни пустые строки, ни биты рамки или инверсии. Копирование развёрнуто по восемь байт с отдельным остатком.

Цветные входы принимают HL с адресом запроса:

| Смещение | Размер | Значение |
|---:|---:|---|
| 0 | word | Адрес VRAM |
| 2 | byte | Ширина в экранных байтах |
| 3 | byte | Высота в пикселях |
| 4 | word | Буфер битмапа width×height |
| 6 | word | Буфер серий атрибутов |
| 8 | word | Размер буфера атрибутов |
| 10 | word | Callback программного определения цвета, только для Save |

Перед callback HL содержит адрес экранного байта, C — оставшуюся высоту текущего столбца. Callback возвращает A=цвет и C=длину вертикальной серии (1…исходное C), сохраняя B/DE/HL. Он использует программную таблицу приложения, не читает аппаратный цвет, не меняет экран и не вызывает повторно функции `uiBitmap*`. Можно вернуть C=1 для отдельного атрибута каждого байта; длинные одинаковые области выгодно объединять. Все 256 значений цветового регистра допустимы. Серии сохраняются как `{длина, цвет}` в порядке столбцов, от верхней строки к нижней. Их длины для каждого столбца в сумме равны высоте снимка; отдельный терминатор не нужен. Верхняя граница памяти для произвольной раскраски — 2×width×height, для известных приложению полос достаточно меньшего буфера.

`uiBitmapSaveColors` возвращает CF=0 при успехе, DE — за записанные серии. CF=1 означает нехватку буфера атрибутов: снимок частичный и восстанавливать его нельзя; VRAM при этом не изменён. `uiBitmapRestoreColors` принимает только успешный неизменённый снимок и пишет каждую серию сразу с сохранённым цветом, возвращая A=последний записанный цвет. Размер буфера и callback при Restore не используются; геометрия и указатели в запросе должны оставаться прежними. При нулевых размерах обе функции ничего не делают, значение A после Restore не определено. AF/BC/DE/HL разрушаются, SP сохранён. Обе используют обычный стек, собственные малые рабочие данные и потому нереентерабельны. Программное зеркало текущей палитры приложения надо обновить из A либо явно установить требуемый цвет после Restore. Формат битмапа тот же, что у сырых входов; он подходит и для кэша тулбара.


## Выравнивание текста и кнопки

`ui-text-align.inc` предоставляет `uiAlignText`: HL — ASCIIZ в локальной памяти, B — левый край X/2, C — ширина поля/2, D — ширина глифа в пикселях (4/6/8), E — нижняя линия текста, A — UI_ALIGN_LEFT/CENTER/RIGHT (0/1/2). Возвращает DE с координатами текстового BIOS, сохраняя HL и BC; CF означает, что строка не помещается. Процедура не вызывает BIOS и не обращается к памяти другого банка. Путь сначала сокращает вызывающая программа. Для непрозрачного изменяемого поля NC дополняет строку пробелами, а правое поле — ведущими пробелами.

`ui-button.inc` компилируется вместе с приложением и использует эти координаты, пиксельные прямоугольники и текстовые службы. `uiDrawButton`, HL=UI_BUTTON, очищает прямоугольник, рисует рамку и центрированную подпись; `uiSelectButton`, HL=тот же объект, A=0/1, меняет только фокус. AF/BC/DE/HL разрушаются, процедуры нереентерабельны. Новые точки входа в таблицу ядра не добавлены.

UI_BUTTON занимает 10 байт: text:word, left:byte (X/2), width:byte (ширина/2), top:byte, height:byte, font:byte, selected:byte, normal:byte, focused:byte. Кнопка целиком находится на экране; X и ширина кратны восьми пикселям, ширина 8–248, высота не меньше 12. Строка вместе с отступами помещается в рамку. Последние два байта задают палитры обычного и выбранного состояния. Фокус перекрашивает неизменные байты всего прямоугольника, без инверсии и чтения аппаратного регистра цвета; после вызова восстановлена обычная палитра. В текущем цветном оформлении монохромный дисплей не различает фокус: отдельный монохромный профиль потребует собственного оформления.

По умолчанию UI_BUTTON_CURSOR/PRINT используют текстовые входы BIOS основной памяти; UI_BUTTON_COLOR пишет регистр цвета. Их можно переопределить через DEFINE до включения файла. Для банкового клиента нужны локальные обёртки MXCALL, а не прямые вызовы адресов BIOS чужого банка. NC задаёт UI_BUTTON_COLOR=ncSetColor, поддерживая программную копию палитры.

`G_MODAL_WINDOW` с `DEFINE UI_MODAL_BORDER` у реализации ui.inc оставляет по два пикселя снаружи верхней и нижней рамки, сохраняя общие габариты G_WINDOW. Снимок сохраняет именно заданную ширину/высоту: бордюр не расширяет область рисования. Обычный G_WINDOW для панелей не меняется. NC сохраняет до 3072 байт пикселей и 1024 байт серий цветов; весь временный буфер 4096 байт переиспользует рабочую область сортировки.

### Shared modal template

`ui-modal.inc` is the shared MAIN modal component for NC, the editor and other utilities. Include it after the UI drawing implementation, `ui-text-align.inc` and `ui-button.inc`; enable `UI_MODAL_BORDER` and `UI_PIXEL_RECT` for the drawing implementation. A caller supplies the desired body height and a grid of button labels; it does not calculate a window rectangle, separators, button coordinates, or a clearing rectangle.

```asm
errorDialog:
    UI_MODAL_BEGIN 12,1,1 ; body pixels, button rows, columns
    UI_MODAL_BUTTON okText,0,0,1 ; caption, row, column, selected
    UI_MODAL_END
okText: DB 'OK [Enter]',0

    ; At runtime: set the palette, then draw the complete modal.
    ld hl,errorDialog
    ld a,6 ; actual installed glyph width: 4, 6 or 8
    call uiModalOpen
    ld hl,errorTitle
    call uiModalTitle
    ld hl,errorDetails
    call uiModalBodyText
```

The standard grids are 1×1 (OK/Abort), 1×2 (OK/Cancel) and 2×2 (Yes/All/No/Skip all). Supply buttons in row-major order with one to four columns and as many rows as fit on screen. The API owns the button coordinates and sets the font width on open.

The style owns a centered 256-pixel-wide double frame, 5-pixel frame clearance, 4-pixel padding in each header/body/button area, 14-pixel buttons, and 4-pixel gaps between button rows. Header and body separators are generated from the body height; a second button row automatically increases the window height. Header text and single-line body text are centered. Multiline content uses `uiModalBodyY` (A=body-relative baseline offset, E=absolute baseline, HL preserved); the body text field is X=72..311 pixels. The body height excludes its surrounding padding.

Use `UI_MODAL_BUTTONS_BEGIN rows,columns` or `UI_MODAL_COMPACT_BUTTONS_BEGIN rows,columns` when the title and buttons are the entire dialog. These presets draw one separator and place the first button row five pixels below it. Open them with the same modal API and supply only a title. Compact grids keep a common button width when `UI_MODAL_ROW` centers a shorter final row.

`uiModalOpen` takes HL=descriptor and A=glyph width. `uiModalClear` takes HL=descriptor and redraws the template and buttons. When switching layouts, call it and print the new title and content. Both calls destroy AF/BC/DE/HL. `uiModalTitle` and `uiModalBodyText` take HL=ASCIIZ and destroy AF/BC/DE/HL. Text must fit the body text field.

With `UI_MODAL_KEYS`, call `uiModalFocusInit` before opening or redrawing a button dialog. Supply HL=descriptor, A=initial zero-based index, B=navigation mode and C=populated count (0 for the whole descriptor). `UI_FOCUS_GRID` follows rows and columns with wraparound. `UI_FOCUS_LINEAR` walks all buttons in order. `UI_FOCUS_PAIR` selects the first or last button directly with Left or Right. Tab cycles through all populated cells in every mode.

`uiModalChoose` waits for Enter or Escape and returns A=selected index with CF clear, or CF set on Escape. `uiModalReadChoice` also returns other unhandled key codes in A with CF set so applications can process shortcuts. For nonblocking input, pass a key in A to `uiModalFocusKey`. It returns CF set for a consumed navigation key, or CF clear with the original key in A. Read `uiModalFocusIndex` for the current selection. All four routines destroy AF/BC/DE/HL. Text fields can use `lineEdit` with their own key handling.

The component keeps one current modal context and is not reentrant. `UI_MODAL_SNAPSHOT` optionally names a callback taking HL=generated G_MODAL_WINDOW list and preserving registers; it is called only on open, before painting. NC uses it for bitmap capture and palette setup. `UI_MODAL_BUTTON_DRAW` optionally replaces the default `uiDrawButton`; NC uses its font/palette adapter. Clearing does not recapture the underlay. Closing/restoring the underlay belongs to the client. The generated descriptor layout and style constants are private to the API.

For dynamic lists, `uiModalOpenCount` has the same contract as open and additionally takes B=populated button count; it must be between 1 and the declared grid capacity. It draws only those cells. `UI_MODAL_ROW columns` before the first button of a partial final row centers that row using its own column count. `uiModalButtonAt` takes HL=descriptor and A=cell index and returns HL=UI_BUTTON (AF/BC/DE destroyed), so dynamic captions and keyboard focus do not depend on descriptor offsets.

`python scripts/tests/check-ui-modal.py` checks the standard layouts and title-and-button layouts, including partially populated final rows. It verifies frame clearance, spacing, centering and byte alignment, then renders the NC drive dialog for 4–8 drives on both panels.

`UI_MODAL_COMPACT_BEGIN` selects the same style at 144 pixels wide. Use `uiModalPanelOpen` with HL=descriptor, A=glyph width, B=populated count and C=left/right panel (0/1). The API places the frame, text, separators and buttons wholly over that half of the screen. Reopening the same descriptor on the other panel applies a relocation delta. Descriptors and button records must be writable. `uiModalTitle` derives its field from the frame.

`uiDrawCheckbox` (UI_PIXEL_RECT) draws a graphical 8×8 checkbox, independently of the current font. HL is its byte-aligned VRAM address; A=0 draws an empty square and A≠0 draws the tick bitmap. The caller sets the palette and provides a rectangle fitting the screen. AF/BC/DE are destroyed, HL advances eight rows. Each row is one whole-byte write, so bitmap and color attributes agree and the tick also works on a monochrome display.

The geometry-only `ui-clear-window.inc` is shared by direct and queued backends. `UI_BUTTON_RECOLOR` can replace the byte rewrite loop of `ui-button.inc`: HL=pixel X, D=Y, BC=pixel width, E=height, current palette already selected; the rectangle is byte aligned. NC's EXEC backend queues this operation for MAIN. Button geometry, focus policy and text alignment stay in the common implementation.

### Clearing an existing window

`uiClearWindow` (UI_PIXEL_RECT) takes HL pointing to the same `G_WINDOW` or `G_MODAL_WINDOW` descriptor used by `uiDraw`. A is the relative X offset in screen bytes; absolute descriptors ignore it. The caller sets the color. AF/BC/DE/HL are destroyed. The routine clears content, separators and buttons, preserving the double border and modal outer padding. Geometry comes from the descriptor: horizontal inset 4 pixels; vertical inset 3 for normal windows and 5 for modal windows. Redraw separators and controls afterwards. Do not supply a separate height or width.

`uiToolbar` принимает поля шириной 1–4 экранных байта; сумма ширин равна 48. Текст каждой подписи обязан помещаться в своё поле с текущей шириной глифа. Функция центрирует текст, выравнивает начало для движков 4×8/8×8, заполняет фон окончательным цветом и рисует глифы установленным движком FNT1 без консольного переноса и прокрутки. AF/BC/DE/HL и регистр цвета разрушаются; курсор и оба байта инверсии восстанавливаются. Используется редактором и NC; кэширование панели остаётся политикой приложения.


С опцией `UI_MODAL_KEYS` функция `uiModalGetch` ждёт клавишу в кнопочном окне или просмотрщике, временно скрывая текстовый курсор. Возвращает A и флаги как `bios_getch` и восстанавливает прежний режим курсора. Поле ввода использует `lineEdit` с обычным курсором. Опция `LINEEDIT_RECALL` добавляет бит 4 в B: первая печатная клавиша заменяет подставленный текст, навигация и Backspace снимают это состояние.

## Полоса прокрутки

Включите `UI_SCROLL_BAR` и `UI_PIXEL_RECT` перед включением реализации `ui.inc`. `uiDrawScrollbar` принимает в HL адрес структуры `UI_SCROLLBAR`. Сначала передаются X (слово) и Y (байт) в пикселях, затем длина, толщина и направление (0 для вертикальной полосы, 1 для горизонтальной, 2 для горизонтальной по экранным байтам, 3 для вертикальной в одном экранном байте). Длина и толщина задаются от 1 до 255. В режимах 2 и 3 X должен быть кратен 8, толщина равна 8. В режиме 2 длина задаётся в экранных байтах. Прямоугольник должен находиться внутри экрана. За ними следуют три слова в единицах содержимого: полный размер, размер видимой области и позиция её начала.

В режимах 0 и 1 компонент заполняет дорожку узором 55h и рисует сплошной бегунок. Режимы 2 и 3 рисуют дорожку толщиной один пиксель и бегунок толщиной семь пикселей, прижатый к нижней или правой границе соответственно. Бегунок занимает всю дорожку, если содержимое помещается в видимой области. В остальных случаях его длина пропорциональна видимой области, минимум восемь пикселей вдоль дорожки (один экранный байт в режиме 2). Если дорожка короче восьми пикселей, бегунок занимает её целиком. Позиция ограничивается диапазоном 0…(полный размер − видимый размер). Цвет задаёт вызывающая сторона через IO_COLOR. AF, BC, DE и HL разрушаются, координаты текстового курсора и инверсия сохраняются. Обработку клавиш, пересчёт позиции и повторный вызов выполняет приложение.

`UI_MODAL_NOTICE` объявляет компактное окно без кнопок. Открывайте его через `uiModalOpen`, затем передайте короткий текст в `uiModalTitle`. Для сообщения о ходе операции приложение рисует окно перед вызовом ввода-вывода и снимает его после возврата.


## Справка из файла

Подключите `ui-help.inc` вместе с `ui-modal.inc`, `ui-scrollbar.inc` и `file-io.inc`. `uiShowHelp` принимает в HL абсолютный путь ASCIIZ. Перед включением задайте макросы `UI_HELP_OPEN` (HL — дескриптор окна), `UI_HELP_KEY` (A — клавиша, CF — Ctrl), `UI_HELP_CLOSE` (восстановление фона) и `UI_HELP_COLOR` (цвет текста, сохраняет BC/DE/HL). `UI_HELP_BUFFER` — 128 доступных для записи байт в вызывающей странице отдельно от `flHandle`. Просмотрщик использует общий файловый объект библиотеки, поэтому вызывается между другими файловыми операциями. Ошибка возвращается в A и CF, открытый файл закрывается.

Формат текста — KOI8-R, окончания LF или CRLF. Табуляция переводит вывод на колонки 14 и 27, считая с единицы. Окно показывает девять строк по 28 знакомест, длинные строки обрезаются. При открытии просмотрщик считает строки и запоминает файловые смещения каждых восьми строк для первых восьми контрольных точек. Шаг на строку переиспользует экранные пиксели через uiCopyTextRect и выводит только открывшуюся строку. При движении вперёд чтение продолжается из текущего 128-байтового буфера. При обратном шаге и переходах просмотрщик использует ближайшую сохранённую позицию. Реализация требует uiCopyTextRect (UI_TEXT_RECT). Приложение задаёт политику сохранения экрана и показа ошибок.

В профилях EXEC код просмотрщика находится в UI_GFX.DLL. Протокол DLL описан в `abi/ui-gfx.inc`, банковая библиотека подключает его сама. Подключите `ui-bank-help.inc` и задайте макросы `UI_HELP_KEY`, `UI_HELP_HANDLE`, `UI_HELP_REGS`, `UI_HELP_BANK` и `UI_HELP_PIXELS`. Перед строкой пути должен лежать байт цвета для восстановления фона. `GFX_HELP_OPEN` принимает адрес пути в HL, банк в B, длину с NUL в C и адрес буфера фона в MAIN в DE. Если адрес буфера равен нулю, фон после закрытия перерисовывает вызывающая программа. `GFX_HELP_KEY` принимает клавишу в C и Ctrl в бите 0 регистра B. DLL возвращает A=0, пока окно открыто, A=1 после закрытия или CF=1 и код ошибки в A. Вызов `uiShowHelp` обрабатывает этот обмен и возвращает управление после закрытия или при ошибке. Ожидание клавиш и таймер остаются в EXEC.
