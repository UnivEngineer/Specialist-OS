; RAMFOS использует отдельный формат кассеты. Профиль без кассетного
; драйвера возвращает исторический код прерывания чтения.
ramfosTapeReadUnavailable:
    ld a,3Fh
    scf
    ret

; C85Ah меняет видимые приложению режим и порядок трёх пар импульсов.
ramfosCycleTapeMode:
    ld d,4
.rotate:
    ld hl,ramfosTapePresetsEnd-1
    ld e,11
    ld a,(ramfosTapePresets)
.shift:
    ld c,a
    ld a,(hl)
    ld (hl),c
    dec hl
    dec e
    jp nz,.shift
    ld (hl),a
    dec d
    jp nz,.rotate
    ld hl,ramfosTapeMode
    ld a,(hl)
    inc a
    cp '3'
    jp nz,.save
    ld a,'0'
.save:
    ld (hl),a
    xor a
    ret
