; Автономный тест полного текстового пути BIOS с прокруткой, Intel 8080.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    ORG 0
entry:
    call bench_init
    jp c,bench_finish
    call bank_open
    jp c,moduleFailed
    call bench_clear
    call reset_timing
    ld de,32
charLine:
    ld hl,text
charLoop:
    ld a,(hl)
    or a
    jp z,charEnd
    ld c,a
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    inc hl
    jp charLoop
charEnd:
    call timer_sample
    dec de
    ld a,d
    or e
    jp nz,charLine
    ld hl,(v_elapsed)
    ld (charTicks),hl
    ld hl,(v_elapsed+2)
    ld (charTicks+2),hl
    call bench_clear
    call reset_timing
    ld de,32
stringLine:
    ld hl,text
    MXOS_CALL MXOS_BIOS_PRINT_STRING
    call timer_sample
    dec de
    ld a,d
    or e
    jp nz,stringLine
    ld hl,(v_elapsed)
    ld (stringTicks),hl
    ld hl,(v_elapsed+2)
    ld (stringTicks+2),hl
    call bench_clear
    xor a
    ld (bankTextMode),a
    call bank_run_native
    jp c,moduleFailed
    ld hl,(v_elapsed)
    ld (bankCharTicks),hl
    ld hl,(v_elapsed+2)
    ld (bankCharTicks+2),hl
    call bench_clear
    ld a,1
    ld (bankTextMode),a
    call bank_run_native
    jp c,moduleFailed
    ld hl,summary
    call puts
    ld hl,charTicks
    call print_time_ms
    ld hl,stringText
    call puts
    ld hl,stringTicks
    call print_time_ms
    ld hl,bankText
    call puts
    ld hl,bankCharTicks
    call print_time_ms
    ld hl,bankStringText
    call puts
    ld hl,v_elapsed
    call print_time_ms
    ld hl,msText
    call puts
    call bank_close
    jp bench_finish
moduleFailed:
    call bank_close
    ld hl,bankErrorText
    call puts
    jp bench_finish
text: DB '0123456789 ABCDEFGHIJKLMNOPQRSTUVWXYZ abcdefghij',13,10,0
summary: DB 13,10,'TEXTBEN: 32 x 48 chars + CR/LF, PIT 2 MHz',13,10,'Main char:   ',0
stringText: DB ' ms',13,10,'Main string: ',0
bankText: DB ' ms',13,10,'BANK char:   ',0
bankStringText: DB ' ms',13,10,'BANK string: ',0
msText: DB ' ms',13,10,'Includes wrapping and scrolling.',13,10,0
charTicks: DD 0
stringTicks: DD 0
bankCharTicks: DD 0
    INCLUDE "bank-runtime.inc"
    INCLUDE "standalone.inc"
programEnd:
    ASSERT programEnd < 08000h
