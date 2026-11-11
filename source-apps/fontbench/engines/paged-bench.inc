; Построчные таблицы занимают прежние фазы и буфер будущего отчёта.
; После этапа прежние фазы восстанавливаются вне измеряемого интервала.
    DEFINE PAGED_CUSTOM_PHASES
PAGED_FONT_BASE = 07E00h
PAGED_PRE_BASE = 03000h
PAGED_PHASE0 = 03000h
PAGED_PHASE1 = 04000h
PAGED_PHASE2 = 04800h
PAGED_PHASE3 = 05000h
PAGED_PHASE4 = 05800h
    IFDEF FONT_BLOCK32
    INCLUDE "block32.inc"
    ELSE
    INCLUDE "paged.inc"
    ENDIF
pagedCoreStart:
pagedRaw:
    push af
    ld a,0
    jp pagedEntry
pagedPre:
    push af
    ld a,1
    jp pagedEntry
paged8:
    push af
    ld a,2
pagedEntry:
    push bc
    push de
    push hl
    ld (pagedMode),a
    ld hl,(bios_vars.cursorY)
    ld a,c
    add a,a
    add a,a
    ld b,a
    ld a,(pagedMode)
    cp 2
    ld a,b
    jp z,pagedWidth
    sub c
pagedWidth:
    add a,h
    ld (bios_vars.cursorX),a
    ld a,h
    rrca
    rrca
    add a,090h
    ld d,a
    ld a,l
    sub 7
    ld e,a
    pop hl
    push hl
    ld a,(pagedMode)
    or a
    jp z,pagedRawCall
    dec a
    jp z,pagedPreCall
    call paged.raw8
    jp pagedReturn
pagedPreCall:
    call paged.pre6
    jp pagedReturn
pagedRawCall:
    call paged.raw6
pagedReturn:
    pop hl
    pop de
    pop bc
    pop af
    or a
    ret
pagedCoreEnd:
pagedSelect:
    ld hl,(output+1)
    ld de,pagedCoreStart
    call bios_cmp_hl_de
    ret c
    ld de,pagedCoreEnd
    call bios_cmp_hl_de
    ret nc
    ld a,1
    ld (pagedDirty),a
    IFDEF FONT_BLOCK32
    call blockBuildFont
    ELSE
    ld hl,FONT_ADDR
    ld de,PAGED_FONT_BASE
    ld bc,2048
    call bench_copy
    ENDIF
    ld hl,pagedPhases
    ld (pagedTable+1),hl
pagedBuildPhase:
pagedTable: ld hl,0
    ld a,(hl)
    inc hl
    or a
    ret z
    ld d,a
    ld e,0
    ld a,(hl)
    inc hl
    ld (pagedSource+2),a
    ld a,(hl)
    inc hl
    ld (pagedOp),a
    ld (pagedOp+1),a
    ld a,(hl)
    inc hl
    ld (pagedOp+2),a
    ld (pagedOp+3),a
    ld a,(hl)
    inc hl
    ld (pagedMask+1),a
    ld (pagedTable+1),hl
pagedSource: ld hl,PAGED_FONT_BASE
    IFDEF FONT_BLOCK32
    ld bc,1280
    ELSE
    ld bc,2048
    ENDIF
pagedBuildByte:
    ld a,(hl)
pagedOp:
    nop
    nop
    nop
    nop
pagedMask: and 0FFh
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,pagedBuildByte
    jp pagedBuildPhase
pagedPhases:
    DB 030h,PAGED_FONT_BASE/256,087h,0,0FFh
    DB 040h,PAGED_FONT_BASE/256,00Fh,0,00Fh
    DB 048h,PAGED_FONT_BASE/256,00Fh,0,0C0h
    DB 050h,040h,00Fh,0,3
    DB 058h,030h,087h,0,0FFh
    DB 0
pagedRestore:
    ld a,(pagedDirty)
    or a
    ret z
    xor a
    ld (pagedDirty),a
    call font4RestorePhase0
    ld a,1
    ld (packedOverlay),a
    call font4RestorePackedOverlay
    ld hl,02800h
    ld de,05000h
    ld bc,2048
pagedRestore6:
    ld a,(hl)
    rrca
    rrca
    rrca
    rrca
    push af
    and 3
    ld (de),a
    inc de
    pop af
    and 0F0h
    ld (de),a
    inc de
    inc hl
    dec bc
    ld a,b
    or c
    jp nz,pagedRestore6
    ret
pagedDirty: DB 0
pagedMode: DB 0
