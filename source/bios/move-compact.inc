; Перенос BIOS для STD, загрузки, восстановления MEM и отображённого ROM.
; moveCore: HL=источник, DE=исключающий конец, BC=назначение.
; moveBytesForward: HL=источник, DE=назначение, BC=длина без перекрытия.
; HL/BC возвращают исключающие концы. AF/DE изменяются, SP сохраняется.
    INCLUDE "../common/move-mem-entries.inc"

moveCore:
    jp moveBiosLow

; RUN перед STD и возврат BIOS до проверки MEM оставляют видимый низкий путь.
biosMoveUseLow:
    push hl
    ld hl,moveBiosLow
    ld (moveCore+1),hl
    pop hl
    ret

; Проверить совместимость полного MEM после bootstrap или его восстановления.
biosMoveUseMem:
    push af
    push hl
    ld hl,(MEM_RUNTIME_REVISION_ADDR)
    ld a,l
    cp moveMemRevision & 255
    jp nz,biosMoveKeepLow
    ld a,h
    cp moveMemRevision >> 8
    jp nz,biosMoveKeepLow
    ld hl,moveMemCore
    ld (moveCore+1),hl
    pop hl
    pop af
    ret
biosMoveKeepLow:
    pop hl
    pop af
    jp biosMoveUseLow

moveBiosLow:
    push bc
    ld a,e
    sub l
    ld c,a
    ld a,d
    sbc a,h
    ld b,a
    pop de
    or c
    jp z,moveBiosForwardDone
    ld a,d
    cp h
    jp c,moveBiosForward
    jp nz,moveBiosBack
    ld a,e
    cp l
    jp c,moveBiosForward
moveBiosBack:
    add hl,bc
    push hl
    ex de,hl
    add hl,bc
    push hl
    ex de,hl
moveBiosBackByte:
    dec hl
    dec de
    ld a,(hl)
    ld (de),a
    dec bc
    ld a,b
    or c
    jp nz,moveBiosBackByte
    pop bc
    pop hl
    ret
moveBytesForward:
    ld a,b
    or c
    jp z,moveBiosForwardDone
moveBiosForward:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec bc
    ld a,b
    or c
    jp nz,moveBiosForward
moveBiosForwardDone:
    ld b,d
    ld c,e
    ret
moveCoreEnd:
