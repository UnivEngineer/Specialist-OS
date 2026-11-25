; DE указывает на экран. Выбираем порог Z/RawZ/RawLim и полярность на пакет.
    ld a,(realMode)
    ld b,32
    add a,a
    jp z,compactThresholdReady
    rrca
    dec a
    ld b,1
    jp z,compactThresholdReady
    dec b
compactThresholdReady:
    ld a,b
    ld (compactLeftMin+1),a
    ld (compactRightMin+1),a
    ld (compactLeftUpMin+1),a
    ld (compactRightUpMin+1),a
    ld a,(bios_vars.inverse)
    sub 1
    sbc a,a
    cpl
    ld (compactEvenOddPolarity+1),a
    ld (compactEvenEvenPolarity+1),a
    ld (compactOddEvenPolarity+1),a
    ld (compactOddOddPolarity+1),a
    ld (compactEvenOddUpPolarity+1),a
    ld (compactEvenEvenUpPolarity+1),a
    ld (compactOddEvenUpPolarity+1),a
    ld (compactOddOddUpPolarity+1),a
    ld hl,0
    add hl,sp
    ld (savedSP+1),hl
    ld hl,(text)
    ld sp,hl
    MACRO PAIR_INPUT label, leftMin, rightMin, evenEven, evenOdd, oddEven, base
label:
    pop hl
    ld a,l
leftMin: cp 0
    jp c,stopBeforePair
    ld a,h
rightMin: cp 0
    jp c,stopBeforePair
    and 1
    ld b,a
    ld a,l
    and 1
    add a,a
    or b
    ld (.parity+1),a
    ld a,h
    COMPACT4_POINTER b,c,base
    ld a,l
    COMPACT4_POINTER h,l,base
.parity: ld a,0
    or a
    jp z,evenEven
    dec a
    jp z,evenOdd
    dec a
    jp z,oddEven
    ; Оба глифа в младших полубайтах. Поворачиваем левый и берём правый из HL.
    ld a,b
    ld b,h
    ld h,a
    ld a,c
    ld c,l
    ld l,a
    ENDM

    MACRO COMPACT4_PAIR label, polarity, mask, rotateBefore, rotateAfter, up, next
label:
    ld a,(bc)
    IF rotateBefore
    DUP 4
        rrca
    EDUP
    ENDIF
    xor (hl)
    and mask
    xor (hl)
    IF rotateAfter
    DUP 4
        rrca
    EDUP
    ENDIF
polarity: xor 0
    ld (de),a
    IF up
    dec e
    ELSE
    inc e
    ENDIF
    inc c
    inc l
    ld a,l
    and 7
    jp nz,label
    jp next
    ENDM

    PAIR_INPUT compactPair,compactLeftMin,compactRightMin,compactEvenEven,compactEvenOdd,compactOddEven,FONT_ADDR
    COMPACT4_PAIR compactOddOdd,compactOddOddPolarity,0F0h,1,0,0,compactNext
    COMPACT4_PAIR compactEvenEven,compactEvenEvenPolarity,0Fh,1,0,0,compactNext
    COMPACT4_PAIR compactOddEven,compactOddEvenPolarity,0F0h,0,1,0,compactNext
    COMPACT4_PAIR compactEvenOdd,compactEvenOddPolarity,0Fh,0,0,0,compactNext
compactNext:
    dec e
    inc d
    ld a,d
compactLimit: cp 0
    jp nz,compactPairUp
    jp finish

    PAIR_INPUT compactPairUp,compactLeftUpMin,compactRightUpMin,compactEvenEvenUp,compactEvenOddUp,compactOddEvenUp,FONT4_REVERSE_ADDR
    COMPACT4_PAIR compactOddOddUp,compactOddOddUpPolarity,0F0h,1,0,1,compactNextUp
    COMPACT4_PAIR compactEvenEvenUp,compactEvenEvenUpPolarity,0Fh,1,0,1,compactNextUp
    COMPACT4_PAIR compactOddEvenUp,compactOddEvenUpPolarity,0F0h,0,1,1,compactNextUp
    COMPACT4_PAIR compactEvenOddUp,compactEvenOddUpPolarity,0Fh,0,0,1,compactNextUp
compactNextUp:
    inc e
    inc d
    ld a,d
compactUpLimit: cp 0
    jp nz,compactPair
    jp finish
