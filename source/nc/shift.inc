; Обработчик ожидания живёт только внутри ввода NC: внешняя COM его не наследует.
; Сохраняет регистры, положение и инверсию ввода; цвет восстанавливается явно.
    IF !NC_BANKED
ncInputHookBegin:
    push hl
    push de
    ld (ncInputColor),a
    ld hl,ncShiftPoll
    MXOS_CALL_REGS MXOS_BIOS_SET_INPUT_HOOK
    ld (ncPreviousHook),hl
    pop de
    pop hl
    ret
ncInputHookEnd:
    push af
    push hl
    push de
    ld hl,(ncPreviousHook)
    MXOS_CALL_REGS MXOS_BIOS_SET_INPUT_HOOK
    pop de
    pop hl
    pop af
    ret
ncGetch:
    ld a,COLOR_CMDLINE
    call ncInputHookBegin
    call mon2_getchOld
    jp ncInputHookEnd
;----------------------------------------------------------------------------
; MXOS
;
; Опросить Shift и обслужить статус во время ожидания ввода в MAIN.
;
; вход:
;   Вызывается из BIOS при ожидании клавиши, когда матрица опрашивает все клавиши.
;   Регистровых параметров нет.
;
; выход:
;   Панель кнопок может обновиться по Shift. Метка платформы дописывается только
;   при основном вводе NC. Регистры, флаги и состояние текстового ввода сохранены.
;----------------------------------------------------------------------------
ncShiftPoll:
    push af
    call ncStatusIdle
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
    ld hl,(mon2_vars.cursorY)
    push hl
    ld hl,(mon2_vars.inverse)
    push hl
    xor a
    ld (mon2_vars.inverse),a
    call drawToolbar
    pop hl
    ld (mon2_vars.inverse),hl
    pop hl
    ld (mon2_vars.cursorY),hl
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
ncTapeStubText:     DB 'Tape is not implemented',0
ncShiftState:       DB 2
    IF !NC_BANKED
ncInputColor:       DB COLOR_CMDLINE
ncPreviousHook:     DW 0
    ENDIF
