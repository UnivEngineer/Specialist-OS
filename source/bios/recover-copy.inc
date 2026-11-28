; Восстановление общего кода MEM после классической программы.
; Вход: main/MX, DI, исправная DOS в банке 0, установленный стек BIOS.
; Выход: A=1 при повреждении сигнатуры FF00h–FF03h, иначе 0; BC/DE/HL/флаги разрушены.
; До восстановления не вызываем ни транспорт, ни FAR-копировщик.
biosRecoverCopyTail:
    ld hl,biosRecoveryWindow
    ld de,biosRecoveryWindowEnd
    ld bc,BANK_WINDOW
    call moveBiosLow
    ; Эталон сигнатуры находится в DOS; инструкции копировщика идут после неё.
    ld hl,BANK_COPY_GUARD_ADDR
    ld b,4
biosRecoveryCheck:
    call BANK_WINDOW
    ld a,(hl)
    cp c
    jp nz,biosRecoveryBad
    inc hl
    dec b
    jp nz,biosRecoveryCheck
    xor a
    jp biosRecoveryFlag
biosRecoveryBad:
    ld a,1
biosRecoveryFlag:
    push af
    ; Общий MEM берём из DOS; контексты и накладки восстановим отдельно.
    ld hl,MEM_IMAGE_LOCAL_START
    ; MAIN: не затирать установленный AT512KS локальными службами DOS/EXEC.
    ld de,BANK_MAIN_LOCAL_BYTES
    call biosRecoverRange
    ld hl,mx_callPacket
    ; Общий CLIENT включает таблицу вызовов и текстовый шлюз. F800h–F85Fh не копируются.
    ld de,BANK_MAIN_CLIENT_BYTES
    call biosRecoverRange
    ld hl,BANK_LANDING
    ld de,BANK_TRANSPORT_BYTES
    call biosRecoverRange
    ; Эталон накладок MAIN хранится перед кодом DOS. Формат каждой записи
    ; — длина, адрес и байты. Нулевая длина завершает обход.
    ld hl,DOS_BIOS_OVERLAY_ADDR
.overlay:
    call BANK_WINDOW
    inc hl
    or c
    jp z,.done
    ld b,c
    call BANK_WINDOW
    ld e,c
    inc hl
    call BANK_WINDOW
    ld d,c
    inc hl
.copyOverlay:
    call BANK_WINDOW
    ld a,c
    ld (de),a
    inc hl
    inc de
    dec b
    jp nz,.copyOverlay
    jp .overlay
.done:
    pop af
    ret
; HL=общий адрес main/DOS, DE=длина>0. Никаких стековых команд в банке DOS.
biosRecoverRange:
    call BANK_WINDOW
    ld (hl),c
    inc hl
    dec de
    ld a,d
    or e
    jp nz,biosRecoverRange
    ret
biosRecoveryWindow:
    xor a
    ld (IO_PAGE_RAMD),a
    ld c,(hl)
    ld (IO_PAGE_RAM),a
    ret
biosRecoveryWindowEnd:
; Вход после восстановления локальных контекстов. Выход: AF/BC/HL разрушены.
biosMemWarning:
    ld hl,biosMemWarningText
    call printString
    jp getch
biosMemWarningText:
    DB 0Ah,"STD I/O IN MX. KEY?",0Ah,0
    ASSERT BANK_TRANSPORT_BYTES-0300h <= 0C0h
    ASSERT biosRecoveryWindowEnd-biosRecoveryWindow <= 32
