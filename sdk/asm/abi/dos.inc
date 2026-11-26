; MXOS
; Операции DOS и физического обмена.

    IFNDEF MXOS_ABI_DOS_INC
    DEFINE MXOS_ABI_DOS_INC

    INCLUDE "files.inc"

DIR_OP_GET      = 0  ; DE=кластер текущего каталога; 0 означает корень.
DIR_OP_SET      = 1  ; DE=ранее полученный кластер каталога на текущем диске.
DIR_OP_CHDIR    = 2  ; HL=буфер MAX_PATH, ASCIIZ; допускается префикс диска.
DIR_OP_LIST     = 3  ; Как fileList, но вместе с каталогами (. и .. включены).
DIR_OP_GETCWD   = 4  ; HL=буфер MAX_PATH; абсолютный путь A:\..., ASCIIZ. Нужна DOS.
DIR_OP_COMMAND  = 5  ; HL=строка CD/CHDIR, A=FFh если команда не встроена. Нужна DOS.
DIR_OP_RESOLVED = 6  ; DE=контекст последнего успешного fileNamePrepare, до выбора диска.
DIR_OP_MKDIR    = 7  ; HL=путь, создать пустой каталог.
DIR_OP_RMDIR    = 8  ; HL=путь, удалить пустой каталог.

CLI_OP_COMMAND     = 0   ; DE=обработчик совпадений (0=отказ), HL=команда; FFh/CF=0 — внешняя команда; B: вывод/выход/контекст.
CLI_OP_COPY        = 1   ; HL=FILE_COPY_REQUEST; буфер данных принадлежит DOS.
CLI_OP_RENAME      = 2   ; HL/DE=имена на текущем диске и в текущем каталоге.
CLI_OP_DELETE      = 3   ; HL=имя файла; строковые DEL/ERASE принимают также пустые каталоги.
CLI_OP_MKDIR       = 4   ; HL=путь MAX_PATH.
CLI_OP_CHDIR       = 5   ; HL=путь MAX_PATH.
CLI_OP_RMDIR       = 6   ; HL=путь MAX_PATH.
CLI_OP_MOVE        = 7   ; HL=FILE_COPY_REQUEST; на одном томе перенос записи без данных/FAT.
CLI_OP_QUERY       = 8   ; HL=строка без исполнения; B как COMMAND, A=FFh внешняя команда.
CLI_OP_OBSERVER    = 9   ; HL=наблюдатель изменений (0=отключить); A=диск, DE=каталог, C=масштаб.
CLI_OP_ASSOCIATION = 10  ; HL=расширение ASCIIZ в буфере MAX_PATH; CF=0 DE=команда в банке DOS до CR/FF, до следующего вызова CLI; CF=1 нет/ошибка.
; B после COMMAND/QUERY: bit0 — вывод, bit1 — EXIT, bit2 — CD/смена диска.

