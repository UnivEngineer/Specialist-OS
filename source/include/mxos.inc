;-----------------------------------------------------------------------
; MXOS
; Точки входа и настройки сборки MXOS
; DOS.SYS — историческое имя единого ядра MXOS. Префикс bios_ сохранён
; для совместимости ABI; аппаратные и файловые сервисы входят в одно ядро.
;
; Карта памяти (основное ОЗУ, MX):
;   0000-8BFF - Прикладные программы и их буферы; DEBUG временно с 8000h
;   8C00-8CFF - Переходник RUN, продолжение после инициализации Monitor-2
;   8D00-8D7F - Аргументы запуска; 8D80-8DFF - резерв контекста
;   8E00-8FDE - [ 479 B] Системный стек, SP=8FDFh
;   8FDF-8FFF - [  33 B] Переменные BIOS
;   9000-BFFF - [12  KB] Экран; временный образ сменного ядра и переходник
;   C000-EAFF - [11008 B] Бюджет BIOS/DOS
;   EB00-EFFF - [1280 B] Компактный шрифт FONTKOI.FNT
;   F000-F5FF - [1536 B] Загружаемый верхний этап RUN
;   F600-F73F - [ 320 B] FLASHSYS.COM
;   F740-F7FF - [ 192 B] RAMD.COM
;   F800-F9FF - [ 512 B] Драйвер H:
;   FA00-FA81 - [ 130 B] Общая командная строка DOS/NC
;   FA82-FAFF - [ 126 B] Резерв MX
;   FB00-FEFF - [1024 B] Дисковый кэш (четыре сектора)
;   FF00-FFAF - [ 176 B] Хвост RAMD.COM
;   FFB0-FFBB - [  12 B] Состояние NC
;   FFBE-FFBF - Детектор ДОЗУ и метка FORMAT
;   FFC0-FFDF - [  32 B] Общее окно, драйвер ДОЗУ
;   FFE0-FFFF - Регистры оборудования
;
; В STD верхние 4 КБ закрыты портами. BIOS, стек и шрифт доступны;
; строка, файловые драйверы и кэш используются только после возврата в MX.
;
; Старая карта памяти:
;   8FDF-8FFF - [  32 B] Переменные
;   9000-BFFF - [12  KB] Экран
;   C000-CFFF - [4096 B] DOS.SYS
;   D000-E1FF - [4607 B] Системные программы (NC.COM, E.COM, DL-RED.COM)
;   E200-E5FF - [1024 B] Свободно (1024 байт)
;   E600-E7FF - [ 512 B] Драйвер магнитофона
;   E800-E8FF - [ 256 B] Свободно (256 байт)
;   E900-F0FF - [2048 B] Шрифт (можно отключить запуском ROMFNT.COM или опцией LOAD_FONT=0)
;   F100-F8FF - [2048 B] Системные программы (FORMAT.COM, DEBUG.COM, ...)
;   F900-F9FF - [ 256 B] Свободно (256 байт)
;   FA00-FAFF - [ 256 B] Драйвер флеш-диска
;   FB00-FEFF - [1024 B] Дисковый буфер (fat + каталог)
;   FF00-FF81 - [ 130 B] Командная строка
;   FF82-FFBF - [  62 B] Стек
;   FFC0-FFDF - [  32 B] Непереключаемое ОЗУ, драйвер обмена с RAM диском
;   FFE0-FFFF - [  32 B] Регистры оборудования
;-----------------------------------------------------------------------


;-----------------------------------------------------------------------
; Конфигурация сборки
;-----------------------------------------------------------------------


NEW_MEMORY_MAP       =  1       ; Новая карта памяти
    IFNDEF DISK_DRIVER_CHECKED
DISK_DRIVER_CHECKED =  1       ; Встроенный ROM/RAM-драйвер: CF и проверка аргументов
    ENDIF
    ASSERT DISK_DRIVER_CHECKED == 0 || DISK_DRIVER_CHECKED == 1
