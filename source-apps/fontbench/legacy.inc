; Дополнительные старые варианты в освобождённой копии исходной фазы.
    INCLUDE "../../bin/FONTBEN.bios.entries.inc"
    DEFINE FONTBEN_EMBED_OPT
    DEFINE OPT_DRAW_ORIGIN 03800h
    MODULE legacyOpt
    INCLUDE "OPT_DRAW.asm"
    ENDMODULE

; HL=текст, C=0..48. Регистры сохраняются один раз на пакет, CF=ошибка.
; BIOS сохраняет свой фронтенд; сырые full/cache принимают только глифы.
serialBIOS:
    push af
    xor a
    jp serialEntry
serialFull:
    push af
    ld a,1
    jp serialEntry
serialCache:
    push af
    ld a,2
serialEntry:
    ld (serialKind),a
    push bc
    push de
    push hl
    ld (serialRead+1),hl
    ld a,c
    or a
    jp z,serialSuccess
    cp 49
    jp nc,serialError
    ld (serialRemaining),a
    ld a,0C3h
    ld (CACHE_RENDERERRETURN),a
    ld hl,serialContinue
    ld (CACHE_RENDERERRETURN+1),hl
    ld hl,serialFallback
    ld (cacheFallbackExit+1),hl
    ld a,(serialKind)
    or a
    ld hl,serialBiosGlyph
    jp z,serialSelect
    dec a
    ld hl,CACHE_GLYPHBODY
    jp z,serialSelect
    ld hl,cacheBody
serialSelect:
    ld (serialCore+1),hl
serialRead:
    ld hl,0
    ld c,(hl)
    inc hl
    ld (serialRead+1),hl
serialCore:
    jp serialBiosGlyph
serialBiosGlyph:
serialFallback:
    call BASE_PRINTCHAR_BODY
    jp serialContinue
serialContinue:
    ld hl,serialRemaining
    dec (hl)
    jp nz,serialRead
    ; Возвращаем POP HL/DE/BC: скалярный путь не платит за лишний JMP.
    ld hl,0D1E1h
    ld (CACHE_RENDERERRETURN),hl
    ld a,0C1h
    ld (CACHE_RENDERERRETURN+2),a
    ld hl,cacheFallbackScalar
    ld (cacheFallbackExit+1),hl
serialSuccess:
    pop hl
    pop de
    pop bc
    pop af
    or a
    ret
serialError:
    pop hl
    pop de
    pop bc
    pop af
    scf
    ret
serialKind: DB 0
serialRemaining: DB 0

installOptimized:
    ld a,0C3h
    ld (BASE_DRAWCHAR),a
    ld hl,legacyOpt.optimizedDraw
    ld (BASE_DRAWCHAR+1),hl
    ret
restoreOriginal:
    ld a,(originalDraw)
    ld (BASE_DRAWCHAR),a
    ld hl,(originalDraw+1)
    ld (BASE_DRAWCHAR+1),hl
    ret
originalDraw: DS 3,0
bankSerialBulk = bankImage+0200h
    INCLUDE "quad-packed.inc"
    INCLUDE "report-data.inc"
