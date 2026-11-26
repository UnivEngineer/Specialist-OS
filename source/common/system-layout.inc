; MXOS
; Карта системной памяти и параметры сборки.

    IFNDEF MXOS_SYSTEM_LAYOUT_INC
    DEFINE MXOS_SYSTEM_LAYOUT_INC

; Термины: банк ДОЗУ — физические 64 КиБ; FAR-код 0 — main, 1..16 — банки 0..15.
; Общая область — аппаратные 32 байта; хвосты банков содержат отдельные копии кода.
; Карта 1: основная память / BIOS, FAR-код 0, режим MX.
;   0000-8BFF - [35840 B] Область программ и буферов; memtop=8BFFh
;   8C00-8DFF - [  512 B] Область RUN
;   8E00-8FDE - [  479 B] Системный стек, SP=8FDFh
;   8FDF-8FFF - [   33 B] Переменные BIOS
;   9000-BFFF - [12288 B] Экран; временный образ сменного ядра и переходник
;   C000-D7DA - [ 6107 B] BIOS.SYS; точный конец — bin/System.layout.json
;   D7DB-D7FF - [   37 B] Резерв между BIOS.SYS и шрифтовым движком
;   D800-E3FF - [ 3072 B] Сменный шрифтовый движок и его внутренний резерв
;   E400-EBFF - [ 2048 B] Полный FONT.PF по строкам глифов, без заголовка
;   EC00-EC7F - [  128 B] Непрерывный резерв верхней арены / кэш глифов
;   EC80-ED7F - [  256 B] Служебный стек BIOS; начальный SP=ED80h
;   ED80-EE8F - [  272 B] Общий временный буфер A: / текста и UI
;   EE90-EF9F - [  272 B] BIOS_UPPER_VARIABLES, включая состояние текста MXOS
;   EFA0-EFDB - [   60 B] IO_VARIABLES — рабочие переменные IO.SYS и имя шрифта
;   EFDC-EFFF - [   36 B] Резерв
;   F000-F19F - [  416 B] IO.SYS (точный конец — bin/System.layout.json)
;   F1A0-F1BF - [   32 B] Свободно в MAIN перед MEM.SYS
;   F1C0-F33F - [  384 B] MEM.SYS: подготовка CPM32, серия I/O, мост A:, копировщик
;   F340-F376 - [   55 B] MEM.SYS: локальные процедуры и копировщик четвёрок
;   F377-F379 - [    3 B] Состояние CPM32
;   F37A-F3F7 - [  126 B] MEM.SYS: маршрутизация текста и резерв
;   F3F8-F5BF - [  456 B] MEM.SYS: операции памяти и резерв
;   F5C0-F67B - [  188 B] MEM.SYS: локальный блоковый ввод-вывод, контекст, входы BIOS
;   F67C-F6BF - [   68 B] Быстрый регистровый вход MEM и резерв
;   F6C0-F7FF - [  320 B] MAIN: AT512KS; DOS/EXEC: службы A:, проверки и суммы
;   F800-F8B3 - [  180 B] MEM.SYS: публичные входы CLIENT и резерв
;   F8B4-F9BF - [  268 B] MEM.SYS: передача TEXT, прямой MOVE и вызов DLL
;   F9C0-FBFF - [  576 B] Один слот ROM2MX/AT2MX/AT2MXF, одинаковый во всех EXEC
;   FC00-FEFF - [  768 B] MEM.SYS: транспорт и контексты
;   FF00-FFBF - [  192 B] Защищённый хвост копировщика и резерв
;   FFC0-FFDF - [   32 B] Общее аппаратное окно команд/переноса данных
;   FFE0-FFFF - [   32 B] Регистры оборудования
;
; Раскладка D800-EC7F показана для штатного Rotate: движок вместе со шрифтом
; занимает D800-EBFF (5120 Б); свободны EC00-EC7F (128 Б). Другие движки
; могут иначе использовать свой слот D800-EC7F и резервировать память MAIN.
; Например, Packed6K занимает ещё 7C00-8BFF (4096 Б), снижая memtop до 7BFFh.
; Поэтому 8BFFh — штатный максимум memtop, а не неизменная граница свободной памяти.
;
; Для верхней программы RF мост RUN временно находится в AB00h экрана,
; поскольку загружаемое тело может перекрывать область шрифта.
; В STD верхние 4 КБ (F000-FFFF) закрыты портами. BIOS, стек и шрифт доступны;
; файловые/банковые службы IO.SYS недоступны; шрифт и весь кэш доступны.
;
; Карта 2: физический банк ДОЗУ 0 / DOS, FAR-код 1.
; Bootstrap загружает DOS.SYS в этот банк до запуска BIOS.
;   0000-00FF - [  256 B] Резерв векторов/обработчиков, совместимый с моделью CP/M
;   0100-7C7F - Образ DOS и резерв до переменных
;   7C80-81FF - Переменные DOS и резерв до буфера обмена
;   8200-8AFF - [ 2304 B] Буфер файлового обмена и списка каталога, первые 512 байт также использует CLI
;   8B00-8B81 - [  130 B] Локальная командная строка DOS
;   8B82-8BD4 - [   83 B] Сохранённый контекст загрузчика образов
;   8BD5-8BFC - [   40 B] Заголовок загружаемого образа v2/v3
;   8BFD-8BFF - [    3 B] Резерв
;   8C00-8CBF - [  192 B] Реестр выделенных FAR-блоков
;   8CC0-8D3F - [  128 B] Восемь записей DLL по 16 байт и их интервалы
;   8D40-8DE8 - [  169 B] Выбранный профиль FONT и контекст восстановления
;   8DE9-8DF1 - [    9 B] Проверка активных потомков перед выгрузкой DLL
;   8DF2-8DFF - [   14 B] Резерв
;   8E00-8E7F - [  128 B] Пакет файловой операции / FILEOPS_JOB
;   8E80-8EFF - [  128 B] Путь клиента или снимок callback
;   8F00-917F - [  640 B] Реестр физических драйверов
;   9180-93BF - [  576 B] Образ локальной службы H: без инициализатора
;   93C0-93DE - [   31 B] Резерв
;   93DF-93FF - [   33 B] Локальные переменные BIOS
;   9400-95FF - [  512 B] Отдельный неполный файловый сектор
;   9600-96FF - [  256 B] Буфер проверяющего чтения сектора
;   9700-97AF - [  176 B] Пакет физических LBA и временный курсор
;   97B0-97FF - [   80 B] Кадры MAIN-приложений (8 по 9 байт) и резерв
;   9800-99FF - [  512 B] BPB при монтировании, контрольное чтение при RAW_IO 5
;   9A00-9A3F - [   64 B] Открытый неполный FILEHANDLE
;   9A40-9ABF - [  128 B] Первый рабочий путь CLI
;   9AC0-9AE9 - [   42 B] Контекст запроса DOS и регистры наблюдателя
;   9AEA-9AFF - [   22 B] Резерв перед ареной
;   9B00-C9D7 - [11992 B] Арена DLL и установщиков; BAT занимает свободный интервал
;   C9D8-CB7F - [  424 B] Восемь курсоров BAT, общий FILE_HANDLE и 128 байт чтения
;   CB80-CBFF - [  128 B] Второй рабочий путь CLI
;   CC00-CDFF - [  512 B] 64 описателя гранул общего кэша
;   CE00-EDFF - [ 8192 B] Единый пул кэша FAT/каталогов всех дисков
;   EE00-F03F - [  576 B] Служебный стек DOS; начальный SP=F040h
;   F040-F1BF - [  384 B] Банковый сегмент IO.SYS: клавиатура MX и резерв
;   F1C0-F33F - [  384 B] MEM.SYS: подготовка CPM32, серия I/O, мост A:, копировщик
;   F340-F376 - [   55 B] MEM.SYS: локальные процедуры и копировщик четвёрок
;   F377-F379 - [    3 B] Состояние CPM32
;   F37A-F3F7 - [  126 B] MEM.SYS: маршрутизация текста и резерв
;   F3F8-F5BF - [  456 B] MEM.SYS: операции памяти и резерв
;   F5C0-F67B - [  188 B] MEM.SYS: локальный блоковый ввод-вывод, контекст, входы BIOS
;   F67C-F6BF - [   68 B] Быстрый регистровый вход MEM и резерв
;   F6C0-F7FF - [  320 B] MAIN: AT512KS; DOS/EXEC: службы A:, проверки и суммы
;   F800-F8B3 - [  180 B] MEM.SYS: публичные входы CLIENT и резерв
;   F8B4-F9BF - [  268 B] MEM.SYS: передача TEXT, прямой MOVE и вызов DLL
;   F9C0-FBFF - [  576 B] Один слот ROM2MX/AT2MX/AT2MXF, одинаковый во всех EXEC
;   FC00-FEFF - [  768 B] MEM.SYS: транспорт, контексты и общий копировщик
;   FF00-FFBF - [  192 B] Копия хвоста копировщика и резерв
;   FFC0-FFDF - [   32 B] То же физическое 32-байтное окно, что в основной памяти
;   FFE0-FFFF - [   32 B] Регистры оборудования, доступны из банка DOS
;
; Единая арена DOS-DLL: 9B00-C9D7 (11992 Б), точные интервалы без слотов.
; Дисковод — 9D00h, ENUM — 9F00h, FILEOPS — A700h, установщики заканчиваются перед C9D8h.
; Неактивные модули можно вытеснять при пересечении, активные закреплены.
; Верхняя граница DOS-DLL — начало постоянных курсоров BAT.
; FILEOPS_JOB занимает 112 Б: заголовок состояния 26 Б и FILE_COPY_REQUEST 86 Б.
; Буфер пути клиента (128 Б) находится отдельно, в памяти вызывающей программы.
;
; Исполняемый банк приложения (EXEC) получает локальную клавиатуру с F040h и полный хвост MEM с F1C0h.
; Ниже EE00h находятся его собственные код, данные и куча; выше — служебный стек
; из карты 2; размещением и границами управляют загрузчик и менеджер памяти.
;
; Карта 3: банк DATA / RAM-диск, без полного исполняемого профиля MEM.
;   0000-FDFF - [65024 B] FAR-данные; на RAM-диске здесь данные и метаданные тома
;   FE00-FE89 - [  138 B] Удалённый локальный MOVE, прямой и обратный ход
;   FE8A-FECF - [   70 B] Установка параметров CPM32, остаток и резерв
;   FED0-FEF6 - [   39 B] MEM.SYS: регистровый обработчик COMMAND GET/PUT8/16
;   FEF7-FEFE - [    8 B] Приватный стек удалённого MOVE
;   FEFF-FEFF - [    1 B] Резерв
;   FF00-FF03 - [    4 B] Сигнатура установленного хвоста MEM
;   FF04-FFB9 - [  182 B] MEM.SYS: локальный движок CPM32 с селекторами в операндах
;   FFBA-FFBB - [    2 B] Счётчик полных окон CPM32
;   FFBC-FFBE - [    3 B] Резерв, FFBEh — тестовый байт ДОЗУ
;   FFBF-FFBF - [    1 B] Метка готовности EXEC; в DATA равна нулю
;   FFC0-FFDF - [   32 B] То же физическое общее окно COMMAND/COPY
;   FFE0-FFFF - [   32 B] Регистры оборудования, доступны из банка DATA
;
; DATA получает 448 байт MEM с FE00h; для удалённого MOVE используется стек хвоста.
; Для FAR доступно 65024 байта. В FAT целиком исключаются два физических сектора
; FE00-FFFF (512 байт), помечаемые BAD.
; Старую разметку RAM-диска нельзя обновлять записью поверх FE00 с данными.
; Банки RAM-диска начинаются с физического банка 1, исключая банк DOS.
; Константы карт находятся ниже в разделе буферов.
;-----------------------------------------------------------------------

