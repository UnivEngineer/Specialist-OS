; Установить параметры копирования в выбранном банке.
; Вход: HL=регистр маппера, A=селектор банка. В общем окне лежат три пары
; селекторов, счётчик и SP с адресом возврата в странице инициатора.
; Выход: выбран инициатор, SP восстановлен. AF/BC/DE/HL разрушены.
bankCopyClone:
    ld (hl),a
    ld sp,BANK_COPY_SETUP_SOURCE
    pop hl
    pop de
    pop bc
    ld sp,bankCopyLocalParams+6
    push bc
    push de
    push hl
    ld hl,(BANK_COPY_SETUP_COUNT)
    ld (bankCopyCount),hl
    ld a,b
    ld l,c
    ld h,0FFh
    ld (hl),a
    ld hl,(BANK_COPY_SETUP_SP)
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
