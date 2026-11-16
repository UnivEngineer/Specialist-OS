; Обработчик ожидания живёт только внутри ввода NC: внешняя COM его не наследует.
; Сохраняет регистры, положение и инверсию ввода; цвет восстанавливается явно.
ncInputHookBegin:
    push hl
    push de
    ld (ncInputColor),a
    ld hl,ncShiftPoll
    call bios_setInputHook
    ld (ncPreviousHook),hl
    pop de
    pop hl
    ret
ncInputHookEnd:
    push af
    push hl
    push de
    ld hl,(ncPreviousHook)
    call bios_setInputHook
    pop de
    pop hl
    pop af
    ret
ncGetch:
    ld a,COLOR_CMDLINE
    call ncInputHookBegin
    call bios_getch
    jp ncInputHookEnd
ncShiftPoll:
    push af
    ld a,(IO_KEYB_B)
    and 2
    push hl
    ld hl,ncShiftState
    cp (hl)
    pop hl
    jp z,ncShiftDone
    push bc
    push de
    push hl
    ld hl,(bios_vars.cursorY)
    push hl
    ld hl,(bios_vars.inverse)
    push hl
    xor a
    ld (bios_vars.inverse),a
    call drawToolbar
    pop hl
    ld (bios_vars.inverse),hl
    pop hl
    ld (bios_vars.cursorY),hl
    ld a,(ncInputColor)
    ld (IO_COLOR),a
    pop hl
    pop de
    pop bc
ncShiftDone:
    pop af
    ret
ncTapeStub:
    ld hl,ncNoticeWindow
    call ncDialogOpen
    COLOR COLOR_DIALOG
    ld hl,ncNoticeWindow
    call draw
    ld hl,ncTapeStubText
    ld e,120
    call ncPrintCentered
    call ncDialogOkButton
    call ncModalGetch
    jp ncRefreshDirty
ncNoticeWindow:
    G_MODAL_WINDOW 80h,64,96,256,64
    G_LINE 80h,68,136,248
    DB 0
; Модальное окно без поля ввода: нулевая высота отключает и начальную
; отрисовку курсора в getch2. Возвращает AF клавиши, сохраняет BC и cursorCfg.
ncModalGetch:
    push bc
    ld a,(bios_upper.cursorCfg)
    push af
    xor a
    ld (bios_upper.cursorCfg),a
    call bios_getch
    push af
    pop bc
    pop af
    ld (bios_upper.cursorCfg),a
    push bc
    pop af
    pop bc
    ret
ncTapeStubText: DB 'Tape is not implemented',0
ncShiftState: DB 2
ncInputColor: DB COLOR_CMDLINE
ncPreviousHook: DW 0