RAMD_MAX_PAGE        =  16      ; Максимальное количество страниц RAM диска
RAMD_PAGE_END        =  0FFBEh  ; Тестовый байт вне кода и состояния NC
ROM_64K              =  1       ; Включить поддержку ПЗУ 64 Кб Специалиста МХ2
ENABLE_COLOR         =  1       ; Включить поддержку цвета
EMBED_FONT           =  0       ; Встроить шрифт в программу (при запуске шрифт копируется на FONT_ADDR)
LOAD_FONT            =  1       ; Загружать шрифт в ОЗУ
COLOR_BIOS           =  0F0h    ; Цвет командной строки
RAMFOS_COMPATIBILITY =  1       ; Совместимость с RAMFOS (WIP)
BOOT_FROM_TAPE       =  0       ; Включить загрузку с ленты при нажатой клавише после сброса

    IF  NEW_MEMORY_MAP
FONT_ADDR = 0EB00h ; Пять диапазонов KOI-8, по 32 символа
    ELSE
FONT_ADDR = 0E900h  ; Адрес шрфита
    ENDIF

    IF  LOAD_FONT
FONT_ADDR_DIV_8 = FONT_ADDR/8
    ELSE
FONT_ADDR_DIV_8 = -1
    ENDIF

;-----------------------------------------------------------------------
; Переменные ОС в подэкранном пространстве и их начальные значения
; Доступ к переменным из программ польователя:
; ld a, bios_vars.lastKey
;-----------------------------------------------------------------------

    STRUCT BIOS_VARIABLES
_reserv_1     DW      -1              ; 8FDFh -
tapeError     DW      0C800h          ; 8FE1h - Адрес, куда происходит переход при ошибке чтения с ленты
tapeAddr      DW      -1              ; 8FE3h - Адрес программы, загруженной с ленты
_reserv_2     DW      -1              ; 8FE5h -
charGen       DW      FONT_ADDR_DIV_8 ; 8FE7h - Адрес альтернативного знакогенератора / 8
cursorCfg     DB       21h ; 0A9h     ; 8FE9h - Внешний вид курсора (биты: 7 - мигание (если 0), 654 - положение (Y) от низа строки, 3210 - размер по высоте)
koi7          DB       0   ; 0FFh     ; 8FEAh - 0FFh = включен KOI-7, 0 = включен KOI-8
escMode       DB      -1              ; 8FEBh - Обработка ESC-последовательности
keyLocks      DB      0BAh ; 3Ah      ; 8FECh - Caps Lock или Рус/Lat (3A - прописные анг, BA - строчные анг, BB - прописные рус, 3B - строчные рус)
_reserv_3     DW      -1              ; 8FEDh -
lastLastKey   DB      -1              ; 8FEFh - Предпоследняя нажатая клавиша
lastKey       DB      -1              ; 8FF0h - Последняя нажатая клавиша
beepFreq      DB       5Fh            ; 8FF1h - Частота звукового сигнала (также влияет на длительность)
beepDuration  DB       20h            ; 8FF2h - Длительность звукового сигнала (сколько раз повторять посылку)
tapeInverse   DB      0FFh            ; 8FF3h - Признак инверсии данных с ленты
cursorDelay   DB       20h            ; 8FF4h - Задержка мигания курсора
antiBsDelay   DB      0E0h            ; 8FF5h - Задержка защиты от дребезга контактов клавиш
oldSP         DW      -1              ; 8FF6h - Используется для сохранения SP некоторыми функциями
ramPageCount  DB      RAMD_MAX_PAGE   ; 8FF8h - Количество детектированных 64кб страниц RAM-диска
diskWriteErr  DB       0              ; 8FF9h - Признак ошибки записи диска
inverse       DW       0              ; 8FFAh - Инвертирование текста (0=нормальный текст, 0FFFFh=инверсный)
cursorY       DB      -1              ; 8FFCh - Положение курсора по вертикали в пикселях
cursorX       DB      -1              ; 8FFDh - Положение курсора по горизонтали в пикселях / 2
writeDelay    DB      28h             ; 8FFEh - Скорость при записи на ленту
readDelay     DB      3Ch             ; 8FFFh - Скорость при чтении с ленты
    ENDS

; Блок переменных BIOS начинается с адреса 8FDFh
bios_vars   BIOS_VARIABLES = 8FDFh

;-----------------------------------------------------------------------
; Стандартные точки входа ОС
; Отмеченные [OLD] применять не рекомендуется
; Отмеченные [MXOS2] - новые в MXOS 2
;-----------------------------------------------------------------------

