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
    ld a,(bios_vars.inverse)
    push af
    xor a
    ld (bios_vars.inverse),a
    call drawToolbar
    pop af
    ld (bios_vars.inverse),a
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
    call drawWindow1
    CENTER_LINE 20, 5Eh
    ld hl,ncTapeStubText
    call bios_printString
    call ncModalGetch
    jp mainReload
; Модальное окно без поля ввода: нулевая высота отключает и начальную
; отрисовку курсора в getch2. Возвращает AF клавиши, сохраняет BC и cursorCfg.
ncModalGetch:
    push bc
    ld a,(bios_vars.cursorCfg)
    push af
    xor a
    ld (bios_vars.cursorCfg),a
    call bios_getch
    push af
    pop bc
    pop af
    ld (bios_vars.cursorCfg),a
    push bc
    pop af
    pop bc
    ret
ncTapeStubText: DB 'Tape not implemented',0
ncShiftState: DB 2
ncInputColor: DB COLOR_CMDLINE
ncPreviousHook: DW 0
