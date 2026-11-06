; Внутренний вход клиента: C=диск, B=1/2, DE=блок, HL=буфер этой страницы.
; CF/A=ошибка, BC/DE/HL сохраняются. DOS проверяет геометрию и владение.
; В этой версии прямой маршрут разрешён только для основной памяти.
    ORG_PAD LOCAL_BLOCK_ENTRY
localBlock:
    ld a,c
    or a
    jp z,bios_blockIO
    dec a
    jp z,localRamBlock
    cp 6
    jp nz,localBlockBad
    jp LOCAL_DRIVER_ADDR
localRamBlock:
    push bc
    push de
    push hl
    ld a,d
    add a,2
    ld d,e
    ld e,0
    dec b
    jp z,localRamWrite
    ld (localRamCopy),a
    ld (localRamCopy+4),hl
    ex de,hl
    ld (localRamCopy+1),hl
    xor a
    ld (localRamCopy+3),a
    jp localRamRun
localRamWrite:
    ld (localRamCopy+3),a
    ld (localRamCopy+1),hl
    ex de,hl
    ld (localRamCopy+4),hl
    xor a
    ld (localRamCopy),a
localRamRun:
    ld hl,localRamCopy
    call bankCopyFar
    pop hl
    pop de
    pop bc
    ret
localBlockBad:
    ld a,DOS_ERR_ARGUMENT
    scf
    ret
localRamCopy: DB 0
    DW 0
    DB 0
    DW 0,256
localDriverKind: DB 0
