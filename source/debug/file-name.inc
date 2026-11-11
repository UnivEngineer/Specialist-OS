; Пакетное поле 8.3. HL возвращается после расширения, DE/AF сохранены.
; Непечатные байты повреждённого имени заменяются пробелами.
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
        CALL bios_printStringReal
        POP HL
        POP DE
        POP AF
        RET
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

