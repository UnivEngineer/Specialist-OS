; MXOS
; Файловые объекты, пакеты и операции DOS.

    IFNDEF MXOS_ABI_FILES_INC
    DEFINE MXOS_ABI_FILES_INC

    INCLUDE "calls.inc"
    INCLUDE "dos-errors.inc"
    INCLUDE "../formats/directory.inc"

MAX_PATH = 128  ; Путь в байтах, включая завершающий 0.

;-----------------------------------------------------------------------
; Краткая информация о файле: FILE_INFO, 16 байт для списка каталога
;-----------------------------------------------------------------------
; Это результат перечисления файлов, не объект открытого файла для fopen.
FILE_INFO_SIZE = 16  ; размер структуры (байт)

    STRUCT  FILE_INFO                       ; 16 байт
name            BLOCK   FILE_NAME_LENGTH    ; смещение 00h, 8 байт  - имя файла
ext             BLOCK   3                   ; смещение 08h, 3 байта - расширение имени файла
attrib          BLOCK   1                   ; смещение 0Bh, 1 байт  - атрибуты файла
size            BLOCK   4                   ; смещение 0Ch, размер файла в байтах, младшим байтом вперёд
    ENDS

    ASSERT FILE_INFO == FILE_INFO_SIZE

;-----------------------------------------------------------------------
; Единый файловый API: MXOS_DOS_FILEHANDLE, A=FILE_OP_*
;-----------------------------------------------------------------------
; HL указывает на FILE_HANDLE в памяти вызывающей программы.
; Параметры и результаты находятся в объекте; A/CF возвращают код DOS.
; C SDK отображает эти операции на fopen/read/write/fseek/fflush/fclose.
; DOS лениво загружает FILEOPS.DLL в верхнюю арену своей страницы.
FILE_OP_OPEN      = 0  ; Найти/создать файл и заполнить объект.
FILE_OP_READ      = 1  ; Читать FH_COUNT байт в FH_BUFFER.
FILE_OP_WRITE     = 2  ; Записать FH_COUNT байт из FH_BUFFER.
FILE_OP_SEEK      = 3  ; Изменить позицию по FH_SEEK/FH_ORIGIN.
FILE_OP_FLUSH     = 4  ; Сохранить размер, запись каталога и кэш.
FILE_OP_CLOSE     = 5  ; Выполнить FLUSH и закрыть объект.
FILE_OP_OPEN_AT   = 8  ; Имя 8.3 + FH_DRIVE; FH_SEEK.low=родительский каталог.
FILE_OP_OPEN_PATH = 7  ; FH_BUFFER=ASCIIZ-путь в памяти вызывающего.
FILE_OP_OPTIONS   = 6  ; FH_ORIGIN=0 get / 1 exchange, FH_COUNT=флаги DOS.

;-----------------------------------------------------------------------
; Флаги открытия FILE_HANDLE: битовая маска в FH_MODE
;-----------------------------------------------------------------------
; Требуется READ и/или WRITE. CREATE/TRUNCATE/APPEND/VERIFY/SCATTER требуют WRITE.
FILE_MODE_READ     = 1   ; Разрешено чтение.
FILE_MODE_WRITE    = 2   ; Разрешена запись.
FILE_MODE_CREATE   = 4   ; Создать отсутствующий файл.
FILE_MODE_TRUNCATE = 8   ; При открытии обнулить существующий файл.
FILE_MODE_VERIFY   = 32  ; Требовать B=5 для данных этого файла.
FILE_MODE_SCATTER  = 64  ; Псевдослучайный поиск новых свободных кластеров.
FILE_MODE_APPEND   = 16  ; Перед каждой записью перейти в конец файла.