;-----------------------------------------------------------------------
; Конфигурация сборки
;-----------------------------------------------------------------------


; Назначение банка: DATA получает COMMAND/CPM32 с MEM_DATA_START, DOS/EXEC — полный хвост.
MEM_BANK_DATA        = 0
MEM_BANK_EXEC        = 1
MEM_BANK_DOS         = 2
MEM_EXEC_READY_VALUE = 0A7h

MEM_RUNTIME_REVISION = 77  ; Переменные и менеджер резидентов в ядре DOS
MEM_WINDOW_VERSION   = 2   ; Внутренний протокол физического окна
    IFNDEF DOS_IO_BATCH_SECTORS
DOS_IO_BATCH_SECTORS = 32  ; 16/32/64 полных физических секторов за серию
    ENDIF
    ASSERT DOS_IO_BATCH_SECTORS == 16 || DOS_IO_BATCH_SECTORS == 32 || DOS_IO_BATCH_SECTORS == 64

RAMDISK_DATA_BLOCK_LIMIT = 254  ; FE00h–FFFFh: COMMAND и CPM32, два сектора

APP_MAX_DEPTH  = 8
APP_OWNER_BASE = 020h

MEM_ALLOC_RECORDS = 32

MODULE_MAX         = 8
MODULE_RECORD_SIZE = 16
MODULE_STACK_SIZE  = 1024
MODULE_OWNER_BASE  = 080h

