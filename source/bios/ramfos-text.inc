;----------------------------------------------------------------------------
; MXOS
;
; Синхронизировать историческое слово курсора с активным шрифтом.
;
; вход:
;   MAIN в профиле RAMFOS. Параметров нет.
;
; выход:
;   Скопировано слово Y/X. Все регистры и флаги сохранены.
;----------------------------------------------------------------------------

ramfosCursorIn:
    push hl
    ld hl,(ramfos_cursorPos)
    ld (mon2_vars.cursorY),hl
    pop hl
    ret
ramfosCursorOut:
    push hl
    ld hl,(mon2_vars.cursorY)
    ld (ramfos_cursorPos),hl
    pop hl
    ret

; Вывод C809h: C=символ. C80Fh передаёт тот же символ в A.
; C818h: HL=ASCIIZ. Позицию продолжения экспортируем в C63Eh.
ramfosPrintA:
    push bc
    ld c,a
    call ramfosPrint
    pop bc
    ret
ramfosPrint:
    call ramfosCursorIn
    call printChar
    jp ramfosCursorOut
ramfosString:
    call ramfosCursorIn
    call printStringZ
    jp ramfosCursorOut
