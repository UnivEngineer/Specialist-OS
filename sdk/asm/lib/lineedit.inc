; Общий однострочный редактор CLI и диалогов NC, исходник включается в программу.
; HL=буфер MAX_PATH, DE=конец начального текста; B: bit0=Escape завершает ввод,
; bit1=вернуть CTRL/служебную клавишу вызывающему, bit2=вернуться при пустой строке.
; bit3=вернуть обычные Up/Down вызывающему (A=19h/1Ah, CF=0).
; bit4 (LINEEDIT_RECALL): первая печатная клавиша заменяет подставленный текст.
; Навигация и Backspace снимают состояние подстановки.
; C=ширина окна (0 — до правого края). Позиция начала берётся из BIOS.
; Ширина символа 4/6/8 px берётся из FONT_ENGINE_HEADER+5; X BIOS в полупикселях.
; Возврат A=Enter/Escape либо переданная клавиша; CF=1 для переданного CTRL.
; Буфер ASCIIZ. AF/BC/DE/HL разрушены, кроме указанных результатов.
; Последний столбец экрана не используется, поэтому ввод не прокручивает экран.
    INCLUDE "../formats/font.inc"
lineEdit:
    ld (lineEditBuffer),hl
    ld a,b
    ld (lineEditFlags),a
    xor a
    ld (de),a
    ld (lineEditOffset),a
    ex de,hl
    call bios_sub_hl_de
    ld a,l
    ld (lineEditLength),a
    ld (lineEditPosition),a
    ld a,(FONT_ENGINE_HEADER+5)
    or a
    rra
    ld d,a ; Шаг символа в единицах cursorX.
    ld hl,(bios_vars.cursorY)
    ld a,c
    or a
    jp nz,lineEditWidthReady
    ; Если после приглашения осталось не более 8 ячеек, начать новую строку.
    ld a,d
    add a,a
    add a,a
    add a,a
    ld b,a
    ld a,192
    sub b
    cp h
    jp z,lineEditOriginNewline
    jp nc,lineEditOriginReady
lineEditOriginNewline:
    ld c,0Ah
    call bios_printChar
    ld hl,(bios_vars.cursorY)
lineEditOriginReady:
    ; Оставить последнюю ячейку пустой: печать не должна прокрутить экран.
    ld a,192
    sub d
    sub h
    ld b,0
lineEditWidthLoop:
    inc b
    sub d
    jp nc,lineEditWidthLoop
    dec b
    ld a,b
lineEditWidthReady:
    ld (lineEditWidth),a
    ld (lineEditOrigin),hl
    jp lineEditRedraw
lineEditLoop:
    call bios_getch
    jp nc,lineEditKey
    push af
    ld a,(lineEditFlags)
    and 2
    jp z,lineEditControlIgnored
    pop af
    ret
lineEditControlIgnored:
    pop af
lineEditKey:
    cp 0Dh
    ret z
    ld c,a
    ld a,(lineEditFlags)
    and 8
    ld a,c
    jp z,lineEditOrdinaryKey
    cp 019h
    ret z
    cp 01Ah
    ret z
lineEditOrdinaryKey:
    cp 07Fh
    jp z,lineEditBackspace
    cp 8
    jp z,lineEditLeft
    cp 018h
    jp z,lineEditRight
    cp 00Ch
    jp z,lineEditHome
    cp 00Ah
    jp z,lineEditEnd
    cp 019h
    IFDEF LINEEDIT_HISTORY
    jp z,cliHistoryUp
    ELSE
    jp z,lineEditHome
    ENDIF
    cp 01Ah
    IFDEF LINEEDIT_HISTORY
    jp z,cliHistoryDown
    ELSE
    jp z,lineEditEnd
    ENDIF
    cp 01Bh
    IFDEF LINEEDIT_HISTORY
    jp z,cliHistoryClear
    ELSE
    jp z,lineEditClear
    ENDIF
    cp 020h
    jp c,lineEditUnhandled
    cp 08Ah
    jp z,lineEditUnhandled
    cp 08Bh
    jp z,lineEditUnhandled
    ld c,a
    IFDEF LINEEDIT_RECALL
    ld a,(lineEditFlags)
    and 10h
    jp nz,lineEditReplace
    ENDIF
    ld a,(lineEditLength)
    cp MAX_PATH-1
    jp z,lineEditLoop
    ld e,a
    ld d,0
    ld hl,(lineEditBuffer)
    add hl,de
    inc a
    ld (lineEditLength),a
    ld a,(lineEditPosition)
    ld b,a
lineEditInsertLoop:
    ld a,(hl)
    inc hl
    ld (hl),a
    dec hl
    ld a,e
    cp b
    jp z,lineEditInserted
    dec hl
    dec e
    jp lineEditInsertLoop
lineEditInserted:
    ld (hl),c
    ld a,b
    inc a
    ld (lineEditPosition),a
    ; Добавление в конец без сдвига окна печатает только новый символ.
    ld b,a
    ld a,(lineEditLength)
    cp b
    jp nz,lineEditRedraw
    ld a,(lineEditOffset)
    ld c,a
    ld a,b
    sub c
    ld b,a
    ld a,(lineEditWidth)
    cp b
    jp c,lineEditRedraw
    jp z,lineEditRedraw
    ld c,(hl)
    IFDEF LINEEDIT_BATCH
    call LINEEDIT_PRINT_CHAR
    ELSE
    call bios_printChar
    ENDIF
    jp lineEditLoop
    IFDEF LINEEDIT_RECALL