KEY_CLICK_TICKS   = 64    ; 8.192 ms, CH1 divisor 256
KEY_CLICK_DIVISOR = 1000  ; 2 kHz from CLK0

    IFNDEF DISK_DRIVER_CHECKED
DISK_DRIVER_CHECKED = 1  ; Встроенный ROM/RAM-драйвер: CF и проверка аргументов
    ENDIF
    ASSERT DISK_DRIVER_CHECKED == 0 || DISK_DRIVER_CHECKED == 1

RAMD_MAX_PAGE = 16      ; Максимальное количество страниц RAM диска
RAMD_PAGE_END = 0FFBEh  ; Тестовый байт вне кода копировщика
ROM_64K       = 1       ; Включить поддержку ПЗУ 64 Кб Специалиста МХ2

    IFNDEF ENABLE_COLOR
ENABLE_COLOR = 1  ; Включить поддержку цвета
    ENDIF

COLOR_BIOS = 0F0h  ; Цвет командной строки

DRIVER_RECORD_SIZE = 80

; Физический диск C: — системное ПЗУ. Файлы MXOS могут загружаться с дискеты A:.
    IFDEF MX_FLOPPY_BOOT
BOOT_VOLUME = 0
    ELSE
BOOT_VOLUME = 2
    ENDIF