bios_init               = 0C000h ; Теплая перезагрузка

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
bios_reboot             = 0C800h ; Восстановить звук и запустить NC.COM
bios_getch              = 0C803h ; Ожидание ввода с клавиатуры
bios_tapeRead           = 0C806h ; Чтение байта с магнитофона
bios_printChar          = 0C809h ; Вывод символа на экран
bios_tapeWrite          = 0C80Ch ; Запись байта на магнитофон
bios_input              = 0C80Fh ; Ввод строки с клавиатуры
bios_keyCheck           = 0C812h ; Получить код нажатой клавиши, = bios_keyScan
bios_printHexByte       = 0C815h ; Вывод HEX числа на экран (байт)
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
bios_rebootAlt          = 0C839h ; Запустить NC.COM
    ENDIF
bios_rebootAlt2         = 0C83Ch ; Восстановить звук и запустить NC.COM
bios_fileList           = 0C83Fh ; Получить список файлов
bios_fileGetSetDrive    = 0C842h ; Получить/установить активное устройство
bios_fileCreate         = 0C845h ; HL=дескриптор с точным N, DE=исходный буфер
bios_fileLoad           = 0C848h ; DE=назначение; читать подготовленное тело по месту, CF=ошибка
bios_fileDelete         = 0C84Bh ; Удалить файл
bios_fileRename         = 0C84Eh ; Переименовать файл
bios_fileLoadInfo       = 0C851h ; Загрузить информацию о файле
bios_imageOpen          = 0C854h ; HL=дескриптор, открыть родной COM/EXE MXOS
bios_fileGetSetAttr     = 0C857h ; Получить/установить атрибуты файла
bios_fileNamePrepare    = 0C85Ah ; Преобразовать имя файла во внутренний формат
bios_fileExec           = 0C85Dh ; Запустить файл; BAT затирает ОЗУ 0000h-8000h
bios_installDriver      = 0C860h ; Установить драйвер накопителя
bios_diskDriver         = 0C863h ; Драйвер выбранного диска

bios_fileLoad2          = 0C866h ; Загрузить файл по адресу de

; Новые точки C800h MXOS2
bios_printCharReal      = 0C869h ; [MXOS2] Вывод символа на экран (только реальные символы)
bios_printDecWord       = 0C86Ch ; [MXOS2] Вывод числа экран в десятичной форме
bios_setRAMDDriver      = 0C86Fh ; [MXOS2] B=0 обычный, 1 байтовый, 2 регистрация шаблона HL (32 байта)
bios_getDriveFreeSpace  = 0C872h ; [MXOS2] Получить оставшийся свободный объем диска
bios_upperCase          = 0C875h ; [MXOS2] Перевод кода символа КОИ-8 в верхний регистр
bios_strcmp             = 0C878h ; [MXOS2] Сравнение строк, чувствительное к регистру
bios_stricmp            = 0C87Bh ; [MXOS2] Сравнение строк, нечувствительное к регистру
bios_sub_hl_de          = 0C87Eh ; [MXOS2] Вычитание hl и de: hl = hl - de
bios_div_hl_2           = 0C881h ; [MXOS2] Деление hl на 2: hl = hl / 2
bios_mul_hl_de          = 0C884h ; [MXOS2] Умножение hl на de: hl = hl * de
bios_div_hl_de          = 0C887h ; [MXOS2] Деление hl на de: hl = hl / de, de = hl % de
bios_fileCreatePrepare  = 0C88Ah ; [MXOS2] Создать файл и выделить цепочку FAT без записи данных
bios_reservedC88D       = 0C88Dh ; Зарезервировано; кластерный перенос удалён, используйте C8D2h
bios_installVolume      = 0C890h ; [MXOS2] Зарегистрировать драйвер файловой системы тома
bios_getVolume          = 0C893h ; [MXOS2] Получить таблицу и контекст драйвера тома
bios_mountVolume        = 0C896h ; [MXOS2] Повторно смонтировать выбранный том
bios_flushVolume        = 0C899h ; [MXOS2] Сохранить изменения выбранного тома
bios_beepTimer          = 0C89Ch ; [MXOS2] Звуковой сигнал через ВИ53, ABI RAMFOS beep
bios_diskWriteChecked   = 0C89Fh ; [MXOS2] Общий B=5: BC=адрес драйвера, DE=сектор, HL=данные
bios_timerInit          = 0C8A2h ; [MXOS2] ВИ53: каскад CLK1/256/65536; AF/HL разрушены
bios_timerRead          = 0C8A5h ; [MXOS2] HL=убывающий счётчик ВИ53; AF/HL разрушены
bios_uiDraw             = 0C8A8h ; [MXOS2] HL=список объектов, A=смещение X в байтах; AF/BC/DE/HL разрушены
bios_uiInverseRect      = 0C8ABh ; [MXOS2] HL=экран, B=ширина, C=высота; DE сохранён, AF/BC/HL разрушены
bios_uiToolbar          = 0C8AEh ; [MXOS2] HL=строка F1-F8, B/C=цвет клавиш/подписей; AF/BC/DE/HL разрушены
bios_uiCopyRect         = 0C8B1h ; [MXOS2] HL/BC=источник/назначение, D/E=ширина/высота; AF/BC/DE/HL разрушены
bios_fileAssociation    = 0C8B4h ; [MXOS2] HL=расширение, DE/BC=буфер/ёмкость; CF=0 HL=команда, диск сохранён
bios_argNext            = 0C8B7h ; [MXOS2] HL=курсор -> HL=разделитель, DE=слово, B=длина; CF=конец, C сохранён
bios_fileSelectDrive    = 0C8BAh ; [MXOS2] A=диск 0..7; BC/DE/HL сохранены, CF=ошибка
bios_fileRenameOnDrive  = 0C8BDh ; [MXOS2] HL/DE=старое/новое компактное имя на текущем диске
bios_fileDeleteOnDrive  = 0C8C0h ; [MXOS2] HL=компактное имя на текущем диске, CF=ошибка
bios_fileCopy           = 0C8C3h ; [MXOS2] HL=FILE_COPY_REQUEST, CF/A=результат; диск восстановлен
bios_fileGetDrives      = 0C8C6h ; [MXOS2] HL/BC=буфер/ёмкость в байтах (>=8); B=количество
bios_getError           = 0C8C9h ; [MXOS2] A=код DOS, CF=ошибка; BC/DE/HL сохранены
bios_installBlockDriver = 0C8CCh ; A=диск, HL=драйвер, C=log2 физического блока (7..9)
bios_sectorIO           = 0C8CFh ; B=1/2, DE=логический сектор, HL=буфер; BC/DE/HL сохранены
bios_fileStream         = 0C8D2h ; [MXOS2] A=1/2, DE=FILE_STREAM_CURSOR, HL=буфер, BC=байты


