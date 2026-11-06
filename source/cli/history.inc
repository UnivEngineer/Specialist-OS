; История принадлежит CLI.COM, не занимает резидентную память DOS.
; 32 записи MAX_PATH в кольце. browse=0 — черновик, 1 — последняя команда.
; Подпрограммы разрушают AF/BC/DE/HL и флаги; внешних ABI-входов нет.
CLI_HISTORY_COUNT = 32
    ASSERT MAX_PATH == 128
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
    call cliHistorySlot
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
    ld a,(cliHistoryNext)
    call cliHistorySlot
    ex de,hl
    ld hl,CMD_LINE
    call cliHistoryCopy
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
; A=индекс, HL=запись; BC сохранён, AF/DE разрушены.
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
; HL/DE — два буфера MAX_PATH; перенос вместе с нулём, без хвостового мусора.
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
    call cliHistorySlot
cliHistoryRecallCopy:
    ld de,CMD_LINE
    call cliHistoryCopy
    ex de,hl
    ld de,CMD_LINE+1
    call bios_sub_hl_de
    ld a,l
    ld (lineEditLength),a
    ld (lineEditPosition),a
    xor a
    ld (lineEditOffset),a
    jp lineEditRedraw

; Внешняя COM может затереть весь CLI: сохраняем историю на RAM-диске B:.
; Обе процедуры сохраняют диск и CWD B:, ошибки истории не мешают командам.
cliHistoryLoad:
    ld a,(cliHistoryDisabled)
    or a
    ret nz
    call cliHistoryDiskBegin
    jp c,cliHistoryDisable
    ld hl,cliHistoryDescriptor
    call bios_fileLoadInfo
    jp nc,cliHistoryLoadSize
    MXCALL MX_DOS_GETERROR
    cp DOS_ERR_NOT_FOUND
    jp z,cliHistoryDiskEnd
    jp cliHistoryDisable
cliHistoryLoadSize:
    ld hl,(cliHistoryDescriptor+FILE_DESCRIPTOR.size+2)
    ld a,h
    or l
    jp nz,cliHistoryDiskEnd
    ld hl,(cliHistoryDescriptor+FILE_DESCRIPTOR.size)
    ld de,cliHistoryEnd-cliHistoryData
    call bios_cmp_hl_de
    jp nz,cliHistoryDiskEnd
    ld hl,cliHistoryDescriptor
    ld de,cliHistoryData
    call bios_fileLoad2
    jp c,cliHistoryLoadFailed
    ld hl,(cliHistoryData)
    ld de,0434Dh ; 'MC'
    call bios_cmp_hl_de
    jp nz,cliHistoryInvalid
    ld hl,(cliHistoryData+2)
    ld de,03148h ; 'H1'
    call bios_cmp_hl_de
    jp nz,cliHistoryInvalid
    ld a,(cliHistoryCount)
    cp CLI_HISTORY_COUNT+1
    jp nc,cliHistoryInvalid
    ld a,(cliHistoryNext)
    cp CLI_HISTORY_COUNT
    jp nc,cliHistoryInvalid
    ; Проверяем терминаторы всех записей до использования строк.
    ld hl,cliHistoryEntries
    ld c,CLI_HISTORY_COUNT
cliHistoryCheckEntry:
    ld b,MAX_PATH
    xor a
cliHistoryCheckByte:
    or (hl)
    jp z,cliHistoryCheckTail
    xor a
    inc hl
    dec b
    jp nz,cliHistoryCheckByte
    jp cliHistoryInvalid
cliHistoryCheckTail:
    ld e,b
    ld d,0
    add hl,de
    dec c
    jp nz,cliHistoryCheckEntry
    jp cliHistoryDiskEnd
cliHistoryInvalid:
    xor a
    ld (cliHistoryCount),a
    ld (cliHistoryNext),a
    jp cliHistoryDiskEnd
cliHistoryLoadFailed:
    xor a
    ld (cliHistoryCount),a
    ld (cliHistoryNext),a
    jp cliHistoryDisable
cliHistorySave:
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
    call bios_fileCreate
    jp c,cliHistoryDisable
    jp cliHistoryDiskEnd
; Ошибки B:, включая полный том, отключают только дисковое сохранение.
; Кольцо в памяти продолжает работать, ошибка не прерывает команду CLI.
cliHistoryDisable:
    ld a,1
    ld (cliHistoryDisabled),a
    jp cliHistoryDiskEnd
cliHistoryDiskBegin:
    ld e,0
    call bios_fileGetSetDrive
    ld (cliHistoryOldDrive),a
    xor a
    ld (cliHistoryDiskReady),a
    ld a,1
    MXCALL MX_DOS_FILESELECTDRIVE
    ret c
    ld a,DIR_OP_GET
    MXCALL MX_DOS_DIRECTORY
    ret c
    ex de,hl
    ld (cliHistoryOldDirectory),hl
    ld a,1
    ld (cliHistoryDiskReady),a
    ld de,0
    ld a,DIR_OP_SET
    MXCALL MX_DOS_DIRECTORY
    ret
cliHistoryDiskEnd:
    ld a,(cliHistoryDiskReady)
    or a
    jp z,cliHistoryRestoreDrive
    ld hl,(cliHistoryOldDirectory)
    ex de,hl
    ld a,DIR_OP_SET
    MXCALL MX_DOS_DIRECTORY
cliHistoryRestoreDrive:
    ld a,(cliHistoryOldDrive)
    MXCALL MX_DOS_FILESELECTDRIVE
    ret
cliHistoryOldDrive: DB 0
cliHistoryDisabled: DB 0
cliHistoryDiskReady: DB 0
cliHistoryOldDirectory: DW 0
cliHistoryDescriptor:
    DB 'CLI     HST',2
    DS FILE_DESCR_SIZE-12,0
cliHistoryBrowse: DB 0
cliHistoryDraft: DS MAX_PATH,0
cliHistoryData:
    DB 'MCH1'
cliHistoryCount: DB 0
cliHistoryNext: DB 0
cliHistoryEntries: DS CLI_HISTORY_COUNT*MAX_PATH,0
cliHistoryEnd:
