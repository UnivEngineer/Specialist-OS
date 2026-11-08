; Испытательный обход: прямой аппаратный переход, без нового ABI ОС.
; Сохраняется весь используемый резерв DOS, включая локальный стек и текст.
; IRQ и файловые вызовы запрещены до возврата. Атомарный memcpy_far в main
; допустим: он сохраняет и восстанавливает все 32 байта тестового окна.
    INCLUDE "../../bin/banked/transport-imports.inc"
    IFDEF FONT_CORPUS_TEST
BANK_TEST_BYTES = 00800h
    ELSE
BANK_TEST_BYTES = 00200h
    ENDIF
; Входа нет; AF/BC/DE/HL разрушены. Окно/резерв DOS восстановлены до RET.
bank_run:
    ld hl,backupRequest
    call bankCopyFar
    ret c
    ld hl,installRequest
    call bankCopyFar
    ret c
    IFDEF FONT_CORPUS_TEST
    ld a,(matrixFlags)
    and 32
    jp z,bankBulkReady
    ; Проверка переноса не должна проходить за счёт уже совпадающего буфера.
    ld hl,bankSerialBulk
    ld bc,FONT_CORPUS_SIZE
bankBulkPoison:
    ld (hl),0
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,bankBulkPoison
    call reset_timing
    ld hl,bankSerialRequest
    call bankCopyFar
    ret c
    call timer_sample
bankBulkReady:
    ENDIF
    ld hl,0FFC0h
    ld de,savedWindow
    ld bc,32
    call bench_copy
    ld hl,windowImage
    IFDEF FONT_CORPUS_TEST
    ld a,(matrixFlags)
    and 80
    jp z,bankWindowChosen
    ld hl,window20Image
bankWindowChosen:
    ENDIF
    ld de,0FFC0h
    ld bc,32
    call bench_copy
    ld hl,0
    add hl,sp
    ld (mainSP),hl
    IFDEF FONT_CORPUS_TEST
    ld a,(matrixFlags)
    and 32
    jp z,bankResetTiming
    ; Копирование уже учтено. Установка тестового окна вне всех замеров.
    call timer_read
    ld (v_last),hl
    jp bankTimingReady
bankResetTiming:
    ENDIF
    call reset_timing
    IFDEF FONT_CORPUS_TEST
bankTimingReady:
    ld a,(matrixFlags)
    and 80
    jp z,bankEnterNormal
    ; В банковом образе по C7FEh лежит адрес начального входа C000h.
    ld sp,0C7FEh
    xor a
    jp 0FFC6h
bankEnterNormal:
    ENDIF
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
    IFDEF FONT_CORPUS_TEST
    cp 2
    jp z,bankQuad
    cp 3
    jp z,bankSerial
    ENDIF
    call timer_sample
    jp bankServiceDone
bankCursor:
    call bios_setCursorPos
    jp bankServiceDone
bankCharacter:
bankOutput:
    call bios_printChar
    IFDEF FONT_CORPUS_TEST
    jp bankServiceDone
; Четыре кода передаются регистрами: источник остается в странице DOS.
bankQuad:
    ld a,b
    ld (bankQuadChars),a
    ld a,c
    ld (bankQuadChars+1),a
    ld a,d
    ld (bankQuadChars+2),a
    ld a,e
    ld (bankQuadChars+3),a
    ld hl,bankQuadChars
    ld c,4
bankQuadOutput:
    call quadRender
    jp nc,bankServiceDone
    ld hl,failText
    ld (bitmapResult),hl
    jp bankServiceDone
; Чтение 48 байт через восьмибайтный переходник в общем окне.
; Стековые обращения выполняются только после восстановления main.
bankSerial:
    ld a,(matrixFlags)
    and 32
    jp z,bankSerialBytes
    ld de,-0C200h
    add hl,de
    ld de,bankSerialBulk
    add hl,de
    jp bankSerialBufferReady
bankSerialBytes:
    ex de,hl
    ld hl,bankSerialChars
    ld b,48
bankSerialCopy:
    ; Значение записываемого байта выбирает банк: всегда страница DOS 0.
    xor a
    call 0FFD8h
    ld (hl),a
    inc de
    inc hl
    dec b
    jp nz,bankSerialCopy
    ld hl,bankSerialChars
bankSerialBufferReady:
    ld c,48
bankSerialOutput:
    call serialBIOS
    jp nc,bankServiceDone
    ld hl,failText
    ld (bitmapResult),hl
    jp bankServiceDone
    ENDIF
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
    IFDEF FONT_CORPUS_TEST
    ld (IO_PAGE_RAMD),a
    ld a,(de)
    ld (IO_PAGE_RAM),a
    ret
    ENDIF
    DS 32-($-windowImage),0
; memcpy_far: FAR-код 0=main, 1=страница DOS.
backupRequest: DB 1
    DW 0C000h
    DB 0
    DW savedDos,BANK_TEST_BYTES
installRequest: DB 0
    DW bankImage
    DB 1
    DW 0C000h,BANK_TEST_BYTES
restoreRequest: DB 0
    DW savedDos
    DB 1
    DW 0C000h,BANK_TEST_BYTES
    IFDEF FONT_CORPUS_TEST
bankSerialRequest: DB 1
    DW 0C200h
    DB 0
    DW bankSerialBulk,FONT_CORPUS_SIZE
    ENDIF
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
    IFNDEF FONT_CORPUS_TEST
savedDos: DS BANK_TEST_BYTES,0
    ENDIF
    IFDEF FONT_CORPUS_TEST
bankQuadChars: DS 4,0
bankSerialChars: DS 48,0
    ENDIF
