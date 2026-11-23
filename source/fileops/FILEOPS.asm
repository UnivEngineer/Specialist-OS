; Lazy DOS file-operation engine. Native MXOS container, DRIVER placement.
    INCLUDE "../include/mxos.inc"
    DEFINE BANKED_DOS_BUILD 1
KD_OBJECT = DOS_BANK_OBJECT_ADDR
    IFDEF MX_FLOPPY_BOOT
    INCLUDE "../../bin/mx/fileops-imports.inc"
    ELSE
    INCLUDE "../../bin/fileops-imports.inc"
    ENDIF
    ORG DOS_FILEOPS_ADDR
    jp foEntry
    INCLUDE "../include/fileops-host.inc"

;----------------------------------------------------------------------------
; MXOS
;
; Выполнить запрос резидентной части DOS к движку файловых операций.
;
; вход:
;   a = внутренняя операция оверлея. Для синхронных COPY/MOVE hl указывает на
;   FILE_COPY_REQUEST. Для пошагового запуска используется FILEOPS_JOB. Вход вызывается
;   в банке DOS.
;
; выход:
;   Синхронные операции восстанавливают контекст диска и каталогов. Управляющие
;   операции возвращают результат своего шага, состояние хранится в foJob. Запрос пути
;   возвращает hl = адрес текста, bc = длина с нулём и a = версия пути.
;----------------------------------------------------------------------------

foEntry:
    cp 11
    jp z,foDeleteSync
    cp 12
    jp z,foGetPath
    cp 13
    jp z,foSkipPrepare
    cp 10
    jp z,foMkdir
    cp 4
    jp nc,foControl
    push af
    xor a
    ld (foAsync),a
    pop af
    push af
    push hl
    call foContextSave
    pop hl
    pop af
    call foSync
    push af
    call foContextRestore
    pop af
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Выбрать проверяемый или уже подготовленный путь синхронного COPY/MOVE.
;
; вход:
;   hl = FILE_COPY_REQUEST, a = 0 для COPY, 1 для MOVE, 2 или 3 для соответствующей
;   внутренней операции с проверенным запросом.
;
; выход:
;   При успехе cf = 0, при ошибке cf = 1 и a = статус файловой операции. Буферы и поля
;   прогресса принадлежат текущему запуску. af, bc, de и hl рабочие.
;----------------------------------------------------------------------------

foSync:
    push af
    xor a
    ld (foPathText),a
    pop af
    cp 2
    jp nc,foInternal
    or a
    jp z,foCopy
    jp foCopyForMove
foInternal:
    sub 2
foInternalMode:
    ld (treeMoveMode),a
    xor a
    ld (v_dosError),a
    ld (v_copyRequest),hl
    ld e,FILE_COPY_REQUEST.bufferSize
    call fc_ReadWord
    ld a,h
foCapacity:
    ld (v_copyCapacity),a
    jp fc_RangeReady
    INCLUDE "step.inc"
    INCLUDE "copy.inc"
    INCLUDE "tree.inc"
    INCLUDE "confirm.inc"
    INCLUDE "mkdir.inc"
    INCLUDE "prepare.inc"
    INCLUDE "delete.inc"
fileopsEnd:
    ASSERT $ <= DOS_SERVICE_END
    END
