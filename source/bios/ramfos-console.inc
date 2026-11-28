ramfosInput:
    call ramfosCursorIn
    call input
    jp ramfosCursorOut
ramfosPrintHex:
    call ramfosCursorIn
    call printHexByte
    jp ramfosCursorOut
ramfosKeyWait:
    ld a,(ramfosPendingCode)
    cp 0FFh
    jp z,.wait
    push af
    ld a,0FFh
    ld (ramfosPendingCode),a
    pop af
    or a
    ret
.wait:
    call ramfosCursorIn
    call getch
    or a
    jp ramfosCursorOut

; C800h: начать сеанс RAMFOS с исходным состоянием ввода и звука.
; Параметров нет. Экран очищается, AF/BC/DE/HL и флаги могут измениться.
ramfosInit:
    call ioKeyMode82
    ld de,01510h
    ld bc,01010h
    xor a
    inc a
    call beepTimer
    xor a
    ld h,a
    ld l,a
    ld (mon2_vars.inverse),hl
    dec a
    ld (ramfosPendingCode),a

ramfosClear:
    push af
    push hl
    ld a,0FFh
    ld (ramfosLineId),a
    ld a,0F1h
    call ramfosSetColor
    call clearScreen
    ld hl,8
    ld (mon2_vars.cursorY),hl
    ld (ramfos_cursorPos),hl
    pop hl
    pop af
    ret

; Аппаратный регистр цвета используется только на запись.
ramfosSetColor:
    ld (IO_COLOR),a
    ret

; Опрос матрицы с декодированием букв и состоянием раскладки.
ramfosKeyScan:
    call keyScan2
    cp 0FFh
    jp z,.done
    push bc
    push de
    ld b,a
    call keyDecodeCurrent
    jp nz,.event
    ld a,0FFh
.event:
    push af
    ld a,e
    ld (mon2_vars.keyLocks),a
    pop af
    pop de
    pop bc
.done:
    or a
    ret
ramfosKeyCheck:
    ld a,(ramfosPendingCode)
    cp 0FFh
    jp nz,.ready
    call ramfosKeyScan
    ld (ramfosPendingCode),a
    inc a
    ret z
.ready:
    ld a,0FFh
    or a
    ret

; F85Ah: HL = вход программы, DE = строка параметров. Возврат ведёт в BIOS.
ramfosStart:
    di
    ld sp,STACK_ADDR
    push hl
    ld hl,biosCompatExit
    ex (sp),hl
    jp (hl)

ramfosLineId:      DB 0FFh
ramfosPendingCode: DB 0FFh
