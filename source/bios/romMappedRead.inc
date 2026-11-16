; Верхнее назначение: HL>=8000h, DE=ROM-адрес, C=MX-банк.
; Входные регистры уже сохранены diskDriver. Код и стек — выше 8000h.
; Место выбирается по layout.json. Новой точки ABI нет.
romMappedRead:
            ld      a, c
            ld      (IO_PAGE_ROM), a
            call    romCopySector
            ld      (IO_PAGE_RAM), a
    IF DISK_DRIVER_CHECKED
            jp      popDBHA_success
    ELSE
            jp      popDBHA_ret
    ENDIF

; Нижнее назначение: C=ROM-банк. Общий секторный буфер только в MAIN.
romBufferedRead:
            push    hl
            ld      hl, BIOS_DISK_BUFFER
            ld      a, c
            ld      (IO_PAGE_ROM), a
            call    romCopySector
            ld      (IO_PAGE_RAM), a
            pop     de
            ld      hl, BIOS_DISK_BUFFER
            ld      bc, 256
            call    moveBytesForward
    IF DISK_DRIVER_CHECKED
            jp      popDBHA_success
    ELSE
            jp      popDBHA_ret
    ENDIF

    IF ROM_64K
romStdMappedRead:
            xor     a
            ld      (IO_PAGE_STD), a
            call    romCopySector
            ld      (IO_PAGE_RAM-800h), a
            jp      romStdReadExit
    ENDIF

; Вход HL=верхнее назначение, DE=источник; длина ровно 256 байт.
; AF/BC/DE/HL и флаги разрушены. Маппер восстанавливает вызывающий.
; Области не пересекаются: используем прямой вход без выбора направления.
romCopySector:
            ex      de, hl
            ld      bc, 256
            jp      moveBytesForward
