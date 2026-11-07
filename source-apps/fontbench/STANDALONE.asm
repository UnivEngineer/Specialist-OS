; Автономное сравнение BIOS и готовых сдвигов 5x7, без правок BIOS.
    INCLUDE "../../source/include/mxos.inc"
    ORG 0
entry:
    call bench_init
    jp c,bench_finish
    ld hl,02800h/8
    ld (bios_vars.charGen),hl
    ; Первый вывод настраивает полиморфные инструкции BIOS вне измерения.
    ld c,'A'
    call bios_printChar
    call bench_clear
    ld hl,bios_printChar
    ld (output+1),hl
    call renderScreen
    ld hl,(v_elapsed)
    ld (biosTicks),hl
    ld hl,(v_elapsed+2)
    ld (biosTicks+2),hl
    ; Сохраняем 36 колонок экрана, затрагиваемых 48 глифами шириной 6.
    ld hl,09000h
    ld de,06000h
    ld bc,02400h
saveFrame:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,saveFrame
    call bench_clear
    ld hl,02000h
    ld (output+1),hl
    call renderScreen
    ld hl,09000h
    ld de,06000h
    ld bc,02400h
compareFrame:
    ld a,(de)
    cp (hl)
    jp nz,mismatch
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,compareFrame
    ld hl,matchText
    jp report
mismatch:
    ld hl,failText
report:
    ld (bitmapResult),hl
    ld hl,(v_elapsed)
    ld (shiftTicks),hl
    ld hl,(v_elapsed+2)
    ld (shiftTicks+2),hl
    call bench_clear
    call bank_run
    ld hl,(v_elapsed)
    ld (bankBiosTicks),hl
    ld hl,(v_elapsed+2)
    ld (bankBiosTicks+2),hl
    call checkBankFrame
    ld hl,02000h
    ld (bankOutput+1),hl
    call bench_clear
    call bank_run
    call checkBankFrame
    ld hl,000CFh
    call bios_setCursorPos
    ld hl,title
    call puts
    ld hl,biosTicks
    call print_time_ms
    ld hl,shiftText
    call puts
    ld hl,shiftTicks
    call print_time_ms
    ld hl,bankBiosText
    call puts
    ld hl,bankBiosTicks
    call print_time_ms
    ld hl,bankShiftText
    call puts
    ld hl,v_elapsed
    call print_time_ms
    ld hl,msText
    call puts
    ld hl,(bitmapResult)
    call puts
    jp bench_finish
; Проверка результатов обоих банковых вариантов по снимку штатного BIOS.
checkBankFrame:
    ld hl,09000h
    ld de,06000h
    ld bc,02400h
checkBankByte:
    ld a,(de)
    cp (hl)
    jp nz,bankMismatch
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,checkBankByte
    ret
bankMismatch:
    ld hl,failText
    ld (bitmapResult),hl
    ret
; 24 строки по 48 печатных ASCII, без прокрутки и управляющих кодов.
; AF/BC/DE/HL разрушены, v_elapsed содержит полное время цикла.
renderScreen:
    call reset_timing
    ld de,24
    ld hl,00007h
row:
    ld (bios_vars.cursorY),hl
    push hl
    ld b,48
    ld c,' '
glyph:
output:
    call bios_printChar
    inc c
    dec b
    jp nz,glyph
    call timer_sample
    pop hl
    ld a,l
    add a,8
    ld l,a
    dec de
    ld a,d
    or e
    jp nz,row
    ret
title: DB 'FONTBEN: 1152 glyphs, PIT 2 MHz',13,10,'BIOS 5x7:    ',0
shiftText: DB ' ms',13,10,'Shifted 5x7: ',0
bankBiosText: DB ' ms',13,10,'DOS BIOS:    ',0
bankShiftText: DB ' ms',13,10,'DOS shifted: ',0
msText: DB ' ms',13,10,0
matchText: DB 'Bitmap: PASS',13,10,0
failText: DB 'Bitmap: FAIL',13,10,0
biosTicks: DD 0
shiftTicks: DD 0
bankBiosTicks: DD 0
bitmapResult: DW 0
    INCLUDE "../textbench/bank-runtime.inc"
bankImage:
    INCBIN "../../bin/FONTBEN.bank.bin"
    INCLUDE "../textbench/standalone.inc"
    ASSERT $ <= 02000h
    DS 02000h-$,0
    INCBIN "../../bin/FONTBEN.renderer.bin"
    ASSERT $ <= 02800h
    DS 02800h-$,0
    INCBIN "../../assets/system-rom/FONT.FNT"
    ASSERT $ == 03000h
    INCBIN "../../bin/FONTBEN.shifts.bin"
    ASSERT $ == 06000h
programEnd:
