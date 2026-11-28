; Резидентные движки 6×8 с общим входом FONT_ENGINE_INIT.
; FONT_MODE=0 (Rotate): повороты на лету, нижняя память не требуется.
; FONT_MODE=1 (Packed): 4096 байт нижних таблиц; FONT_MODE=2 (Fast): 10240.
; Перед Attach DE = база подготовленных страниц MAIN или 0 для Rotate.
; Attach подключает входы BIOS, выбирает KOI-8 и возвращает A=0, CF=0.
    INCLUDE "../common/system.inc"
    INCLUDE "../../bin/bios-imports.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
    IFNDEF FONT_MODE
FONT_MODE = 0
    ENDIF
    IF FONT_MODE == 2
FONT_LOWER_BYTES = 10240
    ELSE
FONT_LOWER_BYTES = FONT_MODE*4096
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
    ASSERT $ == FONT_ENGINE_TEXT_EXT
    DW FONT_ENGINE_TEXT_LIM
    ASSERT $ == FONT_ENGINE_HEADER
    DB "FNT1",FONT_ENGINE_VERSION,6,8,FONT_MODE
    DW imageEnd-FONT_ENGINE_BASE,FONT_ADDR
    DW FONT_LOWER_BYTES
    DW initializerName
    ASSERT $ == FONT_ENGINE_TEXT_LIM
    jp printStringLim
initializerName: DB "Init6.dll",0
printChar2     = biosEngineControl
printCharBody  = biosEngineControl
calcCursorAd_0 = biosEnginePosition
    INCLUDE "char.inc"
printChar_real2:
printCharRawHelper:
    ld hl,(bios_upper.charGen)
    call scalar
    ld hl,(mon2_vars.cursorY)
    ld a,h
    cp 192
    ret c
    jp fontLineEnd
    INCLUDE "string.inc"
    IF FONT_MODE != 0
    INCLUDE "string-lim.inc"
    ENDIF
    INCLUDE "packet6.inc"
    IFDEF FONT_BENCH_STREAM
    INCLUDE "stream.inc"
    ENDIF
    IF FONT_MODE == 2
    INCLUDE "full-scalar.inc"
scalar = scalarPre.draw
    ELSE
    IF FONT_MODE == 1
    INCLUDE "packed-scalar.inc"
    ELSE
    INCLUDE "rotate-address.inc"
    INCLUDE "rotate-scalar.inc"
    ENDIF
    ENDIF
    INCLUDE "install.inc"
    INCLUDE "bench.inc"
PSEUDO_STEP = 3
PSEUDO_WRAP = 190
    INCLUDE "pseudo.inc"
    FONT_PSEUDO_ENTRY
    ASSERT $ <= FONT_ADDR
    DS FONT_ADDR-$,0
fontData:
    IF FONT_MODE == 1
    INCBIN "../../bin/fonts/packed-font.bin"
    ELSE
    INCBIN "../../bin/fonts/paged-font.bin"
    ENDIF
    FONT_PSEUDO_TAIL
    IF FONT_MODE == 0
    INCLUDE "string-lim.inc"
    ENDIF
imageEnd:
    ASSERT $ <= FONT_ENGINE_END

    IFDEF FONT_BENCH_MAIN
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
