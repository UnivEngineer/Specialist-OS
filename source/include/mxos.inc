;-----------------------------------------------------------------------
; MXOS
; Точки входа и настройки сборки MXOS
; BIOS.SYS — резидентные аппаратные службы; DOS.SYS — файловое ядро и CLI.
; Префикс bios_ сохранён для совместимости ABI обеих частей.
;
; Термины: банк ДОЗУ — физические 64 КиБ; FAR-код 0 — main, 1..16 — банки 0..15.
; Общая область — аппаратные 32 байта; хвосты банков содержат отдельные копии кода.
; Карта 1: основная память / BIOS, FAR-код 0, режим MX.
;   0000-8BFF - [35840 B] Область программ и буферов; memtop=8BFFh
;   8C00-8DFF - [  512 B] Область RUN
;   8E00-8FDE - [  479 B] Системный стек, SP=8FDFh
;   8FDF-8FFF - [   33 B] Переменные BIOS
;   9000-BFFF - [12288 B] Экран; временный образ сменного ядра и переходник
;   C000-E1FF - [ 8703 B] BIOS.SYS и резерв; точный конец — System.layout.json
;   E200-E9FF - [ 2048 B] Полный FONT.FNT по строкам глифов, без заголовка
;   EA00-EC7F - [  640 B] Непрерывный резерв верхней арены / кэш глифов
;   EC80-ED7F - [  256 B] Служебный стек BIOS; начальный SP=ED80h
;   ED80-EE8F - [  272 B] Общий временный буфер A: / текста и UI
;   EE90-EF9F - [  272 B] BIOS_UPPER_VARIABLES, включая состояние текста MXOS
;   EFA0-EFFF - [   96 B] IO_VARIABLES — рабочие переменные IO.SYS
;   F000-F37C - [  893 B] IO.SYS (подробная раскладка — source/io/MAINBIOS.asm)
;   F37D-F3FF - [  131 B] Резерв
;   F400-F47F - [  128 B] MEM.SYS: регистровые GET32/PUT32
;   F480-F73F - [  704 B] MEM.SYS: операции памяти и текстовый шлюз
;   F740-F7FF - [  192 B] MEM.SYS: локальные адаптеры/контекст и резерв, как в DOS
;   F800-F8BF - [  192 B] MEM.SYS: общие публичные входы CLIENT и резерв
;   F8C0-F9FF - [  320 B] Отдельный слот FLASHSYS.COM
;   FA00-FB9F - [  416 B] Один слот ROM/FLASH/FLASHFST, тот же адрес в DOS
;   FBA0-FBF5 - [   86 B] Снимок COPY для callback командной строки
;   FBF6-FBFF - [   10 B] Резерв хвоста
;   FC00-FEFF - [  768 B] MEM.SYS: транспорт и контексты
;   FF00-FFBF - [  192 B] Защищённый хвост копировщика и резерв
;   FFC0-FFDF - [   32 B] Общее аппаратное окно команд/переноса данных
;   FFE0-FFFF - [   32 B] Регистры оборудования
;
; Для верхней программы RF мост RUN временно находится в AB00h экрана,
; поскольку загружаемое тело может перекрывать область шрифта.
; В STD верхние 4 КБ (F000-FFFF) закрыты портами. BIOS, стек и шрифт доступны;
; файловые/банковые службы IO.SYS недоступны; шрифт и весь кэш доступны.
;
; Карта 2: физический банк ДОЗУ 0 / DOS, FAR-код 1.
; Bootstrap загружает DOS.SYS в этот банк до запуска BIOS.
;   0000-00FF - [  256 B] Резерв векторов/обработчиков, совместимый с моделью CP/M
;   0100-6982 - [26755 B] Плотный код и инициализированные данные DOS (kernelEnd)
;   6983-6FFF - [ 1661 B] Резерв роста DOS перед рабочими объектами
;   7000-7055 - [   86 B] Локальная копия запроса файла/копирования
;   7100-717F - [  128 B] Путь клиента или снимок запроса для callback
;   7200-747F - [  640 B] Реестр физических драйверов, 8 записей
;   7500-773F - [  576 B] Подготовленный образ активной службы H:
;   7800-7BFF - [ 1024 B] Резидентный слот DOS-DLL, отдельно от установщиков
;   7C00-7C81 - [  130 B] Локальная командная строка DOS
;   7C82-7CFF - [  126 B] Резерв
;   7D00-7DBF - [  192 B] Реестр выделенных FAR-блоков
;   7DC0-7E0F - [   80 B] Реестр восьми загруженных модулей DOS
;   7E10-7EFF - [  240 B] Свободная рабочая область DOS
;   7F00-8EFF - [ 4096 B] Пачка файлового обмена или списка каталога
;   8F00-8FDE - [  223 B] Свободная рабочая область DOS
;   8FDF-8FFF - [   33 B] Локальный резерв совместимых переменных; diskWriteErr=8FF9h
;   9000-AFFF - [ 8192 B] Единый пул кэша FAT/каталогов всех дисков
;   B000-B1FF - [  512 B] 64 описателя гранул кэша по 128 байт
;   B200-B207 - [    8 B] Описание отдельного неполного файлового сектора
;   B208-B3FF - [  504 B] Резерв
;   B400-B5FF - [  512 B] Отдельный неполный файловый сектор, с меткой диска
;   B600-B6FF - [  256 B] Буфер проверяющего чтения физического сектора
;   B700-B7FF - [  256 B] Резерв буферов DOS
;   B800-B9FF - [  512 B] Независимый буфер чтения BPB и заголовка FAT
;   BA00-BFFF - [ 1536 B] Резерв кэша и будущих буферов DOS
;   C000-D7FF - [ 6144 B] Общая арена DRIVER-инициализатора или одной DOS-DLL
;   D800-EFFF - [ 6144 B] Восемь кадров BAT: контекст, строка, буфер чтения 512 Б
;   F000-F3FF - [ 1024 B] Служебный стек DOS; начальный SP=F400h
;   F400-F47F - [  128 B] MEM.SYS: регистровые GET32/PUT32
;   F480-F73F - [  704 B] MEM.SYS: операции памяти и текстовый шлюз
;   F740-F7FF - [  192 B] MEM.SYS: локальные адаптеры/контекст и резерв, как в main
;   F800-F8BF - [  192 B] MEM.SYS: общие публичные входы CLIENT и резерв
;   F8C0-F9FF - [  320 B] Резерв DOS; в main здесь FLASHSYS
;   FA00-FB9F - [  416 B] Один активный слот физических служб H:, как в main
;   FBA0-FBF5 - [   86 B] Снимок COPY
;   FBF6-FBFF - [   10 B] Резерв хвоста
;   FC00-FEFF - [  768 B] MEM.SYS: транспорт, контексты и общий копировщик
;   FF00-FFBF - [  192 B] Копия хвоста копировщика и резерв
;   FFC0-FFDF - [   32 B] То же физическое 32-байтное окно, что в основной памяти
;   FFE0-FFFF - [   32 B] Регистры оборудования, доступны из банка DOS
; Банки данных: 0000-FEFF — [65280 B] данные, FF00-FF9B — [156 B] сигнатура и хвост MEM,
; FF9C-FFBF — [36 B] резерв; окно/регистры FFC0-FFFF — [64 B].
; Последний физический сектор каждого банка данных помечается BAD в FAT.
; Банки RAM-диска начинаются с физического банка 1, исключая банк DOS.
; Константы обеих карт находятся ниже в разделе буферов; это не номера ABI.
;-----------------------------------------------------------------------


;-----------------------------------------------------------------------
; Конфигурация сборки
;-----------------------------------------------------------------------


    DEFINE DOS_SPLIT_BUILD 1
BIOS_DOS_SPLIT       = 1       ; BIOS C000h в main, DOS 0100h в физической странице 0
BIOS_VERSION_MAJOR  = 6
BIOS_VERSION_MINOR  = 0
DOS_VERSION_MAJOR   = 2
DOS_VERSION_MINOR   = 0
; Назначение банка: данные получают только FF00h, DOS — полный исполняемый хвост.
MEM_BANK_DATA = 0
MEM_BANK_EXEC = 1
MEM_BANK_DOS = 2


MXOS_GROUP_BDOS       = 0
MXOS_GROUP_MEMORY     = 3
; Публичный memcpy: банки 0=main, 1..16=ДОЗУ; слова little-endian.
; Нулевой размер — успех, предел диапазонов FF00h, перекрытие запрещено.
; Пакет, код и стек инициатора не должны попадать в область назначения.
MEM_ABI_VERSION = 00101h       ; Публичный ABI MEM 1.1
MEM_RUNTIME_REVISION = 22      ; Повысить при несовместимом изменении внутреннего ABI
MEM_WINDOW_VERSION = 2         ; Внутренний протокол физического окна
MEM_CAP_COPY = 1
MEM_CAP_LAZY_WINDOW = 2
MEM_CAP_MEMORY_OPS = 4
MEM_CAP_MODULES = 8
MEM_OP_MOVE = 0
MEM_OP_GET8 = 1
MEM_OP_GET16 = 2
MEM_OP_PUT8 = 3
MEM_OP_PUT16 = 4
MEM_OP_PRINT = 5
MEM_OP_ALLOC = 6
MEM_OP_FREE = 7
MEM_OP_REALLOC = 8
MEM_OP_ARENA = 9
MEM_OP_RAMLIMIT = 10
MEM_OP_FILL = 11
MEM_OP_QUERY = 12
MEM_OP_DISKCLAIM = 13
MEM_OP_RAMDISK = 14
MEM_RAMDISK_STATUS = 0
MEM_RAMDISK_COMMIT = 1
RAMDISK_SERVICE_KEY = 04452h ; Исторический ключ RAMDSVC; штатный B: принадлежит DOS.
MEM_CAP_ALLOCATOR = 16
MEM_CAP_DWORD = 32
RESIDENT_RETAIN = 0
RESIDENT_FIND = 1
RESIDENT_ENUM = 2
APP_MAX_DEPTH = 8
APP_OWNER_BASE = 020h
MEM_ALLOC_ANY = 0FFh
MEM_ALLOC_RECORDS = 32
MEM_ERROR_NOMEM = 0E3h
MEM_ERROR_OWNER = 0E4h
MEM_ERROR_BANK = 0E0h
MEM_ERROR_STACK = 0E1h
MEM_ERROR_RANGE = 0E2h
MEM_ERROR_HANDLE = 0E5h
MEM_ERROR_ACTIVE = 0E6h
MODULE_TARGET_MAIN = 0
MODULE_TARGET_BANK = 1
MODULE_TARGET_DOS = 2 ; DLL в резидентном слоте DOS или временной арене DRIVER
MODULE_MAX = 8
MODULE_RECORD_SIZE = 10
MODULE_STACK_SIZE = 1024
MODULE_OWNER_BASE = 080h
    STRUCT MEM_COPY_REQUEST