lineEditReplace:
    call lineEditCancelRecall
    ld hl,(lineEditBuffer)
    ld (hl),c
    inc hl
    ld (hl),0
    ld a,1
    ld (lineEditLength),a
    ld (lineEditPosition),a
    xor a
    ld (lineEditOffset),a
    jp lineEditRedraw
lineEditCancelRecall:
    push af
    ld a,(lineEditFlags)
    and 0EFh
    ld (lineEditFlags),a
    pop af
    ret
    ENDIF
lineEditUnhandled:
    push af
    ld a,(lineEditFlags)
    and 2
    jp z,lineEditUnhandledIgnore
    pop af
    or a
    ret
lineEditUnhandledIgnore:
    pop af
    jp lineEditLoop
lineEditBackspace:
    IFDEF LINEEDIT_RECALL
    call lineEditCancelRecall
    ENDIF
    ld a,(lineEditPosition)
    or a
    jp z,lineEditLoop
    dec a
    ld (lineEditPosition),a
    ld e,a
    ld d,0
    ld hl,(lineEditBuffer)
    add hl,de
    ld d,h
    ld e,l
    inc hl
lineEditDeleteLoop:
    ld a,(hl)
    ld (de),a
    inc hl
    inc de
    or a
    jp nz,lineEditDeleteLoop
    ld hl,lineEditLength
    dec (hl)
    jp nz,lineEditRedraw
    ld a,(lineEditFlags)
    and 4
    jp z,lineEditRedraw
    ; Сначала стереть последний знак, затем вернуть управление панели.
    call lineEditWindow
    call lineEditPaintOnly
    ld a,07Fh
    or a
    ret
    jp lineEditRedraw
lineEditLeft:
    ld a,(lineEditPosition)
    or a
    jp z,lineEditLoop
    dec a
    jp lineEditMoved
lineEditRight:
    ld a,(lineEditLength)
    ld b,a
    ld a,(lineEditPosition)
    cp b
    jp z,lineEditLoop
    inc a
    jp lineEditMoved
lineEditHome:
    xor a
    jp lineEditMoved
lineEditEnd:
    ld a,(lineEditLength)
lineEditMoved:
    IFDEF LINEEDIT_RECALL
    call lineEditCancelRecall
    ENDIF
    ld (lineEditPosition),a
    call lineEditWindow
    jp nz,lineEditPaint
    call lineEditCursor
    jp lineEditLoop
lineEditClear:
    ld a,(lineEditFlags)
    and 1
    ld a,01Bh
    ret nz
    xor a
    ld hl,(lineEditBuffer)
    ld (hl),a
    ld (lineEditLength),a
    ld (lineEditPosition),a
lineEditRedraw:
    call lineEditWindow
lineEditPaint:
    call lineEditPaintOnly
    jp lineEditLoop
lineEditPaintOnly:
    ld hl,(lineEditOrigin)
    ld (bios_vars.cursorY),hl
    ld a,(lineEditOffset)
    ld e,a
    ld d,0
    ld hl,(lineEditBuffer)
    add hl,de
    ld a,(lineEditWidth)
    ld b,a
    IFDEF LINEEDIT_BATCH
    call LINEEDIT_PAINT
    ELSE
lineEditPaintLoop:
    ld a,(hl)
    or a
    ld c,' '
    jp z,lineEditPaintSpace
    ld c,a
    inc hl
lineEditPaintSpace:
    call bios_printChar
    dec b
    jp nz,lineEditPaintLoop
    ENDIF
    call lineEditCursor
    ret
; ZF=окно не сдвинулось; AF/BC разрушены.
lineEditWindow:
    ld a,(lineEditOffset)
    ld c,a
    ld a,(lineEditPosition)
    sub c
    jp c,lineEditWindowLeft
    ld b,a
    ld a,(lineEditWidth)
    cp b
    jp z,lineEditWindowRight
    jp nc,lineEditWindowSame
lineEditWindowRight:
    ld b,a
    ld a,(lineEditPosition)
    sub b
    inc a
    jp lineEditWindowSave
lineEditWindowLeft:
    ld a,(lineEditPosition)
lineEditWindowSave:
    ld (lineEditOffset),a
    cp c
    ret
lineEditWindowSame:
    xor a
    ret
lineEditCursor:
    ld a,(lineEditOffset)
    ld b,a
    ld a,(lineEditPosition)
    sub b
    ld b,a
    ld a,(FONT_ENGINE_HEADER+5)
    cp 6
    ld a,b
    jp z,lineEditCursor6
    add a,a
    ld b,a
    ld a,(FONT_ENGINE_HEADER+5)
    cp 4
    ld a,b
    jp z,lineEditCursorReady
    add a,a ; 8 px = 4 единицы cursorX.
    jp lineEditCursorReady
lineEditCursor6:
    add a,a
    add a,b
lineEditCursorReady:
    ld hl,(lineEditOrigin)
    add a,h
    ld h,a
    ld (bios_vars.cursorY),hl
    ret
lineEditOrigin:   DW 0
lineEditWidth:    DB 0
lineEditLength:   DB 0
lineEditPosition: DB 0
lineEditOffset:   DB 0
lineEditBuffer:   DW 0
lineEditFlags:    DB 0