;-----------------------------------------------------------------------
; Адрес таблицы переходов драйвера магнитофона
;-----------------------------------------------------------------------

TAPE_DRIVER_JUMPS = bios_tapeReadOld    ; старая таблица переходов магнитофона, сохранена для ABI

;-----------------------------------------------------------------------
; Непереключаемое ОЗУ ПК "Специалист-MX" (0FFC0h-0FFDF, 32 байта)
;-----------------------------------------------------------------------

bios_RAMDRead   = 0FFC0h ; чтение ДОЗУ в выбранном C86Fh режиме (без RAMD.COM — байт)
bios_RAMDWrite  = 0FFD0h ; запись ДОЗУ в выбранном C86Fh режиме

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
IO_PAGE_ROM     =  0FFFEh   ; порт включения ПЗУ
IO_PAGE_STD     =  0FFFFh   ; порт включения STD режима

;-----------------------------------------------------------------------
; Опции драйвера диска bios_diskDriver/0C863h
;-----------------------------------------------------------------------
; Номер функции передаётся в регистре b:
;   B = 1    - записать  сектор (256 байт). DE = номер сектора, HL = буфер.
;   B = 2    - прочитать сектор (256 байт). DE = номер сектора, HL = буфер.
;   B = 3    - информация о диске. DE = размер диска, HL = DISK_INFO,
;              BC = шаг выделения по чипам (0: линейно). Ненулевой шаг
;              также объявляет поддержку B=4: дождаться отложенных записей.
;   B = 4    - дождаться завершения всех отложенных операций записи.
;              Требуется не менее 320 свободных байт стека, BC/DE/HL сохраняются.
;   B = 5    - записать сектор (256 байт) с чтением и сравнением.
;              DE = номер сектора, HL = буфер, CF=ошибка.
;   B = 0FFh - адрес имени и описания в HL (две строки с нулём).
;              HL = имя,0,описание,0 (ASCII, до 22/30 символов); CF=0.
;              B = технология DISK_TECH_*, C=возможности DISK_CAP_*.
;              CF=1 или HL=0: описания и сведений о возможностях нет.
;              AF разрушается.
DISK_TECH_ROM          = 'O'
DISK_TECH_RAM          = 'R'
DISK_TECH_FLASH        = 'F'
DISK_TECH_FLOPPY       = 'D' ; Зарезервировано для дискетного драйвера
DISK_TECH_HDD          = 'H' ; Зарезервировано для драйвера жёсткого диска
DISK_CAP_READ          = 01h
DISK_CAP_WRITE         = 02h
DISK_CAP_VERIFY        = 04h    ; B=5: синхронная запись с чтением и сравнением
DISK_CAP_FLUSH         = 08h    ; B=4: завершение отложенной записи

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
IMAGE_HEADER_SIZE = 16
FILE_DESCR_SIZE  = 32   ; размер дескриптора файла в каталоге (байт)
FILE_NAME_LENGTH = 8    ; длина имени файла  (байт)
DOS_ERR_IO = 1
DOS_ERR_NOT_FOUND = 2
DOS_ERR_EXISTS = 3
DOS_ERR_FULL = 4
DOS_ERR_DIR_FULL = 5
DOS_ERR_ACCESS = 6
DOS_ERR_ARGUMENT = 7
DOS_ERR_CORRUPT = 8
DOS_ERR_NO_DRIVER = 9
DOS_ERR_UNSUPPORTED = 10
DOS_ERR_BAD_IMAGE = 11

