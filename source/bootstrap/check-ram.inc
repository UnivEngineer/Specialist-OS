;----------------------------------------------------------------------------
; MXOS
; Определить число физических банков, учитывая зеркала отсутствующих страниц.
; Основа — алгоритм RAMFOS (vinxru, 2013), доработка SpaceEngineer, 2022.
; вход: стек в MAIN, прерывания выключены. Регистровых параметров нет.
; выход: A — число банков, Z при отсутствии ДОЗУ. AF/BC/DE/HL изменены.
; Пробные байты восстановлены. В общем окне остаются входы чтения и записи.
;----------------------------------------------------------------------------
checkBootRam:
    ld hl,bootByteImage
    ld de,bootByteEnd
    ld bc,BOOT_WINDOW
    call moveCore
    ld hl,BOOT_RAM_PROBE
    ld a,BOOT_MAX_PAGES-1
.mark:
    ; При зеркалировании несколько селекторов указывают на один байт.
    ; Обратный порядок восстановления возвращает его исходное значение.
    call BOOT_BYTE_READ
    push bc
    ld c,a
    inc c
    call BOOT_BYTE_WRITE
    dec a
    jp p,.mark
    xor a
.count:
    call BOOT_BYTE_READ
    dec c
    cp c
    jp nz,.counted
    inc a
    cp BOOT_MAX_PAGES
    jp nz,.count
.counted:
    ld b,a
    xor a
.restore:
    pop de
    ld c,e
    call BOOT_BYTE_WRITE
    inc a
    cp BOOT_MAX_PAGES
    jp nz,.restore
    ld a,b
    or a
    ret

;----------------------------------------------------------------------------
; MXOS
; Прочитать или записать байт ДОЗУ до установки MEM.
; вход: образ скопирован по BOOT_WINDOW, стек в MAIN. A — физический банк,
; HL — адрес, C — значение для BOOT_BYTE_WRITE.
; выход: выбран MAIN. BOOT_BYTE_READ меняет только C, BOOT_BYTE_WRITE
; меняет только байт в банке. Флаги и SP сохранены.
;----------------------------------------------------------------------------
bootByteImage:
    ld (0FFFDh),a
    ld c,(hl)
    ld (0FFFCh),a
    ret
bootByteWriteImage:
    ld (0FFFDh),a
    ld (hl),c
    ld (0FFFCh),a
    ret
bootByteEnd:
BOOT_BYTE_READ  = BOOT_WINDOW
BOOT_BYTE_WRITE = BOOT_WINDOW+bootByteWriteImage-bootByteImage
    ASSERT bootByteEnd-bootByteImage <= 32
