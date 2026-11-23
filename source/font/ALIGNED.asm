; Резидентные движки 8×8 и 4×8. Шаг строки BIOS — 10 пикселей;
; вывод рисует восемь строк. X измеряется в парах пикселей. Обычные
; профили выравнивают X вниз к границе знакоместа, FONT_GRAPHIC_X
; допускает любое значение X. При 4×8 сохраняется соседний полубайт.
; Attach: DE = база подготовленного нижнего пререндера или 0, если
; FONT_LOWER_BYTES=0. Возвращает A=0, CF=0 и выбирает KOI-8.
    INCLUDE "../include/mxos.inc"
FONT_MODE = 0
    IFNDEF GRAPH_PRE_MODE
GRAPH_PRE_MODE = 0
    ENDIF
    IFDEF FONT4_FAST
FONT_LOWER_BYTES = FONT_BYTES
    ELSE
FONT_LOWER_BYTES = GRAPH_PRE_MODE*6144
    ENDIF
    IFDEF FONT4_COMPACT
FONT4_REVERSE_ADDR = FONT_ADDR-0400h
    ENDIF
    ORG FONT_ENGINE_BASE
    IFDEF FONT_BENCH_MAIN
    jp benchInitialize
    ELSE
    jp attach
    ENDIF
    jp printChar
    jp printCharRaw
    jp printStringZ
    jp printStringRawZ
    jp printStringRawLim
    jp setEncoding
    jp scalar
    jp service
    jp fontPseudoDraw
    DS FONT_ENGINE_HEADER-$,0
    IFDEF FONT_GRAPHIC_X
    DB 'FNT1',FONT_ENGINE_VERSION,CELL_WIDTH,8,FONT_FORMAT_GRAPH8+GRAPH_PRE_MODE
    ELSE
    IF CELL_WIDTH == 8
    DB 'FNT1',FONT_ENGINE_VERSION,8,8,4
    ELSE
    IFDEF FONT4_FAST
    DB 'FNT1',FONT_ENGINE_VERSION,4,8,FONT_FORMAT_4_FAST
    ELSE
    IFDEF FONT4_COMPACT
    DB 'FNT1',FONT_ENGINE_VERSION,4,8,FONT_FORMAT_4_COMPACT
    ELSE
    DB 'FNT1',FONT_ENGINE_VERSION,4,8,FONT_FORMAT_4_PACKED
    ENDIF
    ENDIF
    ENDIF
    ENDIF
    DW imageEnd-FONT_ENGINE_BASE,FONT_ADDR,FONT_LOWER_BYTES,initializerName
initializerName: DB 'InitA.dll',0
printChar2    = bios_fontControl
printCharBody = bios_fontControl
    INCLUDE "char.inc"
printCharRawHelper:
    call scalar
    ld hl,(bios_vars.cursorY)
    ld a,h
    cp 192
    ret c
    jp fontLineEnd
scalar:
    ld hl,(bios_vars.cursorY)
    ld a,h
    cp 192
    jp nc,scalarWrap
scalarPosition:
    ld a,h
    IFNDEF FONT_GRAPHIC_X
    and 256-CELL_WIDTH/2
    ld h,a
    ENDIF
    add a,CELL_WIDTH/2
    ld (bios_vars.cursorX),a
    ld a,l
    sub 7
    ld e,a
    ld a,h
    rrca
    rrca
    and 63
    add a,90h
    ld d,a
    IFDEF FONT4_COMPACT
    INCLUDE "compact4-scalar.inc"
    ELSE
    IF CELL_WIDTH == 4
    ld a,h
    and 2
    ld (half+1),a
    ENDIF
    ld l,c
    IFDEF FONT_GRAPHIC_X
    ld a,h
    and 3
    jp nz,graphicScalar
    ENDIF
    ld h,FONT_ADDR/256
    ld a,(bios_vars.inverse)
    or a
    jp nz,scalarInverse
    IF CELL_WIDTH == 4
half: ld a,0
    or a
    jp nz,scalarNormalRight
    ENDIF
    jp normalLeft