FAT_ITEM_SIZE    = 2    ; размер записи в таблице FAT (байт)

SECTOR_SIZE  = 00100h   ; размер сектора 256 байт
CLUSTER_SIZE = 00100h   ; размер кластера 256 байт

; Специальные коды кластеров FAT
FAT12_BAD = 00FF7h  ; плохой кластер
FAT16_BAD = 0FFF7h
FAT12_EOC = 00FFFh  ; последний кластер файла
FAT16_EOC = 0FFFFh
FAT12_RES = 00FFFh  ; зарезервированный кластер
FAT16_RESERVED_MIN = 0FFF0h
FAT16_EOC_MIN = 0FFF8h
FAT16_RES = 0FFF0h

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

; Вызывающая программа выделяет запрос копирования и независимый буфер.
; Имена source/destination — компактные 8.3, первые 11 байт дескриптора.
; Курсор потока активного FAT-тома; хранится у вызывающей программы.
; Не содержит размера файла: вызывающая сторона ограничивает чтение EOF.
    STRUCT FILE_STREAM_CURSOR
cluster     DW 0
offset      DW 0
    ENDS

    STRUCT FILE_COPY_REQUEST
buffer              DW 0
bufferSize          DW 0        ; Байты, минимум 256; неполный сектор не используется.
sourceDrive         DB 0
destinationDrive    DB 0
source              FILE_DESCRIPTOR
destination         FILE_DESCRIPTOR
sourceCluster       DW 0        ; Рабочие поля изменяет только DOS.
destinationCluster  DW 0
blocksLeft          DW 0
sourceOffset        DW 0
destinationOffset   DW 0
    ENDS
FILE_OP_ERROR  = 1
FILE_OP_EXISTS = 2

;-----------------------------------------------------------------------
; Информация о файле (компактный вариант дескриптора)
; Почти совместимо с дескриптором оригинального Коммандера
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

FILE_INFO_SIZE = 16     ; размер структуры (байт)

    STRUCT FILE_INFO    ; 16 байт
name            BLOCK   FILE_NAME_LENGTH    ; смещение 00h, 8 байт  - имя файла
ext             BLOCK   3                   ; смещение 08h, 3 байта - расширение имени файла
attrib          BLOCK   1                   ; смещение 0Bh, 1 байт  - атрибуты файла
reserved        BLOCK   2                   ; два нулевых байта компактной записи, не адрес
size            BLOCK   2                   ; смещение 0Eh, младшие 16 бит точного размера файла
    ENDS

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
blockSize           DW 256      ; Физический блок выбранного драйвера
sectorShift         DB 8        ; log2 логического сектора
blockShift          DB 8        ; log2 блока драйвера
clusterBytes        DW 256      ; Размер кластера в байтах, до 32768
fatCount            DB 1        ; Число копий FAT
physicalBlocks      DW 0        ; Ёмкость драйвера B=3, в физических блоках
    ENDS

