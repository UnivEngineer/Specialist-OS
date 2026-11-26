    INCLUDE "../../sdk/asm/hardware/spec_mx.inc"
; Компактный движок KOI-8 6×8, 162 встроенных глифа по 8 байт.
; Карта заменяет отсутствующие коды исходным глифом DEL. Переключение
; KOI-7R меняет карту и подставляет пробел для кода 7Fh.
; Attach не требует нижних таблиц, восстанавливает KOI-8 и подключает
; входы BIOS. Выход A=0, CF=0.
    INCLUDE "../../sdk/asm/mxos.inc"
    INCLUDE "../../sdk/asm/formats/font.inc"
IMAGE_FLAGS = IMAGE_FLAG_MAIN
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
    DB 'FNT1',FONT_ENGINE_VERSION,6,8,3
    DW imageEnd-FONT_ENGINE_BASE,fontData,0,initializerName
    ASSERT $ == FONT_ENGINE_TEXT_LIM
    jp printStringLim
initializerName: DB 'InitKoi.dll',0
printChar2     = bios_fontControl
printCharBody  = bios_fontControl
calcCursorAd_0 = bios_fontPosition
    INCLUDE "char.inc"
printCharRawHelper:
    call scalar
    ld hl,(bios_vars.cursorY)
    ld a,h
    cp 192
    ret c
    jp fontLineEnd
scalar:
    ld h,glyphMap/256
    ld l,c
    ld l,(hl)
    ld h,0
    add hl,hl
    add hl,hl
    add hl,hl
    ld de,fontData
    add hl,de
    ex de,hl
    call printChar_position
    jp scalar6.scalar6Draw
    INCLUDE "scalar-position.inc"
    DEFINE SCALAR_LINEAR
    INCLUDE "rotate-scalar.inc"
; Save public registers once per string. There is no paged-font check or
; unsuccessful quad probe for each glyph in this compact scalar renderer.
printStringZ:
    push af
    push bc
    push de
stringNext:
    ld a,(hl)
    or a
    jp z,stringDone
    ld c,a
    push hl
    call printCharBody
    pop hl
    inc hl
    jp stringNext
printStringRawZ:
    push af
    push bc
    push de
rawNext:
    ld a,(hl)
    or a
    jp z,stringDone
    ld c,a
    push hl
    call printCharRawHelper
    pop hl
    inc hl
    jp rawNext
printStringRawLim:
    push af
    push bc
    push de
limNext:
    ld a,b
    or c
    jp z,stringDone
    push bc
    push hl
    ld c,(hl)
    call printCharRawHelper
    pop hl
    pop bc
    inc hl
    dec bc
    jp limNext
; Компактный движок использует тот же посимвольный путь, что и обычный Z.
printStringLim:
    push af
    push bc
    push de
stringLimNext:
    ld a,b
    or c
    jp z,stringDone
    push bc
    push hl
    ld c,(hl)
    call printCharBody
    pop hl
    pop bc
    inc hl
    dec bc
    jp stringLimNext
stringDone:
    pop de
    pop bc
    pop af
    ret
attach:
    xor a
    call setEncoding ; Reattaching an existing module also restores KOI8.
    ld hl,fontData
    ld (bios_upper.charGen),hl
    xor a
    ld (bios_upper.koi7),a
    ld (encodingOld+1),a
    INCLUDE "bind.inc"
    xor a
    ret
setEncoding:
    push af
    push bc
    push de
    push hl
    or a
    jp z,encodingReady
    ld a,0FFh
encodingReady:
encodingOld: cp 0
    jp z,encodingDone
    ld (encodingOld+1),a
    ld (bios_upper.koi7),a
    ld hl,glyphMap+060h
    ld de,glyphMap+0E0h
    ld b,31
encodingSwap:
    ld c,(hl)
    ld a,(de)
    ld (hl),a
    ld a,c
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,encodingSwap
    ; HL указывает на 7Fh. Не меняем глиф-заполнитель пропущенных кодов.
    ld a,(encodingDel)
    ld c,(hl)
    ld (hl),a
    ld a,c
    ld (encodingDel),a
encodingDone:
    pop hl
    pop de
    pop bc
    pop af
    ret
encodingDel: DB (blankGlyph-fontData)/8
service:
    or a
    jp z,query
    cp FONT_RENDER_DISABLE
    jp z,query
    ld a,DOS_ERR_UNSUPPORTED
    scf
    ret
query:
    ld hl,0
    ld bc,0
    ld de,fontData
    xor a
    ret
    INCLUDE "bench.inc"
PSEUDO_STEP = 3
PSEUDO_WRAP = 190
    DEFINE PSEUDO_COMPACT
    INCLUDE "pseudo.inc"
    FONT_PSEUDO_ENTRY
    DS (-$)&255,0
; Byte-aligned table gives the glyph index in two loads, no range search.
glyphMap:       INCBIN  "../../bin/fonts/compact-map.bin"
fontData:       INCBIN  "../../bin/fonts/compact-font.bin"
blankGlyph:     DS      8,0
    FONT_PSEUDO_TAIL
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
