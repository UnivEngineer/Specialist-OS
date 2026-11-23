BENCH_DOS_BUFFER_KIB = 8
; Общий прикладной FILEHANDLE для контроллера и MAIN/DOS/BANK измерителей.
; BH_DESCRIPTOR/BH_DRIVE — локальные дескриптор и байт номера диска.
; CF/A — статус DOS; регистры разрушены. Чужой DISKBEN.TMP проверяется до CREATE.
bhCreate:
    ld a,FILE_MODE_READ|FILE_MODE_WRITE|FILE_MODE_CREATE|FILE_MODE_TRUNCATE
    IFDEF BH_SCATTER
    ld b,a
    ld a,(BH_SCATTER)
    or a
    ld a,b
    jp z,bhOpen
    or FILE_MODE_SCATTER
    ENDIF
    jp bhOpen
;----------------------------------------------------------------------------
; Открыть существующий тестовый файл для чтения и записи.
;
; вход:
;   Имя и диск заданы в BH_DESCRIPTOR и BH_DRIVE. bhHandle свободен.
;
; выход:
;   cf = 0: заполнен bhHandle. cf = 1: a = код ошибки DOS.
;   Регистры могут измениться.
;----------------------------------------------------------------------------

bhWriteOpen:
    ld a,FILE_MODE_READ|FILE_MODE_WRITE
    jp bhOpen
;----------------------------------------------------------------------------
; Переоткрыть тестовый файл в режиме чтения.
;
; вход:
;   Имя и диск заданы в BH_DESCRIPTOR и BH_DRIVE. Если bhHandle активен,
;   сначала выполняется его закрытие.
;
; выход:
;   cf = 0: bhHandle открыт с начала файла. cf = 1: a = код ошибки закрытия
;   или открытия. Регистры могут измениться.
;----------------------------------------------------------------------------

bhReadOpen:
    call bhClose
    ret c
    ld a,FILE_MODE_READ
bhOpen:
    push af
    ld hl,bhHandle
    ld b,FILE_HANDLE
    xor a
bhClear:
    ld (hl),a
    inc hl
    dec b
    jp nz,bhClear
    ld hl,BH_DESCRIPTOR
    ld de,bhHandle
    ld b,11
bhName:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bhName
    ld a,(BH_DRIVE)
    ld (bhHandle+FH_DRIVE),a
    pop af
    ld (bhHandle+FH_MODE),a
    ld a,FILE_OP_OPEN
    jp bhCall
bhCall:
    ld hl,bhHandle
    MXOS_CALL MXOS_DOS_FILEHANDLE
    ret
;----------------------------------------------------------------------------
; Закрыть тестовый файл и синхронизировать его дескриптор.
;
; вход:
;   bhHandle содержит состояние файла.
;
; выход:
;   cf = 0: обработка завершена. Если файл был открыт, его дескриптор скопирован
;   в BH_DESCRIPTOR. cf = 1: a = код ошибки DOS. Регистры могут измениться.
;----------------------------------------------------------------------------

bhClose:
    ld a,(bhHandle+FH_STATE)
    or a
    ret z
    ld a,FILE_OP_CLOSE
    call bhCall
    ret c
    jp bhDescriptor
;----------------------------------------------------------------------------
; Сбросить буферы тестового файла и обновить его дескриптор.
;
; вход:
;   bhHandle открыт.
;
; выход:
;   cf = 0: обновлён BH_DESCRIPTOR. cf = 1: a = код ошибки DOS.
;   Регистры могут измениться.
;----------------------------------------------------------------------------

bhFlush:
    ld a,FILE_OP_FLUSH
    call bhCall
    ret c
bhDescriptor:
    ld hl,bhHandle
    ld de,BH_DESCRIPTOR
    ld b,FILE_DESCRIPTOR
bhCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bhCopy
    xor a
    ret
; A=1 запись / 2 чтение (как внутренний поток), HL=буфер, BC=байты.
; Проверяем фактическую длину: ранний EOF не является успехом бенчмарка.
bhTransfer:
    ld (bhHandle+FH_BUFFER),hl
    ld h,b
    ld l,c
    ld (bhHandle+FH_COUNT),hl
    ld (bhExpected),hl
    xor 3
    call bhCall
    ret c
    ld hl,(bhExpected)
    ex de,hl
    ld hl,(bhHandle+FH_COUNT)
    ld a,h
    cp d
    jp nz,bhShort
    ld a,l
    cp e
    ret z
bhShort:
    ld a,DOS_ERR_IO
    scf
    ret
bhHandle:   DS FILE_HANDLE,0
bhExpected: DW 0
