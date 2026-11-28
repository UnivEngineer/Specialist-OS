; Общая оболочка автономных тестов. Только main, без подмены банков и BIOS.
; bench_init: AF/BC/DE/HL разрушены, CF=1 при неподвижном таймере.
bench_init:
    di
    ld a,(bios_upper.koi7)
    ld (savedKoi),a
    ld a,(mon2_vars.inverse)
    ld (savedInverse),a
    ld a,(bios_upper.escMode)
    ld (savedEsc),a
    ld hl,(bios_upper.charGen)
    ld (savedFont),hl
    xor a
    call FONT_ENGINE_ENCODING
    ld (mon2_vars.inverse),a
    dec a
    ld (bios_upper.escMode),a
    call mx_getMemTop
    IFDEF FONT_NATIVE_GRID
    ld de,035FFh ; controller + temporary corpus, below resident tables
    ELSE
    ld de,084FFh
    ENDIF
    call mon2_cmp_hl_de
    jp c,bench_no_memory
    ld a,TIMER_RAW
    MXOS_CALL_REGS MXOS_DOS_TIMER
    jp c,bench_no_timer
    ld a,1
    ld (benchTimerHeld),a
    call pitTimerInit
    jp c,bench_no_timer
    call timer_probe
    jp z,bench_no_timer
    or a
    ret
bench_no_timer:
    call bench_release_timer
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
    call bench_release_timer
    IFDEF FONT_CORPUS_TEST
    ld a,(benchQuick)
    or a
    jp nz,bench_restore
    ENDIF
    ld hl,keyText
    call puts
waitKey:
    call mon2_getchOld
    cp 13
    jp nz,waitKey
bench_restore:
    push af
    call bench_release_timer
    pop af
    ld a,(savedKoi)
    call FONT_ENGINE_ENCODING
    ld a,(savedInverse)
    ld (mon2_vars.inverse),a
    ld a,(savedEsc)
    ld (bios_upper.escMode),a
    ld hl,(savedFont)
    ld (bios_upper.charGen),hl
    ret

bench_release_timer:
    ld a,(benchTimerHeld)
    or a
    ret z
    xor a
    ld (benchTimerHeld),a
    ld a,TIMER_RELEASE
    MXOS_CALL_REGS MXOS_DOS_TIMER
    ret
benchTimerHeld: DB 0

puts:
    IFDEF FONT_NATIVE_GRID
    jp gridPuts
    ENDIF
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
    jp mon2_printStringOld
bench_clear:
    ld c,01Fh
    jp mon2_printCharOld
timing_printChar:
    IFDEF FONT_NATIVE_GRID
    jp gridPrintChar
    ENDIF
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
    jp mon2_printCharOld
    INCLUDE "../../sdk/asm/lib/timing-pit.inc"
timer_read   = io_pitRead
timing_print = puts

    MODULE textMath
    DEFINE MATH_DIV
    INCLUDE "../../sdk/asm/lib/math.inc"
    ENDMODULE
timing_div_hl_de = textMath.div_hl_de
PIT_HZ           = 2000000
    IFDEF FONT_NATIVE_GRID
    DEFINE TIMING_NUMBER_WIDTH 4
    ELSE
TIMING_NUMBER_WIDTH = 8
    ENDIF
decimalBuffer: DS 11,0
DECIMAL = decimalBuffer
    MACRO LOAD_DE address
    push hl
    ld hl,(address)
    ex de,hl
    pop hl
    ENDM
    INCLUDE "../../sdk/asm/lib/timing.inc"
    INCLUDE "../../sdk/asm/lib/timing-ms.inc"
savedKoi:       DB 0
savedInverse:   DB 0
savedEsc:       DB 0
savedFont:      DW 0
v_elapsed:      DD 0
v_setup:        DD 0
v_last:         DW 0
noTimer:        DB 'PIT timer unavailable',13,10,0
noMemory:       DB 'Need main RAM through 84FFh',13,10,0
keyText:        DB 13,10,'Press Enter to return',13,10,0
