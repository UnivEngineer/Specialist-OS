; Обработчик ожидания живёт только внутри ввода NC: внешняя COM его не наследует.
; Сохраняет регистры, положение и инверсию ввода; цвет восстанавливается явно.
    IF !NC_BANKED
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
    IF NC_STATUS_DEFER
    call ncStatusIdle
    ENDIF
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
    ELSE
ncGetch EQU ncBankGetch
    ENDIF
ncTapeStub:
    ld hl,ncErrorModal
    call ncModalOpen
    ld hl,ncTapeStubText
    call uiModalBodyText
    call uiModalGetch
    jp ncRefreshDirty
ncTapeStubText: DB 'Tape is not implemented',0
ncShiftState: DB 2
    IF !NC_BANKED
ncInputColor: DB COLOR_CMDLINE
ncPreviousHook: DW 0
    ENDIF
