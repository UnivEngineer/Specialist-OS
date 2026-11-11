; Обычные регистровые контракты BIOS сохранены. HL строки относится к DOS.
bankDosPrintChar:
j_printChar:
printChar:
    MXOS_CALL MXOS_BIOS_PRINT_CHAR
    ret
bankDosPrintString:
j_printString:
printString:
    push af
    ld a,MEM_OP_PRINT
    call mx_memory
    pop af
    ret
bankDosPrintDecWord:
    MXOS_CALL MXOS_BIOS_PRINT_DEC_WORD
    ret
bankDosGetch:
j_getch:
    MXOS_CALL MXOS_BIOS_GETCH
    ret