BOOT_VOLUME_LETTER = 'A'+BOOT_VOLUME
SYSTEM_DRIVE       = 2
RAM_DRIVE          = 1
FLOPPY_DRIVE       = 0
EXTERNAL_DRIVE     = 7

DOS_IO_VERIFY_FILES = 1  ; Проверять запись данных FILEHANDLE при DISK_CAP_VERIFY.

; Верхние системные переменные BIOS основной памяти, EE90h-EF9Fh.
; Адреса открытых полей проверяются по смещениям структуры.
    STRUCT BIOS_UPPER_VARIABLES
commandLine   BLOCK  130        ; Буфер редактора NC/CLI и совместимого ABI командной строки
diskInfo      BLOCK  48         ; Локальный результат запроса геометрии физического драйвера
driverInfo    BLOCK  64         ; Локальная копия имени/описания из реестра DOS
rawLease      BLOCK  6          ; Разрешение прямого RAW и последнее состояние ошибки
cacheLimit    DW     0          ; Исключающая верхняя граница кэша глифов
cacheEntries  DW     0          ; Число действующих записей кэша
cacheEpoch    DB     0          ; Счётчик инвалидирования
cacheStart    DW     0          ; Первый байт после загруженного файла шрифта
cacheInvalid  DB     1          ; 1 — индекс/битмапы не действуют
charGen       DW     FONT_ADDR  ; Физический адрес RAM-шрифта
cursorCfg     DB     021h
koi7          DB     0          ; Только чтение; менять через bios_setFontEncoding / MXOS_BIOS_SET_FONT_ENCODING
escMode       DB     0FFh
cursorDelay   DB     020h
antiBsDelay   DB     0E0h
ramPageCount  DB     RAMD_MAX_PAGE
diskWriteErr  DB     0
padding       BLOCK  7
    ENDS

bios_upper              EQU 0EE90h
bios_upper.commandLine  EQU bios_upper + BIOS_UPPER_VARIABLES.commandLine
bios_upper.diskInfo     EQU bios_upper + BIOS_UPPER_VARIABLES.diskInfo
bios_upper.driverInfo   EQU bios_upper + BIOS_UPPER_VARIABLES.driverInfo
bios_upper.rawLease     EQU bios_upper + BIOS_UPPER_VARIABLES.rawLease
bios_upper.cacheLimit   EQU bios_upper + BIOS_UPPER_VARIABLES.cacheLimit
bios_upper.cacheEntries EQU bios_upper + BIOS_UPPER_VARIABLES.cacheEntries
bios_upper.cacheEpoch   EQU bios_upper + BIOS_UPPER_VARIABLES.cacheEpoch
bios_upper.cacheStart   EQU bios_upper + BIOS_UPPER_VARIABLES.cacheStart
bios_upper.cacheInvalid EQU bios_upper + BIOS_UPPER_VARIABLES.cacheInvalid
bios_upper.escMode      EQU bios_upper + BIOS_UPPER_VARIABLES.escMode
bios_upper.cursorDelay  EQU bios_upper + BIOS_UPPER_VARIABLES.cursorDelay
bios_upper.antiBsDelay  EQU bios_upper + BIOS_UPPER_VARIABLES.antiBsDelay
bios_upper.ramPageCount EQU bios_upper + BIOS_UPPER_VARIABLES.ramPageCount
bios_upper.diskWriteErr EQU bios_upper + BIOS_UPPER_VARIABLES.diskWriteErr
bios_upper.padding      EQU bios_upper + BIOS_UPPER_VARIABLES.padding
    ASSERT BIOS_UPPER_VARIABLES == 272
    ASSERT bios_upper.charGen   == bios_upper + BIOS_UPPER_VARIABLES.charGen
    ASSERT bios_upper.cursorCfg == bios_upper + BIOS_UPPER_VARIABLES.cursorCfg
    ASSERT bios_upper.koi7      == bios_upper + BIOS_UPPER_VARIABLES.koi7
    ASSERT CMD_LINE             == bios_upper.commandLine
    ASSERT BIOS_UPPER_VARIABLES.diskInfo - BIOS_UPPER_VARIABLES.commandLine == CMD_LINE_SIZE
    ASSERT BIOS_UPPER_VARIABLES.driverInfo - BIOS_UPPER_VARIABLES.diskInfo == DISK_INFO