sourceBank DB 0
sourceOffset DW 0
destinationBank DB 0
destinationOffset DW 0
count DW 0
    ENDS
; TEXT_PRINT: один банковый запрос копирования и печати (HL=NEAR пакет).
TEXT_PRINT_Z = 0
TEXT_PRINT_RAW_Z = 1
TEXT_PRINT_RAW_LIM = 2
TEXT_PRINT_SHARED = 1
TEXT_PRINT_MAIN_BUFFER = 2
    STRUCT TEXT_PRINT_REQUEST
kind DB 0
method DB 0
source DW 0
count DW 0
buffer DW 0
capacity DW 0
    ENDS
    INCLUDE "syscall-ids.inc"
; Пакет родных групп: порядок совпадает с PUSH AF/BC/DE/HL на 8080.
    STRUCT MX_REGS
hl DW 0
de DW 0
bc DW 0
af DW 0
    ENDS
; Вызов с регистрами исполнителя. Код операции расположен после CALL.
    MACRO MXOS_CALL operation
        call mx_callRegs
        DW operation
    ENDM
    IFNDEF DISK_DRIVER_CHECKED
DISK_DRIVER_CHECKED =  1       ; Встроенный ROM/RAM-драйвер: CF и проверка аргументов
    ENDIF
    ASSERT DISK_DRIVER_CHECKED == 0 || DISK_DRIVER_CHECKED == 1
RAMD_MAX_PAGE        =  16      ; Максимальное количество страниц RAM диска
RAMD_PAGE_END        =  0FFBEh  ; Тестовый байт вне кода копировщика
ROM_64K              =  1       ; Включить поддержку ПЗУ 64 Кб Специалиста МХ2
ENABLE_COLOR         =  1       ; Включить поддержку цвета
COLOR_BIOS           =  0F0h    ; Цвет командной строки
RAMFOS_COMPATIBILITY =  1       ; Совместимость с RAMFOS (WIP)

FONT_ADDR = 0E400h ; Полный построчный шрифт, восемь страниц по 256 байт
FONT_BYTES = 00800h
FONT_PRERENDER_BYTES = 02800h ; Три сдвинутые фазы 6x8 для 256 кодов
FONT_RENDER_QUERY = 0
FONT_RENDER_BUILD = 1
FONT_RENDER_DISABLE = 2
;-----------------------------------------------------------------------
; Переменные ОС в подэкранном пространстве и их начальные значения
; Доступ к переменным из программ польователя:
; ld a, bios_vars.lastKey
;-----------------------------------------------------------------------

    STRUCT BIOS_VARIABLES
keyboardHook    DW      -1              ; 8FDFh — обработчик клавиатуры Монитора-2
tapeError       DW      0C800h          ; 8FE1h - Адрес, куда происходит переход при ошибке чтения с ленты
tapeAddr        DW      -1              ; 8FE3h - Адрес программы, загруженной с ленты
inputHook       DW      -1              ; 8FE5h — обработчик строки Монитора-2
monitorFont     DW      018A0h          ; 8FE7h — исторический адрес шрифта / 8
currentChar     DB       0
cursorVisible   DB       0
inputBusy       DB       0
keyLocks        DB      0BAh ; 3Ah      ; 8FECh - Caps Lock или Рус/Lat (3A - прописные анг, BA - строчные анг, BB - прописные рус, 3B - строчные рус)
keyHandler      DW      -1              ; 8FEDh -
lastLastKey     DB      -1              ; 8FEFh - Предпоследняя нажатая клавиша
lastKey         DB      -1              ; 8FF0h - Последняя нажатая клавиша
beepFreq        DB       5Fh            ; 8FF1h - Частота звукового сигнала (также влияет на длительность)
beepDuration    DB       20h            ; 8FF2h - Длительность звукового сигнала (сколько раз повторять посылку)
tapeInverse     DB       0FFh           ; 8FF3h - Признак инверсии данных с ленты
tapeWriteCode0  DB       20h
tapeWriteCode1  DB       0E0h
oldSP           DW      -1              ; 8FF6h - Используется для сохранения SP некоторыми функциями
screenAddressLo DB       0
screenAddressHi DB       0
inverse         DW       0              ; 8FFAh - Инвертирование текста (0=нормальный текст, 0FFFFh=инверсный)
cursorY         DB      -1              ; 8FFCh - Положение курсора по вертикали в пикселях
cursorX         DB      -1              ; 8FFDh - Положение курсора по горизонтали в пикселях / 2
writeDelay      DB      28h             ; 8FFEh - Скорость при записи на ленту
readDelay       DB      3Ch             ; 8FFFh - Скорость при чтении с ленты
    ENDS

; Верхние системные переменные BIOS основной памяти, EE90h-EF9Fh.
; DISK_INFO объявлена ниже; размер поля проверяется после её определения.
    STRUCT BIOS_UPPER_VARIABLES
commandLine   BLOCK 130 ; Буфер редактора NC/CLI и совместимого ABI командной строки
diskInfo      BLOCK 48  ; Локальный результат запроса геометрии физического драйвера
driverInfo    BLOCK 64  ; Локальная копия имени/описания из реестра DOS
rawLease      BLOCK 6   ; Разрешение прямого RAW и последнее состояние ошибки
cacheLimit    DW 0      ; Исключающая верхняя граница кэша глифов
cacheEntries  DW 0      ; Число действующих записей кэша
cacheEpoch    DB 0      ; Счётчик инвалидирования
cacheStart    DW 0      ; Первый байт после загруженного файла шрифта
cacheInvalid  DB 1      ; 1 — индекс/битмапы не действуют
charGen       DW FONT_ADDR ; Физический адрес RAM-шрифта
cursorCfg     DB 021h
koi7          DB 0 ; Только чтение; менять через bios_setFontEncoding / MXOS_BIOS_SET_FONT_ENCODING
escMode       DB 0FFh
cursorDelay   DB 020h
antiBsDelay   DB 0E0h
ramPageCount  DB RAMD_MAX_PAGE
diskWriteErr  DB 0
padding       BLOCK 7
    ENDS
bios_upper BIOS_UPPER_VARIABLES = 0EE90h

; Плотные рабочие переменные IO.SYS, отдельно от кода и общего буфера текста.
; Банк DOS не должен хранить NEAR-указатели на эти поля как свои локальные.
    STRUCT IO_VARIABLES
bbBusy          DB 0
bbFast          DB 0
bbPage          DB 0
bbFunction      DB 0
bbMode          DB 0
bbPacketAddress DW 0
bbSource        DW 0
bbChunk         DW 0
bbScanLeft      DW 0
bbMore          DB 0
bbRegs          BLOCK 8
bbCopy          BLOCK 8
mainIoRequest   BLOCK 5
mainIoCopy      BLOCK 8
fbRequest       DW 0
fbEnd           DW 0
fbCursor        BLOCK 4
romPage         DB 0
romBufferPage   DB 0 ; FAR-банк буфера физического A:
romBlocksHi     DB 0 ; Старший байт ёмкости A: для размера страницы AT29C
romSource       DW 0
romTarget       DW 0
romChunk        DW 0
romCheck        DB 0
romSavedSP      DW 0
romLoops        DB 0
romSavedWindow  BLOCK 32
    ENDS
io_vars IO_VARIABLES = 0EFA0h
    ASSERT io_vars + IO_VARIABLES <= 0F000h

; Блок переменных BIOS начинается с адреса 8FDFh
bios_vars   BIOS_VARIABLES = 8FDFh

;-----------------------------------------------------------------------
; Стандартные точки входа ОС
; Отмеченные [OLD] применять не рекомендуется
; Отмеченные [MXOS2] - новые в MXOS 2
;-----------------------------------------------------------------------

; Локальная таблица DOS в её странице; приложения используют общий MXOS_CALL.
    IFDEF DOS_REFERENCE_BUILD
DOS_ENTRY_BASE = 04000h ; Только линковка BIOS и CPU-стенды, не штатная DOS.
    ELSE
DOS_ENTRY_BASE = 00100h
    ENDIF
