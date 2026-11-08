; Двенадцать байт переходов + двадцать байт текста, все байты окна сохранены.
window20Image:
    ld (IO_PAGE_RAM),a
    jp bankWindowService
    ld (IO_PAGE_RAMD),a
    ret
    DS 12-($-window20Image),0
    DS 20,0
bankWindowService:
    or a
    jp z,bankReturned
    ld (bankHL),hl
    ld hl,0
    add hl,sp
    ld (bankSP),hl
    ld sp,08500h
    push bc
    push de
    ld hl,(bankHL)
    push hl
    dec a
    jp z,bankWindowText
    dec a
    jp z,bankWindowCursor
    call timer_sample
    jp bankWindowDone
bankWindowCursor:
    call bios_setCursorPos
    jp bankWindowDone
bankWindowText:
    ld a,(matrixFlags)
    and 64
    jp z,bankWindowDirect
    ; Источник DE находится в DOS, назначение — отдельный буфер main.
    ; Копировщик восстанавливает тестовое окно прежде, чем мы вернёмся в DOS.
    ex de,hl
    ld (bankLineRequest+1),hl
    ld a,c
    ld (bankLineRequest+6),a
    ld hl,bankLineRequest
    call bankCopyFar
bankLineCopied:
    jp c,bankLineError
    ld hl,bankSerialChars
    jp bankWindowRender
bankWindowDirect:
    ld hl,0FFCCh
    ; C=20/8, рендер тот же, что в остальных сериализованных вариантах.
bankWindowRender:
bankWindowOutput:
    call serialBIOS
    jp nc,bankWindowDone
    ld hl,failText
    ld (bitmapResult),hl
    jp bankWindowDone
bankLineError:
    ld hl,failText
    ld (bitmapResult),hl
bankWindowDone:
    pop hl
    pop de
    pop bc
    ld (bankHL),hl
    ld hl,(bankSP)
    ld sp,hl
    ld hl,(bankHL)
    xor a
    jp 0FFC6h
bankLineRequest: DB 1
    DW 0
    DB 0
    DW bankSerialChars,48
