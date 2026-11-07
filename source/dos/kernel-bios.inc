; Обычные регистровые контракты BIOS сохранены. HL строки относится к DOS.
bankDosPrintChar:
j_printChar:
printChar:
    MXCALL MX_BIOS_PRINTCHAR
    ret
bankDosPrintString:
j_printString:
printString:
    MXCALL MX_BIOS_PRINTSTRING
    ret
bankDosPrintDecWord:
    MXCALL MX_BIOS_PRINTDECWORD
    ret
bankDosGetch:
j_getch:
    MXCALL MX_BIOS_GETCH
    ret
