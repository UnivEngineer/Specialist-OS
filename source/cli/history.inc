; История принадлежит CLI.COM, не занимает резидентную память DOS.
; 32 записи MAX_PATH в файле B: или запасном кольце BSS. browse=0 — черновик.
; Подпрограммы разрушают AF/BC/DE/HL и флаги; внешних ABI-входов нет.
CLI_HISTORY_COUNT = 32
    ASSERT MAX_PATH == 128

;----------------------------------------------------------------------------
; MXOS
;
; Добавить завершённую команду в кольцо истории CLI.
;
; вход:
;   CMD_LINE содержит строку с завершающим нулём в пределах MAX_PATH.
;
; выход:
;   Пустая строка и повтор последней команды пропускаются. В остальных случаях
;   обновлены запись, индекс следующей записи и число занятых мест. af, bc, de, hl
;   рабочие.
;----------------------------------------------------------------------------

cliHistoryAdd:
    ld hl,CMD_LINE
    ld a,(hl)
    or a
    ret z
    ld a,(cliHistoryCount)
    or a
    jp z,cliHistoryAppend
    ld a,(cliHistoryNext)
    dec a
    call cliHistoryReadSlot
    jp c,cliHistoryMemoryAppend
    ex de,hl
    ld hl,CMD_LINE
cliHistoryCompare:
    ld a,(de)
    cp (hl)
    jp nz,cliHistoryAppend
    or a
    ret z
    inc hl
    inc de
    jp cliHistoryCompare
cliHistoryAppend:
    ld a,(cliHistoryOnDisk)
    or a
    jp z,cliHistoryMemoryAppend
    call cliHistoryWriteSlot
    ret nc
cliHistoryMemoryAppend:
    ld a,(cliHistoryNext)
    call cliHistorySlot
    ex de,hl
    ld hl,CMD_LINE
    call cliHistoryCopy
cliHistoryAdvance:
    ld a,(cliHistoryNext)
    inc a
    and CLI_HISTORY_COUNT-1
    ld (cliHistoryNext),a
    ld hl,cliHistoryCount
    ld a,(hl)
    cp CLI_HISTORY_COUNT
    ret z
    inc (hl)
    ret
;----------------------------------------------------------------------------
; MXOS
;
; Найти адрес записи в кольце истории.
;
; вход:
;   a = индекс, используются младшие пять бит.
;
; выход:
;   hl = cliHistoryEntries + (индекс × MAX_PATH). bc сохранён; de, a и флаги
;   изменяются.
;----------------------------------------------------------------------------

cliHistorySlot:
    and CLI_HISTORY_COUNT-1
    ld l,a
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,cliHistoryEntries
    add hl,de
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Скопировать текст записи истории вместе с завершающим нулём.
;
; вход:
;   hl = источник с нулём в пределах MAX_PATH, de = буфер назначения достаточной длины.
;
; выход:
;   hl и de указывают за нулевым байтом, a = 0, z = 1, cf = 0. bc сохранён.
;----------------------------------------------------------------------------

cliHistoryCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp nz,cliHistoryCopy
    ret
cliHistoryUp:
    ld a,(cliHistoryCount)
    ld b,a
    ld a,(cliHistoryBrowse)
    cp b
    jp z,lineEditLoop
    or a
    jp nz,cliHistoryOlder
    ld hl,CMD_LINE
    ld de,cliHistoryDraft
    call cliHistoryCopy
    xor a
cliHistoryOlder:
    inc a
    jp cliHistoryRecall
cliHistoryDown:
    ld a,(cliHistoryBrowse)
    or a
    jp z,lineEditLoop
    dec a
    jp cliHistoryRecall
cliHistoryClear:
    xor a
    ld (cliHistoryBrowse),a
    jp lineEditClear
