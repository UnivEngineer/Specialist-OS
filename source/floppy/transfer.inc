;----------------------------------------------------------------------------
; MXOS
;
; Передать сектор между ВГ93 и буфером в банке DOS.
; вход: b=1 для записи, 2 для чтения, 4 для flush; de=LBA, hl=буфер 512 байт.
; выход: cf=0 — готово; при cf=1 a=ошибка DOS, для NOT READY — DOS_ERR_NO_MEDIA.
;   bc/de/hl сохраняет diskDriver.
;----------------------------------------------------------------------------
fdTransfer:
    ld a,b
    cp 4
    jp z,fdSuccess           ; Контроллер завершает запись синхронно.
    dec a
    cp 2
    jp nc,fdArgument
    ld a,d
    cp 5
    jp c,fdRangeOk
    jp nz,fdArgument
    ld a,e
    cp 0A0h                  ; Исключающая граница 05A0h = 1440.
    jp nc,fdArgument
fdRangeOk:
    push bc
    push hl
    ex de,hl
    ld b,0
fdCylinder:
    ld a,l
    sub 18
    ld l,a
    ld a,h
    sbc a,0
    ld h,a
    jp c,fdHead
    inc b
    jp fdCylinder
fdHead:
    ld a,l
    add a,18
    ld c,a
    xor a
    ld (IO_FDD_DRIVE),a
    ld (IO_FDD_DENSITY),a
    ld a,c
    cp 9
    ld a,0
    jp c,fdSelectSide
    ld a,c
    sub 9
    ld c,a
    ld a,1
fdSelectSide:
    ld (IO_FDD_SIDE),a
    ; После сброса регистр дорожки может не совпадать с положением головки.
fdTrackKnown:
    ld a,0
    or a
    jp nz,fdSeek
    ld a,009h                ; RESTORE, загрузка головки, шаг 12 мс.
    call fdPosition
    jp c,fdPositionFailed
    ld a,1
    ld (fdTrackKnown+1),a
fdSeek:
    ld a,(IO_FDD_TRACK)
    cp b
    jp z,fdSector
    ld a,b
    ld (IO_FDD_DATA),a
    ld a,019h                ; SEEK, загрузка головки, шаг 12 мс.
    call fdPosition
    jp c,fdPositionFailed
fdSector:
    ld a,c
    inc a                    ; В ID сектора нумерация с 1.
    ld (IO_FDD_SECTOR),a
    pop hl
    pop bc
    ld a,b
    dec a
    ld a,084h                ; READ SECTOR, выдержка установки головки.
    jp nz,fdCommand
    ld a,0A4h                ; WRITE SECTOR, обычная адресная метка данных.
fdCommand:
    ld (IO_FDD_CMD),a
    push af
    call fdFirstByte
    pop de                   ; D=команда, флаги результата сохранены.
    ret c
    ld a,d
    and 020h
    ld de,IO_FDD_DATA
    ld c,2
    ld b,0
    jp nz,fdWrite
fdRead:
    ld (IO_FDD_REQ),a
    ld a,(de)
    ld (hl),a
    inc hl
    dec b
    jp nz,fdRead
    dec c
    jp nz,fdRead
    jp fdFinish
fdWrite:
    ld (IO_FDD_REQ),a
    ld a,(hl)
    ld (de),a
    inc hl
    dec b
    jp nz,fdWrite
    dec c
    jp nz,fdWrite
fdFinish:
    call fdWaitIdle
    ret c
    and 0FCh                 ; NOT READY, WP, fault, RNF, CRC, lost data.
    jp nz,fdStatusError
fdSuccess:
    xor a
    ret
fdPositionFailed:
    pop hl
    pop bc
    ret
fdArgument:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

;----------------------------------------------------------------------------
; MXOS
;
; Выполнить RESTORE/SEEK. Ожидание ограничено, в том числе без носителя.
; вход: a=команда типа I; для SEEK номер дорожки уже в регистре данных.
; выход: cf/a — статус. bc, de, hl сохранены.
;----------------------------------------------------------------------------
fdPosition:
    ld (IO_FDD_CMD),a
    nop
    nop
    call fdWaitIdle
    ret c
    and 098h                 ; Дорожка 0 и загруженная головка — обычные биты.
    ret z
    jp fdStatusError

;----------------------------------------------------------------------------
; MXOS
;
; Дождаться первого DRQ перед включением побайтной аппаратной синхронизации.
; вход: выдана команда чтения/записи. hl указывает на буфер.
; выход: cf=0 — DRQ; cf=1, a=ошибка контроллера. bc разрушается, hl сохранён.
;----------------------------------------------------------------------------
fdFirstByte:
    ld bc,0
fdWaitDrq:
    ld a,(IO_FDD_CMD)
    or a
    jp m,fdStatusError
    rrca
    jp nc,fdEarlyEnd
    rrca
    jp c,fdSuccess
    dec bc
    ld a,b
    or c
    jp nz,fdWaitDrq
    jp fdAbort
fdEarlyEnd:
    rlca
    jp fdStatusError

;----------------------------------------------------------------------------
; MXOS
;
; Дождаться сброса BUSY или прервать зависшую команду ВГ93.
; вход: команда уже записана; первый опрос после задержки принятия команды.
; выход: cf=0, a=статус ВГ93; cf=1, a=No media при NOT READY или I/O при тайм-ауте.
;   bc, de, hl сохранены.
;----------------------------------------------------------------------------
fdWaitIdle:
    push bc
    ld bc,0
fdBusy:
    ld a,(IO_FDD_CMD)
    or a
    jp m,fdUnready
    rrca
    jp nc,fdIdle
    dec bc
    ld a,b
    or c
    jp nz,fdBusy
    pop bc
fdAbort:
    ld a,(IO_FDD_CMD)
    and 080h
    push af
    ld a,0D0h
    ld (IO_FDD_CMD),a
    pop af
fdStatusError:
    push af
    xor a
    ld (fdTrackKnown+1),a
    pop af
    or a
    jp m,fdNoMedia
    and 040h
    ld a,DOS_ERR_IO
    jp z,fdError
    ld a,DOS_ERR_WRITE_PROTECTED
fdError:
    scf
    ret
fdUnready:
    pop bc
    jp fdStatusError
fdNoMedia:
    ld a,DOS_ERR_NO_MEDIA
    jp fdError
fdIdle:
    rlca
    pop bc
    or a
    ret
