;----------------------------------------------------------------------------
; MXOS NC.COM
; Вывод строки по адресу hl длиной b или до терминатора.
; Выводит только реальные символы, чтобы не портить экран при
; неотформатированном или повреждённом диске.
;
; вход:
;   hl = адрес строки
;   b  = макс. длина печати
;   c  = терминатор
;
; выход:
;   hl = адрес конца строки + 1
;
; 2022-01-31 SpaceEngineer
;----------------------------------------------------------------------------

printString2:
    push    af
    push    de

printString2loop:
    ld      a, (hl)
    cp      c
    jp c,   printString2exit

    push    bc
    push    hl

    call    bios_printCharReal

    pop     hl
    pop     bc

    inc     hl
    dec     b
    jp nz,  printString2loop

printString2exit:
    pop     de
    pop     af
    ret
