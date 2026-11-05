; Общий однострочный редактор CLI и диалогов NC, исходник включается в программу.
; HL=буфер MAX_PATH, DE=конец начального текста; B: bit0=Escape завершает ввод,
; bit1=вернуть CTRL/служебную клавишу вызывающему, bit2=вернуться при пустой строке.
; C=ширина окна (0 — до правого края). Позиция начала берётся из BIOS.
; Возврат A=Enter/Escape, буфер ASCIIZ. AF/BC/DE/HL разрушены.
; Последний столбец экрана не используется, поэтому ввод не прокручивает экран.
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
    ld hl,(bios_vars.cursorY)
    ld a,c
    or a
    jp nz,lineEditWidthReady
    ld a,h
    cp 168
    jp c,lineEditOriginReady
    ld c,0Ah
    call bios_printChar
    ld hl,(bios_vars.cursorY)
lineEditOriginReady:
    ld a,189
    sub h
    ld b,0
lineEditWidthLoop:
    inc b
    sub 3
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
    call bios_printChar
    jp lineEditLoop
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
    add a,a
    add a,b
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