; Плотные рабочие переменные IO.SYS, отдельно от кода и общего буфера текста.
; Банк DOS не должен хранить NEAR-указатели на эти поля как свои локальные.
    STRUCT IO_VARIABLES
bbPage          DB     0
bbFunction      DB     0
bbRegs          BLOCK  8
bbCopy          BLOCK  8
mainIoRequest   BLOCK  5
mainIoCopy      BLOCK  8
fbRequest       DW     0
fbEnd           DW     0
fbCursor        BLOCK  4
romPage         DB     0
romBufferPage   DB     0    ; FAR-банк буфера физического A:
romBlocksHi     DB     0    ; Старший байт ёмкости A: для размера страницы AT29C
romSource       DW     0
romTarget       DW     0
romChunk        DW     0
romCheck        DB     0
romSavedSP      DW     0
romLoops        DB     0
fontName        BLOCK  8    ; MAIN, installed FNT basename padded with zero/space

    ENDS
io_vars IO_VARIABLES = 0EFA0h
    ASSERT io_vars + IO_VARIABLES <= 0F000h

; Страница ДОЗУ 0 / DOS
DOS_BANK_PAGE        = 1                          ; Логический FAR ID физической страницы 0
DOS_VARIABLES_ADDR   = 082A8h                     ; Рабочие переменные DOS под буфером обмена.
DOS_BANK_DATA_ADDR   = 08700h
DOS_BANK_DATA_SIZE   = 08B00h-DOS_BANK_DATA_ADDR
PRESSURE_PATH_BYTES  = 32
    ASSERT DOS_BANK_DATA_SIZE >= 2*512
DOS_BANK_OBJECT_ADDR = 08E00h
DOS_BANK_PATH_ADDR   = 08E80h
DOS_BANK_CODE_ADDR   = 00100h
DOS_BANK_COMPAT_ADDR = 093DFh
DOS_DRIVER_REGISTRY  = 08F00h                     ; 8 записей по DRIVER_RECORD_SIZE
DOS_DRIVER_IMAGE     = 09180h                     ; сохранённая служба H:, без инициализатора
DOS_RESIDENT_WORK    = 09B00h                     ; Переменные резидентов и менеджеров памяти DOS.
DOS_ARENA_START      = 09D00h                     ; Нижняя граница DLL, установщиков и буфера BAT
DOS_FLOPPY_ADDR      = 09D00h                     ; База службы дисковода при загрузке с дискеты MX
DOS_BANK_DRIVER_ADDR = DOS_FLOPPY_ADDR+0200h      ; Стандартная база прикладной DOS DLL
DOS_REQUEST_SCRATCH  = 09AC0h
DOS_CLI_PATH1        = 09A40h
DOS_CLI_PATH2        = 0CB80h
DOS_PARTIAL_RECORD   = 09A00h

; Зарезервированный слот физического драйвера в основной памяти
MEM_IO_SYSTEM     = 0F6C0h           ; EXEC/DOS: локальные службы C:, MAIN: AT512KS
MEM_IO_CHECKED    = MEM_IO_SYSTEM+3
MEM_IO_CHECKSUM   = MEM_IO_SYSTEM+6
LOCAL_DRIVER_ADDR = 0F9C0h
LOCAL_DRIVER_END  = 0FC00h
LOCAL_BLOCK_ENTRY = 0F5C0h