;-----------------------------------------------------------------------
; Объект открытого файла FILE_HANDLE: 64 байта, аналог FILE для C SDK
;-----------------------------------------------------------------------
; Здесь объявлены смещения полей, а FILE_HANDLE — размер объекта.
; Это не запись каталога и не числовой файловый номер: память выделяет
; вызывающая программа, скрытой таблицы открытых файлов DOS не создаёт.
; Перед OPEN объект обнуляют, задают имя 8.3 (11 байт с пробелами),
; FH_DRIVE и FH_MODE. OPEN_PATH вместо имени принимает ASCIIZ в FH_BUFFER;
; путь и объект принадлежат памяти вызывающего. Первые 32 байта — FILE_DESCRIPTOR.
; READ/WRITE получают FH_BUFFER и FH_COUNT, SEEK — FH_SEEK/FH_ORIGIN.
; Рабочие поля позиции/курсора/каталога ведёт DOS; объект сохраняют
; между вызовами и не перекрывают буфером обмена. Слова little-endian.
FH_POSITION   = 32  ; 4 байта: текущая позиция от начала файла.
FH_CURSOR     = 36  ; 4 байта: FILE_STREAM_CURSOR для этой позиции.
FH_DRIVE      = 40  ; 1 байт: диск файла, 0..7 (A:..H:).
FH_MODE       = 41  ; 1 байт: маска FILE_MODE_*.
FH_STATE      = 42  ; 1 байт: 046h — открыт, 0 — закрыт.
FH_ERROR      = 43  ; 1 байт: сохранённый код ошибки DOS, 0 — нет ошибки.
FH_EOF        = 44  ; 1 байт: признак достижения конца при чтении.
FH_DIR_SECTOR = 45  ; 2 байта: сектор с записью файла в каталоге.
FH_DIR_OFFSET = 47  ; 2 байта: смещение записи внутри этого сектора.
FH_BUFFER     = 49  ; 2 байта: адрес буфера READ/WRITE.
FH_COUNT      = 51  ; 2 байта: запрошено на входе, передано на выходе.
FH_SEEK       = 53  ; 4 байта: знаковое смещение SEEK; DOS изменяет поле.
FH_ORIGIN     = 57  ; 1 байт: база SEEK, 0=начало, 1=позиция, 2=конец.
FH_PREVIOUS   = 58  ; 2 байта: предыдущий кластер.
FH_ALLOC_SEED = 60  ; 2 байта: LFSR для SCATTER, OPEN задаёт ACE1h.
FH_VALID      = 62  ; 1 байт: курсор согласован с позицией, если не 0.
FH_OPERATION  = 63  ; 1 байт: C-переходник передаёт отсюда FILE_OP_* в A.
FILE_HANDLE   = 64  ; Полный размер объекта, включая запись каталога.

;-----------------------------------------------------------------------
; Запрос копирования FILE_COPY_REQUEST: MXOS_DOS_FILE_COPY, 86 байт
;-----------------------------------------------------------------------
; HL=запрос в памяти вызывающей программы; данные копируются в банке DOS.
; Реализация находится в DOS.SYS, исполняемой в собственной странице.
; source/destination — полные 32-байтные FILE_DESCRIPTOR. Вызывающий задаёт
; их имена 8.3 и номера дисков, DOS читает/заполняет метаданные и рабочие поля.
; Внешний вызов не требует буфера клиента. Исходный диск восстанавливается.
; Результат: CF=0,A=0 — готово; CF=1,A=FILE_OP_* ниже — отказ копирования.
; Эти два результата исторического API не совпадают с общей шкалой DOS_ERR_*.
    STRUCT FILE_COPY_REQUEST
buffer                  DW 0                ; Внешний запрос: 0; внутренний DOS: адрес пачки данных.
bufferSize              DW 0                ; Внешний запрос: 0; внутренний DOS: ёмкость пачки в байтах.
sourceDrive             DB 0                ; Диск источника, 0..7.
destinationDrive        DB 0                ; Диск назначения, 0..7.
source                  FILE_DESCRIPTOR     ; Имя и метаданные источника.
destination             FILE_DESCRIPTOR     ; Имя и метаданные назначения.
sourceCluster           DW 0                ; Рабочие поля изменяет только DOS.
destinationCluster      DW 0                ; Кластер продолжения записи.
blocksLeft              DW 0                ; Остаток обмена порциями до 256 байт.
sourceOffset            DW 0                ; Смещение курсора источника внутри кластера.
destinationOffset       DW 0                ; Смещение курсора назначения внутри кластера.
sourceDirectory         DW 0                ; Каталог источника, 0=корень.
destinationDirectory    DW 0                ; Каталог назначения, 0=корень.
onExists                DW 0                ; Внешний callback: A=FAR-банк DOS, HL=FILE_DESCRIPTOR назначения.
                                ; Скопировать дескриптор локально до BIOS/текстовых вызовов.
                                ; Ответ A=0 No, 1 Yes, 2 Yes to all, 3 No to all; CF=отмена.
                                ; 0=вернуть EXISTS. Обработчик не вызывает DOS/CLI.
                                ; AF/BC/DE/HL разрушены; CF=отмена, без удаления источника.
    ENDS
    ASSERT FILE_COPY_REQUEST == 86
