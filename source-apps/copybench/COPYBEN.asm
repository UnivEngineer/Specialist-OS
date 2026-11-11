; Автономный тест ROM/main/ДОЗУ и межстраничного копирования. Intel 8080.
; Последние обнаруженные страницы временно используются с полным сохранением
; изменяемых данных, хвоста и общего окна. Файловые вызовы внутри теста запрещены.
    INCLUDE "../../source/include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
PIT_HZ = 2000000
BACKUP = 03000h
EXPECTED = 05000h
LENGTH = 02000h
TARGET = 04000h
    MACRO LOAD_DE address
    push hl
    ld hl,(address)
    ex de,hl
    pop hl
    ENDM
    ORG 0
entry:
    di
    ld hl,0
    add hl,sp
    ld (callerSP),hl
    call bios_getMemTop
    ld de,088FFh
    call bios_cmp_hl_de
    jp c,noMemory
    ld sp,08000h
    ld a,(bios_upper.koi7)
    ld (callerKoi),a
    xor a
    call bios_setFontEncoding
    ld a,(bios_upper.ramPageCount)
    or a
    jp z,noRAM
    dec a
    ld (targetPage),a
    ld hl,title
    call puts
    ld a,(targetPage)
    ld e,a
    ld d,0
    ld b,0
    call bios_printDecWord
    ld hl,working
    call puts
    ld hl,0FFC0h
    ld de,oldWindow
    ld bc,32
    call localCopy
    ld hl,0FF00h
    ld de,oldMainTail
    ld bc,192
    call localCopy
    call byteMode
    ld hl,TARGET
    ld de,BACKUP
    ld bc,LENGTH
    call bankRead
    ld hl,0FF00h
    ld de,oldBankTail
    ld bc,192
    call bankRead
    call readROM
    ; Устанавливаем одинаковый хвост в основной и целевой памяти.
    call byteMode
    ld hl,0FF00h
    ld de,romTailImage
    ld bc,romTailImageEnd-romTailImage
    call bankWrite
    ld hl,romTailImage
    ld de,0FF00h
    ld bc,romTailImageEnd-romTailImage
    call localCopy
    call pitTimerInit
    call timer_probe
    jp z,timerFailed
    call reset_timing
    call directCopy
    call timer_sample
    ld hl,v_elapsed
    ld de,directTicks
    ld bc,4
    call localCopy
    call verify
    ld (directOK),a
    ld hl,directText
    call puts
    ld hl,directTicks
    call print_time_ms
    ld a,(directOK)
    call result
    ; Контрольный путь: сначала ROM -> main, затем встроенный блочный драйвер.
    call reset_timing
    call readROM
    ld hl,builtinBlockImage
    ld de,0FFC0h
    ld bc,32
    call localCopy
    ld hl,EXPECTED
    ld de,TARGET
    ld b,32
blockLoop:
    push bc
    push de
    push hl
    ld a,(targetPage)
    ld c,a
    call bios_RAMDWrite
    pop hl
    pop de
    pop bc
    inc h
    inc d
    dec b
    jp nz,blockLoop
    call timer_sample
    ld hl,v_elapsed
    ld de,blockTicks
    ld bc,4
    call localCopy
    call verify
    ld (blockOK),a
    ld hl,blockText
    call puts
    ld hl,blockTicks
    call print_time_ms
    ld a,(blockOK)
    call result
    call reset_timing
    call readROM
    call byteMode
    ld hl,TARGET
    ld de,EXPECTED
    ld bc,LENGTH
    call bankWrite
    call timer_sample
    ld hl,v_elapsed
    ld de,byteTicks
    ld bc,4
    call localCopy
    call verify
    ld (byteOK),a
    ld hl,byteText
    call puts
    ld hl,byteTicks
    call print_time_ms
    ld a,(byteOK)
    call result
    call ramTests
    call engineTests
    call bankTests
    xor a
    ld (timerError),a
    jp restore
