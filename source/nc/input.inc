; Диалог NC использует общий редактор с полем фиксированной ширины.
; DE=конец начальной строки vars.input; A=Enter/Escape, строка ASCIIZ.
input:
            ld a,COLOR_DIALOG
            call ncInputHookBegin
            ld a,1
            ld (bios_vars.inverse),a
            ld hl,03D6Eh
            ld (bios_vars.cursorY),hl
            ld hl,vars.input
            ld bc,(1 << 8) + P_INPUT_WIDTH
            call lineEdit
            call ncInputHookEnd
            push af
            ld hl,0
            ld (bios_vars.inverse),hl
            pop af
            ret

; Пустая строка оставляет стрелки панели. Непустую редактирует Lineedit;
; F-клавиши, Tab и CTRL возвращаются главному циклу NC с актуальной строкой.
ncReadKey:
            ld hl,(vars.cmdLineEnd)
            ld de,vars.cmdLine
            call bios_cmp_hl_de
            jp z,ncGetch
            ex de,hl
            push de
            ld hl,(vars.cmdTextPos)
            ld (bios_vars.cursorY),hl
            pop de
            ld hl,vars.cmdLine
            ld a,(vars.cmdCapacity)
            ld c,a
            ld b,7
            ld a,COLOR_CMDLINE
            call ncInputHookBegin
            call lineEdit
            call ncInputHookEnd
            push af
            ld a,(lineEditLength)
            ld e,a
            ld d,0
            ld hl,vars.cmdLine
            add hl,de
            ld (vars.cmdLineEnd),hl
            ld hl,(bios_vars.cursorY)
            ld (vars.cmdLinePos),hl
            ld a,(lineEditOffset)
            ld (vars.cmdOffset),a
            pop af
            ret
    INCLUDE "../include/lineedit.inc"
