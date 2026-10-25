; Проверка загружаемого ядра: экран/клавиатура и отказ файловых вызовов.
; Запуск: A:RAMRUN.COM A:RFTEST.COM
    IFNDEF TEST_ORIGIN
TEST_ORIGIN = 0
    ENDIF
    ORG TEST_ORIGIN
    call 0F824h
    jp nz, failed
    jp c, failed
    call 0F827h
    jp nz, failed
    jp c, failed
    ld bc, 01234h
    ld de, 05678h
    ld hl, 09ABCh
    call 0F839h
    jp nc, failed
    jp z, failed
    ld a, c
    cp 034h
    jp nz, failed
    call 0F836h
    jp nc, failed
    ld a, c
    cp 0FFh
    jp nz, failed
    ld hl, passedText
    call 0F818h
    IFDEF TEST_AUTOMATIC
    ld a, 05Ah
    ld (08EFEh), a
    ELSE
    call 0F803h
    ENDIF
    ret
failed:
    ld hl, failedText
    call 0F818h
    IFDEF TEST_AUTOMATIC
    halt
    ELSE
    call 0F803h
    jp 0F800h
    ENDIF
passedText:
    DB "RAMFOS ABI OK. PRESS KEY TO RETURN TO MXOS.",0
failedText:
    DB "RAMFOS ABI FAILED",0
    END
