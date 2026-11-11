; Восстановление общего кода MEM после классической программы.
; Вход: main/MX, DI, исправная DOS в банке 0, установленный стек BIOS.
; Выход: A=1 при повреждении сигнатуры FF00h–FF03h, иначе 0; BC/DE/HL/флаги разрушены.
; До восстановления не вызываем ни транспорт, ни FAR-копировщик.
biosRecoverCopyTail:
    ld hl,biosRecoveryWindow
    ld de,biosRecoveryWindowEnd
    ld bc,BANK_WINDOW
    call memcpy_bc_hl
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
    ; Код один и тот же; пять полей контекста LOCALBLOCK задаёт sysInstall.
    ld hl,MEM_IMAGE_LOCAL_START
    ld de,BANK_LOCAL_BYTES
    call biosRecoverRange
    ld hl,mx_call
    ; Службы EXEC/DOS после bankClientEnd занимают в main слот FLASHSYS.
    ; Восстанавливаем только общий клиент, сохраняя установленный драйвер A:.
    ld de,BANK_MAIN_CLIENT_BYTES
    call biosRecoverRange
    ld hl,BANK_LANDING
    ld de,BANK_TRANSPORT_BYTES
    call biosRecoverRange
    ld hl,biosRecoveryCommand
    ld de,biosRecoveryCommandEnd
    ld bc,BANK_WINDOW
    call memcpy_bc_hl
    xor a
    ld (BANK_PROFILE),a
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
biosRecoveryCommand:
    INCBIN "../../bin/banked/WINDOW.bin",0,8
biosRecoveryCommandEnd:
; Вход после восстановления локальных контекстов. Выход: AF/BC/HL разрушены.
biosMemWarning:
    ld hl,biosMemWarningText
    call printString
    jp getch
biosMemWarningText:
    DB 0Ah,"STD I/O WRITE IN MX MODE. PRESS ANY KEY",0Ah,0
    ASSERT BANK_TRANSPORT_BYTES-0300h <= 0C0h
    ASSERT biosRecoveryWindowEnd-biosRecoveryWindow <= 32
    ASSERT biosRecoveryCommandEnd-biosRecoveryCommand == 8
