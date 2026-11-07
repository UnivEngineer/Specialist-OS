    INCLUDE "r4-abi.inc"
    INCLUDE "r4-far.inc"
; Копирование маленького служебного блока, HL -> DE, B=1..255.
; Разрушает AF/B/DE/HL, сохраняет C.
bankCopyBytes:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bankCopyBytes
    ret
bankPage: DB 0
bankPageCount: DB 0
farRequest: DB 0
    DW TARGET
    DB 0
    DW TARGET,LENGTH
sourcePhysical: DB 0
destinationPhysical: DB 0
otherTail: DS 192,0
farTailImage:
    DISP 0FF00h
    INCLUDE "r4-tail.inc"
    ENT
farTailEnd:
    ASSERT farTailEnd-farTailImage <= 192