dos_dosBoot                = DOS_ENTRY_BASE+000h
dos_fileList               = DOS_ENTRY_BASE+003h
dos_fileGetSetDrive        = DOS_ENTRY_BASE+006h
dos_fileCreate             = DOS_ENTRY_BASE+009h
dos_fileLoad               = DOS_ENTRY_BASE+00Ch
dos_fileDelete             = DOS_ENTRY_BASE+00Fh
dos_fileRename             = DOS_ENTRY_BASE+012h
dos_fileLoadInfo           = DOS_ENTRY_BASE+015h
dos_imageOpen              = DOS_ENTRY_BASE+018h
dos_fileGetSetAttr         = DOS_ENTRY_BASE+01Bh
dos_fileNamePrepare        = DOS_ENTRY_BASE+01Eh
dos_fileExec               = DOS_ENTRY_BASE+021h
dos_fileLoad2              = DOS_ENTRY_BASE+024h
dos_getDriveFreeSpace      = DOS_ENTRY_BASE+027h
dos_fileCreatePrepare      = DOS_ENTRY_BASE+02Ah
dos_dosUnsupported         = DOS_ENTRY_BASE+02Dh
dos_installVolume          = DOS_ENTRY_BASE+030h
dos_getVolume              = DOS_ENTRY_BASE+033h
dos_mountVolume            = DOS_ENTRY_BASE+036h
dos_flushVolume            = DOS_ENTRY_BASE+039h
dos_fileAssociation        = DOS_ENTRY_BASE+03Ch
dos_fileChangeDrive        = DOS_ENTRY_BASE+03Fh
dos_fileCopy               = DOS_ENTRY_BASE+042h
dos_fileGetDrives          = DOS_ENTRY_BASE+045h
dos_dosGetError            = DOS_ENTRY_BASE+048h
dos_logicalSectorIO        = DOS_ENTRY_BASE+04Bh
dos_fileStream             = DOS_ENTRY_BASE+04Eh
dos_fileHandle             = DOS_ENTRY_BASE+051h
dos_directoryService       = DOS_ENTRY_BASE+054h
dos_cliService             = DOS_ENTRY_BASE+057h
dos_setShell               = DOS_ENTRY_BASE+05Ah

bios_init               = 0C000h ; Инициализация из bootstrap / программный возврат в DOS

; Устаревшие точки, лечше не применять
bios_keyScanOld         = 0C003h ; [OLD] Получить код нажатой клавиши, = bios_keyScan
bios_drawCursorOld      = 0C006h ; [OLD] Нарисовать/стереть курсор
bios_restartOld         = 0C009h ; [OLD] Тёплая перезагрузка, устанавливает системный стек
bios_clearScreenOld     = 0C010h ; [OLD] Оистка экрана
bios_printCharOld       = 0C037h ; [OLD] Вывод символа на экран
bios_beep_Old           = 0C170h ; [OLD] Звуковой сигнал
bios_delay_l            = 0C18Fh ; [OLD] Задержка l циклов
bios_delay_b            = 0C190h ; [OLD] Задержка b циклов
bios_setKeybMode82      = 0C254h ; [OLD] Загрузчик: A=82h, флаги/BC/DE/HL сохраняются
bios_setKeybMode91      = 0C25Ah ; [OLD] Загрузчик: A=91h, флаги/BC/DE/HL сохраняются
bios_getchOld           = 0C337h ; [OLD] Ожидание ввода с клавиатуры
bios_tapeReadOld        = 0C377h ; [OLD] Чтение байта с магнитофона
bios_tapeWriteOld       = 0C3D0h ; [OLD] Запись байта на магнитофон
bios_cmp_hl_de          = 0C427h ; [OLD] Сравнить hl и de
bios_memcpy_bc_hl       = 0C42Dh ; [OLD] HL..DE исключительно -> BC, с учётом перекрытия
bios_printStringOld     = 0C438h ; [OLD] Вывод строки на экран

bios_keybTable          = 0C4A0h ; [OLD] Матрица клавиатуры Загрузчика, 96 байт до C4FFh

; Стандартные точки C800h
bios_reboot             = 0C800h ; Программный возврат в DOS и выбранную оболочку, без bootstrap
bios_getch              = 0C803h ; Ожидание ввода с клавиатуры
bios_tapeRead           = 0C806h ; Чтение байта с магнитофона
bios_printChar          = 0C809h ; Вывод символа на экран
bios_tapeWrite          = 0C80Ch ; Запись байта на магнитофон
bios_input              = 0C80Fh ; Ввод строки с клавиатуры
bios_keyCheck           = 0C812h ; Получить код нажатой клавиши, = bios_keyScan
bios_printHexByte       = 0C815h ; Вывод HEX числа на экран (байт)
bios_printStringZ       = 0C818h ; HL=ASCIIZ; управляющие коды обрабатываются
bios_printString        = 0C818h ; Вывод строки на экран
bios_keyScan            = 0C81Bh ; Получить код нажатой клавиши
bios_getCursorPos       = 0C81Eh ; Получить координаты курсора в hl (координаты в пикселях)
bios_setCursorPos       = 0C821h ; Установить координаты курсора из hl (координаты в пикселях)
bios_tapeLoad           = 0C824h ; Загрузить программу с магнитофона
bios_tapeSave           = 0C827h ; Сохранить программу на магнитофон
bios_calcCS             = 0C82Ah ; Расчет контрольной суммы
bios_printHexWord       = 0C82Dh ; Вывод HEX числа на экран (слово)
bios_setMemTop          = 0C830h ; Monitor-2: установить границу из HL; регистры/флаги сохранены
bios_getMemTop          = 0C833h ; Monitor-2: HL=граница памяти; AF/BC/DE и флаги сохранены
bios_printer            = 0C836h ; Напечатать байт на принтере
    IF RAMFOS_COMPATIBILITY
bios_strToHex           = 0C839h ; [MXOS2] Преобразвоние строки в HEX формате в число
    ELSE
bios_rebootAlt          = 0C839h ; Программный возврат в выбранную оболочку
    ENDIF
bios_rebootAlt2         = 0C83Ch ; Программный возврат в выбранную оболочку
bios_fileList           = 0C83Fh ; Получить список файлов
bios_fileGetSetDrive    = 0C842h ; Получить/установить активное устройство
bios_fileCreate         = 0C845h ; HL=дескриптор с точным N, DE=исходный буфер
bios_fileLoad           = 0C848h ; DE=назначение; читать подготовленное тело по месту, CF=ошибка
bios_fileDelete         = 0C84Bh ; Удалить файл
bios_fileRename         = 0C84Eh ; Переименовать файл
bios_fileLoadInfo       = 0C851h ; Загрузить информацию о файле
bios_imageOpen          = 0C854h ; HL=дескриптор, открыть родной COM/EXE MXOS
bios_fileGetSetAttr     = 0C857h ; HL=имя 8+3, C=0 чтение / 1 запись B (R/H/S/A), DIR сохраняется; A=атрибуты, CF=ошибка
bios_fileNamePrepare    = 0C85Ah ; Преобразовать имя файла во внутренний формат
bios_fileExec           = 0C85Dh ; Запустить файл; BAT затирает ОЗУ ниже 7000h
bios_installDriver      = 0C860h ; Установить драйвер накопителя
bios_diskDriver         = 0C863h ; Драйвер выбранного диска

bios_fileLoad2          = 0C866h ; Загрузить файл по адресу de

; Новые точки C800h MXOS2
bios_printCharRaw       = 0C869h ; [MXOS2] A=код 00h-FFh; AF/BC/DE/HL сохранены
bios_printCharReal      = 0C869h ; [MXOS2] Вывод символа на экран (только реальные символы)
bios_printDecWord       = 0C86Ch ; [MXOS2] Вывод числа экран в десятичной форме
bios_setRAMDDriver      = 0C86Fh ; [MXOS2] B=RAMD_DRIVER_*, выбор 32-байтного окна FFC0h
bios_getDriveFreeSpace  = 0C872h ; [MXOS2] Получить оставшийся свободный объем диска
bios_upperCase          = 0C875h ; [MXOS2] Перевод кода символа КОИ-8 в верхний регистр
bios_strcmp             = 0C878h ; [MXOS2] Сравнение строк, чувствительное к регистру
bios_stricmp            = 0C87Bh ; [MXOS2] Сравнение строк, нечувствительное к регистру
bios_sub_hl_de          = 0C87Eh ; [MXOS2] Вычитание hl и de: hl = hl - de
bios_fileCreatePrepare  = 0C88Ah ; [MXOS2] Создать файл и выделить цепочку FAT без записи данных
bios_installVolume      = 0C890h ; [MXOS2] Зарегистрировать драйвер файловой системы тома
bios_getVolume          = 0C893h ; [MXOS2] Получить таблицу и контекст драйвера тома
bios_mountVolume        = 0C896h ; [MXOS2] Повторно смонтировать выбранный том
bios_flushVolume        = 0C899h ; [MXOS2] Сохранить изменения выбранного тома
bios_beepTimer          = 0C89Ch ; [MXOS2] Звуковой сигнал через ВИ53, ABI RAMFOS beep
bios_diskWriteChecked   = 0C89Fh ; [MXOS2] Общий B=5: BC=адрес драйвера, DE=сектор, HL=данные
bios_setFontEncoding    = 0C8A2h ; [MXOS2] A=0 KOI8, иначе KOI7; перестановка шрифта/пререндера, регистры сохранены
bios_fileAssociation    = 0C8B4h ; [MXOS2] HL=расширение, DE/BC=буфер/ёмкость; CF=0 HL=команда, диск сохранён
bios_argNext            = 0C8B7h ; [MXOS2] HL=курсор -> HL=разделитель, DE=слово, B=длина; CF=конец, C сохранён
bios_fileSelectDrive    = 0C8BAh ; [MXOS2] A=диск 0..7; BC/DE/HL сохранены, CF=ошибка
bios_fileRenameOnDrive  = 0C8BDh ; [MXOS2] HL/DE=старое/новое компактное имя на текущем диске
bios_fileDeleteOnDrive  = 0C8C0h ; [MXOS2] HL=компактное имя на текущем диске, CF=ошибка
bios_fileCopy           = 0C8C3h ; [MXOS2] HL=FILE_COPY_REQUEST, CF/A=результат; диск восстановлен
bios_fileGetDrives      = 0C8C6h ; [MXOS2] HL/BC=буфер/ёмкость в байтах (>=8); B=количество
bios_getError           = 0C8C9h ; [MXOS2] A=код DOS, CF=ошибка; BC/DE/HL сохранены
bios_installBlockDriver = 0C8CCh ; [MXOS2] A=диск, HL=драйвер, C=log2 физического блока (7..9)
bios_sectorIO           = 0C8CFh ; [MXOS2] B=1/2, DE=логический сектор, HL=буфер; BC/DE/HL сохранены
bios_fileStream         = 0C8D2h ; [MXOS2] A=1/2, DE=FILE_STREAM_CURSOR, HL=буфер, BC=байты
bios_fileHandle         = 0C8D5h ; [MXOS2] A=FILE_OP_*, HL=FILE_HANDLE; CF/A=код DOS
bios_directory          = 0C8DEh ; [MXOS2] A=DIR_OP_*, пути и буферы принадлежат вызывающему
bios_cli                = 0C8E1h ; [MXOS2] A=CLI_OP_*, общий обработчик CLI/NC/BAT
bios_setShell           = 0C8E4h ; [MXOS2] A=0 NC, 1 CLI/сохранить диск, 2 получить диск CLI
bios_setInputHook       = 0C8E7h ; [MXOS2] HL=callback/0, возврат HL=прежний; DE/AF разрушены
bios_blockIO            = 0C8EAh ; [MXOS2] A=диск, B=1/2, DE=физический блок, HL=буфер; BC/DE/HL сохранены
bios_printStringRawZ    = 0C8EDh ; [MXOS2] HL=ASCIIZ; 01h-FFh рисуются как глифы
bios_printStringReal    = 0C8EDh ; [MXOS2] HL=ASCIIZ main; AF/BC/DE сохранены, HL=нуль; 01h–FFh — глифы
bios_printStringRawLim  = 0C8F0h ; [MXOS2] HL=байты, BC=длина; AF/BC/DE сохранены, HL=конец
bios_textDispatch       = 0C8F3h ; [MXOS2] Внутренний адаптер MEM TEXT