scalarInverse:
    IF CELL_WIDTH == 4
    ld a,(half+1)
    or a
    jp nz,scalarInverseRight
    ENDIF
    jp inverseLeft
    MACRO ALIGNED_ROW inv, right, advance
    ld a,(hl)
    IFDEF FONT4_FAST
    ; В отдельной таблице заполнен только полубайт этого знакоместа.
    IF inv
    IF right
    xor 0Fh
    ELSE
    xor 0F0h
    ENDIF
    ENDIF
    ELSE
    IF inv
    cpl
    ENDIF
    IF CELL_WIDTH == 4
    IF right
    and 0Fh
    ELSE
    and 0F0h
    ENDIF
    ENDIF
    ENDIF
    IF CELL_WIDTH == 4
    ld b,a
    ld a,(de)
    IF right
    and 0F0h
    ELSE
    and 0Fh
    ENDIF
    or b
    ENDIF
    ld (de),a
    IF advance
    inc h
    inc e
    ENDIF
    ENDM
    MACRO ALIGNED_CORE label, inv, right
label:
    DUP 7
        ALIGNED_ROW inv,right,1
    EDUP
    ALIGNED_ROW inv,right,0
    ret
    ENDM
    ALIGNED_CORE normalLeft,0,0
    ALIGNED_CORE inverseLeft,1,0
    IFDEF FONT_GRAPHIC_X
    INCLUDE "graphic8.inc"
    ENDIF
    IF CELL_WIDTH == 4
    IFDEF FONT4_FAST
scalarNormalRight:
    ld a,(fontPreBase+1)
    ld h,a
    ALIGNED_CORE normalRight,0,1
scalarInverseRight:
    ld a,(fontPreBase+1)
    ld h,a
    ALIGNED_CORE inverseRight,1,1
    ELSE
    ALIGNED_CORE scalarNormalRight,0,1
    ALIGNED_CORE scalarInverseRight,1,1
    ENDIF
    ENDIF
    ENDIF
scalarWrap:
    ; Preserve the code only on the exceptional pre-render wrap. Normal
    ; aligned scalar calls do not save BC or reload the cursor twice.
    push bc
    call bios_fontNewLine
    pop bc
    ld hl,(bios_vars.cursorY)
    jp scalarPosition
printChar_real2 = printCharRawHelper
    IF CELL_WIDTH == 8
    INCLUDE "packet8.inc"
    IFDEF FONT_GRAPHIC_X
    INCLUDE "graphic8-packet.inc"
    ENDIF
    IFDEF FONT_ALIGNED_ROWS
    INCLUDE "graphic8-packet.inc"
    ENDIF
    ELSE
    INCLUDE "packet4.inc"
    IFDEF FONT_BENCH_STREAM
    INCLUDE "stream.inc"
    ENDIF
    ENDIF
    INCLUDE "string.inc"
    INCLUDE "install.inc"
    INCLUDE "bench.inc"
PSEUDO_STEP = CELL_WIDTH/2
PSEUDO_WRAP = 192
    IFNDEF FONT_GRAPHIC_X
    DEFINE PSEUDO_ROUND (256-PSEUDO_STEP)
    ENDIF
    INCLUDE "pseudo.inc"
    FONT_PSEUDO_ENTRY
    IFDEF FONT4_COMPACT
    ASSERT $ <= FONT4_REVERSE_ADDR
    ENDIF
    ASSERT $ <= FONT_ADDR
    DS FONT_ADDR-$,0
fontData:
    IF CELL_WIDTH == 8
    INCBIN "../../bin/fonts/win8-paged.bin"
    ELSE
    IFDEF FONT4_COMPACT
    INCBIN "../../bin/fonts/cpm4-compact.bin"
    ELSE
    IFDEF FONT4_FAST
    INCBIN "../../bin/fonts/cpm4-left.bin"
    ELSE
    INCBIN "../../bin/fonts/cpm4-paged.bin"
    ENDIF
    ENDIF
    ENDIF
    FONT_PSEUDO_TAIL
imageEnd:
    ASSERT $ <= FONT_ENGINE_END

    IFDEF FONT_BENCH_MAIN
    IFDEF FONT4_FAST
    DS      05000h-$,0
    INCBIN  "../../bin/fonts/Cpm4Fast.pre"
    ENDIF
    IFDEF FONT_MODE
    IF FONT_MODE
    DS 05000h-$,0
    IF FONT_MODE == 1
    INCBIN "../../bin/fonts/Packed6K.pre"
    ELSE
    INCBIN "../../bin/fonts/Fast12K.pre"
    ENDIF
    ENDIF
    ENDIF
    ASSERT $ <= 07C00h
    ENDIF