;-----------------------------------------------------------------------
; Операции дискового обмена: физический ABI и DOS RAW_IO
;-----------------------------------------------------------------------
; Номер функции передаётся в регистре b:
;   B = 1/2  - записать/прочитать физический сектор. DE = LBA, HL = буфер
;              длиной DISK_INFO.blockSize (A:/B:/H: 256 байт, D: 512 байт).
; В банковой DOS функции 3/FFh обслуживает реестр. DISK_INFO содержит
; смонтированную BPB; физические параметры и строки хранятся в реестре DOS.
;   B = 3    - информация о диске. DE = размер диска, HL = DISK_INFO,
;              BC = шаг выделения по чипам (0: линейно). Ненулевой шаг
;              также объявляет поддержку B=4: дождаться отложенных записей.
;   B = 4    - дождаться завершения всех отложенных операций записи.
;   B = 5    - записать физический сектор с чтением и сравнением.
;              DE = номер сектора, HL = буфер, CF=ошибка.
;              Локальный вход для блока 256 в EXEC требует 320 байт стека.
;              BC/DE/HL сохранены.
;   B = 0FFh - физический BIOS/A: B=DISK_TECH_*, C=DISK_CAP_*, HL=0,
;              CF=0 при успехе; AF разрушается.
; Имена/описания не входят в физический ABI. Получение строк выбранного диска:
;   LD B,0FFh / MXOS_CALL_REGS MXOS_DOS_RAW_IO -> CF=0, B=технология, C=возможности,
;   HL=NEAR-адрес двух ASCIIZ: имя,0,описание,0, вместе <= 64 байт.
; Источник — реестр DOS; межбанковому клиенту возвращается локальная копия.
; Указатель результата не является постоянным: строку нужно скопировать,
; если она требуется после следующих RAW_IO/замены драйвера. CF=1 — ошибка.
DISK_TECH_ROM             = 'O'
DISK_TECH_RAM             = 'R'
DISK_TECH_FLASH           = 'F'
DISK_TECH_FLOPPY          = 'D'  ; Дисковод
DISK_TECH_HDD             = 'H'  ; Зарезервировано для драйвера жёсткого диска
DISK_TECH_MX_SYSTEM_FLASH = 'S'
DISK_TECH_MX_SYSTEM_ROM   = 'T'
DISK_TECH_MX_VV55_FLASH   = 'P'
DISK_TECH_MX_VV55_ROM     = 'Q'
DISK_CAP_READ             = 01h
DISK_CAP_WRITE            = 02h
DISK_CAP_VERIFY           = 04h  ; B=5: синхронная запись с чтением и сравнением
DISK_CAP_FLUSH            = 08h  ; B=4: завершение отложенной записи
RAW_OP_PREPARE            = 6    ; MXOS_DOS_RAW_IO: подготовить локальные точки обмена

;-----------------------------------------------------------------------
; Информация об активном накопителе (диске)
;-----------------------------------------------------------------------

    STRUCT DISK_INFO
sectorSize          BLOCK  2    ; размер сектора, байт (256, 512, 1024)
sectorsPerCluster   BLOCK  2    ; размер кластера, секторов (1, 2, 4, 8)
resSectors          BLOCK  2    ; сколько секторов занимает зарезервированная область = первый сектор таблицы fat
fatSectors          BLOCK  2    ; сколько секторов занимает таблица fat
dirSectors          BLOCK  2    ; сколько секторов занимает корневой каталог
totalSectors        BLOCK  2    ; сколько всего секторов на диске
rootDirMaxFiles     BLOCK  2    ; максимум файлов в корневом каталоге
descrPerSector      BLOCK  2    ; сколько дескрипторов файлов вмещается в один сектор
fatNumCells         BLOCK  2    ; количество ячеек в таблице fat (с насыщением на FFFFh)
dirStartSector      BLOCK  2    ; первый сектор корневого каталога
dataStartSector     BLOCK  2    ; первый сектор области данных
volumeLabel         BLOCK  12   ; метка тома и 0 в конце
isValid             BLOCK  1    ; Состояние DISK_* после монтирования
allocationStep      BLOCK  2    ; Подсказка физического драйвера B=3: шаг по чипам, секторов; 0 = линейно
dataClusters        BLOCK  2    ; Допустимые кластеры данных, без двух резервных записей FAT
blockSize           DW     256  ; Физический блок выбранного драйвера
sectorShift         DB     8    ; log2 логического сектора
blockShift          DB     8    ; log2 блока драйвера
clusterBytes        DW     256  ; Размер кластера в байтах, до 32768
fatCount            DB     1    ; Число копий FAT
physicalBlocks      DW     0    ; Ёмкость драйвера B=3, в физических блоках
    ENDS

DISK_NO_DRIVER = 0
DISK_INVALID   = 1
DISK_VALID     = 2
DISK_NO_MEDIA  = DOS_ERR_NO_MEDIA-DOS_ERR_NO_DRIVER

    ASSERT DISK_INFO == 48

; Буфер командной строки основной памяти в режиме MX.
CMD_LINE      = 0EE90h
CMD_LINE_SIZE = 130

    ENDIF