;-----------------------------------------------------------------------
; Адрес таблицы переходов драйвера магнитофона
;-----------------------------------------------------------------------

TAPE_DRIVER_JUMPS = bios_tapeReadOld    ; старая таблица переходов магнитофона, сохранена для ABI

; Одинаковые публичные входы MEM в main и исполняемых банках.
mx_get32          = 0F7A8h ; A=банк, HL=адрес; BC:DE=DWORD (DE младшее), HL сохранён
mx_put32          = 0F7ABh ; A=банк, HL=адрес, BC:DE=DWORD; BC/DE/HL сохранены
mx_call           = 0F800h ; B=группа, C=номер, DE=параметр/пакет MX_REGS
mx_cpmCall        = 0F803h ; Резерв для будущего адаптера CP/M; пока UNSUPPORTED
mx_callRegs       = 0F806h ; CALL + DW операции; регистры как у исполнителя
bios_fileLoadPath = 0F809h ; HL=FILE_PATH_IO, путь/буфер/вместимость заданы явно
bios_fileSavePath = 0F80Ch ; HL=FILE_PATH_IO, путь/буфер/точная длина заданы явно
mx_memcpyFar      = 0F80Fh ; HL=MEM_COPY_REQUEST; CF/A=статус, BC/DE/HL сохранены
mx_memInfo        = 0F812h ; HL=версия, DE=возможности, A=FAR-код, B=банки, C=тип; CF=0
mx_memory         = 0F815h ; A=MEM_OP_*, регистры/ошибки: source/mem/api.md

;-----------------------------------------------------------------------
; Непереключаемое ОЗУ ПК "Специалист-MX" (0FFC0h-0FFDF, 32 байта)
;-----------------------------------------------------------------------

bios_RAMDRead   = 0FFC0h ; низкоуровневое чтение: только BYTE/BLOCK либо ABI конкретного USER
bios_RAMDWrite  = 0FFD0h ; низкоуровневая запись: только BYTE/BLOCK либо ABI конкретного USER

;-----------------------------------------------------------------------
; Порты устройств ПК "Специалист-MX2" (0FFE0h-0FFFF, 32 байта)
;-----------------------------------------------------------------------

IO_KEYB_A       =  0FFE0h   ; ВВ55 клавиатуры
IO_KEYB_B       =  0FFE1h
IO_KEYB_C       =  0FFE2h
IO_KEYB_MODE    =  0FFE3h
IO_PROG_A       =  0FFE4h   ; ВВ55 программатора/флеш диска
IO_PROG_B       =  0FFE5h
IO_PROG_C       =  0FFE6h
IO_PROG_MODE    =  0FFE7h
IO_FDD_CMD      =  0FFE8h   ; контроллер дисковода ВГ93
IO_FDD_TRACK    =  0FFE9h
IO_FDD_SECTOR   =  0FFEAh
IO_FDD_DATA     =  0FFEBh
IO_TIMER_CH0    =  0FFECh   ; таймер ВИ53
IO_TIMER_CH1    =  0FFEDh
IO_TIMER_CH2    =  0FFEEh
IO_TIMER_MODE   =  0FFEFh
IO_FDD_REQ      =  0FFF0h   ; контроллер дисковода
IO_FDD_MOTOR    =  0FFF1h
IO_FDD_SIDE     =  0FFF2h
IO_FDD_DRIVE    =  0FFF3h
IO_COLOR        =  0FFF8h   ; регистр цвета
IO_PAGE_RAM     =  0FFFCh   ; порт включения основного ОЗУ
IO_PAGE_RAMD    =  0FFFDh   ; порт включения ОЗУ RAM-дисков
IO_PAGE_ROM     =  0FFFEh   ; порт включения ПЗУ/ROM-диска
IO_PAGE_STD     =  0FFFFh   ; порт включения STD режима

;-----------------------------------------------------------------------
; Выбор общего окна RAM-диска: bios_setRAMDDriver/0C86Fh.
; Все регистры и флаги сохраняются. BYTE/BLOCK временно заменяют окно,
; не меняя зарегистрированный шаблон; RESTORE/USER возвращают его.
; До регистрации RESTORE/USER выбирают встроенный блочный драйвер.
; REGISTER: HL=резидентный шаблон ровно 32 байта; окно пока не меняется.
; После сервисных обращений перед секторным I/O требуется RESTORE.
; BYTE: A=страница, HL=адрес, C=байт. BLOCK: C=страница, DE=сектор<<8,
; HL=основная RAM; блок ровно 256 байт, C сохранён, AF/B/DE/HL разрушены.
; USER: раскладка окна принадлежит зарегистрированному физическому драйверу.
; Входы FFC0h/FFD0h для USER не гарантируются; секторный I/O — через C863h.
; Регистрация физического драйвера B: через bios_installDriver независима.
RAMD_DRIVER_RESTORE    = 0
RAMD_DRIVER_BYTE       = 1
RAMD_DRIVER_REGISTER   = 2
RAMD_DRIVER_BLOCK      = 3
RAMD_DRIVER_USER       = 4

;-----------------------------------------------------------------------
; Операции дискового обмена: физический ABI и DOS RAW_IO
;-----------------------------------------------------------------------
; Номер функции передаётся в регистре b:
;   B = 1    - записать  сектор (256 байт). DE = номер сектора, HL = буфер.
;   B = 2    - прочитать сектор (256 байт). DE = номер сектора, HL = буфер.
; В банковой DOS функции 3/FFh обслуживает реестр. DISK_INFO содержит
; смонтированную BPB; физические параметры и строки хранятся в реестре DOS.
;   B = 3    - информация о диске. DE = размер диска, HL = DISK_INFO,
;              BC = шаг выделения по чипам (0: линейно). Ненулевой шаг
;              также объявляет поддержку B=4: дождаться отложенных записей.
;   B = 4    - дождаться завершения всех отложенных операций записи.
;   B = 5    - записать сектор (256 байт) с чтением и сравнением.
;              DE = номер сектора, HL = буфер, CF=ошибка.
;              EXEC: 320 свободных байт стека; DOS: DOS_VERIFY_BUFFER.
;              BC/DE/HL сохранены.
;   B = 0FFh - физический BIOS/A: B=DISK_TECH_*, C=DISK_CAP_*, HL=0,
;              CF=0 при успехе; AF разрушается.
; Имена/описания не входят в физический ABI. Получение строк выбранного диска:
;   LD B,0FFh / MXOS_CALL MXOS_DOS_RAW_IO -> CF=0, B=технология, C=возможности,
;   HL=NEAR-адрес двух ASCIIZ: имя,0,описание,0, вместе <= 64 байт.
; Источник — реестр DOS; межбанковому клиенту возвращается локальная копия.
; Указатель результата не является постоянным: строку нужно скопировать,
; если она требуется после следующих RAW_IO/замены драйвера. CF=1 — ошибка.
DISK_TECH_ROM          = 'O'
DISK_TECH_RAM          = 'R'
DISK_TECH_FLASH        = 'F'
DISK_TECH_FLOPPY       = 'D' ; Зарезервировано для дискетного драйвера
DISK_TECH_HDD          = 'H' ; Зарезервировано для драйвера жёсткого диска
DISK_CAP_READ          = 01h
DISK_CAP_WRITE         = 02h
DISK_CAP_VERIFY        = 04h    ; B=5: синхронная запись с чтением и сравнением
DISK_CAP_FLUSH         = 08h    ; B=4: завершение отложенной записи
RAW_OP_PREPARE        = 6      ; MXOS_DOS_RAW_IO: подготовить локальные точки обмена

; Установка переносимой службы из инициализатора COM в странице DOS.
; Версия 1 обслуживает слот H:. Все указатели относятся к загруженному
; инициализатору C000h–D7FFh; DOS копирует код и строки до его освобождения.
DRIVER_INSTALL_VERSION = 1
DRIVER_RECORD_SIZE = 80
    STRUCT DRIVER_INSTALL