cliHistoryRecall:
    ld (cliHistoryBrowse),a
    ld b,a
    ld hl,cliHistoryDraft
    or a
    jp z,cliHistoryRecallCopy
    ld a,(cliHistoryNext)
    sub b
    call cliHistoryReadSlot
    jp nc,cliHistoryRecallCopy
    xor a
    ld (cliHistoryBrowse),a
    ld hl,cliHistoryDraft
cliHistoryRecallCopy:
    ld de,CMD_LINE
    call cliHistoryCopy
    ex de,hl
    ld de,CMD_LINE+1
    call lineEditMath.sub_hl_de
    ld a,l
    ld (lineEditLength),a
    ld (lineEditPosition),a
    xor a
    ld (lineEditOffset),a
    jp lineEditRedraw

; Внешняя программа может затереть CLI. История на B: переживает новый вход.

;----------------------------------------------------------------------------
; MXOS
;
; Подготовить файл истории B:\CLI.HST и загрузить заголовок.
;
; вход:
;   cliHistoryDisabled разрешает дисковый обмен при нулевом значении. В оперативной
;   памяти выделен блок cliHistoryData.
;
; выход:
;   При корректном файле восстановлены индексы. Записи читаются при обращении.
;   При отсутствии файла создаётся пустое кольцо. Контекст диска восстанавливается.
;----------------------------------------------------------------------------

cliHistoryLoad:
    ld a,(cliHistoryDisabled)
    or a
    ret nz
    call cliHistoryDiskBegin
    jp c,cliHistoryDisable
    ld hl,cliHistoryDescriptor
    MXOS_CALL_REGS MXOS_DOS_FILE_LOAD_INFO
    jp nc,cliHistoryLoadSize
    MXOS_CALL_REGS MXOS_DOS_GET_ERROR
    cp DOS_ERR_NOT_FOUND
    jp nz,cliHistoryDisable
    call cliHistoryDiskEnd
    call cliHistorySave
    ld a,(cliHistoryDisabled)
    or a
    ret nz
    ld a,1
    ld (cliHistoryOnDisk),a
    ret
