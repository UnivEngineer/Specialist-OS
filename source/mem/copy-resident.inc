; Установить параметры копирования в выбранном банке.
; Вход: HL=регистр маппера, A=селектор банка. В общем окне лежат три пары
; селекторов, счётчик и SP с адресом возврата в странице инициатора.
; Выход: выбран инициатор, SP восстановлен. AF/BC/DE/HL разрушены.
bankCopyClone:
    ld (hl),a
    ld sp,BANK_COPY_SETUP_SOURCE
    pop hl
    ld (bankCopySourcePair),hl
    pop hl
    ld (bankCopyDestPair),hl
    pop hl
    ld (bankCopyReturnPair),hl
    ld a,h
    ld h,0FFh
    ex de,hl
    pop hl
    ld (bankCopyCount),hl
    ld (de),a
    ; После выбора инициатора общее окно находится под SP.
    ASSERT BANK_COPY_SETUP_SP == BANK_COPY_SETUP_SOURCE+8
    pop hl
    ld sp,hl
    ret

; Точный остаток: сначала наполняется окно, затем пишется приёмник.
; Вход: HL/A — селектор источника, BC — указатель источника, SP — адрес
; приёмника, в BANK_COPY_TINY_COUNT лежит число байт 1..31. В источнике
; и приёмнике установлены пары выбора банка. bankCopySelectHome возвращает
; в страницу инициатора, где bankCopyReturn восстанавливает SP и регистры.
bankCopyTinySwitch:
    ld (hl),a
    ld h,b
    ld l,c
    ld de,BANK_WINDOW
    ld c,2
bankCopyTinyBegin:
    ld a,(BANK_COPY_TINY_COUNT)
    ld b,a
bankCopyTinyByte:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,bankCopyTinyByte
    dec c
    jp z,bankCopySelectHome
    ld hl,(bankCopyDestPair)
    ld a,h
    ld h,0FFh
    ld (hl),a
    ld hl,0
    add hl,sp
    ex de,hl
    ld l,BANK_WINDOW & 255      ; H ещё FFh: HL указывал за данные в окне.
    jp bankCopyTinyBegin
