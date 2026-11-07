; Испытательный обход: прямой аппаратный переход, без нового ABI ОС.
; Сохраняется весь используемый резерв C000h-C1FFh DOS, включая локальный стек.
; IRQ, файловые вызовы и вложенный банковый транспорт запрещены до возврата.
    INCLUDE "../../bin/banked/transport-imports.inc"
; Входа нет; AF/BC/DE/HL разрушены. Окно/резерв DOS восстановлены до RET.
bank_run:
    ld hl,backupRequest
    call bankCopyFar
    ret c
    ld hl,installRequest
    call bankCopyFar
    ret c
    ld hl,0FFC0h
    ld de,savedWindow
    ld bc,32
    call bench_copy
    ld hl,windowImage
    ld de,0FFC0h
    ld bc,32
    call bench_copy
    ld hl,0
    add hl,sp
    ld (mainSP),hl
    call reset_timing
    xor a
    jp 0FFC0h
bankReturned:
    ld hl,(mainSP)
    ld sp,hl
    ld hl,savedWindow
    ld de,0FFC0h
    ld bc,32
    call bench_copy
    ld hl,restoreRequest
    call bankCopyFar
    ret
; Штатный транспорт и MXCALL внутри DOS: окно не подменяется.
; Четыре порции по 8 строк исключают переполнение периода ВИ53.
bank_run_native:
    ld hl,backupRequest
    call bankCopyFar
    ret c
    ld hl,installRequest
    call bankCopyFar
    ret c
    call reset_timing
    ld a,4
    ld (bankPortions),a
bankPortion:
    ld a,(bankTextMode)
    ld c,a
    ld b,0
    ld de,8
    ld hl,0C000h
    ld a,1
    call bankCall
    call timer_sample
    ld hl,bankPortions
    dec (hl)
    jp nz,bankPortion
    ld hl,restoreRequest
    call bankCopyFar
    ret
; После переключения в main: до установки SP стековых команд нет.
bankService:
    ld (bankHL),hl
    ld hl,0
    add hl,sp
    ld (bankSP),hl
    ld sp,08500h
    push bc
    push de
    ld hl,(bankHL)
    push hl
    or a
    jp z,bankCharacter
    dec a
    jp z,bankCursor
    call timer_sample
    jp bankServiceDone
bankCursor:
    call bios_setCursorPos
    jp bankServiceDone
bankCharacter:
bankOutput:
    call bios_printChar
bankServiceDone:
    pop hl
    pop de
    pop bc
    ld (bankHL),hl
    ld hl,(bankSP)
    ld sp,hl
    ld hl,(bankHL)
    xor a
    jp 0FFD3h
; 32 байта окна: вход, выход, услуга и возврат в физическую страницу 0.
windowImage:
    ld (IO_PAGE_RAMD),a
    jp 0C000h
    ld (IO_PAGE_RAM),a
    jp bankReturned
    push af
    ld (IO_PAGE_RAM),a
    jp bankService
    ld (IO_PAGE_RAMD),a
    pop af
    ret
    DS 32-($-windowImage),0
; memcpy_far: FAR-код 0=main, 1=страница DOS.
backupRequest: DB 1
    DW 0C000h
    DB 0
    DW savedDos,512
installRequest: DB 0
    DW bankImage
    DB 1
    DW 0C000h,512
restoreRequest: DB 0
    DW savedDos
    DB 1
    DW 0C000h,512
bench_copy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,bench_copy
    ret
mainSP: DW 0
bankSP: DW 0
bankHL: DW 0
bankTextMode: DB 0
bankPortions: DB 0
savedWindow: DS 32,0
savedDos: DS 512,0