cliHistoryLoadSize:
    ld hl,(cliHistoryDescriptor+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,cliHistoryDiskEnd
    ld hl,(cliHistoryDescriptor+FILE_DESCRIPTOR.size)
    ld de,cliHistoryEnd-cliHistoryData
    call mon2_cmp_hl_de
    jp nz,cliHistoryDiskEnd
    call cliHistoryOpenRead
    jp c,cliHistoryDisable
    ld hl,0
    ld de,cliHistoryData
    ld bc,6
    ld a,FILE_OP_READ
    call cliHistoryTransfer
    call flClose
    jp c,cliHistoryDisable
    ld hl,(cliHistoryData)
    ld de,0434Dh
    call mon2_cmp_hl_de
    jp nz,cliHistoryInvalid
    ld hl,(cliHistoryData+2)
    ld de,03148h
    call mon2_cmp_hl_de
    jp nz,cliHistoryInvalid
    ld a,(cliHistoryCount)
    cp CLI_HISTORY_COUNT+1
    jp nc,cliHistoryInvalid
    ld a,(cliHistoryNext)
    cp CLI_HISTORY_COUNT
    jp nc,cliHistoryInvalid
cliHistoryUseDisk:
    ld a,1
    ld (cliHistoryOnDisk),a
    jp cliHistoryDiskEnd
cliHistoryInvalid:
    xor a
    ld (cliHistoryCount),a
    ld (cliHistoryNext),a
    jp cliHistoryDiskEnd

; Прочитать одну строку по индексу. В режиме без B: используется кольцо BSS.
cliHistoryReadSlot:
    push af
    ld a,(cliHistoryOnDisk)
    or a
    jp nz,cliHistoryReadDisk
    pop af
    call cliHistorySlot
    or a
    ret
cliHistoryReadDisk:
    pop af
    call cliHistoryFileSlot
    push hl
    call cliHistoryDiskBegin
    call nc,cliHistoryOpenRead
    pop hl
    jp c,cliHistoryDiskError
    ld de,cliHistoryRecord
    ld bc,MAX_PATH
    ld a,FILE_OP_READ
    call cliHistoryTransfer
    call flClose
    jp c,cliHistoryDiskError
    ld hl,cliHistoryRecord
    ld b,MAX_PATH
cliHistoryRecordEnd:
    ld a,(hl)
    or a
    jp z,cliHistoryRecordReady
    inc hl
    dec b
    jp nz,cliHistoryRecordEnd
    jp cliHistoryDiskError
cliHistoryRecordReady:
    call cliHistoryDiskEnd
    ld hl,cliHistoryRecord
    or a
    ret

; Записать текущую команду, затем опубликовать новые индексы в заголовке.
cliHistoryWriteSlot:
    ld hl,cliHistoryRecord
    ld b,MAX_PATH
    xor a
cliHistoryClearRecord:
    ld (hl),a
    inc hl
    dec b
    jp nz,cliHistoryClearRecord
    ld hl,CMD_LINE
    ld de,cliHistoryRecord
    call cliHistoryCopy
    call cliHistoryDiskBegin
    call nc,cliHistoryOpenWrite
    jp c,cliHistoryDiskError
    ld a,(cliHistoryNext)
    call cliHistoryFileSlot
    ld de,cliHistoryRecord
    ld bc,MAX_PATH
    ld a,FILE_OP_WRITE
    call cliHistoryTransfer
    jp c,cliHistoryCloseFailed
    call cliHistoryAdvance
    ld hl,0
    ld de,cliHistoryData
    ld bc,6
    ld a,FILE_OP_WRITE
    call cliHistoryTransfer
    call flClose
    jp c,cliHistoryDiskError
    call cliHistoryDiskEnd
    or a
    ret
cliHistoryCloseFailed:
    call flClose
cliHistoryDiskError:
    call cliHistoryInvalid
    xor a
    ld (cliHistoryOnDisk),a
    inc a
    ld (cliHistoryDisabled),a
    scf
    ret

cliHistoryFileSlot:
    call cliHistorySlot
    ld de,cliHistoryData
    jp lineEditMath.sub_hl_de

cliHistoryOpenRead:
    ld a,FILE_MODE_READ
    jp cliHistoryOpen
cliHistoryOpenWrite:
    ld a,FILE_MODE_WRITE
cliHistoryOpen:
    push af
    call flClear
    ld hl,cliHistoryDescriptor
    ld de,cliHistoryDescriptor+12
    ld bc,flHandle
    call mon2_memcpy_bc_hl
    ld a,(cliHistoryDrive)
    ld (flHandle+FH_DRIVE),a
    pop af
    ld (flHandle+FH_MODE),a
    ld a,FILE_OP_OPEN_AT        ; CLI.HST находится в корне выбранного B:.
    jp flCall

; HL — смещение от начала, DE — буфер, BC — точная длина, A — READ/WRITE.
cliHistoryTransfer:
    ld (flHandle+FH_SEEK),hl
    ex de,hl
    ld (flHandle+FH_BUFFER),hl
    push af
    push bc
    ld a,FILE_OP_SEEK
    call flCall
    pop bc
    pop de
    ret c
    push bc
    ld h,b
    ld l,c
    ld (flHandle+FH_COUNT),hl
    ld a,d
    call flCall
    pop de
    ret c
    ld hl,(flHandle+FH_COUNT)
    call mon2_cmp_hl_de
    ret z
    scf
    ret


;----------------------------------------------------------------------------
; MXOS
;
; Сохранить кольцо истории CLI в скрытом файле CLI.HST на RAM-диске.
;
; вход:
;   cliHistoryData содержит записи и индексы. cliHistoryDisabled = 0 для разрешения
;   записи.
;
; выход:
;   В файл записаны сигнатура MCH1 и кольцо команд. При отказе устанавливается
;   cliHistoryDisabled, кольцо остаётся доступным в памяти. Предпринята попытка
;   восстановить прежний диск и каталог.
;----------------------------------------------------------------------------

cliHistorySave:
    ld a,(cliHistoryOnDisk)
    or a
    ret nz
    ld a,(cliHistoryDisabled)
    or a
    ret nz
    ld hl,0434Dh
    ld (cliHistoryData),hl
    ld hl,03148h
    ld (cliHistoryData+2),hl
    call cliHistoryDiskBegin
    jp c,cliHistoryDisable
    ld hl,cliHistoryEnd-cliHistoryData
    ld (cliHistoryDescriptor+FILE_DESCRIPTOR.size),hl
    ld hl,0
    ld (cliHistoryDescriptor+FILE_DESCRIPTOR.size+2),hl
    ld a,2 ; Скрытый служебный файл.
    ld (cliHistoryDescriptor+FILE_DESCRIPTOR.attrib),a
    ld hl,cliHistoryDescriptor
    ld de,cliHistoryData
    call fileWriteDescriptor
    jp c,cliHistoryDisable
    jp cliHistoryDiskEnd
; Ошибки B:, включая полный том, отключают только дисковое сохранение.
; Кольцо в памяти продолжает работать, ошибка не прерывает команду CLI.
cliHistoryDisable:
    ld a,1
    ld (cliHistoryDisabled),a
    jp cliHistoryDiskEnd

;----------------------------------------------------------------------------
; MXOS
;
; Перейти в корень RAM-диска для чтения или записи истории CLI.
;
; вход:
;   Файловые службы DOS доступны. Слот сохранения контекста истории свободен.
;
; выход:
;   cf = 0 при успешном переходе. cliHistoryOldDrive и, после DIR_OP_GET,
;   cliHistoryOldDirectory сохранены для cliHistoryDiskEnd. При ошибке cf = 1, возможна
;   частичная смена текущего контекста.
;----------------------------------------------------------------------------

cliHistoryDiskBegin:
    ld e,0
    MXOS_CALL_REGS MXOS_DOS_FILE_GET_SET_DRIVE
    ld (cliHistoryOldDrive),a
    xor a
    ld (cliHistoryDiskReady),a
    ld c,MEM_RAMDISK_STATUS
    ld a,MEM_OP_RAMDISK
    MXOS_CALL_REGS MXOS_DOS_MEMORY
    ret c
    ld a,e
    ld (cliHistoryDrive),a
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
    ret c
    ld a,DIR_OP_GET
    MXOS_CALL_REGS MXOS_DOS_DIRECTORY
    ret c
    ex de,hl
    ld (cliHistoryOldDirectory),hl
    ld a,1
    ld (cliHistoryDiskReady),a
    ld de,0
    ld a,DIR_OP_SET
    MXOS_CALL_REGS MXOS_DOS_DIRECTORY
    ret
cliHistoryDiskEnd:
    ld a,(cliHistoryDiskReady)
    or a
    jp z,cliHistoryRestoreDrive
    ld hl,(cliHistoryOldDirectory)
    ex de,hl
    ld a,DIR_OP_SET
    MXOS_CALL_REGS MXOS_DOS_DIRECTORY
cliHistoryRestoreDrive:
    ld a,(cliHistoryOldDrive)
    MXOS_CALL_REGS MXOS_DOS_FILE_SELECT_DRIVE
    ret
cliHistoryOldDrive:         DB 0
cliHistoryDisabled:         DB 0
cliHistoryDiskReady:        DB 0
cliHistoryOldDirectory:     DW 0
cliHistoryDescriptor:
    DB 'CLI     HST',2
    DS FILE_DESCR_SIZE-12,0
cliHistoryBrowse:   DB 0


cliHistoryDrive:    DB 0
cliHistoryOnDisk:   DB 0
