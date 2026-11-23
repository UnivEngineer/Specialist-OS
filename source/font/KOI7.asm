; Движок KOI-7R 6×8. После распознавания управляющих кодов старший бит
; глифа отбрасывается. Вход псевдографики совпадает с RawLim.
; K7_MODE 0/1/2 выбирает Rotate/Packed/Hybrid без нижней резервации;
; режим 3 использует стендовые таблицы по K7_LOWER. Attach возвращает
; A=0, CF=0. Вход SET_FONT_ENCODING сохраняет регистры и флаги;
; раскладка остаётся KOI-7R.
    INCLUDE "../include/mxos.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    DEFINE FONT_KOI7
    IF K7_MODE == 0 || K7_MODE == 3
K7_TABLES = FONT_ADDR
K7_RAW    = FONT_ADDR
    ELSE
K7_TABLES = FONT_ADDR-0400h
K7_RAW    = FONT_ADDR+0400h
    ENDIF
    IF K7_MODE == 3
    ASSERT FONT_ENGINE_BASE == 02000h
K7_LOWER = 05000h
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
    jp printStringRawLim ; fixed KOI-7R has no pseudographic extension
    DS FONT_ENGINE_HEADER-$,0
    DB "FNT1",FONT_ENGINE_VERSION,6,8,FONT_FORMAT_KOI7+K7_MODE
    DW imageEnd-FONT_ENGINE_BASE,K7_RAW,0,initializerName
initializerName: DB 'Init7.dll',0

printChar2     = bios_fontControl
printCharBody  = bios_fontControl
calcCursorAd_0 = bios_fontPosition
fontLineEnd    = bios_fontNewLine
    INCLUDE "char.inc"
printChar_real2:
printCharRawHelper:
    call scalar
    ld hl,(bios_vars.cursorY)
    ld a,h
    cp 192
    ret c
    jp fontLineEnd
    INCLUDE "string.inc"
    INCLUDE "packet6.inc"
scalar:
    ld d,K7_RAW/256
    ld a,c
    and 7Fh
    ld e,a
    call printChar_position
    jp scalar6.scalar6Draw
    IF K7_MODE != 1
    INCLUDE "scalar-position.inc"
    ENDIF
    INCLUDE "rotate-scalar.inc"

    IF K7_MODE != 1
    INCLUDE "koi7-install.inc"
    ENDIF
codeEnd:
    ASSERT $ <= K7_TABLES
    DS K7_TABLES-$,0
    IF K7_MODE == 0
    INCBIN "../../bin/fonts/koi7-rotate.bin"
    ELSE
    IF K7_MODE == 1
    INCBIN "../../bin/fonts/koi7-packed.bin"
    ELSE
    IF K7_MODE == 2
    INCBIN "../../bin/fonts/koi7-hybrid.bin"
    ELSE
    INCBIN "../../bin/fonts/koi7-fast.bin",01000h,0800h
    ENDIF
    ENDIF
    ENDIF
    IF K7_MODE == 1
    ; The page-aligned tables leave a short tail for independent routines.
    INCLUDE "scalar-position.inc"
    INCLUDE "koi7-install.inc"
    ENDIF
imageEnd:
    IFDEF FONT_BENCH_MAIN
    ASSERT $ <= 04C00h
    ELSE
    ASSERT $ <= FONT_ENGINE_END
    ENDIF
    IF K7_MODE == 3
    DS      K7_LOWER-$,0
    INCBIN  "../../bin/fonts/koi7-fast.bin",0,01000h
    ENDIF