MEM_IMAGE_LOCAL_START     = 0F1C0h                  ; Подготовка CPM32 и пакетного обмена
MEM_COPY_PREPARE_ENTRY    = 0F1C0h                  ; CPM32: подготовка общего окна и локальных параметров
MEM_BATCH_ENTRY           = 0F1C3h                  ; Пакет физических секторов в странице клиента
MEM_MAIN_IO_BRIDGE        = 0F240h                  ; JMP в аппаратную часть BIOS; цель задаёт сборщик
MEM_QUEUE_WORK_ENTRY      = 0F310h                  ; Локальная работа callback очереди
MEM_COPY_BYTES_ENTRY      = 0F335h
MEM_COPY_QUADS_ENTRY      = 0F360h
MEM_COPY_STATE            = 0F377h                  ; SP и остаток CPM32, 3 байта
MEM_EXTENSION_ENTRY       = 0F3F8h
MEM_COPY_SELECT_ADDR      = MEM_EXTENSION_ENTRY+3
MEM_BIOS_ROUTE            = MEM_EXTENSION_ENTRY+6
MEM_BIOS_RESULT           = MEM_EXTENSION_ENTRY+9
MEM_MOVE_SELECT_ADDR      = MEM_EXTENSION_ENTRY+12
MEM_SCALAR_RETURN_ENTRY   = MEM_EXTENSION_ENTRY+15  ; Возврат COMMAND GET/PUT без чужого стека
MEM_LOCAL_CHECKED         = 0F62Eh                  ; MAIN: локальный проверяющий цикл BIOS
MEM_CALL_PACKET_ENTRY     = 0F630h
MEM_NATIVE_CALL           = 0F640h
MEM_BANK_KIND             = 0F642h
MEM_LOCAL_SYSTEM          = 0F643h
MEM_LOCAL_INFO_ADDR       = 0F645h                  ; Внутренний обработчик общего образа MEM
MEM_RUNTIME_REVISION_ADDR = 0F658h                  ; Word: версия установленного транспорта
MEM_NEAR_COPY             = 0F67Ah                  ; Слово прежней регистрации в шаблоне. F80Fh вызывает moveCore напрямую.
MAIN_COMMAND_ADDR         = bios_upper.commandLine
MAIN_DISK_INFO            = bios_upper.diskInfo
MAIN_DRIVER_INFO          = bios_upper.driverInfo
MAIN_RAW_LEASE            = bios_upper.rawLease     ; диск+1, ёмкость word, последнее A/CF, признак результата
MEM_TEXT_ENTRY            = 0F8B4h                  ; TEXT расположен после публичного клиента
MEM_SCOPE_ENTRY           = 0F993h                  ; Internal DOS callback in the controller bank
MEM_ROUTE_STATE           = 0F4E6h                  ; scoped DLL handle, entry, bank and owner
MEM_QUEUE_HOOK            = MEM_ROUTE_STATE+6       ; local callback, followed by its gate

MEM_DATA_START = MEM_DATA_LIMIT  ; DATA: постоянная команда и хвост CPM32

MEM_EXEC_READY      = 0FFBFh                                                        ; Признак установленной локальной службы memcpy
BAT_FRAME_SIZE      = 29
BAT_MAX_DEPTH       = 8
BAT_READ_SIZE       = 128
BAT_BUFFER_SIZE     = BAT_FRAME_SIZE * BAT_MAX_DEPTH + FILE_HANDLE + BAT_READ_SIZE
DOS_BANK_MODULE_END = DOS_CLI_PATH2 - BAT_BUFFER_SIZE
DOS_BANK_DRIVER_END = DOS_BANK_MODULE_END
    IFDEF DOS_REFERENCE_BUILD
DOS_BANK_CACHE_TABLE = 0B000h
    ELSE
DOS_BANK_CACHE_TABLE = 0CC00h
    ENDIF
DOS_BANK_CACHE_ADDR    = 0CE00h
DOS_BANK_PARTIAL_ADDR  = 09400h
DOS_VERIFY_BUFFER      = 09600h        ; Локальная проверка 256-байтного блока в банке DOS
DOS_IO_BATCH_ADDR      = 09700h        ; Пакет LBA и временный курсор потока, до 160 байт
DOS_APP_FRAMES         = 097B0h        ; APP_MAX_DEPTH records of 9 bytes
DOS_CACHE_BOOT_ADDR    = 09800h
DOS_PARTIAL_SIZE       = 512
DOS_IMAGE_CONTEXT_ADDR = 08B82h        ; Saved descriptor/header/directory, 83 bytes
DOS_IMAGE_HEADER_ADDR  = 08BD5h        ; Текущий заголовок v2/v3, до 40 байт
DOS_FONT_STATE_ADDR    = 08D40h        ; Профиль FONT и контекст восстановления, 170 байт.
DOS_MODULE_GUARD_ADDR  = 08DE9h        ; Owner-subtree preflight set, 9 bytes
DOS_BANK_CMD_ADDR      = 08B00h
MEM_ALLOC_TABLE        = 08C00h        ; 32 записи по 6 байт, только банк DOS
MODULE_TABLE           = 08CC0h        ; MODULE_MAX записей по MODULE_RECORD_SIZE, только DOS
DOS_BANK_STACK_BOTTOM  = 0EE00h
DOS_BANK_STACK_TOP     = IO_BANK_KEYS

; Основная память / BIOS и временная основная сборка DOS.
; Границы STD по мапперу MMX: вся верхняя страница 4 КБ — устройства.
STD_IO_ADDR   = 0F000h
STD_KEYB_ADDR = 0F800h

; Дисковый кэш принадлежит DOS. Файловые вызовы допустимы только в MX.
    IFNDEF FAT_CACHE_ADDR
FAT_CACHE_ADDR = 09000h  ; Вспомогательная линковочная сборка DOS в main
    ENDIF
    IFNDEF FAT_CACHE_SIZE
FAT_CACHE_SIZE = 8192  ; единый пул метаданных всех дисков, без неполного сектора
    ENDIF
