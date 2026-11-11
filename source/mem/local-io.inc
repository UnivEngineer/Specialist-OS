; Дополнительные локальные службы EXEC/DOS в зарезервированном слоте A:.
; MAIN использует аппаратный драйвер A: и собственный BIOS-проверяющий цикл.
    DS MEM_IO_SYSTEM-$,0
    jp memSystemBlock
    jp localBlockChecked
    jp calcCS
memSystemBlock:
    ld a,b
    cp 2
    jp z,memSystemRead
    push bc
    push de
    push hl
    ld a,b
    ld (memSystemRequest),a
    ld (memSystemRequest+3),hl
    ex de,hl
    ld (memSystemRequest+1),hl
    ld de,memSystemRequest
    ld hl,BIOS_BANK_IO_ADDR
    xor a
    call bankCall
    pop hl
    pop de
    pop bc
    ret
memSystemRequest: DS 5,0
; ROM — физический источник CPM16, MAIN-обработчик для чтения не нужен.
; Внутренний вход: вызывающая DOS проверяет LBA и буфер, RAW PREPARE
; передаёт эту обязанность клиенту. BC/DE/HL сохраняются, CF/A=статус.
memSystemRead:
    push bc
    push de
    push hl
    ld (memRomCopy+4),hl
    ld a,(bankPage)
    ld (memRomCopy+3),a
    ld a,e
    and 07Fh
    ld h,a
    ld l,0
    ld (memRomCopy+1),hl
    ld a,e
    add a,a
    ld a,d
    rla
    or 080h
    ld (memRomCopy),a
    ld hl,memRomCopy
    call bankCopySector
    pop hl
    pop de
    pop bc
    ret
memRomCopy: DB 080h
    DW 0
    DB 0
    DW 0,256
    DEFINE DWC_NO_ERROR_FLAG
    DEFINE DWC_DOS_BUFFER DOS_VERIFY_BUFFER
    INCLUDE "../common/diskWriteChecked.inc"
    UNDEFINE DWC_DOS_BUFFER
    UNDEFINE DWC_NO_ERROR_FLAG
    INCLUDE "../common/localBlockChecked.inc"
    INCLUDE "../common/calcCS.inc"

    INCLUDE "put32-template.inc"