version             DB 1
drive               DB 7
kind                DB 0       ; 0 ROM, 1 FLASH, 2 FLASHFST
blockShift          DB 8
blocks              DW 0
allocationStep      DW 0
capabilities        DB 0
technology          DB 0
image               DW 0       ; код собран для LOCAL_DRIVER_ADDR
imageSize           DW 0
text                DW 0       ; две ASCIIZ-строки, вместе не более 64 байт
    ENDS

; ABI 1 драйвера тома. Аппаратный ABI bios_diskDriver остаётся отдельным.
; Таблица начинается с DB "FS",1,15; далее 15 адресов DW в этом порядке.
; Файловые операции пока сохраняют исторические контракты DOS (8.3, точный размер N).
; Это ещё не независимый байтовый поток и не готовый драйвер RAMFOS.
VOLUME_DRIVER_VERSION = 1
VOLUME_DRIVER_COUNT   = 15
VOLUME_DRIVER_HEADER  = 4
VOL_MOUNT             = 0
VOL_LIST              = 2
VOL_CREATE            = 4
VOL_LOAD              = 6
VOL_DELETE            = 8
VOL_RENAME            = 10
VOL_STAT              = 12
VOL_LOAD_ADDRESS      = 14  ; Зарезервирован: адреса перенесены из FAT в контейнер
VOL_ATTRIBUTES        = 16
VOL_LOAD_AT           = 18
VOL_FREE_SPACE        = 20
VOL_PREPARE_NAME      = 22
VOL_CREATE_PREPARE    = 24
VOL_RESERVED_26       = 26
VOL_FLUSH             = 28

;-----------------------------------------------------------------------
; Структура файловой системы
;-----------------------------------------------------------------------

; Структура каталога
IMAGE_HEADER_SIZE  = 16
IMAGE_FLAG_DRIVER = 01h ; байт 5 COM/EXE: инициализатор в странице DOS
IMAGE_FLAG_BANK = 02h   ; требуется отдельный исполняемый банк
IMAGE_FLAG_MAIN = 04h   ; явно подтверждена совместимость с main/MXOS BIOS
; Допустим ровно один признак DRIVER/BANK/MAIN либо 0 (неизвестная совместимость).
FILE_DESCR_SIZE    = 32     ; размер дескриптора файла в каталоге (байт)
FILE_NAME_LENGTH   = 8      ; длина имени файла  (байт)
MAX_PATH           = 128    ; Путь в байтах, включая завершающий 0.
DIR_OP_GET         = 0      ; DE=кластер текущего каталога; 0 означает корень.
DIR_OP_SET         = 1      ; DE=ранее полученный кластер каталога на текущем диске.
DIR_OP_CHDIR       = 2      ; HL=буфер MAX_PATH, ASCIIZ; допускается префикс диска.
DIR_OP_LIST        = 3      ; Как fileList, но вместе с каталогами (. и .. включены).
DIR_OP_GETCWD      = 4      ; HL=буфер MAX_PATH; абсолютный путь A:\..., ASCIIZ. Нужна DOS.
DIR_OP_COMMAND     = 5      ; HL=строка CD/CHDIR, A=FFh если команда не встроена. Нужна DOS.
DIR_OP_RESOLVED    = 6      ; DE=контекст последнего успешного fileNamePrepare, до выбора диска.
DIR_OP_MKDIR       = 7      ; HL=путь, создать пустой каталог.
DIR_OP_RMDIR       = 8      ; HL=путь, удалить пустой каталог.

CLI_OP_COMMAND     = 0      ; DE=обработчик совпадений (0=отказ), HL=команда; FFh/CF=0 — внешняя команда; B: вывод/выход/контекст.
CLI_OP_COPY        = 1      ; HL=FILE_COPY_REQUEST; буфер данных принадлежит DOS.
CLI_OP_RENAME      = 2      ; HL/DE=имена на текущем диске и в текущем каталоге.
CLI_OP_DELETE      = 3      ; HL=имя файла; каталоги удаляются отдельным RMDIR.
CLI_OP_MKDIR       = 4      ; HL=путь MAX_PATH.
CLI_OP_CHDIR       = 5      ; HL=путь MAX_PATH.
CLI_OP_RMDIR       = 6      ; HL=путь MAX_PATH.
CLI_OP_MOVE        = 7      ; HL=FILE_COPY_REQUEST; на одном томе перенос записи без данных/FAT.
CLI_OP_QUERY       = 8      ; HL=строка без исполнения; B как COMMAND, A=FFh внешняя команда.
CLI_OP_OBSERVER    = 9      ; HL=наблюдатель изменений (0=отключить); A=диск, DE=каталог, C=масштаб.
; B после COMMAND/QUERY: bit0 — вывод, bit1 — EXIT, bit2 — CD/смена диска.

FAT_ITEM_SIZE      = 2      ; размер записи в таблице FAT (байт)

SECTOR_SIZE        = 00100h ; размер сектора 256 байт
CLUSTER_SIZE       = 00100h ; размер кластера 256 байт

; Специальные коды кластеров FAT
FAT12_BAD          = 00FF7h ; плохой кластер
FAT16_BAD          = 0FFF7h
FAT12_EOC          = 00FFFh ; последний кластер файла
FAT16_EOC          = 0FFFFh
FAT12_RES          = 00FFFh ; зарезервированный кластер
FAT16_RESERVED_MIN = 0FFF0h
FAT16_EOC_MIN      = 0FFF8h
FAT16_RES          = 0FFF0h

;-----------------------------------------------------------------------
; Общие коды ошибок DOS: результат A и bios_getError (C8C9h)
;-----------------------------------------------------------------------
; A=0, CF=0 — успех; ненулевой A, CF=1 — ошибка.
; Это коды служб DOS, не номера операций FILE_OP_* и не результат stdio EOF.
DOS_ERR_IO          = 1    ; Ошибка чтения, записи или сохранения кэша.
DOS_ERR_NOT_FOUND   = 2    ; Файл не найден.
DOS_ERR_EXISTS      = 3    ; Файл назначения уже существует.
DOS_ERR_FULL        = 4    ; На томе не хватает свободных кластеров.
DOS_ERR_DIR_FULL    = 5    ; Нет свободной записи в каталоге.
DOS_ERR_ACCESS      = 6    ; Операция запрещена атрибутами или режимом открытия.
DOS_ERR_ARGUMENT    = 7    ; Недопустимый параметр, адрес, размер или режим.
DOS_ERR_CORRUPT     = 8    ; Повреждена структура тома или цепочка кластеров.
DOS_ERR_NO_DRIVER   = 9    ; Для накопителя не установлен драйвер.
DOS_ERR_UNSUPPORTED = 10   ; Формат тома или операция не поддерживаются.
DOS_ERR_BAD_IMAGE   = 11   ; Неверный заголовок или контрольная сумма программы.

;-----------------------------------------------------------------------
; Дескриптор файла (одна запись в каталоге) FAT12/16/32
;-----------------------------------------------------------------------

    STRUCT FILE_DESCRIPTOR  ; 32 байта
name            BLOCK   FILE_NAME_LENGTH    ; смещение 00h, 8 байт  - имя файла
ext             BLOCK   3                   ; смещение 08h, 3 байта - расширение имени файла
attrib          BLOCK   1                   ; смещение 0Bh, 1 байт  - атрибуты файла
ntReserved      BLOCK   1                   ; смещение 0Ch, 1 байт  - используются в Windows NT
createTimeTenth BLOCK   1                   ; смещение 0Dh, 1 байт  - время создания файла, десятки миллисекунд (0-199)
createTime      BLOCK   2                   ; смещение 0Eh, 2 байта - время создания файла, секунды*2 (0-43200)
createDate      BLOCK   2                   ; смещение 10h, 2 байта - дата  создания файла
accessDate      BLOCK   2                   ; смещение 12h: дата последнего доступа FAT16
firstClusterHi  BLOCK   2                   ; смещение 14h, 2 байта - номер первого кластера в FAT, старшее слово
writeTime       BLOCK   2                   ; смещение 16h, 2 байта - время модификации файла, секунды*2 (0-43200)
writeDate       BLOCK   2                   ; смещение 18h, 2 байта - дата  модификации файла
firstCluster    BLOCK   2                   ; смещение 1Ah, 2 байта - номер первого кластера в FAT, младшее слово
size            BLOCK   4                   ; смещение 1Ch, 4 байта - размер файла в байтах
    ENDS

;-----------------------------------------------------------------------
; Внутренний контекст загрузки COM/EXE: IMAGE_LOAD_CONTEXT, 32 байта
;-----------------------------------------------------------------------

; Временный внутренний контекст родного загрузчика C854h/C848h.
; Он действителен до следующей файловой операции и не является записью FAT.
    STRUCT IMAGE_LOAD_CONTEXT
prefix          BLOCK 6
loadAddress     DW 0
entryAddress    DW 0
bodySize        DW 0
bodyChecksum    DW 0
headerChecksum  DW 0
skip            DB 0
_reserved       DB 0
tail            DW 0
_reserved2      BLOCK 6
nextCluster     DW 0
fileSize        DW 0
fileSizeHi      DW 0
    ENDS
    ASSERT IMAGE_LOAD_CONTEXT == FILE_DESCR_SIZE

;-----------------------------------------------------------------------
; Краткая информация о файле: FILE_INFO, 16 байт для списка каталога
;-----------------------------------------------------------------------
; Почти совместима с компактной записью оригинального Коммандера.
; Это результат перечисления файлов, не объект открытого файла для fopen.
FILE_INFO_SIZE = 16     ; размер структуры (байт)

    STRUCT FILE_INFO    ; 16 байт
name            BLOCK   FILE_NAME_LENGTH    ; смещение 00h, 8 байт  - имя файла
ext             BLOCK   3                   ; смещение 08h, 3 байта - расширение имени файла
attrib          BLOCK   1                   ; смещение 0Bh, 1 байт  - атрибуты файла
reserved        BLOCK   2                   ; два нулевых байта компактной записи, не адрес
size            BLOCK   2                   ; смещение 0Eh, младшие 16 бит точного размера файла
    ENDS