CACHE_GRANULES     = FAT_CACHE_SIZE / 128
FAT_CACHE_CAPACITY = FAT_CACHE_SIZE / SECTOR_SIZE  ; сколько секторов вмещается в кэш
    ASSERT FAT_CACHE_CAPACITY > 0 && FAT_CACHE_CAPACITY <= 255
    ASSERT FAT_CACHE_SIZE % SECTOR_SIZE == 0
    ASSERT FAT_CACHE_ADDR % SECTOR_SIZE == 0
    ASSERT FAT_CACHE_ADDR + FAT_CACHE_SIZE <= 10000h

    IFDEF BANKED_DOS_BUILD
BAT_BUFFER = DOS_BANK_MODULE_END
    ELSE
BAT_BUFFER = 0000h  ; Только линковочная основная сборка.
    ENDIF
BAT_HANDLE  = BAT_BUFFER + BAT_FRAME_SIZE * BAT_MAX_DEPTH
BAT_INPUT   = BAT_HANDLE + FILE_HANDLE
BAT_COMMAND = DOS_BANK_DATA_ADDR + DOS_BANK_DATA_SIZE - MAX_PATH

; Резидент RAMDISK.COM делит окно расширений с FLASHSYS.COM.
RAMD_RESIDENT_ADDR = 0F740h  ; Только исторический RAMD; штатная сборка его не устанавливает

SYSTEM_FLASH_ADDR   = MEM_IO_SYSTEM     ; Отдельный слот C:, F6C0h-F7FFh
SYSTEM_FLASH_END    = 0F800h
DISK_DRIVER_ADDR    = 0E800h            ; E800h-E9FFh, вне системного хвоста
NC_LOAD_ADDR        = 00000h            ; NC — обычная программа с собственными буферами
SYSTEM_PROGRAM_ADDR = 0E500h            ; Временные верхние утилиты; RUN здесь не резидентен
SYSTEM_PROGRAM_END  = 0EB00h
DOS_END_MAX         = DISK_DRIVER_ADDR  ; Далее отдельный резерв физического драйвера

; Резидентный принимающий адаптер банковых вызовов BIOS и его отдельный стек.
; Используются после установки общего транспорта; публичный вход — F800h.
BIOS_TEMP_BUFFER       = 0ED80h                                 ; Общая scratch-область MAIN, один синхронный вызов
BIOS_TEMP_BUFFER_END   = bios_upper
BIOS_TEMP_BUFFER_SIZE  = BIOS_TEMP_BUFFER_END-BIOS_TEMP_BUFFER
BIOS_DISK_BUFFER       = BIOS_TEMP_BUFFER                       ; Первые 256 байт: сектор A:
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
BIOS_BANK_KEY_CLOCK_INIT = BIOS_BANK_GATE_ADDR+6
BIOS_BANK_GATE_END     = MEM_IMAGE_LOCAL_START
BIOS_BANK_IO_ADDR      = 0F240h
BIOS_BANK_IO_BUFFER    = BIOS_TEMP_BUFFER                       ; Пачки текста/UI: 256 байт + 5 защиты
BIOS_BANK_STACK_BOTTOM = 0EC80h
BIOS_BANK_STACK_TOP    = 0ED80h

    IFDEF BANKED_DOS_BUILD
DOS_CODE_START = DOS_BANK_CODE_ADDR
; Первая область после образа занята переменными DOS.
DOS_CODE_END_MAX = DOS_VARIABLES_ADDR
    ELSE
DOS_CODE_START   = DOS_ENTRY_BASE  ; Линковочный образ, не исполняется в main.
DOS_CODE_END_MAX = 09000h
    ENDIF

STACK_BOTTOM  = 08E00h
STACK_ADDR    = 08FDFh
NC_STATE_ADDR = 0FFB0h  ; 12 байт; FFBEh — детектор, FFBFh — метка FORMAT

