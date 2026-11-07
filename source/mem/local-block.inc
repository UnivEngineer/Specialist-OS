; Внутренний вход клиента: C=диск, B=1/2, DE=блок, HL=буфер этой страницы.
; CF/A=ошибка, BC/DE/HL сохраняются. DOS проверяет геометрию и владение.
; Для A: установщик задаёт локальный физический обработчик в MEM_LOCAL_A.
    ORG_PAD LOCAL_BLOCK_ENTRY
localBlock:
    ld a,c
    or a
    jp z,localSystemBlock
    dec a
    jp z,localRamBlock
    cp 6
    jp nz,localBlockBad
    jp LOCAL_DRIVER_ADDR
localSystemBlock:
    push hl
    ld hl,(MEM_LOCAL_A)
    ex (sp),hl
    ret
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
    ld a,(bankPage)
    ld (localRamCopy+3),a
    jp localRamRun
localRamWrite:
    ld (localRamCopy+3),a
    ld (localRamCopy+1),hl
    ex de,hl
    ld (localRamCopy+4),hl
    ld a,(bankPage)
    ld (localRamCopy),a
localRamRun:
    ld hl,localRamCopy
    call bankCopySector
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