;-----------------------------------------------------------------------
; Операции над открытым файлом: bios_fileHandle (C8D5h), A=FILE_OP_*
;-----------------------------------------------------------------------
; HL указывает на FILE_HANDLE в памяти вызывающей программы.
; Параметры и результаты находятся в объекте; A/CF возвращают код DOS.
; C SDK отображает эти операции на fopen/read/write/fseek/fflush/fclose.
; Реализация находится в DOS.SYS, исполняемой в собственной странице.
FILE_OP_OPEN       = 0    ; Найти/создать файл и заполнить объект.
FILE_OP_READ       = 1    ; Читать FH_COUNT байт в FH_BUFFER.
FILE_OP_WRITE      = 2    ; Записать FH_COUNT байт из FH_BUFFER.
FILE_OP_SEEK       = 3    ; Изменить позицию по FH_SEEK/FH_ORIGIN.
FILE_OP_FLUSH      = 4    ; Сохранить размер, запись каталога и кэш.
FILE_OP_CLOSE      = 5    ; Выполнить FLUSH и закрыть объект.
FILE_OP_OPTIONS    = 6    ; FH_ORIGIN=0 get / 1 exchange, FH_COUNT=флаги DOS.
DOS_IO_VERIFY_FILES = 1   ; Проверять запись данных FILEHANDLE при DISK_CAP_VERIFY.

;-----------------------------------------------------------------------
; Флаги открытия FILE_HANDLE: битовая маска в FH_MODE
;-----------------------------------------------------------------------
; Требуется READ и/или WRITE. CREATE/TRUNCATE/APPEND/VERIFY/SCATTER требуют WRITE.
FILE_MODE_READ     = 1    ; Разрешено чтение.
FILE_MODE_WRITE    = 2    ; Разрешена запись.
FILE_MODE_CREATE   = 4    ; Создать отсутствующий файл.
FILE_MODE_TRUNCATE = 8    ; При открытии обнулить существующий файл.
FILE_MODE_VERIFY   = 32   ; Требовать B=5 для данных этого файла.
FILE_MODE_SCATTER  = 64   ; Псевдослучайный поиск новых свободных кластеров.
FILE_MODE_APPEND   = 16   ; Перед каждой записью перейти в конец файла.

;-----------------------------------------------------------------------
; Объект открытого файла FILE_HANDLE: 64 байта, аналог FILE для C SDK
;-----------------------------------------------------------------------
; Здесь объявлены смещения полей, а FILE_HANDLE — размер объекта.
; Это не запись каталога и не числовой файловый номер: память выделяет
; вызывающая программа, скрытой таблицы открытых файлов DOS не создаёт.
; Перед OPEN объект обнуляют, задают имя 8.3 (11 байт с пробелами),
; FH_DRIVE и FH_MODE. Первые 32 байта после OPEN — FILE_DESCRIPTOR.
; READ/WRITE получают FH_BUFFER и FH_COUNT, SEEK — FH_SEEK/FH_ORIGIN.
; Рабочие поля позиции/курсора/каталога ведёт DOS; объект сохраняют
; между вызовами и не перекрывают буфером обмена. Слова little-endian.
FH_POSITION        = 32   ; 4 байта: текущая позиция от начала файла.
FH_CURSOR          = 36   ; 4 байта: FILE_STREAM_CURSOR для этой позиции.
FH_DRIVE           = 40   ; 1 байт: диск файла, 0..7 (A:..H:).
FH_MODE            = 41   ; 1 байт: маска FILE_MODE_*.
FH_STATE           = 42   ; 1 байт: 046h — открыт, 0 — закрыт.
FH_ERROR           = 43   ; 1 байт: сохранённый код ошибки DOS, 0 — нет ошибки.
FH_EOF             = 44   ; 1 байт: признак достижения конца при чтении.
FH_DIR_SECTOR      = 45   ; 2 байта: сектор с записью файла в каталоге.
FH_DIR_OFFSET      = 47   ; 2 байта: смещение записи внутри этого сектора.
FH_BUFFER          = 49   ; 2 байта: адрес буфера READ/WRITE.
FH_COUNT           = 51   ; 2 байта: запрошено на входе, передано на выходе.
FH_SEEK            = 53   ; 4 байта: знаковое смещение SEEK; DOS изменяет поле.
FH_ORIGIN          = 57   ; 1 байт: база SEEK, 0=начало, 1=позиция, 2=конец.
FH_PREVIOUS        = 58   ; 2 байта: предыдущий кластер.
FH_ALLOC_SEED      = 60   ; 2 байта: LFSR для SCATTER, OPEN задаёт ACE1h.
FH_VALID           = 62   ; 1 байт: курсор согласован с позицией, если не 0.
FH_OPERATION       = 63   ; 1 байт: C-переходник передаёт отсюда FILE_OP_* в A.
FILE_HANDLE        = 64   ; Полный размер объекта, включая запись каталога.

;-----------------------------------------------------------------------
; Курсор байтового потока FILE_STREAM_CURSOR: bios_fileStream (C8D2h)
;-----------------------------------------------------------------------
; Четыре байта в памяти вызывающей программы: кластер и смещение внутри него.
; DE=адрес курсора, HL=буфер, BC=число байтов, A=1 запись / 2 чтение.
; DOS обновляет курсор и при частичном отказе; размер файла здесь не хранится.
; Вызывающий ограничивает чтение EOF и заранее выделяет цепочку для записи.

    STRUCT FILE_STREAM_CURSOR
cluster     DW 0               ; Текущий кластер FAT-цепочки.
offset      DW 0               ; Смещение в кластере, не абсолютная позиция.
    ENDS

;-----------------------------------------------------------------------
; Запрос копирования FILE_COPY_REQUEST: bios_fileCopy (C8C3h), 86 байт
;-----------------------------------------------------------------------
; HL=запрос в памяти вызывающей программы; данные копируются в банке DOS.
; Реализация находится в DOS.SYS, исполняемой в собственной странице.
; source/destination — полные 32-байтные FILE_DESCRIPTOR. Вызывающий задаёт
; их имена 8.3 и номера дисков, DOS читает/заполняет метаданные и рабочие поля.
; Внешний вызов не требует буфера клиента. Исходный диск восстанавливается.
; Результат: CF=0,A=0 — готово; CF=1,A=FILE_OP_* ниже — отказ копирования.
; Эти два результата исторического API не совпадают с общей шкалой DOS_ERR_*.
    STRUCT FILE_COPY_REQUEST
buffer              DW 0        ; Внешний запрос: 0; внутренний DOS: адрес пачки данных.
bufferSize          DW 0        ; Внешний запрос: 0; внутренний DOS: ёмкость пачки в байтах.
sourceDrive         DB 0        ; Диск источника, 0..7.
destinationDrive    DB 0        ; Диск назначения, 0..7.
source              FILE_DESCRIPTOR      ; Имя и метаданные источника.
destination         FILE_DESCRIPTOR      ; Имя и метаданные назначения.
sourceCluster       DW 0        ; Рабочие поля изменяет только DOS.
destinationCluster  DW 0        ; Кластер продолжения записи.
blocksLeft          DW 0        ; Остаток обмена порциями до 256 байт.
sourceOffset        DW 0        ; Смещение курсора источника внутри кластера.
destinationOffset   DW 0        ; Смещение курсора назначения внутри кластера.
sourceDirectory     DW 0        ; Каталог источника, 0=корень.
destinationDirectory DW 0       ; Каталог назначения, 0=корень.
onExists            DW 0        ; HL=запрос; A=0 No, 1 Yes, 2 Yes to all, 3 No to all; CF=отмена.
                                ; 0=вернуть EXISTS. Обработчик не вызывает DOS/CLI.
                                ; AF/BC/DE/HL разрушены; CF=отмена, без удаления источника.
    ENDS
FILE_OP_ERROR  = 1              ; Общий отказ копирования.
FILE_OP_EXISTS = 2              ; Назначение существует, перезапись не выполнена.

; Целый файл по пути: descriptor — рабочее место вызывающей программы.
; Буфер данных и путь не перекрывают запрос; слова little-endian.
; Форматонезависимая загрузка тела: MXOS_BIOS_FILE_BODY, HL=FILE_BODY_IO.
; Дескриптор уже найден на выбранном диске; header/tail — число байтов.
; trailer=0 пропускает хвост; иначе он читается в указанный NEAR-буфер.
; actual — число прочитанных байтов тела. Структура/буферы принадлежат main.
    STRUCT FILE_BODY_IO
descriptor DS FILE_DESCR_SIZE,0
buffer     DW 0
header     DB 0
tail       DB 0
trailer    DW 0
actual     DW 0
    ENDS

    STRUCT FILE_PATH_IO
path        DW 0
buffer      DW 0
count       DW 0
actual      DW 0
descriptor  FILE_DESCRIPTOR
    ENDS