DISK_NO_DRIVER = 0
DISK_INVALID   = 1
DISK_VALID     = 2

;-----------------------------------------------------------------------
; Буферы в памяти
;-----------------------------------------------------------------------

; Границы STD по мапперу MMX: вся верхняя страница 4 КБ — устройства.
STD_IO_ADDR = 0F000h
STD_KEYB_ADDR = 0F800h

; Дисковый кэш, в STD недоступен.
    IFNDEF FAT_CACHE_ADDR
FAT_CACHE_ADDR = 0FB00h ; адрес кэша секторов fat в памяти
    ENDIF
    IFNDEF FAT_CACHE_SIZE
FAT_CACHE_SIZE = 1024   ; размер кэша в байтах
    ENDIF
FAT_CACHE_CAPACITY = FAT_CACHE_SIZE / SECTOR_SIZE   ; сколько секторов вмещается в кэш
    ASSERT FAT_CACHE_CAPACITY > 0 && FAT_CACHE_CAPACITY <= 255
    ASSERT FAT_CACHE_SIZE % SECTOR_SIZE == 0
    ASSERT FAT_CACHE_ADDR % SECTOR_SIZE == 0
    ASSERT FAT_CACHE_ADDR + FAT_CACHE_SIZE <= 10000h

; Временный буфер BAT: 0000h-7FFFh и байт терминатора по 8000h.
; Программы могут затереть буфер: исполнитель перечитывает BAT после команды.
BAT_BUFFER = 0000h
BAT_BUFFER_SIZE = 08000h

; Резидент RAMD.COM делит окно расширений с FLASHSYS.COM.
RAMD_RESIDENT_ADDR = 0F740h ; F740h-F7FFh (192 байта), FLASHSYS ниже

SYSTEM_FLASH_ADDR = 0F600h ; F600h-F73Fh
DISK_DRIVER_ADDR = 0F800h  ; F800h-F9FFh
NC_LOAD_ADDR = 00000h      ; NC — обычная программа с собственными буферами
SYSTEM_PROGRAM_ADDR = 0F000h ; Верхний этап RUN
SYSTEM_PROGRAM_END = 0F600h
DOS_END_MAX = FONT_ADDR   ; Далее компактный системный шрифт
    IFDEF DOS_SPLIT_BUILD
DOS_CODE_START = 04000h   ; Промежуточная отдельная сборка, без подключения к NC
DOS_CODE_END_MAX = 08C00h ; До переходников и системного стека
    ELSE
DOS_CODE_START = 0C000h
DOS_CODE_END_MAX = DOS_END_MAX
    ENDIF

; Стек доступен в STD, командная строка нужна только в MX.
    ASSERT NEW_MEMORY_MAP
CMD_LINE   = 0FA00h ; FA00h-FA81h, только MX
CMD_LINE_SIZE = 130
APP_MEMORY_END = 08C00h   ; Далее переходники запуска и системный стек
STACK_BOTTOM = 08E00h
STACK_ADDR = 08FDFh
NC_STATE_ADDR = 0FFB0h ; 12 байт; FFBEh — детектор, FFBFh — метка FORMAT

    ASSERT DOS_END_MAX <= FONT_ADDR
    ASSERT SYSTEM_PROGRAM_END <= SYSTEM_FLASH_ADDR
    ASSERT CMD_LINE >= DISK_DRIVER_ADDR + 0200h
    ASSERT CMD_LINE + CMD_LINE_SIZE <= FAT_CACHE_ADDR
    ASSERT SYSTEM_FLASH_ADDR + 0140h <= RAMD_RESIDENT_ADDR
    ASSERT RAMD_RESIDENT_ADDR + 00C0h <= DISK_DRIVER_ADDR
    ASSERT DISK_DRIVER_ADDR + 0200h <= FAT_CACHE_ADDR
    ASSERT FAT_CACHE_ADDR + FAT_CACHE_SIZE <= 0FF00h
    ASSERT CMD_LINE_SIZE == 130
    ASSERT STACK_BOTTOM < STACK_ADDR && STACK_ADDR <= STD_IO_ADDR
    ASSERT DOS_END_MAX <= STD_IO_ADDR
    IF LOAD_FONT
        ASSERT FONT_ADDR + 0500h <= STD_IO_ADDR
    ENDIF


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
