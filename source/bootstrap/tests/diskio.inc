; A=страница инициатора, DE=NEAR запроса в ней; вход через bankCall.
; FAR-буфер явно задан в запросе. Страница DOS не подменяет его контекст.
; Выход CF/A=код DOS; BC/DE/HL разрушены. Служба нереентерабельна.
; Сам транспорт вложенный: bankCopyFar сохраняет и восстанавливает окно.
dosBufferedIO:
    push af
    ld a,(dosIoBusy)
    or a
    jp nz,dosIoBusyError
    inc a
    ld (dosIoBusy),a
    pop af
    ld (dosIoCopy),a
    ex de,hl
    ld (dosIoCopy+1),hl
    ld bc,DOS_IO_REQUEST_SIZE
    call dosIoRange
    jp c,dosIoExit
    ld hl,dosIoRequest
    ld (dosIoCopy+4),hl
    ld hl,DOS_IO_REQUEST_SIZE
    ld (dosIoCopy+6),hl
    ld a,DOS_IO_PAGE
    ld (dosIoCopy+3),a
    ld hl,dosIoCopy
    call bankCopyFar
    jp c,dosIoBad
    ld a,(dosIoRequest)
    cp 8
    jp nc,dosIoBad
    add a,a
    add a,a
    ld e,a
    ld d,0
    ld hl,dosIoDrivers
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    inc hl
    ld c,(hl)
    inc hl
    ld b,(hl)
    ld a,d
    or e
    jp z,dosIoMissing
    ex de,hl
    ld (dosIoCall+1),hl
    ld h,b
    ld l,c
    ld (dosIoLength),hl
    ld (dosIoCopy+6),hl
    ; Таблица принадлежит DOS; допустимы только три физических размера.
    ld a,h
    or a
    jp nz,dosIoLargeBlock
    ld a,l
    cp 128
    jp nz,dosIoBad
    jp dosIoBlockReady
dosIoLargeBlock:
    ld a,l
    or a
    jp nz,dosIoBad
    ld a,h
    dec a
    cp 2
    jp nc,dosIoBad
dosIoBlockReady:
    ld a,(dosIoRequest+4)
    cp DOS_IO_PAGE
    jp z,dosIoBad                 ; Внутренние клиенты DOS вызывают драйвер локально.
    ld hl,(dosIoRequest+5)
    call dosIoRange
    jp c,dosIoExit
    ld a,(dosIoRequest+4)
    ld hl,bankPageCount
    cp (hl)
    jp c,dosIoPageReady
    jp nz,dosIoBad
dosIoPageReady:
    ld a,(dosIoRequest+1)
    cp 2
    jp z,dosIoRead
    cp 1
    jp z,dosIoWrite
    cp 5
    jp nz,dosIoBad
dosIoWrite:
    ld a,(dosIoRequest+4)
    ld (dosIoCopy),a
    ld hl,(dosIoRequest+5)
    ld (dosIoCopy+1),hl
    ld a,DOS_IO_PAGE
    ld (dosIoCopy+3),a
    ld hl,DOS_IO_BUFFER
    ld (dosIoCopy+4),hl
    ld hl,dosIoCopy
    call bankCopyFar
    jp c,dosIoBad
    ld b,1
    ld hl,DOS_IO_BUFFER
    call dosIoTransfer
    jp c,dosIoExit
    ld a,(dosIoRequest+1)
    cp 5
    jp nz,dosIoSuccess
    ld b,2
    ld hl,DOS_IO_VERIFY
    call dosIoTransfer
    jp c,dosIoExit
    ld hl,(dosIoLength)
    ld b,h
    ld c,l
    ld hl,DOS_IO_BUFFER
    ld de,DOS_IO_VERIFY
dosIoCompare:
    ld a,(de)
    cp (hl)
    jp nz,dosIoFailure
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,dosIoCompare
    jp dosIoSuccess
dosIoRead:
    ld b,2
    ld hl,DOS_IO_BUFFER
    call dosIoTransfer
    jp c,dosIoExit
    ld a,DOS_IO_PAGE
    ld (dosIoCopy),a
    ld hl,DOS_IO_BUFFER
    ld (dosIoCopy+1),hl
    ld a,(dosIoRequest+4)
    ld (dosIoCopy+3),a
    ld hl,(dosIoRequest+5)
    ld (dosIoCopy+4),hl
    ld hl,dosIoCopy
    call bankCopyFar
    jp c,dosIoBad
dosIoSuccess:
    xor a
dosIoExit:
    push af
    xor a
    ld (dosIoBusy),a
    pop af
    ret
dosIoBusyError:
    pop af
    ld a,DOS_ERR_ACCESS
    scf
    ret
dosIoMissing:
    ld a,DOS_ERR_NO_DRIVER
    jp dosIoError
dosIoFailure:
    ld a,DOS_ERR_IO
    jp dosIoError
dosIoBad:
    ld a,DOS_ERR_ARGUMENT
dosIoError:
    scf
    jp dosIoExit

; B=1/2, HL=локальный буфер. DE берётся из уже скопированного запроса.
dosIoTransfer:
    ex de,hl
    ld hl,(dosIoRequest+2)
    ex de,hl
    call dosIoCall
    ret nc
    ; Старые аппаратные процедуры обещают CF, но не числовое значение A.
    ld a,DOS_ERR_IO
    scf
    ret
dosIoCall:
    jp 0

; HL+BC должны оставаться ниже защищённого хвоста; HL/BC сохраняются.
dosIoRange:
    push hl
    add hl,bc
    jp c,dosIoRangeBad
    ld a,h
    cp DOS_IO_CLIENT_END / 256
    jp c,dosIoRangeGood
    jp nz,dosIoRangeBad
    ld a,l
    or a
    jp nz,dosIoRangeBad
dosIoRangeGood:
    pop hl
    or a
    ret
dosIoRangeBad:
    pop hl
    ld a,DOS_ERR_ARGUMENT
    scf
    ret

; Локальная регистрация: A=диск, HL=вход, BC=размер физического блока.
; Только при свободной службе; AF/DE/HL разрушены, BC сохранён.
dosIoInstall:
    cp 8
    jp nc,dosIoInstallBad
    push af
    ld a,(dosIoBusy)
    or a
    jp nz,dosIoInstallBusy
    pop af
    add a,a
    add a,a
    push hl
    ld e,a
    ld d,0
    ld hl,dosIoDrivers
    add hl,de
    pop de
    ld (hl),e
    inc hl
    ld (hl),d
    inc hl
    ld (hl),c
    inc hl
    ld (hl),b
    xor a
    ret
dosIoInstallBusy:
    pop af
dosIoInstallBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
dosIoDrivers: DS 8*4,0
dosIoBusy: DB 0
dosIoLength: DW 0
dosIoRequest: DS DOS_IO_REQUEST_SIZE,0
dosIoCopy: DS 8,0