timerFailed:
    ld a,1
    ld (timerError),a
restore:
    call byteMode
    ld hl,TARGET
    ld de,BACKUP
    ld bc,LENGTH
    call bankWrite
    ld hl,0FF00h
    ld de,oldBankTail
    ld bc,192
    call bankWrite
    ld hl,oldMainTail
    ld de,0FF00h
    ld bc,192
    call localCopy
    ld hl,oldWindow
    ld de,0FFC0h
    ld bc,32
    call localCopy
restored:
    ld a,(timerError)
    or a
    ld hl,timerMessage
    jp nz,messageExit
    ld hl,doneText
messageExit:
    call puts
    ld hl,keyText
    call puts
waitKey:
    call bios_getch
    ld hl,exitNewline
    call puts
exit:
    ld a,(callerKoi)
    call bios_setFontEncoding
    ld hl,(callerSP)
    ld sp,hl
    ret
noRAM:
    ld hl,noRAMText
    jp messageExit
noMemory:
    ld hl,noMemoryText
    call puts
    ret
result:
    or a
    ld hl,okText
    jp z,puts
    ld hl,failText
    jp puts
puts:
    jp bios_printString
byteMode:
    ld hl,builtinByteImage
    ld de,0FFC0h
    ld bc,32
    jp localCopy
; HL=адрес банка, DE=main, BC=размер. Разрушает AF/BC/DE/HL.
bankRead:
    push bc
    ld a,(targetPage)
    call bios_RAMDRead
    ld a,c
    ld (de),a
    pop bc
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,bankRead
    ret
bankWrite:
    push bc
    ld a,(de)
    ld c,a
    ld a,(targetPage)
    call bios_RAMDWrite
    pop bc
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,bankWrite
    ret
localCopy:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,localCopy
    ret
; Сравнение без дополнительного буфера; 0=совпало, 1=ошибка.
verify:
    call byteMode
    ld hl,TARGET
    ld de,EXPECTED
    ld bc,LENGTH
verifyLoop:
    push bc
    ld a,(targetPage)
    call bios_RAMDRead
    ld a,(de)
    cp c
    pop bc
    jp nz,verifyBad
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,verifyLoop
    xor a
    ret
verifyBad:
    ld a,1
    ret
    INCLUDE "romcopy.inc"
    INCLUDE "builtin_byte.inc"
    INCLUDE "builtin_block.inc"
    INCLUDE "ramtests.inc"
    INCLUDE "banktests.inc"
    INCLUDE "copyengines.inc"
    INCLUDE "enginetests.inc"
inputBuffer: DS 11,0
; Локальные настройки общей измерительной библиотеки приложений.
    INCLUDE "../../source/include/timing-pit.inc"
timer_read = pitTimerRead
timing_print = puts

    MODULE copyMath
    DEFINE MATH_DIV
    INCLUDE "../../source/include/math.inc"
    ENDMODULE
timing_div_hl_de = copyMath.div_hl_de
TIMING_NUMBER_WIDTH = 8
DECIMAL = inputBuffer
timing_printChar:
    ld c,a
    jp bios_printChar
    INCLUDE "../../source/include/timing.inc"
    INCLUDE "../../source/include/timing-ms.inc"
callerSP: DW 0
callerKoi: DB 0
targetPage: DB 0
expectedPointer: DW 0
directOK: DB 0
blockOK: DB 0
byteOK: DB 0
timerError: DB 0
v_elapsed: DD 0
v_setup: DD 0
v_last: DW 0
directTicks: DD 0
blockTicks: DD 0
byteTicks: DD 0
oldWindow: DS 32,0
oldMainTail: DS 192,0
oldBankTail: DS 192,0
    INCLUDE "text.inc"
    INCLUDE "../../bin/COPYBEN.variants.inc"
programEnd:
    ASSERT programEnd <= 02000h