;-----------------------------------------------------------------------
; Заголовок исполняемого файла EXE/COM (собственный формат MXOS2)
;-----------------------------------------------------------------------
; COM и EXE имеют одинаковый контейнер: заголовок 16 байт, затем тело.
; COM/EXE исполняет DOS; RUN выбирает среду совместимости классических программ.
; Все 16-битные поля little-endian: сначала младший байт, затем старший.
;
; Смещение Размер  Содержимое
;   00h      2     0CDh,020h: INT 20h при запуске как COM в MS DOS.
;   02h      2     Сигнатура ASCII "MX" (04Dh,058h).
;   04h      1     Версия контейнера: 01h.
;   05h      1     00h: main/нейтральный DLL, 01h: DRIVER, 02h: BANK, 04h: MAIN; сочетания запрещены.
;   06h      2     Абсолютный адрес загрузки тела.
;   08h      2     Абсолютный адрес входа в программу.
;   0Ah      2     Размер тела N в байтах, N > 0.
;   0Ch      2     Контрольная сумма тела, штатная bios_calcCS (Monitor-2).
;   0Eh      2     bios_calcCS байтов заголовка 00h..0Dh включительно.
;   10h      N     Тело: машинный код 8080 и данные программы.
;
; Размер файла строго N+16. Вход находится внутри загруженного тела;
; диапазон загрузки проверяется загрузчиком. Заголовок в тело не копируется.
; Перемещения адресов и BSS в версии 1 не предусмотрены.
; Обе суммы аддитивные, не полиномиальный CRC; результат BC записывается
; как C,B. Сумма тела считается по N байтам, без заголовка; в конце файла
; дополнительной суммы нет. Проверка выполняется при загрузке с накопителя.
; Сигнатуры MZ/PE нет; байты тела предназначены для процессора 8080.
; Первые два байта обеспечивают выход, если файл запущен как COM в MS DOS.

;-----------------------------------------------------------------------
; Информация об активном накопителе (диске)
;-----------------------------------------------------------------------

    STRUCT DISK_INFO
sectorSize          BLOCK   2   ; размер сектора, байт (256, 512, 1024)
sectorsPerCluster   BLOCK   2   ; размер кластера, секторов (1, 2, 4, 8)
resSectors          BLOCK   2   ; сколько секторов занимает зарезервированная область = первый сектор таблицы fat
fatSectors          BLOCK   2   ; сколько секторов занимает таблица fat
dirSectors          BLOCK   2   ; сколько секторов занимает корневой каталог
totalSectors        BLOCK   2   ; сколько всего секторов на диске
rootDirMaxFiles     BLOCK   2   ; максимум файлов в корневом каталоге
descrPerSector      BLOCK   2   ; сколько дескрипторов файлов вмещается в один сектор
fatNumCells         BLOCK   2   ; количество ячеек в таблице fat (= fatSectors * sectorSize / 2)
dirStartSector      BLOCK   2   ; первый сектор корневого каталога
dataStartSector     BLOCK   2   ; первый сектор области данных
volumeLabel         BLOCK  12   ; метка тома и 0 в конце
isValid             BLOCK   1   ; 0 = нет драйвера, 1 = диск не распознан, 2 = диск распознан, корректно отформатирован
allocationStep      BLOCK   2   ; Подсказка физического драйвера B=3: шаг по чипам, секторов; 0 = линейно
dataClusters        BLOCK   2   ; Допустимые кластеры данных, без двух резервных записей FAT
blockSize           DW    256   ; Физический блок выбранного драйвера
sectorShift         DB      8   ; log2 логического сектора
blockShift          DB      8   ; log2 блока драйвера
clusterBytes        DW    256   ; Размер кластера в байтах, до 32768
fatCount            DB      1   ; Число копий FAT
physicalBlocks      DW      0   ; Ёмкость драйвера B=3, в физических блоках
    ENDS

DISK_NO_DRIVER = 0
DISK_INVALID   = 1
DISK_VALID     = 2

;-----------------------------------------------------------------------
; Буферы в памяти
; Поле геометрии верхних переменных точно соответствует структуре драйвера.
    ASSERT DISK_INFO == 48
    ASSERT BIOS_UPPER_VARIABLES == 272
;-----------------------------------------------------------------------

; Страница ДОЗУ 0 / DOS
DOS_BANK_PAGE         = 1               ; Логический FAR ID физической страницы 0
DOS_BANK_DATA_ADDR    = 07F00h
DOS_BANK_DATA_SIZE    = 01000h
DOS_BANK_OBJECT_ADDR  = 07000h
DOS_BANK_PATH_ADDR    = 07100h
DOS_BANK_CODE_ADDR    = 00100h
DOS_BANK_COMPAT_ADDR  = 08FDFh
DOS_DRIVER_REGISTRY   = 07200h ; 8 записей по DRIVER_RECORD_SIZE
DOS_DRIVER_IMAGE      = 07500h    ; сохранённая служба H:, без инициализатора
DOS_SERVICE_ADDR      = 07800h    ; Один резидентный DOS-модуль, включая данные
DOS_SERVICE_END       = 07C00h    ; Исключающая верхняя граница
DOS_BANK_DRIVER_ADDR  = 0C000h

; Зарезервированный слот физического драйвера в основной памяти
MEM_IO_SYSTEM        = 0F8C0h ; Только EXEC/DOS; в MAIN здесь FLASHSYS
MEM_IO_CHECKED       = MEM_IO_SYSTEM+3
MEM_IO_CHECKSUM      = MEM_IO_SYSTEM+6
LOCAL_DRIVER_ADDR     = 0FA00h
LOCAL_DRIVER_END      = 0FBA0h
LOCAL_BLOCK_ENTRY     = 0F740h
MEM_IMAGE_LOCAL_START = 0F380h
MEM_EXTENSION_ENTRY   = 0F480h
MEM_COPY_SELECT_ADDR  = MEM_EXTENSION_ENTRY+3
MEM_BIOS_ROUTE        = MEM_EXTENSION_ENTRY+6
MEM_BIOS_RESULT       = MEM_EXTENSION_ENTRY+9
MEM_MOVE_SELECT_ADDR  = MEM_EXTENSION_ENTRY+12
MEM_COMMAND_SHADOW    = 0F7DAh
MEM_NEAR_COPY         = 0F7FAh ; Локальный стековый memcpy: HL=начало, DE=конец, BC=приёмник
MEM_LOCAL_CHECKED    = 0F7AEh ; MAIN: локальный проверяющий цикл BIOS
MEM_CALL_ENTRY        = 0F7B0h
MEM_NATIVE_CALL       = 0F7C0h
MEM_BANK_KIND         = 0F7C2h
MEM_LOCAL_A           = 0F7C3h
MEM_LOCAL_INFO_ADDR   = 0F7C5h ; Внутренний обработчик общего образа MEM
MEM_RUNTIME_REVISION_ADDR = 0F7D8h ; Word: версия установленного транспорта
MAIN_COMMAND_ADDR     = bios_upper.commandLine
MAIN_DISK_INFO        = bios_upper.diskInfo
MAIN_DRIVER_INFO      = bios_upper.driverInfo
MAIN_RAW_LEASE        = bios_upper.rawLease      ; диск+1, ёмкость word, последнее A/CF, признак результата
CLIENT_COPY_SNAPSHOT  = 0FBA0h
DOS_BANK_DRIVER_END   = 0D780h
DOS_BANK_MODULE_END   = 0EF80h ; DOS-DLL может занять BAT только при v_batDepth=0
DOS_BANK_CACHE_TABLE  = 0B000h
DOS_BANK_CACHE_ADDR   = 09000h
DOS_BANK_PARTIAL_ADDR = 0B400h
DOS_VERIFY_BUFFER     = 0B600h ; Не пересекает исходный неполный сектор B400h–B5FFh
DOS_CACHE_BOOT_ADDR   = 0B800h
DOS_PARTIAL_SIZE      = 512
DOS_BANK_CMD_ADDR     = 07C00h
MEM_ALLOC_TABLE       = 07D00h ; 32 записи по 6 байт, только банк DOS
MODULE_TABLE          = 07DC0h ; MODULE_MAX записей по MODULE_RECORD_SIZE, только DOS
DOS_BANK_STACK_BOTTOM = 0EF80h
DOS_BANK_STACK_TOP    = 0F380h

; Основная память / BIOS и временная основная сборка DOS.
; Границы STD по мапперу MMX: вся верхняя страница 4 КБ — устройства.
STD_IO_ADDR   = 0F000h
STD_KEYB_ADDR = 0F800h

; Дисковый кэш принадлежит DOS. Файловые вызовы допустимы только в MX.
    IFNDEF FAT_CACHE_ADDR
FAT_CACHE_ADDR = 09000h ; Вспомогательная линковочная сборка DOS в main
    ENDIF
    IFNDEF FAT_CACHE_SIZE
FAT_CACHE_SIZE = 8192   ; единый пул метаданных всех дисков, без неполного сектора
    ENDIF
CACHE_GRANULES = FAT_CACHE_SIZE / 128
FAT_CACHE_CAPACITY = FAT_CACHE_SIZE / SECTOR_SIZE   ; сколько секторов вмещается в кэш
    ASSERT FAT_CACHE_CAPACITY > 0 && FAT_CACHE_CAPACITY <= 255
    ASSERT FAT_CACHE_SIZE % SECTOR_SIZE == 0
    ASSERT FAT_CACHE_ADDR % SECTOR_SIZE == 0
    ASSERT FAT_CACHE_ADDR + FAT_CACHE_SIZE <= 10000h

; Буфер BAT исключён из образа DOS; в штатной системе принадлежит её банку.
BAT_READ_SIZE   = 512
BAT_FRAME_SIZE  = 768
BAT_MAX_DEPTH   = 8
    IFDEF BANKED_DOS_BUILD
BAT_BUFFER      = 0D780h
BAT_BUFFER_SIZE = 01800h
    ELSE
BAT_BUFFER      = 0000h
BAT_BUFFER_SIZE = DOS_ENTRY_BASE ; Только линковочная основная сборка.
    ENDIF

; Резидент RAMDISK.COM делит окно расширений с FLASHSYS.COM.
RAMD_RESIDENT_ADDR = 0F740h ; Только исторический RAMD; штатная сборка его не устанавливает

SYSTEM_FLASH_ADDR      = 0F8C0h ; Отдельный слот A:, F8C0h-F9FFh
SYSTEM_FLASH_END       = 0FA00h
DISK_DRIVER_ADDR       = 0E800h ; E800h-E9FFh, вне системного хвоста
NC_LOAD_ADDR           = 00000h ; NC — обычная программа с собственными буферами
SYSTEM_PROGRAM_ADDR    = 0E500h ; Временные верхние утилиты; RUN здесь не резидентен
SYSTEM_PROGRAM_END     = 0EB00h
DOS_END_MAX            = DISK_DRIVER_ADDR ; Далее отдельный резерв физического драйвера