FILE_OP_ERROR  = 1  ; Общий отказ копирования.
FILE_OP_EXISTS = 2  ; Назначение существует, перезапись не выполнена.

; Batch enumeration: A=operation, HL=ENUM_REQUEST in the caller's bank.
ENUM_OPEN         = 0
ENUM_NEXT         = 1
ENUM_CLOSE        = 2
ENUM_RECURSIVE    = 1
ENUM_CHECK_CANCEL = 2
ENUM_MAX_DEPTH    = 32
    STRUCT ENUM_ENTRY
descriptor      FILE_DESCRIPTOR
path            BLOCK MAX_PATH,0    ; Canonical ASCIIZ; bytes after NUL undefined.
    ENDS
    ASSERT ENUM_ENTRY == 160
    STRUCT ENUM_REQUEST
handle          DW 0
path            DW 0
buffer          DW 0
capacity        DW 0
count           DW 0
flags           DB 0
state           DB 0    ; 0 running, 1 end; CLOSE is required in either state.
    ENDS

; Cooperative FILEOPS: A=operation, HL=FILEOPS_JOB in the caller's bank.
FILEOPS_BEGIN         = 0
FILEOPS_STEP          = 1
FILEOPS_ANSWER        = 2
FILEOPS_CANCEL        = 3
FILEOPS_CLOSE         = 4
FILEOPS_SKIP_PREPARE  = 5
FILEOPS_NO_PREPARE    = 1
FILEOPS_PHASE_COPY    = 0
FILEOPS_PHASE_PREPARE = 1
FILEOPS_PHASE_DELETE  = 2
FILEOPS_COPY          = 0
FILEOPS_MOVE          = 1
FILEOPS_DELETE        = 2
FILEOPS_RUNNING       = 0
FILEOPS_CONFIRM       = 1
FILEOPS_DONE          = 2
FILEOPS_FAILED        = 3
    STRUCT FILEOPS_JOB
handle          DW 0                ; Opaque, generation checked; BEGIN ignores input.
mode            DB 0                ; COPY/MOVE/DELETE, consumed only by BEGIN.
state           DB 0                ; RUNNING/CONFIRM/DONE/FAILED.
answer          DB 0                ; No/Yes/YesAll/NoAll, 0..3.
error           DB 0                ; DOS_ERR_* for FAILED.
bytes           DD 0                ; Bytes successfully written, including earlier files.
files           DW 0                ; Files completed (not directories).
flags           DB 0                ; BEGIN: FILEOPS_NO_PREPARE skips the preflight.
phase           DB 0                ; COPY/PREPARE/DELETE, separate from completion state.
known           DB 0                ; 1: total is complete; 0: never show percent/bar.
reserved        DB 0
total           DD 0                ; Logical source bytes, accumulated during preparation.
required        DW 0                ; Conservative additional destination clusters.
path            DW 0                ; Optional MAX_PATH buffer in the caller's bank.
totalFiles      DW 0                ; Complete source file count when known=1.
request         FILE_COPY_REQUEST   ; BEGIN input; returned only for CONFIRM.
    ENDS
    ASSERT FILEOPS_JOB <= 128

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
parent     DW 0                ; Явный кластер родителя, 0=корень.
    ENDS

    STRUCT FILE_PATH_IO
path        DW 0
buffer      DW 0
count       DW 0
actual      DW 0
descriptor  FILE_DESCRIPTOR
    ENDS
    ASSERT FILE_PATH_IO == 40

    ENDIF
