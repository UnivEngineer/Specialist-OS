; C254h/C25Ah возвращают A=82h/91h, сохраняя BC/DE/HL и флаги.
; Установщик профиля меняет младший байт адреса ВВ55.
    ASSERT $ == mon2_setKeybMode82
setKeybMode82:
    ld a,082h
    ld (IO_KEYB_MODE),a
    ret
    ASSERT $ == mon2_setKeybMode91
setKeybMode91:
    ld a,091h
    ld (IO_KEYB_MODE),a
    ret