; Резидентный принимающий адаптер банковых вызовов BIOS и его отдельный стек.
; Используются после установки общего транспорта; публичный вход — F800h.
BIOS_TEMP_BUFFER       = 0ED80h ; Общая scratch-область MAIN, один синхронный вызов
BIOS_TEMP_BUFFER_END   = bios_upper
BIOS_TEMP_BUFFER_SIZE  = BIOS_TEMP_BUFFER_END-BIOS_TEMP_BUFFER
BIOS_DISK_BUFFER       = BIOS_TEMP_BUFFER ; Первые 256 байт: сектор A:
BIOS_DISK_BUFFER_END   = BIOS_DISK_BUFFER+256
BIOS_IO_ADDR           = 0F000h
FONT_ARENA_START       = FONT_ADDR
FONT_ARENA_END         = 0EC80h
GLYPH_CACHE_START      = bios_upper.cacheStart
GLYPH_CACHE_INVALID    = bios_upper.cacheInvalid
GLYPH_CACHE_END        = FONT_ARENA_END
GLYPH_CACHE_LIMIT      = bios_upper.cacheLimit
GLYPH_CACHE_ENTRIES    = bios_upper.cacheEntries
GLYPH_CACHE_EPOCH      = bios_upper.cacheEpoch
BIOS_BANK_GATE_ADDR    = 0F000h
MEM_MAIN_PUT32_TEMPLATE = 0F220h ; MAIN: шаблон в IO, драйвер A: занимает F8C0h–F9FFh
BIOS_BANK_GATE_END     = 0F240h
BIOS_BANK_IO_ADDR      = 0F240h
BIOS_BANK_IO_BUFFER    = BIOS_TEMP_BUFFER ; Пачки текста/UI: 256 байт + 5 защиты
BIOS_BANK_STACK_BOTTOM = 0EC80h
BIOS_BANK_STACK_TOP    = 0ED80h

    IFDEF BANKED_DOS_BUILD
DOS_CODE_START         = DOS_BANK_CODE_ADDR
DOS_CODE_END_MAX       = DOS_BANK_OBJECT_ADDR
    ELSE
    IFDEF DOS_SPLIT_BUILD
DOS_CODE_START         = DOS_ENTRY_BASE ; Линковочный образ, не исполняется в main.
DOS_CODE_END_MAX       = 09000h
    ELSE
DOS_CODE_START         = 0C000h
DOS_CODE_END_MAX       = DOS_END_MAX
    ENDIF
    ENDIF

; Стек доступен в STD, командная строка нужна только в MX.
CMD_LINE               = MAIN_COMMAND_ADDR ; только основная память, MX
CMD_LINE_SIZE          = 130
APP_MEMORY_END         = 08C00h ; До переходника RUN, аргументов и системного стека
LEGACY_APP_MEMORY_END  = 08C00h ; Физическая граница обычной памяти
STACK_BOTTOM           = 08E00h
STACK_ADDR             = 08FDFh
NC_STATE_ADDR          = 0FFB0h ; 12 байт; FFBEh — детектор, FFBFh — метка FORMAT

;-----------------------------------------------------------------------
; Статические проверки
;-----------------------------------------------------------------------

    ASSERT DOS_BANK_DATA_ADDR >= 07E10h
    ASSERT DOS_BANK_DATA_ADDR+DOS_BANK_DATA_SIZE <= DOS_BANK_COMPAT_ADDR
    ASSERT BAT_FRAME_SIZE*BAT_MAX_DEPTH <= 01800h
    ASSERT DOS_BANK_OBJECT_ADDR+FILE_HANDLE <= DOS_BANK_PATH_ADDR
    ASSERT DOS_BANK_CODE_ADDR == 00100h
    ASSERT DOS_BANK_PATH_ADDR+MAX_PATH <= DOS_DRIVER_REGISTRY
    ASSERT MEM_ALLOC_TABLE+MEM_ALLOC_RECORDS*6 <= MODULE_TABLE
    ASSERT MODULE_TABLE+MODULE_MAX*MODULE_RECORD_SIZE <= DOS_BANK_DATA_ADDR
    ASSERT DOS_BANK_COMPAT_ADDR+BIOS_VARIABLES <= DOS_BANK_DRIVER_ADDR
    ASSERT DOS_BANK_CMD_ADDR+CMD_LINE_SIZE <= DOS_BANK_COMPAT_ADDR
    ASSERT CACHE_GRANULES <= 64
    ASSERT (CACHE_GRANULES & (CACHE_GRANULES-1)) == 0
    ASSERT DOS_BANK_CACHE_ADDR+FAT_CACHE_SIZE <= DOS_BANK_CACHE_TABLE
    ASSERT DOS_BANK_CACHE_TABLE+(CACHE_GRANULES+1)*8 <= DOS_BANK_PARTIAL_ADDR
    ASSERT DOS_BANK_PARTIAL_ADDR+DOS_PARTIAL_SIZE <= DOS_VERIFY_BUFFER
    ASSERT DOS_VERIFY_BUFFER+256 <= DOS_CACHE_BOOT_ADDR
    ASSERT DOS_CACHE_BOOT_ADDR+512 <= DOS_BANK_DRIVER_ADDR
    ASSERT DOS_BANK_DRIVER_END <= 0D800h
    ASSERT DOS_DRIVER_IMAGE+576 <= DOS_SERVICE_ADDR
    ASSERT DOS_SERVICE_END <= DOS_BANK_CMD_ADDR
    ASSERT DOS_BANK_CMD_ADDR+CMD_LINE_SIZE <= DOS_BANK_STACK_BOTTOM
    ASSERT DOS_BANK_STACK_BOTTOM < DOS_BANK_STACK_TOP
    ASSERT DOS_BANK_STACK_TOP == MEM_IMAGE_LOCAL_START
    ASSERT DOS_BANK_STACK_TOP-DOS_BANK_STACK_BOTTOM == 1024
    ASSERT BIOS_DISK_BUFFER_END <= BIOS_TEMP_BUFFER_END
    ASSERT BIOS_TEMP_BUFFER_SIZE == 272
    ASSERT BIOS_BANK_STACK_TOP == BIOS_TEMP_BUFFER
    ASSERT BIOS_BANK_IO_BUFFER+261 <= BIOS_TEMP_BUFFER_END
    ASSERT FONT_ARENA_END <= BIOS_BANK_STACK_BOTTOM
    ASSERT SYSTEM_PROGRAM_END <= SYSTEM_FLASH_ADDR
    ASSERT CMD_LINE + CMD_LINE_SIZE <= MAIN_DISK_INFO
    ASSERT SYSTEM_FLASH_ADDR + 0140h == SYSTEM_FLASH_END
    ASSERT SYSTEM_FLASH_END <= LOCAL_DRIVER_ADDR
    ASSERT LOCAL_DRIVER_END <= CLIENT_COPY_SNAPSHOT
    ASSERT CLIENT_COPY_SNAPSHOT + FILE_COPY_REQUEST <= 0FC00h
    ASSERT RAMD_RESIDENT_ADDR + 00C0h <= mx_call
    ASSERT BIOS_BANK_IO_BUFFER+261 <= MAIN_COMMAND_ADDR
    ASSERT MAIN_DRIVER_INFO + 64 <= MAIN_RAW_LEASE
    ASSERT bios_upper + BIOS_UPPER_VARIABLES <= io_vars
    ASSERT DOS_DRIVER_REGISTRY + 8*DRIVER_RECORD_SIZE <= DOS_DRIVER_IMAGE
    ASSERT DOS_DRIVER_IMAGE + LOCAL_DRIVER_END-LOCAL_DRIVER_ADDR <= DOS_BANK_CACHE_ADDR
    ASSERT FAT_CACHE_ADDR + FAT_CACHE_SIZE <= 0FF00h
    ASSERT CMD_LINE_SIZE == 130
    ASSERT STACK_BOTTOM < STACK_ADDR && STACK_ADDR <= STD_IO_ADDR
    ASSERT DOS_END_MAX <= STD_IO_ADDR
    ASSERT FONT_ADDR + FONT_BYTES <= BIOS_BANK_STACK_BOTTOM


;-----------------------------------------------------------------------
; Всякие полезности
;-----------------------------------------------------------------------

; Макрос для заполнения памяти от текущего адреса до указанного
    MACRO ORG_PAD adr
         IF $ > adr
           ; вывод сообщения об ошибке
           ASSERT 0
           DISPLAY /l, "Error! ORG_PAD failed! ", $, " is more than ", adr
         ELSE
           ; заполнение памяти
           BLOCK adr-$, 0FFh
         ENDIF
         ORG adr
    ENDM

    MACRO ORG_PAD0 adr
         IF $ > adr
           ; вывод сообщения об ошибке
           ASSERT 0
           DISPLAY /l, "Error! ORG_PAD0 failed! ", $, " is more than ", adr
         ELSE
           ; заполнение памяти
           BLOCK adr-$, 0
         ENDIF
         ORG adr
    ENDM

; Макросы для проверки текущего адреса
    MACRO ASSERT_EQUAL adr
         IF $ != adr
           ASSERT 0
           DISPLAY /l, "Error! Entry point has been shifted (", $, " != ", adr, ")"
         ENDIF
    ENDM

    MACRO ASSERT_DONT_FIT adr
         IF $ > adr
           ASSERT 0
           DISPLAY /l, "Error! Image did not fit (", $, " > ", adr, ")"
         ENDIF
    ENDM

;-----------------------------------------------------------------------
; Конец
;-----------------------------------------------------------------------
