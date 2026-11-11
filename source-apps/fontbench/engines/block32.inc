; РљРѕРјРїР°РєС‚РЅС‹Р№ 6x8: РїСЏС‚СЊ Р±Р»РѕРєРѕРІ РїРѕ 32 РіР»РёС„Р°, Р±РµР· С‘/РЃ. Р РµРіРёСЃС‚СЂС‹ СЃРѕС…СЂР°РЅРµРЅС‹.
; HL=С‚РµРєСЃС‚, C=РґР»РёРЅР°, DE=СЌРєСЂР°РЅ; РґР»РёРЅР° 6x8 РєСЂР°С‚РЅР° С‡РµС‚С‹СЂС‘Рј. IRQ РІС‹РєР»СЋС‡РµРЅС‹.
    ASSERT (PAGED_FONT_BASE & 0FFh) = 0
    ASSERT (PAGED_PRE_BASE & 0FFh) = 0
    IFNDEF PAGED_CUSTOM_PHASES
PAGED_PHASE0 = PAGED_PRE_BASE
PAGED_PHASE1 = PAGED_PRE_BASE+00800h
PAGED_PHASE2 = PAGED_PRE_BASE+01000h
PAGED_PHASE3 = PAGED_PRE_BASE+01800h
PAGED_PHASE4 = PAGED_PRE_BASE+02000h
    ENDIF
    MODULE paged
    MACRO PAGE_ROW kind, advance
    ld a,(bc)
    IF kind = 0
    DUP 4
    rrca
    EDUP
    and 3
    ENDIF
    IF kind = 1
    rrca
    rrca
    and 0Fh
    ENDIF
    IF kind < 3
    ld (.merge+1),a
    ld a,(hl)
    IF kind = 0
    add a,a
    add a,a
    ENDIF
    IF kind = 1
    DUP 4
    rrca
    EDUP
    and 0F0h
    ENDIF
    IF kind = 2
    rrca
    rrca
    and 0C0h
    ENDIF
.merge: or 0
    ELSE
    or (hl)
    ENDIF
    ld (de),a
    IF advance
    inc c
    inc l
    inc e
    ENDIF
    ENDM
    MACRO PAGE_COLUMN offset, kind
    ld hl,(@blockPointers+offset*2+2)
    IF kind = 3
    ld a,h
    add a,(PAGED_PHASE3-PAGED_FONT_BASE)/256
    ld h,a
    ENDIF
    IF kind = 4
    ld a,h
    add a,(PAGED_PHASE1-PAGED_FONT_BASE)/256
    ld h,a
    ENDIF
    ld b,h
    ld c,l
    ld hl,(@blockPointers+offset*2)
    IF kind >= 3
    ld a,h
    IF kind = 3
    add a,(PAGED_PHASE0-PAGED_FONT_BASE)/256
    ENDIF
    IF kind = 4
    add a,(PAGED_PHASE4-PAGED_FONT_BASE)/256
    ENDIF
    IF kind = 5
    add a,(PAGED_PHASE2-PAGED_FONT_BASE)/256
    ENDIF
    ld h,a
    ENDIF
    DUP 7
    PAGE_ROW kind,1
    EDUP
    PAGE_ROW kind,0
    ld a,e
    sub 7
    ld e,a
    inc d
    ENDM
; Independent kernels: only initialization selects the entry point.
; No mode byte, conditional branch or patched dispatch inside either loop.
raw6:
    push af
    push bc
    push de
    push hl
    call raw6Helper
    jp done
pre6:
    push af
    push bc
    push de
    push hl
    call pre6Helper
    jp done
raw6Helper:
    ld a,c
    or a
    ret z
    ld (remaining),a
    ld (text),hl
raw6Next:
    ld hl,(text)
    ld a,(hl)
    ld (codes+0),a
    inc hl
    ld a,(hl)
    ld (codes+1),a
    inc hl
    ld a,(hl)
    ld (codes+2),a
    inc hl
    ld a,(hl)
    ld (codes+3),a
    inc hl
    ld (text),hl
    call @blockPreparePointers
    PAGE_COLUMN 0,0
    PAGE_COLUMN 1,1
    PAGE_COLUMN 2,2
    ld a,(remaining)
    sub 4
    ld (remaining),a
    jp nz,raw6Next
    ret
pre6Helper:
    ld a,c
    or a
    ret z
    ld (remaining),a
    ld (text),hl
pre6Next:
    ld hl,(text)
    ld a,(hl)
    ld (codes+0),a
    inc hl
    ld a,(hl)
    ld (codes+1),a
    inc hl
    ld a,(hl)
    ld (codes+2),a
    inc hl
    ld a,(hl)
    ld (codes+3),a
    inc hl
    ld (text),hl
    call @blockPreparePointers
    PAGE_COLUMN 0,3
    PAGE_COLUMN 1,4
    PAGE_COLUMN 2,5
    ld a,(remaining)
    sub 4
    ld (remaining),a
    jp nz,pre6Next
    ret
raw8: ; Р­С‚РѕС‚ РІР°СЂРёР°РЅС‚ С‚РµСЃС‚РёСЂСѓРµС‚ С‚РѕР»СЊРєРѕ 6x8.
    ret
done:
    pop hl
    pop de
    pop bc
    pop af
    ret
text: dw 0
codes: ds 4
remaining: db 0
    ENDMODULE
