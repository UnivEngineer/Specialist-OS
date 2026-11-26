;----------------------------------------------------------------------------
; MXOS
;
; Вывести имя файла 8.3 с точкой и фиксированной шириной полей.
;
; вход:
;   hl указывает на 11 байтов имени в каталоге. debugNameBuffer свободен для сборки
;   строки.
;
; выход:
;   hl продвинут на 11. Байты ниже 20h напечатаны как пробелы. af и de сохранены, b
;   используется как счётчик.
;----------------------------------------------------------------------------

printFileNameReal:
        PUSH AF
        PUSH DE
        LD DE,debugNameBuffer
        LD B,8
        CALL debugNameCopy
        LD A,'.'
        LD (DE),A
        INC DE
        LD B,3
        CALL debugNameCopy
        XOR A
        LD (DE),A
        PUSH HL
        LD HL,debugNameBuffer
        CALL bios_printStringRawZ
        POP HL
        POP DE
        POP AF
        RET

;----------------------------------------------------------------------------
; MXOS
;
; Перенести поле имени в строку для печати DEBUG.
;
; вход:
;   hl = поле каталога, de = адрес назначения, b = ненулевая длина поля.
;
; выход:
;   hl и de продвинуты на длину. Байты ниже 20h заменены пробелами. b = 0, a =
;   последний записанный байт, c сохранён.
;----------------------------------------------------------------------------

debugNameCopy:
        LD A,(HL)
        CP 20h
        JP NC,debugNamePrintable
        LD A,' '
debugNamePrintable:
        LD (DE),A
        INC HL
        INC DE
        DEC B
        JP NZ,debugNameCopy
        RET
debugNameBuffer: DS 13,0