;-----------------------------------------------------------------------
; Статические проверки
;-----------------------------------------------------------------------

    ASSERT DOS_BANK_DATA_ADDR >= DOS_CODE_START
    ASSERT DOS_BANK_DATA_ADDR+DOS_BANK_DATA_SIZE <= DOS_BANK_COMPAT_ADDR
    ASSERT DOS_BANK_OBJECT_ADDR+FILE_HANDLE <= DOS_BANK_PATH_ADDR
    ASSERT DOS_BANK_CODE_ADDR == 00100h
    ASSERT DOS_BANK_PATH_ADDR+MAX_PATH <= DOS_DRIVER_REGISTRY
    ASSERT MEM_ALLOC_TABLE+MEM_ALLOC_RECORDS*6 <= MODULE_TABLE
    ASSERT MODULE_TABLE+MODULE_MAX*MODULE_RECORD_SIZE <= DOS_BANK_OBJECT_ADDR
    ASSERT DOS_BANK_COMPAT_ADDR+BIOS_VARIABLES <= DOS_BANK_DRIVER_ADDR
    ASSERT DOS_BANK_CMD_ADDR+CMD_LINE_SIZE <= DOS_BANK_COMPAT_ADDR
    ASSERT CACHE_GRANULES <= 64
    ASSERT (CACHE_GRANULES & (CACHE_GRANULES-1)) == 0
    IFNDEF DOS_REFERENCE_BUILD
    ASSERT DOS_BANK_CACHE_TABLE+CACHE_GRANULES*8 <= DOS_BANK_CACHE_ADDR
    ENDIF
    IFDEF BANKED_DOS_BUILD
    ASSERT BAT_INPUT+BAT_READ_SIZE == DOS_CLI_PATH2
    ASSERT DOS_CLI_PATH2+MAX_PATH <= DOS_BANK_CACHE_TABLE
    ENDIF
    ASSERT DOS_BANK_CACHE_ADDR+FAT_CACHE_SIZE <= DOS_BANK_STACK_BOTTOM
    ASSERT DOS_PARTIAL_RECORD+8 <= DOS_ARENA_START
    ASSERT DOS_BANK_PARTIAL_ADDR+DOS_PARTIAL_SIZE <= DOS_VERIFY_BUFFER
    ASSERT DOS_VERIFY_BUFFER+256 <= DOS_CACHE_BOOT_ADDR
    ASSERT DOS_CACHE_BOOT_ADDR+512 <= DOS_ARENA_START
    ASSERT DOS_BANK_DRIVER_END <= 0D800h
    ASSERT DOS_DRIVER_REGISTRY+8*DRIVER_RECORD_SIZE <= DOS_DRIVER_IMAGE
    ASSERT DOS_BANK_CMD_ADDR+CMD_LINE_SIZE <= MEM_ALLOC_TABLE
    ASSERT DOS_BANK_CMD_ADDR+CMD_LINE_SIZE <= DOS_BANK_STACK_BOTTOM
    ASSERT DOS_BANK_STACK_BOTTOM < DOS_BANK_STACK_TOP
    ASSERT DOS_BANK_STACK_TOP == IO_BANK_KEYS
    ASSERT DOS_BANK_STACK_TOP-DOS_BANK_STACK_BOTTOM == 576
    ASSERT BIOS_DISK_BUFFER_END <= BIOS_TEMP_BUFFER_END
    ASSERT BIOS_TEMP_BUFFER_SIZE == 272
    ASSERT BIOS_BANK_STACK_TOP == BIOS_TEMP_BUFFER
    ASSERT BIOS_BANK_IO_BUFFER+261 <= BIOS_TEMP_BUFFER_END
    ASSERT FONT_ARENA_END <= BIOS_BANK_STACK_BOTTOM
    ASSERT SYSTEM_PROGRAM_END <= SYSTEM_FLASH_ADDR
    ASSERT CMD_LINE + CMD_LINE_SIZE <= MAIN_DISK_INFO
    ASSERT SYSTEM_FLASH_ADDR + 0140h == SYSTEM_FLASH_END
    ASSERT SYSTEM_FLASH_END <= LOCAL_DRIVER_ADDR
    ASSERT LOCAL_DRIVER_END <= 0FC00h
    ASSERT RAMD_RESIDENT_ADDR + 00C0h <= mx_callPacket
    ASSERT BIOS_BANK_IO_BUFFER+261 <= MAIN_COMMAND_ADDR
    ASSERT MAIN_DRIVER_INFO + 64 <= MAIN_RAW_LEASE
    ASSERT bios_upper + BIOS_UPPER_VARIABLES <= io_vars
    ASSERT DOS_CACHE_BOOT_ADDR+512 <= DOS_PARTIAL_RECORD
    ASSERT DOS_DRIVER_REGISTRY + 8*DRIVER_RECORD_SIZE <= DOS_BANK_DRIVER_ADDR
    ASSERT DOS_DRIVER_IMAGE + LOCAL_DRIVER_END-LOCAL_DRIVER_ADDR <= DOS_BANK_DRIVER_ADDR
    ASSERT FAT_CACHE_ADDR + FAT_CACHE_SIZE <= 0FF00h
    ASSERT CMD_LINE_SIZE == 130
    ASSERT STACK_BOTTOM < STACK_ADDR && STACK_ADDR <= STD_IO_ADDR
    ASSERT DOS_END_MAX <= STD_IO_ADDR
    ASSERT FONT_ADDR + FONT_BYTES <= BIOS_BANK_STACK_BOTTOM

    ENDIF
