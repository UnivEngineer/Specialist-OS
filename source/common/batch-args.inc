; Распаковать аргументы записи в MX_REGS по адресу BATCH_REGS.
; HL указывает на маску. Биты 0..3 задают наличие слов HL/DE/BC/AF в этом
; порядке. Отсутствующие поля обнуляются. Бит 4 заменяет HL в пакете
; адресом сразу за упакованными словами. DE указывает на этот же адрес.
; AF, BC, DE, HL меняются. Вызывающий проверяет размер записи.
    ld c,(hl)
    inc hl
    ex de,hl
    ld hl,0
    ld (BATCH_REGS),hl
    ld (BATCH_REGS+2),hl
    ld (BATCH_REGS+4),hl
    ld (BATCH_REGS+6),hl
    ld hl,BATCH_REGS
    ld b,4
unpackArguments:
    ld a,c
    rrca
    ld c,a
    jp nc,unpackSkip
    ld a,(de)
    ld (hl),a
    inc de
    inc hl
    ld a,(de)
    ld (hl),a
    inc de
    dec hl
unpackSkip:
    inc hl
    inc hl
    dec b
    jp nz,unpackArguments
    ld a,c
    and 1 ; original bit4: HL points after the packed register words
    jp z,unpackCall
    ld hl,BATCH_REGS
    ld (hl),e
    inc hl
    ld (hl),d
unpackCall:
