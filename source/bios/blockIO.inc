; Физический запрос без монтирования FAT и изменения текущего каталога.
; A=диск 0..7, B=1 запись/2 чтение, DE=номер физического блока,
; HL=буфер блока. B=3 получает размер, B=FFh — описание драйвера.
; Размер блока задаётся установленным драйвером.
; Для 1/2 BC/DE/HL сохранены; для 3/FFh возвращается ответ драйвера.
; AF разрушен, CF=ошибка. Нереентерабельно.
; Страницу носителя из линейного номера блока вычисляет его драйвер.
blockIO:
    IFDEF BANKED_DOS_BUILD
    cp 8
    jp nc,blockIOError
    ELSE
    call biosPhysicalDriveCheck
    ret c
    ENDIF
    push af
    ld a,b
    cp 3
    jp z,blockIOModeReady
    cp 0FFh
    jp z,blockIOModeReady
    dec a
    cp 2
    jp nc,blockIOModeError
blockIOModeReady:
    pop af
    push hl
    ld hl,v_drive
    cp (hl)
    pop hl
    jp nz,blockIOSlow
    IFDEF BANKED_DOS_BUILD
    jp j_diskDriver
    ELSE
    ld a,(v_dosBanked)
    or a
    jp z,j_diskDriver
    ld a,(v_drive)
    ENDIF
blockIOSlow:
    push hl
    push de
    push bc
    ld c,a
    ld a,(v_drive)
    push af
    ld a,c
    ld (v_drive),a
    add a,a
    ld e,a
    ld d,0
    ld hl,v_drives
    add hl,de
    ld e,(hl)
    inc hl
    ld d,(hl)
    ex de,hl
    ld (blockIOCall+1),hl
    pop af
    ld (blockIORestore+1),a
    pop bc
    pop de
    pop hl
    call blockIOCall
blockIORestore:
    ld a,0
    ld (v_drive),a
    ret
blockIOCall:
    jp 0
blockIOError:
    scf
    ret
blockIOModeError:
    pop af
    scf
    ret
