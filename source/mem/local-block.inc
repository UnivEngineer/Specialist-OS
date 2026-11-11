; Внутренний вход клиента: C=диск, B=1/2/4/5, DE=блок, HL=буфер этой страницы.
; CF/A=ошибка, BC/DE/HL сохраняются. DOS проверяет геометрию и владение.
; Для A: установщик задаёт локальный физический обработчик в MEM_LOCAL_A.
    ORG_PAD LOCAL_BLOCK_ENTRY
localBlock:
    ld a,b
    cp 5
    jp z,bankLocalChecked
    ld a,c
    or a
    jp z,localSystemBlock
    dec a
    jp z,localRamBlock
    cp 6
    jp nz,bankLocalBlockBad
    jp LOCAL_DRIVER_ADDR
localSystemBlock:
    ld a,(bankPage)
    or a
    jp nz,MEM_IO_SYSTEM
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
localRamCopy: DB 0
    DW 0
    DB 0
    DW 0,256
localDriverKind: DB 0
    ASSERT $ <= mx_get32
    DS mx_get32-$,0
    jp memGetDword
    jp memPutDword
    ASSERT $ <= MEM_LOCAL_CHECKED
    DS MEM_LOCAL_CHECKED-$,0
memLocalChecked: DW 0
