; Общая оболочка автономных тестов. Только main, без подмены банков и BIOS.
; bench_init: AF/BC/DE/HL разрушены, CF=1 при неподвижном таймере.
bench_init:
    di
    ld a,(bios_vars.koi7)
    ld (savedKoi),a
    ld a,(bios_vars.inverse)
    ld (savedInverse),a
    ld a,(bios_vars.escMode)
    ld (savedEsc),a
    ld hl,(bios_vars.charGen)
    ld (savedFont),hl
    xor a
    ld (bios_vars.koi7),a
    ld (bios_vars.inverse),a
    dec a
    ld (bios_vars.escMode),a
    call bios_getMemTop
    ld de,084FFh
    call bios_cmp_hl_de
    jp c,bench_no_memory
    call bios_timerInit
    call timer_probe
    jp z,bench_no_timer
    or a
    ret
bench_no_timer:
    ld hl,noTimer
    jp bench_error
bench_no_memory:
    ld hl,noMemory
bench_error:
    call puts
    scf
    ret
; Ожидание клавиши и восстановление режима текста. Регистры разрушены.
bench_finish:
    ld hl,keyText
    call puts
waitKey:
    call bios_getch
    cp 13
    jp nz,waitKey
    ld a,(savedKoi)
    ld (bios_vars.koi7),a
    ld a,(savedInverse)
    ld (bios_vars.inverse),a
    ld a,(savedEsc)
    ld (bios_vars.escMode),a
    ld hl,(savedFont)
    ld (bios_vars.charGen),hl
    ret
puts:
    IFDEF FONT_CORPUS_TEST
    push af
    ld a,(reportMode)
    or a
    jp z,reportPutsScreen
    pop af
    jp reportPuts
reportPutsScreen:
    pop af
    ENDIF
    jp bios_printString
bench_clear:
    ld c,01Fh
    jp bios_printChar
timing_printChar:
    IFDEF FONT_CORPUS_TEST
    push af
    ld a,(reportMode)
    or a
    jp z,reportCharScreen
    pop af
    jp reportPutChar
reportCharScreen:
    pop af
    ENDIF
    ld c,a
    jp bios_printChar
timer_read = bios_timerRead
timing_print = puts
timing_div_hl_de = bios_div_hl_de
PIT_HZ = 2000000
TIMING_NUMBER_WIDTH = 8
decimalBuffer: DS 11,0
DECIMAL = decimalBuffer
    MACRO LOAD_DE address
    push hl
    ld hl,(address)
    ex de,hl
    pop hl
    ENDM
    INCLUDE "../../source/include/timing.inc"
    INCLUDE "../../source/include/timing-ms.inc"
savedKoi: DB 0
savedInverse: DB 0
savedEsc: DB 0
savedFont: DW 0
v_elapsed: DD 0
v_setup: DD 0
v_last: DW 0
noTimer: DB 'PIT timer unavailable',13,10,0
noMemory: DB 'Need main RAM through 84FFh',13,10,0
keyText: DB 13,10,'Press Enter to return',0
