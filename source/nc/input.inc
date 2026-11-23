; Диалог NC использует общий редактор с полем фиксированной ширины.
; DE=конец начальной строки vars.input; A=Enter/Escape, строка ASCIIZ.
input:
    IF !NC_BANKED
            ld a,COLOR_DIALOG
            call ncInputHookBegin
    ENDIF
            ld a,1
            ld (bios_vars.inverse),a
            call ncModalInputCursor
            ld (bios_vars.cursorY),hl
            ld hl,vars.input
            ld a,(ncInputChars)
            ld c,a
            ld b,1
            call lineEdit
    IF !NC_BANKED
            call ncInputHookEnd
    ENDIF
            push af
            ld hl,0
            ld (bios_vars.inverse),hl
            pop af
            ret

;----------------------------------------------------------------------------
; MXOS
;
; Прочитать клавишу в главном цикле NC и разрешить отложенное чтение статуса
; только на время этого ожидания.
;
; вход:
;   Командная строка и положение её курсора готовы. Регистровых параметров нет.
;
; выход:
;   A = клавиша, CF = признак Ctrl. Командная строка и её курсор обновлены.
;   Остальные регистры и флаги не определены.
;----------------------------------------------------------------------------
ncReadKey:
            ld a,1
            ld (ncStatusInput),a
            call ncReadMainKey
            push af
            xor a
            ld (ncStatusInput),a
            pop af
            ret

;----------------------------------------------------------------------------
; MXOS
;
; Дождаться клавиши через BIOS или через редактор непустой командной строки.
;
; вход:
;   Командная строка и положение её курсора готовы. Регистровых параметров нет.
;
; выход:
;   A = клавиша, CF = признак Ctrl. Стрелки вверх и вниз возвращаются панели,
;   как и функциональные клавиши, Tab и Ctrl. Остальные регистры и флаги не определены.
;----------------------------------------------------------------------------
ncReadMainKey:
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
            ld b,15
    IF !NC_BANKED
            ld a,COLOR_CMDLINE
            call ncInputHookBegin
    ENDIF
            ld a,(ncCommandEditResume)
            or a
            jp z,ncCommandEditBegin
            xor a
            ld (ncCommandEditResume),a
            call lineEditRedraw
            jp ncCommandEditDone
ncCommandEditBegin:
            call lineEdit
ncCommandEditDone:
    IF !NC_BANKED
            call ncInputHookEnd
    ENDIF
            push af
            ; Panel movement cannot enter another editor. Reuse the prompt's
            ; position/offset on the next read instead of restarting at End.
            cp 019h
            jp z,ncCommandEditKeep
            cp 01Ah
            jp nz,ncCommandEditSaved
ncCommandEditKeep:
            ld a,1
            ld (ncCommandEditResume),a
ncCommandEditSaved:
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
ncCommandEditResume: DB 0
    DEFINE LINEEDIT_BATCH 1
LINEEDIT_PRINT_CHAR EQU ncPrintChar
LINEEDIT_PAINT      EQU ncPrintField
    INCLUDE "../include/lineedit.inc"
